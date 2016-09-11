.MESSAGE "<-- RTOS ver.2.1.0 alpha -->"
;=== BEGIN "RTOS-kernel.inc" ===============================================

; Ядро RTOS
; (Предупреждение: сюда вам лазить не нужно, непосредственно, эти методы не используются в прикладном коде!)



.ifdef	RTOS_FEATURE_PLANTASK
;***************************************************************************
;*
;*  Базовая диспетчеризация задач,
;*  строго необходима для работы всех функций RTOS
;*
;***************************************************************************



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
; Памятка: в режиме  с оптимизацией RTOS_OPTIMIZE_FOR_SMALL_MEMORY: защита стеком отключена (Стек вообще не используется внутри метода!) - портит содержимое регистров TEMP1, TEMP2, TEMP3, TEMP4 и ZH:ZL.
; Примечание: хотя, в данном случае, необязательно сберегать регистры, т.к. данный метод вызывается из суперцикла "MAIN", между запусками прикладных Задач - когда сбережение регистров, обычно, не требуется.
; Памятка: SREG сохраняется всегда.

;----- Code


RTOS_METHOD_ProcessTaskQueue:

	.ifndef	RTOS_OPTIMIZE_FOR_SMALL_MEMORY					; Эксперимент! Хотя вообще-то, сберегать регистры - излишне, в данном (не интерактивном) методе...
		PUSH	temp1
		PUSH	temp2
		PUSH	temp3
		PUSH	temp4
	.endif

		;=== начало критической секции					; (примечание: чтений+запись разделяемой памяти - должны быть в единой транзакции!)
		INR	temp4,	SREG						; Save global interrupt flag
		CLI								; запрещаем прерывания

		; Загрузить код текущей Задачи к исполнению:
		; Примечание: при простое ядра и пустой очереди, будет прочитан код текущей задачи == 0 (спец.код "Нет Задачи") - в таком случае, далее, будет осуществлён запуск системной задачи "холостой цикл" (Task_Idle)...
		LDI	ZL,	Low (RTOS_TaskQueue)				; Загрузить адрес начала "очереди задач" -> в регистр Z
		LDI	ZH,	High(RTOS_TaskQueue)
		LD	temp3,	Z						; Загрузить первый байт из очереди RTOS_TaskQueue[0] -> temp3

		; Сдвинуть "очередь задач" на одну позицию назад:
		LDI	temp2,	RTOS_TaskQueueSize-1				; переменная цикла (инициализация)
	Loop__RTOS_METHOD_ProcessTaskQueue:
		LDD	temp1,	Z+1 						; RTOS_TaskQueue[i+1] -> temp1
		ST	Z+,	temp1						; temp1 -> RTOS_TaskQueue[i++]
		CPI	temp1,	$00						; Сравниваем RTOS_TaskQueue[i] со спец.кодом "Нет Задачи"
		BREQ	EndShift__RTOS_METHOD_ProcessTaskQueue			; Если равен? Значит, хвост очереди пуст - пропускаем его сдвиг...
		DEC	temp2							; переменная цикла (декрементация)
		BRNE	Loop__RTOS_METHOD_ProcessTaskQueue
		ST	Z,	temp2						; в последний элемент очереди поместить спец.код "Нет Задачи":	RTOS_TaskQueue[RTOS_TaskQueueSize]=0	(замечу, что сейчас, temp2==0)
	EndShift__RTOS_METHOD_ProcessTaskQueue:
	
		OUTR	SREG,	temp4						; Restore global interrupt flag
		;=== конец критической секции


		; Получить реальный адрес точки входа в подпрограмму Задачи:
		; Примечание: алгоритм реализован на "индексных переходах" - см. объяснение приёма в "AVR. Учебный курс. Ветвления на индексных переходах" (с) http://easyelectronics.ru/avr-uchebnyj-kurs-vetvleniya.html
		; (текущее состояние: temp3 == код текущей Задачи к исполнению)

	.ifdef	MAPPED_FLASH_START
		; Вариант кода для AVR8L Instruction Set: где нет РОН R0:R15, и нет инструкции LPM (вместо неё LD).
		; see recipe: http://www.avrfreaks.net/index.php?module=PNphpBB2&file=viewtopic&t=82385&highlight=
		; проверено: это работает на ATtiny10!
		
		MOV	ZL,	temp3						; индекс целевой Задачи -> ZL
		CLR	ZH
		LSL	ZL							; temp1 *= 2
		ADC	ZH,	ZH						; saving carry bit	(Замечу: в инструкции, я могу использовать регистр-приёмник ZH также и как второй регистр-источник. РОН и АЛУ - разные вещи. И здесь это корректно арифметически, потому что он содержит =0, в этой время.)
		SUBI	ZL,	Low (-(RTOS_TaskProcs*2 + MAPPED_FLASH_START))
		SBCI	ZH,	High(-(RTOS_TaskProcs*2 + MAPPED_FLASH_START))	; Add
		
		LD	temp1,	Z+						; загрузить из "индексной таблицы" мл.байт адреса перехода -> temp1
		LD	temp2,	Z						; загрузить из "индексной таблицы" ст.байт адреса перехода -> temp2
		
	.else	//Вариант кода для остальных AVR

		MOV	ZL,	temp3						; индекс целевой Задачи -> ZL
		CLR	ZH
		LSL	ZL							; temp1 *= 2
		ADC	ZH,	ZH						; saving carry bit	(Замечу: в инструкции, я могу использовать регистр-приёмник ZH также и как второй регистр-источник. РОН и АЛУ - разные вещи. И здесь это корректно арифметически, потому что он содержит =0, в этой время.)
		SUBI	ZL,	Low (-RTOS_TaskProcs*2)
		SBCI	ZH,	High(-RTOS_TaskProcs*2)				; Add
	
		LPM	temp1,	Z+						; загрузить из "индексной таблицы" мл.байт адреса перехода -> temp1
		LPM	temp2,	Z						; загрузить из "индексной таблицы" ст.байт адреса перехода -> temp2
	.endif

		; Перейти к исполнению Задачи:

		MOV	ZL,	temp1						; Загрузить адрес точки входа в Задачу -> в регистр Z
		MOV	ZH,	temp2

		OUTR	SREG,	temp4						; Restore SREG also (to be transparent)
	.ifndef	RTOS_OPTIMIZE_FOR_SMALL_MEMORY
		POP	temp4
		POP	temp3
		POP	temp2
		POP	temp1
	.endif

		; Трюк: Вызов подпрограммы, с переназначенной "точкой выхода" из неё.
		; Суть: Вместо стандартного RCALL, мы входим в подпрограмму простым RJMP, предварительно затолкнув в стек адрес желаемой "точки выхода".
		; Применимо здесь: посколько вызов вложенной подпрограммы осуществляется прямо перед выходом RET из этой головной процедуры. Таким образом, CALL + RET === JMP

		IJMP 								; Запуск Задачи!!!
										; Замечу, при этом, минимализируется использование стека:
										; 	Обрати внимание - сюда мы пришли по RCALL из главного цикла.
										; 	Значит, в стеке у нас лежит адрес возврата...
										; 	А уходим в задачу по IJMP, который стек не меняет.
										; 	Но это не страшно - ведь из Задачи мы выйдем по RET!



;---------------------------------------------------------------------------
;
; Добавить Задачу в очередь RTOS_TaskQueue, 
; послать на выполнение
;
;---------------------------------------------------------------------------

;----- Subroutine Register Variables

;.def	TaskCode	= (см.выше)	; (входной, не изменяется)	Номер-индекс Задачи в таблице RTOS_TaskProcs

; Памятка: в режиме без оптимизации RTOS_OPTIMIZE_FOR_SMALL_MEMORY: кроме параметровых, больше регистров не портит - поскольку защищает используемые регистры Стеком.
; Памятка: в режиме  с оптимизацией RTOS_OPTIMIZE_FOR_SMALL_MEMORY: защита стеком отключена (Стек вообще не используется внутри метода!) - портит содержимое регистров TEMP1, TEMP2, ZH:ZL.
; Памятка: SREG сохраняется всегда.

;----- Code


RTOS_METHOD_AddTask:

	.ifndef	RTOS_OPTIMIZE_FOR_SMALL_MEMORY
		PUSH	temp1
		PUSH	temp2
		PUSH	ZL
		PUSH	ZH
	.endif

		;=== начало критической секции					; (примечание: чтений+запись разделяемой памяти - должны быть в единой транзакции!)
		INR	temp2,	SREG						; Save global interrupt flag
		CLI								; запрещаем прерывания

		; Сканируем очередь сначала до конца, ищем первую пустую ячейку:
		LDI	ZL,	Low (RTOS_TaskQueue)
		LDI	ZH,	High(RTOS_TaskQueue)
	Loop__RTOS_METHOD_AddTask:
		LD	temp1,	Z+
		CPI	temp1,	$00						; проверка: ячейка пустая?
		BREQ	Found__RTOS_METHOD_AddTask				; если пустая - прерываем сканирование на этой позиции...
		CPI	ZL,	Low (RTOS_TaskQueue + RTOS_TaskQueueSize)	; условие цикла:
		BRNE	Loop__RTOS_METHOD_AddTask				; 	ПОКА адрес в Z не дойдёт до конца данных...
		CPI	ZH,	High(RTOS_TaskQueue + RTOS_TaskQueueSize)	; (трюк: замечу, здесь не используется традиционная переменная-счётчик, чтобы сэкономить регистр)
		BRNE	Loop__RTOS_METHOD_AddTask

		; (состояние: прошли всю очередь, но так и не нашли пустой ячейки - ошибка!)
	.ifndef	RTOS_OPTIMIZE_FOR_SMALL_MEMORY
		PUSH	ErrorCode
		LDI	ErrorCode,	10					; код ошибки: 10 - "очередь Задач" переполнена, не могу добавить новую Задачу!
		RCALL	RTOS_METHOD_ErrorHandler
		POP	ErrorCode
	.endif
		RJMP	End__RTOS_METHOD_AddTask

		; Если нашли пустое место (фактически, это текущий "хвост" очереди) - то дополняем очередь требуемой Задачей:
	Found__RTOS_METHOD_AddTask:
		ST	-Z,	TaskCode

	End__RTOS_METHOD_AddTask:
		OUTR	SREG,	temp2						; Restore global interrupt flag
		;=== конец критической секции

	.ifndef	RTOS_OPTIMIZE_FOR_SMALL_MEMORY
		POP	ZH
		POP	ZL
		POP	temp2
		POP	temp1
	.endif
		RET




;***************************************************************************
;*
;*  Условная Диспетчеризация Задач
;*
;***************************************************************************



.if defined(RTOS_FEATURE_PLANTIMER) || defined(RTOS_FEATURE_PLANWAITER)
;---------------------------------------------------------------------------
; 
; Служба таймеров RTOS
; (примечание: данный системный метод работает в связке с обработчиком прерывания RTOS_TIMER_SERVICE)
; 
;---------------------------------------------------------------------------

;----- Subroutine Register Variables

; Без параметров.

; Памятка: в режиме без оптимизации RTOS_OPTIMIZE_FOR_SMALL_MEMORY: не портит содержимое РОН - поскольку защищает используемые регистры Стеком.
; Памятка: в режиме  с оптимизацией RTOS_OPTIMIZE_FOR_SMALL_MEMORY: защита стеком отключена - портит содержимое регистров TEMP1, TEMP2, TEMP3, YH:YL, ZH:ZL и TaskCode.
; Примечание: хотя, в данном случае, необязательно сберегать регистры, т.к. данный метод вызывается из суперцикла "MAIN", между запусками прикладных Задач - когда сбережение регистров, обычно, не требуется.
; Памятка: SREG сохраняется всегда.

;----- Code


RTOS_METHOD_TimerService:

	.ifndef	RTOS_OPTIMIZE_FOR_SMALL_MEMORY
		PUSH	temp1
		PUSH	temp3
	.endif
		;=== начало критической секции					; (примечание: чтение+запись разделяемой памяти - должны быть в единой транзакции!)
		INR	temp3,	SREG						; Save global interrupt flag
		CLI								; запрещаем прерывания
		
		LDS	temp1,	RTOS_TimerServiceCounter			; Счётчик времени "количество мс, с момента прошлой отработки данного метода" -> temp1
		TST	temp1
		BRNE	DoProcessing__RTOS_METHOD_TimerService			; Если "уже прошла 1мс или более", то надо обработать все Пулы...
		RJMP	Exit__RTOS_METHOD_TimerService				; Иначе, идём на выход...
	DoProcessing__RTOS_METHOD_TimerService:
		
	.ifndef	RTOS_OPTIMIZE_FOR_SMALL_MEMORY
		PUSH	YL	
		PUSH	YH
		PUSH	ZL
		PUSH	ZH
		PUSH	temp2
		PUSH	TaskCode
	.endif


.ifdef	RTOS_FEATURE_PLANTIMER

		;== Обработать Таймеры
		; см. описание идеи http://easyelectronics.ru/avr-uchebnyj-kurs-operacionnaya-sistema-tajmernaya-sluzhba.html

		LDI	YL,	Low (RTOS_TimersPool)				; Загрузить адрес начала "пула Таймеров" -> в регистр Y
		LDI	YH,	High(RTOS_TimersPool)

	LoopTimer__RTOS_METHOD_TimerService:
		LD	temp1,	Y						; прочитать "Идентификатор Задачи" очередного таймера -> temp1

		CPI	temp1,	$00						; проверка: если "Нет Задачи"?
		BREQ	SkipTimer__RTOS_METHOD_TimerService			; то переход к следующему элементу...


		; (состояние: текущий элемент непустой - надо декрементировать его счётчик)
		LDS	temp1,	RTOS_TimerServiceCounter			; Счётчик времени "количество мс, с момента прошлой отработки данного метода" -> temp1

		LDD	ZL,	Y+1						; 
		SUB	ZL,	temp1 						; Уменьшить мл.байт "счётчика таймера"
		STD	Y+1,	ZL						;

		LDD	ZH,	Y+2						;
		SBCI	ZH,	0						; Уменьшить ст.байт "счётчика таймера"	(если был заём разряда "carry bit")
		STD	Y+2,	ZH						;

		BRCC 	SkipTimer__RTOS_METHOD_TimerService			; Если не было заёма ("счётчик таймера">=0), то счёт ещё не окончен...


		; (состояние: "счётчик таймера" досчитал до нуля - запустить связанную Задачу)
		LD	TaskCode,	Y					; Заполняем параметры
		RCALL	RTOS_METHOD_AddTask					; Вызываем системный метод	(Замечу: он использует/изменяет регистры TEMP1, TEMP2, ZH:ZL и TaskCode.)
		
		LDI	temp1,	$00
		ST	Y,	temp1						; Задача обработана - Таймер самоудаляется...


	SkipTimer__RTOS_METHOD_TimerService:
		SUBI	YL,	Low (-3)					; Переходим к следующему таймеру:
		SBCI	YH,	High(-3)					; 	адрес Y+=3
		CPI	YL,	Low (RTOS_TimersPool + RTOS_TimersPoolSize*3)	; условие цикла:
		BRNE	LoopTimer__RTOS_METHOD_TimerService			; 	ПОКА адрес в Y не дойдёт до конца данных...
		CPI	YH,	High(RTOS_TimersPool + RTOS_TimersPoolSize*3)	; (трюк: замечу, здесь, не используется традиционная переменная-счётчик, чтобы сэкономить регистр)
		BRNE	LoopTimer__RTOS_METHOD_TimerService

.endif	//RTOS_FEATURE_PLANTIMER



.ifdef	RTOS_FEATURE_PLANWAITER

		;== Обработать Выжидатели

		LDI	YL,	Low (RTOS_WaitersPool)				; Загрузить адрес начала "пула Выжидателей" -> в регистр Y
		LDI	YH,	High(RTOS_WaitersPool)

	LoopWaiter__RTOS_METHOD_TimerService:
		LD	temp1,	Y						; прочитать "Идентификатор Задачи" очередного выжидателя -> temp1

		CPI	temp1,	$00						; проверка: если "Нет Задачи"?
		BREQ	SkipWaiter__RTOS_METHOD_TimerService			; то переход к следующему элементу...


		; (состояние: текущий элемент непустой - надо проверить его Условие)
		LDD	ZL,	Y+1						; Устанавливаем "12-битный адрес тестируемой ячейки памяти" -> Z
		LDD	ZH,	Y+2
		ANDI	ZH,	(0b1111<<WAITER_ADDRESS_HIGH)
		LD	ZL,	Z						; Вычитываем "содержимое тестируемого байта" -> ZL (во временную переменную)

		LDD	ZH,	Y+2
		BST	ZH,	WAITER_BIT_STATE				; "Ожидаемое состояние бита" -> T
		ANDI	ZH,	(0b111<<WAITER_BIT_POSITION)
		SWAP	ZH							; "Номер позиции [0..7] тестируемого бита" -> ZH (во временную переменную)

		; Замечу: инструкции INC/DEC не переписывают "Carry Bit" (используемый арифметическими инструкциями)!
		; Manual: The C flag in SREG is not affected by the operation, thus allowing the INC/DEC instruction to be used on a loop counter in multiple-precision computations.
		ROL	ZL							; (предварительная коррекция)
		INC	ZH
	ShiftBitForWaiter__RTOS_METHOD_TimerService:
		ROR	ZL							; сдвинуть вправо (с использованием "Carry Bit")
		DEC	ZH							; переменная цикла (декрементация)
		BRNE	ShiftBitForWaiter__RTOS_METHOD_TimerService

		ANDI	ZL,	(1<<0)						; "Тестируемый бит" -> в нулевой разряд ZL
		BLD	ZH,	0						; "Ожидаемое состояние бита" -> в нулевой разряд ZH	(замечу, после цикла, ZH=0)
		EOR	ZL,	ZH						; Операция "исключающего или" такова: Если биты совпадают, то ZL станет нулевым.
		BRNE 	SkipWaiter__RTOS_METHOD_TimerService			; Если не ноль - Условие не сработало...


		; (состояние: Условие сработало - запустить связанную Задачу)
		LD	TaskCode,	Y					; Заполняем параметры
		RCALL	RTOS_METHOD_AddTask					; Вызываем системный метод	(Замечу: он использует/изменяет регистры TEMP1, TEMP2, ZH:ZL и TaskCode.)

		LDI	temp1,	$00
		ST	Y,	temp1						; Задача обработана - Выжидатель самоудаляется...


	SkipWaiter__RTOS_METHOD_TimerService:
		SUBI	YL,	Low (-3)					; Переходим к следующему таймеру:
		SBCI	YH,	High(-3)					; 	адрес Y+=3
		CPI	YL,	Low (RTOS_WaitersPool + RTOS_WaitersPoolSize*3)	; условие цикла:
		BRNE	LoopWaiter__RTOS_METHOD_TimerService			; 	ПОКА адрес в Y не дойдёт до конца данных...
		CPI	YH,	High(RTOS_WaitersPool + RTOS_WaitersPoolSize*3)	; (трюк: замечу, здесь, не используется традиционная переменная-счётчик, чтобы сэкономить регистр)
		BRNE	LoopWaiter__RTOS_METHOD_TimerService

.endif	//RTOS_FEATURE_PLANWAITER


	.ifndef	RTOS_OPTIMIZE_FOR_SMALL_MEMORY
		POP	TaskCode
		POP	temp2
		POP	ZH
		POP	ZL
		POP	YH
		POP	YL
	.endif
		LDI	temp1, 0
		STS	RTOS_TimerServiceCounter, temp1				; Обработка произведена - обнулим Счётчик.
		
	Exit__RTOS_METHOD_TimerService:
		OUTR	SREG,	temp3						; Restore global interrupt flag
		;=== конец критической секции
	.ifndef	RTOS_OPTIMIZE_FOR_SMALL_MEMORY
		POP	temp3
		POP	temp1
	.endif
		RJMP	RTOS_METHOD_TimerService_ExitPoint			; Экономим стек 
										; 	(Этот трюк возможен: поскольку данный метод RTOS может вызываться только из одного фиксированного места в коде "рабочего цикла".)
										; 	(А стек нам необходимо иметь в запасе: поскольку из кода данного метода вызывается вложенная подпрограмма RTOS_METHOD_AddTask...)
.endif	//RTOS_FEATURE_PLANTIMER || RTOS_FEATURE_PLANWAITER



;---------------------------------------------------------------------------
;
; ЗАПУСК отложенного выполнения ЗАДАЧИ
; 
; RTOS_METHOD_AddTimer:		Установить Таймер в пуле RTOS_TimersPool
; 
; RTOS_METHOD_AddWaiter:	Установить Выжидатель в пуле RTOS_WaitersPool
; 
;---------------------------------------------------------------------------

;----- Subroutine Register Variables

;.def	TaskCode		= (см.выше)	; (входной, не изменяется)	Номер-индекс Задачи в таблице RTOS_TaskProcs
;.def	PoolParameter2byte	= (см.выше)	; (входной, не изменяется)	2-ой байт элемента Пула (описание формата см. в RTOS_data.inc)
;.def	PoolParameter3byte	= (см.выше)	; (входной, не изменяется)	3-ий байт элемента Пула (описание формата см. в RTOS_data.inc)

; Памятка: в режиме без оптимизации RTOS_OPTIMIZE_FOR_SMALL_MEMORY: кроме параметровых, больше регистров не портит - поскольку защищает используемые регистры Стеком.
; Памятка: в режиме  с оптимизацией RTOS_OPTIMIZE_FOR_SMALL_MEMORY: защита стеком отключена (Стек вообще не используется внутри метода!) - портит содержимое регистров TEMP1, TEMP2, TEMP3, TEMP4, ZH:ZL.
; Памятка: SREG сохраняется всегда.

;----- Code


.ifdef	RTOS_FEATURE_PLANTIMER
RTOS_METHOD_AddTimer:
	.ifndef	RTOS_OPTIMIZE_FOR_SMALL_MEMORY
		PUSH	ZL
		PUSH	ZH
		PUSH	temp1
		PUSH	temp2
		PUSH	temp3
		PUSH	temp4
	.endif
		LDI	ZL,	Low (RTOS_TimersPool + RTOS_TimersPoolSize*3)	; Загрузить адрес конца Пула -> в регистр Z
		LDI	ZH,	High(RTOS_TimersPool + RTOS_TimersPoolSize*3)
		LDI	temp2,	RTOS_TimersPoolSize				; переменная цикла (инициализация)
		RJMP	RTOS_METHOD_AddPoolElement
.endif	//RTOS_FEATURE_PLANTIMER



.ifdef	RTOS_FEATURE_PLANWAITER
RTOS_METHOD_AddWaiter:
	.ifndef	RTOS_OPTIMIZE_FOR_SMALL_MEMORY
		PUSH	ZL
		PUSH	ZH
		PUSH	temp1
		PUSH	temp2
		PUSH	temp3
		PUSH	temp4
	.endif
		LDI	ZL,	Low (RTOS_WaitersPool + RTOS_WaitersPoolSize*3)	; Загрузить адрес конца Пула -> в регистр Z
		LDI	ZH,	High(RTOS_WaitersPool + RTOS_WaitersPoolSize*3)
		LDI	temp2,	RTOS_WaitersPoolSize				; переменная цикла (инициализация)
		;RJMP	RTOS_METHOD_AddPoolElement
.endif	//RTOS_FEATURE_PLANWAITER



.if defined(RTOS_FEATURE_PLANTIMER) || defined(RTOS_FEATURE_PLANWAITER)
RTOS_METHOD_AddPoolElement:
		;=== начало критической секции					; (примечание: чтение+запись разделяемой памяти - должны быть в единой транзакции!)
		INR	temp4,	SREG						; Save global interrupt flag
		CLI								; запрещаем прерывания


		; Цикл: ищем существующий Элемент (для апдейта), а заодно, ищем пустую ячейку (для нового)
		LDI	temp3,	0						; Установить признак "ещё не нашёл" указатель на пустую ячейку в Пуле	(temp3=0)
	Loop__RTOS_METHOD_AddPoolElement:
		SUBI	ZL,	Low (3)						; Переходим к следующему :
		SBCI	ZH,	High(3)						; 	адрес Z-=3

		LD	temp1,	Z						; прочитать "Идентификатор Задачи" очередного Элемента
		CP	temp1,	TaskCode					; проверить: совпадает ли с искомой Задачей?
		BREQ	UpdateExisting__RTOS_METHOD_AddPoolElement		; если совпадает - идём апдейтить этот Элемент...

		CPI	temp1,	$00						; проверка: ячейка пустая?
		BRNE	NotEmpty__RTOS_METHOD_AddPoolElement
		MOV	temp3,	temp2						; если нашёл пустую ячейку - запоминаем её смещение, на будущее...	Замечу: т.к. присваивание temp3=<счётчик> происходит ДО декрементации счётчика, то в temp3=(<фактический индекс> + 1).
	NotEmpty__RTOS_METHOD_AddPoolElement:

		DEC	temp2							; переменная цикла (декрементация)
		BRNE	Loop__RTOS_METHOD_AddPoolElement


		; (состояние: существующий Элемент не найден - нужно добавить новый...)
		TST	temp3							; проверить статус: если "не нашёл" указатель на пустую ячейку в пуле?	(temp3==0 ?)	Замечу: что случайно temp3==0 быть не может - т.к. даже при <фактическом индексе>==0, тем не менее temp3==1.
		BRNE	FillNew__RTOS_METHOD_AddPoolElement			; если таки нашёл - перейти к заполнению нового Элемента


		; (состояние: прошли весь пул, но так и не нашли пустой ячейки - ошибка!)
	.ifndef	RTOS_OPTIMIZE_FOR_SMALL_MEMORY
		PUSH	ErrorCode
		LDI	ErrorCode,	20					; код ошибки: 20 - "Пул" переполнен, не могу зарегистрировать новый Элемент!	(Примечание: сейчас Z==<адресу начала Пула>, для идентификации: Какой именно Пул?)
		RCALL	RTOS_METHOD_ErrorHandler
		POP	ErrorCode
	.endif
		RJMP	End__RTOS_METHOD_AddPoolElement


	FillNew__RTOS_METHOD_AddPoolElement:
		DEC	temp3							; произведена коррекция, теперь:		temp3 = <фактический индекс> пустой ячейки
		ADD	temp2,	temp3						; смещение в байтах, относительно начала пула:	temp2 = 3*temp3
		ADD	temp2,	temp3						; 	(замечу: после цикла, было temp2==0)
		ADD	temp2,	temp3						; 	(замечу: temp3<=85, поэтому через границу байта мы не вылезем, 3*85=255)
		CLR	temp3
		ADD	ZL,	temp2						; Перевести указатель Z на последнюю пустую ячейку, найденную ранее
		ADC	ZH,	temp3						; 	(замечу: после цикла, было Z==<адресу начала Пула>)
		ST	Z,	TaskCode					; Запрограммировать Элемент
	UpdateExisting__RTOS_METHOD_AddPoolElement:
		STD	Z+1,	PoolParameter2byte
		STD	Z+2,	PoolParameter3byte


	End__RTOS_METHOD_AddPoolElement:
		OUTR	SREG,	temp4						; Restore global interrupt flag
		;=== конец критической секции
	.ifndef	RTOS_OPTIMIZE_FOR_SMALL_MEMORY
		POP	temp4
		POP	temp3
		POP	temp2
		POP	temp1
		POP	ZH
		POP	ZL
	.endif
		RET
.endif	//RTOS_FEATURE_PLANTIMER || RTOS_FEATURE_PLANWAITER





.ifndef	RTOS_OPTIMIZE_FOR_SMALL_MEMORY	; Примечание: данные методы является второстепенными, без них можно обойтись.
;***************************************************************************
;*
;*  Второстепенные (необязательные и отладочные) методы RTOS
;*
;***************************************************************************



;---------------------------------------------------------------------------
; 
; ОТМЕНА отложенного выполнения ЗАДАЧИ
; 
; RTOS_METHOD_RemoveTimer:	Удалить Таймер из пула RTOS_TimersPool
; 
; RTOS_METHOD_RemoveWaiter:	Удалить Выжидатель из пула RTOS_WaitersPool
; 
;---------------------------------------------------------------------------

;----- Subroutine Register Variables

;.def	TaskCode	= (см.выше)	; (входной, не изменяется)	Номер-индекс Задачи в таблице RTOS_TaskProcs

; Памятка: кроме параметровых, больше регистров не портит - поскольку защищает используемые регистры Стеком.
; Памятка: SREG сохраняется всегда.

;----- Code


.ifdef	RTOS_FEATURE_PLANTIMER
RTOS_METHOD_RemoveTimer:
		PUSH	ZL
		PUSH	ZH
		PUSH	temp1
		LDI	ZL,	Low (RTOS_TimersPool)
		LDI	ZH,	High(RTOS_TimersPool)
		LDI	temp1,	RTOS_TimersPoolSize				; переменная цикла (инициализация)
		RJMP	RTOS_METHOD_RemovePoolElement
.endif	//RTOS_FEATURE_PLANTIMER


.ifdef	RTOS_FEATURE_PLANWAITER
RTOS_METHOD_RemoveWaiter:
		PUSH	ZL
		PUSH	ZH
		PUSH	temp1
		LDI	ZL,	Low (RTOS_WaitersPool)
		LDI	ZH,	High(RTOS_WaitersPool)
		LDI	temp1,	RTOS_WaitersPoolSize				; переменная цикла (инициализация)
		;RJMP	RTOS_METHOD_RemovePoolElement
.endif	//RTOS_FEATURE_PLANWAITER


.if defined(RTOS_FEATURE_PLANTIMER) || defined(RTOS_FEATURE_PLANWAITER)
RTOS_METHOD_RemovePoolElement:
		PUSH	temp2
		PUSH	temp3
		;=== начало критической секции
		INR	temp3,	SREG						; Save global interrupt flag
		CLI								; запрещаем прерывания
	Loop__RTOS_METHOD_RemovePoolElement:
		LD	temp2,	Z						; прочитать "Идентификатор Задачи" очередного Элемента
		
		CP	temp2,	TaskCode					; проверить: совпадает ли с искомой Задачей?
		BRNE	NotFound__RTOS_METHOD_RemovePoolElement
		LDI	temp2,	$00						; = спец.код "Нет Задачи"
		ST	Z,	temp2						; выключаем Элемент
		RJMP	End__RTOS_METHOD_RemovePoolElement			; и сразу идём на выход, т.к. Элемент с таким идентификатором может быть только один...
	NotFound__RTOS_METHOD_RemovePoolElement:
		
		SUBI	ZL,	Low (-3)					; Переходим к следующему Элементу:
		SBCI	ZH,	High(-3)					; 	адрес Z+=2
		DEC	temp1							; 	переменная цикла (декрементация)
		BRNE	Loop__RTOS_METHOD_RemovePoolElement
		; (состояние: перебрал все Элементы, не нашёл искомого - пофиг, идём на выход...)
		
	End__RTOS_METHOD_RemovePoolElement:
		OUTR	SREG,	temp3						; Restore global interrupt flag
		;=== конец критической секции
		POP	temp3
		POP	temp2
		POP	temp1
		POP	ZH
		POP	ZL
		RET
.endif	//RTOS_FEATURE_PLANTIMER || RTOS_FEATURE_PLANWAITER



;---------------------------------------------------------------------------
; 
; ОЧИСТКА ПАМЯТИ  (инициализация)
; Данные методы следует вызывать только раз, на этапе инициализации МК, когда никакой полезной работы ещё не ведётся.
; Также, на этапе выполнения, при реализации нестандартного управления, данным методом можно принудительно очистить всю очередь/пул...
; 
; RTOS_METHOD_ClearTaskQueue:	Очистить "очеред задач" RTOS_TaskQueue 
; 
; RTOS_METHOD_ClearTimers:	Очистить "пул Таймеров" RTOS_TimersPool
; 
; RTOS_METHOD_ClearWaiters:	Очистить "пул Выжидателей" RTOS_WaitersPool
; 
;---------------------------------------------------------------------------

;----- Subroutine Register Variables

; Без параметров.

; Памятка: данный метод не портит содержимое РОН - поскольку защищает используемые регистры Стеком.
; Памятка: SREG сохраняется всегда.

;----- Code


.ifdef	RTOS_FEATURE_PLANTIMER
RTOS_METHOD_ClearTimers:
		PUSH	ZL
		PUSH	ZH
		PUSH	temp1
		LDI	ZL,	Low (RTOS_TimersPool)
		LDI	ZH,	High(RTOS_TimersPool)
		LDI	temp1,	RTOS_TimersPoolSize*3				; переменная цикла (инициализация)
		RJMP	RTOS_METHOD_ClearMemory
.endif	//RTOS_FEATURE_PLANTIMER


.ifdef	RTOS_FEATURE_PLANWAITER
RTOS_METHOD_ClearWaiters:
		PUSH	ZL
		PUSH	ZH
		PUSH	temp1
		LDI	ZL,	Low (RTOS_WaitersPool)
		LDI	ZH,	High(RTOS_WaitersPool)
		LDI	temp1,	RTOS_WaitersPoolSize*3				; переменная цикла (инициализация)
		RJMP	RTOS_METHOD_ClearMemory
.endif	//RTOS_FEATURE_PLANWAITER


RTOS_METHOD_ClearTaskQueue:
		PUSH	ZL
		PUSH	ZH
		PUSH	temp1
		LDI	ZL,	Low (RTOS_TaskQueue)
		LDI	ZH,	High(RTOS_TaskQueue)
		LDI	temp1,	RTOS_TaskQueueSize				; переменная цикла (инициализация)
		;RJMP	RTOS_METHOD_ClearMemory

RTOS_METHOD_ClearMemory:
		PUSH	temp2
		PUSH	temp3
		LDI	temp2,	$00						; = спец.код "Нет Задачи"
		;=== начало критической секции
		INR	temp3,	SREG						; Save global interrupt flag
		CLI								; запрещаем прерывания
	Loop__RTOS_METHOD_ClearMemory:
		ST	Z+,	temp2
		DEC	temp1							; переменная цикла (декрементация)
		BRNE	Loop__RTOS_METHOD_ClearMemory

		OUTR	SREG,	temp3						; Restore global interrupt flag
		;=== конец критической секции
		POP	temp3
		POP	temp2
		POP	temp1
		POP	ZH
		POP	ZL
		RET



;---------------------------------------------------------------------------
; 
; Если при выполнении кода ядра RTOS произошла системная ошибка (kernel panic)...
; (Например: "очередь задач" уже заполнена, а прикладной код вызвал RTOS_METHOD_AddTask) ???
; 
; Сбойная операция, в таких случаях, прерывается, стараясь сохранить/восстановить критические системные данные.
; И также, этот ErrorHandler-метод вызывается из кода ядра, передавая ему код ошибки.
; Можно здесь, каким-то образом, просигнализировать об ошибке пользователю, записать в лог, или забить (по-умолчанию)...
; 
; 
; Замечание: Переполнение очереди и пулов - деструктивно, для логики многих Задач! 
; Что делать? Используйте ловушку RTOS_METHOD_ErrorHandler, для выявления таких ситуаций, на этапе отладки устройства.
; А также, в эту ловушку можно поместить дежурный код, перезапускающий критические Процессы, в production-устройствах...
; 
;---------------------------------------------------------------------------

;----- Subroutine Register Variables

;.def	ErrorCode	= (см.выше)	; (входной, не изменяется)			Код системной ошибки
;.def	TaskCode	= (см.выше)	; (входной, не изменяется)			Номер-индекс Задачи, при манипуляциях с которой произошла ошибка
;.def	ZH:ZL		= (см.выше)	; [входной, не изменяется, не обязательный]	Адрес начала Пула, для идентификации "Какой именно Пул?"

; Внимание: Код этого обработчика обязательно должен быть "register safe"! Т.е. обязан сохранять все используемые внутри "временные регистры", поскольку вызывается также из обработчиков прерываний... В том числе и "параметровые регистры" - можно не сохранять, но тогда менять их содержимое нельзя, по спецификации метода!

;----- Code

	; Реестр используемых кодов ошибок (возможные значения ErrorCode):
	; 	 0 - нет ошибки
	; 
	; 	10 - "очередь Задач"  переполнена, не могу добавить новую Задачу!
	; 		TaskCode = Номер-индекс Задачи, которую не смог добавить
	; 
	; 	20 - "Пул" переполнен, не могу зарегистрировать новый Элемент!
	; 		TaskCode = Номер-индекс Задачи, которую не смог добавить
	; 		Z        = Адрес начала Пула, для идентификации в "Какой именно Пул?"

RTOS_METHOD_ErrorHandler:
		;RJMP	RESET		; "Семь бед - один Ресет!" (с) Радикальный вариант решения всех проблем, по методу Билла Гейтса...
		RET



.endif	//RTOS_OPTIMIZE_FOR_SMALL_MEMORY

.endif	//RTOS_FEATURE_PLANTASK	(Базовая диспетчеризация задач, строго необходима для работы всех функций RTOS)


;=== END "RTOS-kernel.inc" =================================================
; coded by (c) DI HALT, 2008  http://easyelectronics.ru/
; coded by (c) Celeron, 2014  http://inventproject.info/
