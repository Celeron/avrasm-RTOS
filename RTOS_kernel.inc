.MESSAGE "<-- RTOS ver.1.9 alpha -->"
;=== BEGIN "RTOS-kernel.inc" ===============================================

; Ядро RTOS
; (Предупреждение: сюда вам лазить не нужно, непосредственно, эти методы не используются в прикладном коде!)


.ifdef	RTOS_FEATURE_PLANTASK
;***************************************************************************
;*
;*  Базовая диспетчеризация задач
;*
;***************************************************************************


.ifndef	RTOS_OPTIMIZE_FOR_SMALL_MEMORY	; Примечание: данный метод необязательно запускать при инициализации RTOS, если используется RAMFLUSH (macrobaselib.inc) при инициализации микроконтроллера.
;---------------------------------------------------------------------------
; 
; Очистить "очеред задач" RTOS_TaskQueue (проинициализировать)
;
; Данный метод следует вызывать только раз, на этапе инициализации МК, когда никакой полезной работы ещё не ведётся.
; Также, на этапе выполнения, при реализации нестандартного управления, данным методом можно принудительно очистить всю очередь задач. (не рекомендуется к использованию!)
; 
;---------------------------------------------------------------------------

;----- Subroutine Register Variables

; Без параметров.

; Памятка: данный метод не портит содержимое РОН - поскольку защищает используемые регистры Стеком.
; Памятка: SREG сохраняется.

;----- Code


RTOS_METHOD_ClearTaskQueue:

		PUSHF						; сохранить temp1 и SREG в стеке	(замечу: заодно, сохраняется текущее состояние "флага прерываний")
		PUSH	temp2
		PUSH	ZH
		PUSH	ZL

		LDI	ZL,	Low (RTOS_TaskQueue)
		LDI	ZH,	High(RTOS_TaskQueue)
		LDI	temp1,	$00				; = спец.код "Нет Задачи"
		LDI	temp2,	RTOS_TaskQueueSize		; переменная цикла

		;=== начало критической секции
		CLI						; запрещаем прерывания
	Loop__RTOS_METHOD_ClearTaskQueue:
		ST	Z+,	temp1
		DEC	temp2
		BRNE	Loop__RTOS_METHOD_ClearTaskQueue

		POP	ZL
		POP	ZH
		POP	temp2
		POPF						; Restore global interrupt flag
		;=== конец критической секции
		RET



.endif	//RTOS_OPTIMIZE_FOR_SMALL_MEMORY

;---------------------------------------------------------------------------
;
; Добавить Задачу в очередь RTOS_TaskQueue, 
; послать на выполнение
;
;---------------------------------------------------------------------------

;----- Subroutine Register Variables

;.def	TaskCode	= (см.выше)	; (входной, не изменяется)	Номер-индекс Задачи в таблице RTOS_TaskProcs

; Памятка: в режиме без оптимизации RTOS_OPTIMIZE_FOR_SMALL_MEMORY: кроме параметровых, больше регистров не портит - поскольку защищает используемые регистры Стеком.
; Памятка: в режиме  с оптимизацией RTOS_OPTIMIZE_FOR_SMALL_MEMORY: защита стеком отключена - использует/портит содержимое регистров TEMP1, TEMP2, TEMP3, ZH:ZL.
; Памятка: SREG сохраняется всегда.

;----- Code


RTOS_METHOD_AddTask:

	.ifndef	RTOS_OPTIMIZE_FOR_SMALL_MEMORY
		PUSH	temp1
		PUSH	temp2
		PUSH	temp3
		PUSH	ZL
		PUSH	ZH
	.endif

		;=== начало критической секции			; (примечание: чтений+запись разделяемой памяти - должны быть в единой транзакции!)
		INR	temp3,	SREG				; Save global interrupt flag
		CLI						; запрещаем прерывания

		; Сканируем очередь сначала до конца, ищем первую пустую ячейку:
		LDI	ZL,	Low (RTOS_TaskQueue)
		LDI	ZH,	High(RTOS_TaskQueue)
		LDI	temp2,	RTOS_TaskQueueSize		; переменная цикла (инициализация)
	Loop__RTOS_METHOD_AddTask:
		LD	temp1,	Z+
		CPI	temp1,	$00				; проверка: ячейка пустая?
		BREQ	Found__RTOS_METHOD_AddTask		; если пустая - прерываем сканирование на этой позиции...
		DEC	temp2					; переменная цикла (декрементация)
		BRNE	Loop__RTOS_METHOD_AddTask

	.ifndef	RTOS_OPTIMIZE_FOR_SMALL_MEMORY
		; Если достигли конца очереди, но так и не нашли пустой ячейки - ошибка!
		LDI	ErrorCode,	10			; код: 10 - "очередь Задач" переполнена, не могу добавить новую Задачу!
		RCALL	RTOS_METHOD_ErrorHandler
	.endif
		RJMP	End__RTOS_METHOD_AddTask

		; Если нашли пустое место (фактически, это текущий "хвост" очереди) - то дополняем очередь требуемой Задачей:
	Found__RTOS_METHOD_AddTask:
		ST	-Z,	TaskCode

	End__RTOS_METHOD_AddTask:
		OUTR	SREG,	temp3				; Restore global interrupt flag
		;=== конец критической секции

	.ifndef	RTOS_OPTIMIZE_FOR_SMALL_MEMORY
		POP	ZH
		POP	ZL
		POP	temp3
		POP	temp2
		POP	temp1
	.endif
		RET



;---------------------------------------------------------------------------
; 
; Обработчик очереди Задач (диспетчер)
;
; см. описание идеи http://easyelectronics.ru/avr-uchebnyj-kurs-operacionnaya-sistema-dispetcher-zadach.html
; 
;---------------------------------------------------------------------------

;----- Subroutine Register Variables

; Без параметров.

; Памятка: в режиме без оптимизации RTOS_OPTIMIZE_FOR_SMALL_MEMORY: кроме ZH:ZL (совершенно необходимых для "Indirect Jump" в задачу), больше регистров не портит - поскольку защищает используемые регистры Стеком.
; Памятка: в режиме  с оптимизацией RTOS_OPTIMIZE_FOR_SMALL_MEMORY: защита стеком отключена - использует/портит содержимое регистров TEMP1, TEMP2, TEMP3, TEMP4 и ZH:ZL.
; Примечание: хотя, в данном случае, необязательно сберегать регистры, т.к. данный метод вызывается из суперцикла "MAIN", между запусками прикладных Задач - когда сбережение регистров, обычно, не требуется.
; Памятка: SREG сохраняется всегда.

;----- Code


RTOS_METHOD_ProcessTaskQueue:

	.ifndef	RTOS_OPTIMIZE_FOR_SMALL_MEMORY			; Эксперимент! Хотя вообще-то, сберегать регистры - излишне, в данном (не интерактивном) методе...
		PUSH	temp1
		PUSH	temp2
		PUSH	temp3
		PUSH	temp4
	.endif

		;=== начало критической секции			; (примечание: чтений+запись разделяемой памяти - должны быть в единой транзакции!)
		INR	temp4,	SREG				; Save global interrupt flag
		CLI						; запрещаем прерывания

		; Загрузить код текущей Задачи к исполнению:
		; Примечание: при простое ядра и пустой очереди, будет прочитан код текущей задачи == 0 (спец.код "Нет Задачи") - в таком случае, далее, будет осуществлён запуск системной задачи "холостой цикл" (Task_Idle)...
		LDI	ZL,	Low (RTOS_TaskQueue)		; Загрузить адрес начала "очереди задач" -> в регистр Z
		LDI	ZH,	High(RTOS_TaskQueue)
		LD	temp3,	Z				; Загрузить первый байт из очереди RTOS_TaskQueue[0] -> temp3

		; Сдвинуть "очередь задач" на одну позицию назад:
		LDI	temp2,	RTOS_TaskQueueSize-1		; переменная цикла (инициализация)
	Loop__RTOS_METHOD_ProcessTaskQueue:
		LDD	temp1,	Z+1 				; RTOS_TaskQueue[i+1] -> temp1
		ST	Z+,	temp1				; temp1 -> RTOS_TaskQueue[i++]
		CPI	temp1,	$00				; Сравниваем RTOS_TaskQueue[i] со спец.кодом "Нет Задачи"
		BREQ	EndShift__RTOS_METHOD_ProcessTaskQueue	; Если равен? Значит, хвост очереди пуст - пропускаем его сдвиг...
		DEC	temp2					; переменная цикла (декрементация)
		BRNE	Loop__RTOS_METHOD_ProcessTaskQueue
		ST	Z,	temp2				; в последний элемент очереди поместить спец.код "Нет Задачи":	RTOS_TaskQueue[RTOS_TaskQueueSize]=0	(замечу, что сейчас, temp2==0)
	EndShift__RTOS_METHOD_ProcessTaskQueue:
	
		OUTR	SREG,	temp4				; Restore global interrupt flag
		;=== конец критической секции


		; Получить реальный адрес точки входа в подпрограмму Задачи:
		; Примечание: алгоритм реализован на "индексных переходах" - см. объяснение приёма в "AVR. Учебный курс. Ветвления на индексных переходах" (с) http://easyelectronics.ru/avr-uchebnyj-kurs-vetvleniya.html
		; (текущее состояние: temp3 == код текущей Задачи к исполнению)

	.ifdef	MAPPED_FLASH_START
		; Вариант кода для AVR8L Instruction Set: где нет РОН R0:R15, и нет инструкции LPM (вместо неё LD).
		; see recipe: http://www.avrfreaks.net/index.php?module=PNphpBB2&file=viewtopic&t=82385&highlight=
		; проверено: это работает на ATtiny10!
		
		MOV	ZL,	temp3				; индекс целевой Задачи -> ZL
		CLR	ZH
		LSL	ZL					; temp1 *= 2
		ADC	ZH,	ZH				; saving carry bit	(Замечу: в инструкции, я могу использовать регистр-приёмник ZH также и как второй регистр-источник. РОН и АЛУ - разные вещи. И здесь это корректно арифметически, потому что он содержит =0, в этой время.)
		SUBI	ZL,	Low (-(RTOS_TaskProcs*2 + MAPPED_FLASH_START))
		SBCI	ZH,	High(-(RTOS_TaskProcs*2 + MAPPED_FLASH_START))	; Add
		
		LD	temp1,	Z+				; загрузить из "индексной таблицы" мл.байт адреса перехода -> temp1
		LD	temp2,	Z				; загрузить из "индексной таблицы" ст.байт адреса перехода -> temp2
		
	.else	//Вариант кода для остальных AVR

		MOV	ZL,	temp3				; индекс целевой Задачи -> ZL
		CLR	ZH
		LSL	ZL					; temp1 *= 2
		ADC	ZH,	ZH				; saving carry bit	(Замечу: в инструкции, я могу использовать регистр-приёмник ZH также и как второй регистр-источник. РОН и АЛУ - разные вещи. И здесь это корректно арифметически, потому что он содержит =0, в этой время.)
		SUBI	ZL,	Low (-RTOS_TaskProcs*2)
		SBCI	ZH,	High(-RTOS_TaskProcs*2)		; Add
	
		LPM	temp1,	Z+				; загрузить из "индексной таблицы" мл.байт адреса перехода -> temp1
		LPM	temp2,	Z				; загрузить из "индексной таблицы" ст.байт адреса перехода -> temp2
	.endif

		; Перейти к исполнению Задачи:

		MOV	ZL,	temp1				; Загрузить адрес точки входа в Задачу -> в регистр Z
		MOV	ZH,	temp2

		OUTR	SREG,	temp4				; Restore SREG also (to be transparent)
	.ifndef	RTOS_OPTIMIZE_FOR_SMALL_MEMORY
		POP	temp4
		POP	temp3
		POP	temp2
		POP	temp1
	.endif

		; Трюк: Вызов подпрограммы, с переназначенной "точкой выхода" из неё.
		; Суть: Вместо стандартного RCALL, мы входим в подпрограмму простым RJMP, предварительно затолкнув в стек адрес желаемой "точки выхода".
		; Применимо здесь: посколько вызов вложенной подпрограммы осуществляется прямо перед выходом RET из этой головной процедуры. Таким образом, CALL + RET === JMP

		IJMP 						; Запуск Задачи!!!
								; Замечу, при этом, минимализируется использование стека:
								; 	Обрати внимание - сюда мы пришли по RCALL из главного цикла.
								; 	Значит, в стеке у нас лежит адрес возврата...
								; 	А уходим в задачу по IJMP, который стек не меняет.
								; 	Но это не страшно - ведь из Задачи мы выйдем по RET!

.endif	//RTOS_FEATURE_PLANTASK

.ifdef	RTOS_FEATURE_PLANTASKTIMED
;***************************************************************************
;*
;*  Диспетчеризация Задач по Таймеру
;*
;***************************************************************************


.ifndef	RTOS_OPTIMIZE_FOR_SMALL_MEMORY	; Примечание: данный метод необязательно запускать при инициализации RTOS, если используется RAMFLUSH (macrobaselib.inc) при инициализации микроконтроллера.
;---------------------------------------------------------------------------
; 
; Очистить "пул таймеров" RTOS_TimersPool (проинициализировать)
; 
; Данный метод следует вызывать только раз, на этапе инициализации МК, когда никакой полезной работы ещё не ведётся.
; Также, на этапе выполнения, при реализации нестандартного управления, данным методом можно принудительно выключить/очистить все таймеры. (не рекомендуется к использованию!)
;
;---------------------------------------------------------------------------

;----- Subroutine Register Variables

; Без параметров.

; Памятка: данный метод не портит содержимое РОН - поскольку защищает используемые регистры Стеком.
; Памятка: SREG сохраняется.

;----- Code


RTOS_METHOD_ClearTimers:

		PUSHF						; сохранить temp1 и SREG в стеке	(замечу: заодно, сохраняется текущее состояние "флага прерываний")
		PUSH	temp2
		PUSH	ZH
		PUSH	ZL

		LDI	ZL,	Low (RTOS_TimersPool)
		LDI	ZH,	High(RTOS_TimersPool)
		LDI	temp1,	$00				; = спец.код "Нет Задачи"
		LDI	temp2,	RTOS_TimersPoolSize		; переменная цикла

		;=== начало критической секции
		CLI						; запрещаем прерывания
	Loop__RTOS_METHOD_ClearTimers:
		ST	Z+,	temp1				; Event
		ST	Z+,	temp1				; Counter Lo
		ST	Z+,	temp1				; Counter Hi
		DEC	temp2
		BRNE	Loop__RTOS_METHOD_ClearTimers
	
		POP	ZL
		POP	ZH
		POP	temp2
		POPF						; Restore global interrupt flag
		;=== конец критической секции
		RET



.endif	//RTOS_OPTIMIZE_FOR_SMALL_MEMORY

;---------------------------------------------------------------------------
;
; Установить (добавить или обновить) Таймер в пуле RTOS_TimersPool, 
; для отложенного запуска Задачи
; 
;---------------------------------------------------------------------------

;----- Subroutine Register Variables

;.def	TaskCode	= (см.выше)	; (входной, не изменяется)	Номер-индекс Задачи в таблице RTOS_TaskProcs
;.def	TimeDelayLow	= (см.выше)	; (входной, не изменяется)	Время Задержки, в милисекундах (мл.байт)
;.def	TimeDelayHigh	= (см.выше)	; (входной, не изменяется)	Время Задержки, в милисекундах (ст.байт)

; Памятка: в режиме без оптимизации RTOS_OPTIMIZE_FOR_SMALL_MEMORY: кроме параметровых, больше регистров не портит - поскольку защищает используемые регистры Стеком.
; Памятка: в режиме  с оптимизацией RTOS_OPTIMIZE_FOR_SMALL_MEMORY: защита стеком отключена - использует/портит содержимое регистров TEMP1, TEMP2, TEMP3, TEMP4, TEMP5, ZH:ZL.
; Памятка: SREG сохраняется всегда.

;----- Code


RTOS_METHOD_AddTimer:

	.ifndef	RTOS_OPTIMIZE_FOR_SMALL_MEMORY
		PUSH	temp1
		PUSH	temp2
		PUSH	temp3
		PUSH	temp4
		PUSH	temp5
		PUSH	ZL
		PUSH	ZH
	.endif

		;=== начало критической секции			; (примечание: чтений+запись разделяемой памяти - должны быть в единой транзакции!)
		INR	temp3,	SREG				; Save global interrupt flag
		CLI						; запрещаем прерывания

		; Цикл: ищем существующий Таймер (для апдейта), а заодно, ищем пустую ячейку (для нового)
		LDI	ZL,	Low (RTOS_TimersPool)		; Загрузить адрес начала "пула таймеров" -> в регистр Z
		LDI	ZH,	High(RTOS_TimersPool)
		LDI	temp4,	0				; Установить признак "ещё не нашёл" указатель на пустую ячейку в пуле, для нового таймера	(temp4=temp5=0)
		LDI	temp5,	0
		LDI	temp2,	RTOS_TimersPoolSize		; переменная цикла (инициализация)
	Loop__RTOS_METHOD_AddTimer:
		LD	temp1,	Z				; прочитать "Идентификатор Задачи" очередного таймера

		CP	temp1,	TaskCode			; проверить: совпадает ли с искомой Задачей?
		BREQ	UpdateExisting__RTOS_METHOD_AddTimer	; если совпадает - идём апдейтить этот таймер...

		CPI	temp1,	$00				; проверка: ячейка пустая?
		BRNE	NotEmpty__RTOS_METHOD_AddTask
		MOV	temp4,	ZL				; если нашёл пустую ячейку - запоминаем её адрес, на будущее...
		MOV	temp5,	ZH	
	NotEmpty__RTOS_METHOD_AddTask:

		SUBI	ZL,	Low(-3)				; Переходим к следующему таймеру:
		SBCI	ZH,	High(-3)			; 	адрес Z+=2
		DEC	temp2					; 	переменная цикла (декрементация)
		BRNE	Loop__RTOS_METHOD_AddTimer
		; (состояние: существующий Таймер не найден - нужно добавить новый...)

		MOV	ZL,	temp4				; переводим указатель Z на последнюю пустую ячейку, найденную ранее	(примечание: получается, что пул будет заполняться с конца, но это не важно)
		MOV	ZH,	temp5
		OR	temp4,	temp5				; проверить статус: если "не нашёл" указатель на пустую ячейку в пуле?	(temp4==0 and temp5==0 ?)
		BRNE	FillNew__RTOS_METHOD_AddTimer		; если таки нашёл - перейти к заполнению нового таймера...

	.ifndef	RTOS_OPTIMIZE_FOR_SMALL_MEMORY
		; Если прошли весь пул, но так и не нашли пустой ячейки - ошибка!
		LDI	ErrorCode,	20			; код: 20 - "пул Таймеров" переполнен, не могу зарегистрировать новый Таймер!
		RCALL	RTOS_METHOD_ErrorHandler
	.endif
		RJMP	End__RTOS_METHOD_AddTimer

		; Запрограммировать Таймер
	FillNew__RTOS_METHOD_AddTimer:
		ST	Z,	TaskCode
	UpdateExisting__RTOS_METHOD_AddTimer:
		STD	Z+1,	TimeDelayLow
		STD	Z+2,	TimeDelayHigh

	End__RTOS_METHOD_AddTimer:
		OUTR	SREG,	temp3				; Restore global interrupt flag
		;=== конец критической секции

	.ifndef	RTOS_OPTIMIZE_FOR_SMALL_MEMORY
		POP	ZH
		POP	ZL
		POP	temp5
		POP	temp4
		POP	temp3
		POP	temp2
		POP	temp1
	.endif
		RET



.ifndef	RTOS_OPTIMIZE_FOR_SMALL_MEMORY	; Примечание: данный метод является второстепенным, без него можно обойтись.
;---------------------------------------------------------------------------
;
; Убрать (удалить) Таймер из пула RTOS_TimersPool, 
; для отмены отложенного запуска Задачи
; 
;---------------------------------------------------------------------------

;----- Subroutine Register Variables

;.def	TaskCode	= (см.выше)	; (входной, не изменяется)	Номер-индекс Задачи в таблице RTOS_TaskProcs

; Памятка: кроме параметровых, больше регистров не портит - поскольку защищает используемые регистры Стеком.
; Памятка: SREG сохраняется.

;----- Code


RTOS_METHOD_RemoveTimer:

		PUSHF						; сохранить temp1 и SREG в стеке	(замечу: заодно, сохраняется текущее состояние "флага прерываний")
		PUSH	temp2
		PUSH	ZH
		PUSH	ZL

		LDI	ZL,	Low (RTOS_TimersPool)
		LDI	ZH,	High(RTOS_TimersPool)
		LDI	temp2,	RTOS_TimersPoolSize		; переменная цикла

		;=== начало критической секции
		CLI						; запрещаем прерывания
	Loop__RTOS_METHOD_RemoveTimer:
		LD	temp1,	Z				; прочитать "Идентификатор Задачи" очередного таймера
		
		CP	temp1,	TaskCode			; проверить: совпадает ли с искомой Задачей?
		BRNE	NotFound__RTOS_METHOD_RemoveTimer
		LDI	temp1,	$00				; = спец.код "Нет Задачи"
		ST	Z,	temp1				; выключаем Таймер
		RJMP	End__RTOS_METHOD_RemoveTimer		; и сразу идём на выход, т.к. Таймер с таким идентификатором может быть только один...
	NotFound__RTOS_METHOD_RemoveTimer:

		SUBI	ZL,	Low(-3)				; Переходим к следующему таймеру:
		SBCI	ZH,	High(-3)			; 	адрес Z+=2
		DEC	temp2					; 	переменная цикла (декрементация)
		BRNE	Loop__RTOS_METHOD_RemoveTimer
		; (состояние: перебрал все таймеры, не нашёл искомого - пофиг, идём на выход...)
	
	End__RTOS_METHOD_RemoveTimer:
		POP	ZL
		POP	ZH
		POP	temp2
		POPF						; Restore global interrupt flag
		;=== конец критической секции
		RET



.endif	//RTOS_OPTIMIZE_FOR_SMALL_MEMORY

.endif	//RTOS_FEATURE_PLANTASKTIMED

.ifdef	RTOS_FEATURE_PLANTASKWAITER
;***************************************************************************
;*
;*  Флаговая автоматизация
;*
;***************************************************************************


; TODO:
;RTOS_METHOD_ClearWaiterQueue
;RTOS_METHOD_AddWaiter
;RTOS_METHOD_RemoveWaiter



.endif	//RTOS_FEATURE_PLANTASKWAITER

;***************************************************************************
;*
;*  Общее системное
;*
;***************************************************************************


.ifndef	RTOS_OPTIMIZE_FOR_SMALL_MEMORY	; Примечание: данный метод является второстепенным, без него можно обойтись.
;---------------------------------------------------------------------------
; 
; Если при выполнении кода ядра RTOS произошла системная ошибка (kernel panic)? 
; (например: очередь задач уже заполнена, а прикладной код вызвал RTOS_METHOD_AddTask ?)
; 
; Сбойная операция, в таких случаях, прерывается, стараясь сохранить/восстановить критические системные данные.
; И также, этот ErrorHandler-метод вызывается из кода ядра, передавая ему код ошибки.
; Можно здесь, каким-то образом, просигнализировать об ошибке пользователю, записать в лог, или забить (по-умолчанию)...
; 
;---------------------------------------------------------------------------

;----- Subroutine Register Variables

;.def	ErrorCode	= (см.выше)	; (входной, не изменяется)			Код системной ошибки
;.def	TaskCode	= (см.выше)	; [входной, не обязательный, не изменяется]	Номер-индекс Задачи, при манипуляциях с которой произошла ошибка

; Памятка: также использует/портит содержимое регистров...

;----- Code

	; Реестр используемых кодов ошибок (возможные значения ErrorCode):
	; 	 0 - нет ошибки
	; 	10 - "очередь Задач" переполнена, не могу добавить новую Задачу!
	; 	20 - "пул Таймеров"   переполнен, не могу зарегистрировать новый Таймер!
	; 	30 - "пул Выжидающих" переполнен, не могу зарегистрировать нового Выжидающего!

RTOS_METHOD_ErrorHandler:
		RET

.endif	//RTOS_OPTIMIZE_FOR_SMALL_MEMORY




;=== END "RTOS-kernel.inc" =================================================
; coded by (c) DI HALT, 2008 @ http://easyelectronics.ru/
; coded by (c) Celeron, 2014 @ http://we.easyelectronics.ru/my/Celeron/
