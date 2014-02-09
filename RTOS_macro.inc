.IFNDEF	_RTOSMACRO__INCLUDED_
.EQU	_RTOSMACRO__INCLUDED_ = 1
.MESSAGE "Note: <RTOS-macro.inc> have included, only once!" 
.include "macrobaselib.inc"	; Библиотека базовых Макроопределений (требуется)
;=== BEGIN "RTOS-macro.inc" ================================================

; Макросы, реализующие прикладной программный интерфейс RTOS (API)
; (Назначение: здесь всё, что вам может понадобиться, от RTOS, в прикладном коде проекта.)


;***************************************************************************
;*
;*  Глобальные константы и Псевдонимы регистров для RTOS
;*
;***************************************************************************


; Директивами условной компиляции можно отключать отдельные возможности RTOS, уменьшая размер ядра (оптимизация под простые задачи и слабые МК):

.EQU	RTOS_FEATURE_PLANTASK = 1		; поддержка Базовой диспетчеризации задач	(последовательный запуск, в порядке очереди)
.EQU	RTOS_FEATURE_PLANTIMER = 1		; поддержка Диспетчеризации Задач по Таймеру	(безусловно отложенный запуск на <X> миллисекунд)
.EQU	RTOS_FEATURE_PLANWAITER = 1		; поддержка Флаговой автоматизации		(диспетчеризация Задач по состоянию бита в памяти РОН, Управляющем Регистре/Порте, или в ячейке SRAM:  запуск задачи, если/как-только бит в байте установлен?  или если/как-только бит в байте снят?)

.EQU	RTOS_OPTIMIZE_FOR_SMALL_MEMORY = 1	; используйте это только для самых младших МК!	(отключить в системных методах RTOS защиту регистров стеком, отключить некритичные проверки, выкинуть второстепенные методы из ядра, отключить всё что только можно - чтобы минимизировать объём кода RTOS, и главное, минимизировать используемый/требуемый объём ОЗУ/стека)
						; Итого, только для работы RTOS (в режиме со включенной оптимизацией RTOS_OPTIMIZE_FOR_SMALL_MEMORY) - в стеке требуется гарантированно оставить 10 байт (для RTOS)!
						; Напомню: Также, требуется выделить SRAM под "очередь Задач", пулы Таймеров и Выжидателей.
						; Оставшееся ОЗУ - можно использовать для прикладных данных... И если, в прикладном коде, вы используете вызовы Подпрограмм (CALL Subroutine) - обязательно просчитайте достаточную глубину Стека (для адресов возврата, как минимум)!
						; Например: на МК ATtiny10 (при 32 байтах SRAM), можно использовать RTOS с одним Таймером (-3байта) и одним Флаговым Автоматом (-3байта), и очередь глубиной 4шт. Задачи (-4байта); ещё -10 байт на Стек RTOS; остаётся 12 байт SRAM на прикладную логику (10 на данные + 2 про запас). Profit!		/Замечу: при этом, в сегменте кода система будет занимать 350-450 байт - остальное на прикладную логику./

; Смотри также: константы Настройки Скорости - в макросах инициализации RTOS_INIT, USART_INIT... (их невозможно настроить автоматически, нужно править вручную)


;---------------------------------------------------------------------------


; Временные переменные, используемые в ядре RTOS:
;.def	temp	= R16	; (определено в macrobaselib.inc)
;.def	temp1	= R16	; (определено в macrobaselib.inc)
;.def	temp2	= R17	; (определено в macrobaselib.inc)
;.def	temp3	= R18	; (определено в macrobaselib.inc)
;.def	temp4	= R19	; (определено в macrobaselib.inc)



; Определение параметровых регистров для подпрограмм-методов ядра RTOS:
; (указание: не используйте здесь "временные регистры" - нельзя мультиплексировать их с "параметровыми регистрами"!)

.def	TaskCode		= R25	; Номер-индекс Задачи в таблице RTOS_TaskProcs

.def	PoolParameter2byte	= R24	; 2-ой байт элемента Пула (описание формата см. в RTOS_data.inc)	/используется для настройки Таймера и Флагового Автомата/
.def	PoolParameter3byte	= R23	; 3-ий байт элемента Пула (описание формата см. в RTOS_data.inc)	/используется для настройки Таймера и Флагового Автомата/

;.def	TimeDelayLow		= R24	; Время Задержки, в милисекундах (мл.байт)	/используется для настройки Таймера/
;.def	TimeDelayHigh		= R23	; Время Задержки, в милисекундах (ст.байт)	/используется для настройки Таймера/

;.def	WaiterTemplateLow	= R24	; 2-ой байт элемента пула Выжидателей (описание формата см. в RTOS_data.inc)	/используется для настройки Флагового автомата/
;.def	WaiterTemplateHigh	= R23	; 3-ий байт элемента пула Выжидателей (описание формата см. в RTOS_data.inc)	/используется для настройки Флагового автомата/

.def	ErrorCode		= R24	; Код системной ошибки	/передаётся в RTOS_METHOD_ErrorHandler/




;***************************************************************************
;*
;*  Методы постановки Задач в очередь
;*
;***************************************************************************


.ifdef	RTOS_FEATURE_PLANTASK
;---------------------------------------------------------------------------
; Базовая диспетчеризация задач (последовательный запуск, в порядке очереди)



	; Добавить Задачу в очередь RTOS_TaskQueue, послать на выполнение
	; Памятка: под параметры и временные переменные - портит содержимое всех регистров, упомянутых в секции "Subroutine Register Variables", соответствующего системного метода, и в зависимости от текущего режима оптимизации...
	; Пример вызова:  PLAN_TASK tasknumber
	.MACRO 	PLAN_TASK
		.if @0==$00
		.ERROR "system <Task_Idle> is forbidden to use!"
		.endif
		LDI	TaskCode,	@0			; Заполняем параметры
		RCALL	RTOS_METHOD_AddTask			; Вызываем системный метод
	.ENDM



	.ifndef	RTOS_OPTIMIZE_FOR_SMALL_MEMORY
	; Добавить Задачу в очередь RTOS_TaskQueue, послать на выполнение	(БЕЗОПАСНАЯ ВЕРСИЯ)
	; Памятка: данный макрос не портит содержимое РОН (даже для передачи параметров) - поскольку защищает используемые регистры Стеком.
	; Пример вызова:  SAFE_PLAN_TASK tasknumber
	.MACRO 	SAFE_PLAN_TASK
		PUSH	TaskCode				; Здесь, сберегаем только параметровые регистры!
		PLAN_TASK	@0
		POP	TaskCode
	.ENDM
	.endif



.endif	//RTOS_FEATURE_PLANTASK

.ifdef	RTOS_FEATURE_PLANTIMER
;---------------------------------------------------------------------------
; Диспетчеризация Задач по Таймеру (безусловно отложенный запуск на <X> миллисекунд)



	; Установить (добавить или обновить) Таймер в пуле RTOS_TimersPool, для отложенного запуска Задачи
	; Памятка: под параметры и временные переменные - портит содержимое всех регистров, упомянутых в секции "Subroutine Register Variables", соответствующего системного метода, и в зависимости от текущего режима оптимизации...
	; Пример вызова:  PLAN_TIMER tasknumber,delay
	.MACRO	PLAN_TIMER
		.if @0==$00
		.ERROR "system <Task_Idle> is forbidden to use!"
		.endif
		.if @1==0
		.ERROR "0ms time delay is forbidden for <RTOS_METHOD_AddTimer> method!"
		.endif
		LDI	TaskCode,		@0		; Заполняем параметры
		LDI	PoolParameter2byte,	Low(@1)
		LDI	PoolParameter3byte,	High(@1)
		RCALL	RTOS_METHOD_AddTimer			; Вызываем системный метод
	.ENDM



	.ifndef	RTOS_OPTIMIZE_FOR_SMALL_MEMORY
	; Установить (добавить или обновить) Таймер в пуле RTOS_TimersPool, для отложенного запуска Задачи	(БЕЗОПАСНАЯ ВЕРСИЯ)
	; Памятка: данный макрос не портит содержимое РОН (даже для передачи параметров) - поскольку защищает используемые регистры Стеком.
	; Пример вызова:  SAFE_PLAN_TIMER tasknumber,delay
	.MACRO	SAFE_PLAN_TIMER
		PUSH	TaskCode				; Здесь, сберегаем только параметровые регистры!
		PUSH	PoolParameter2byte
		PUSH	PoolParameter3byte
		PLAN_TIMER	@0,@1
		POP	PoolParameter3byte
		POP	PoolParameter2byte
		POP	TaskCode
	.ENDM
	.endif



	.ifndef	RTOS_OPTIMIZE_FOR_SMALL_MEMORY
	; Убрать (удалить) Таймер из пула RTOS_TimersPool, для отмены отложенного запуска Задачи
	; Памятка: под параметры и временные переменные - портит содержимое всех регистров, упомянутых в секции "Subroutine Register Variables", соответствующего системного метода, и в зависимости от текущего режима оптимизации...
	; Пример вызова:  REMOVE_TIMER tasknumber
	.MACRO 	REMOVE_TIMER
		.if @0==$00
		.ERROR "system <Task_Idle> is forbidden to use!"
		.endif
		LDI	TaskCode,	@0			; Заполняем параметры
		RCALL	RTOS_METHOD_RemoveTimer			; Вызываем системный метод
	.ENDM
	.endif



	.ifndef	RTOS_OPTIMIZE_FOR_SMALL_MEMORY
	; Убрать (удалить) Таймер из пула RTOS_TimersPool, для отмены отложенного запуска Задачи	(БЕЗОПАСНАЯ ВЕРСИЯ)
	; Памятка: данный макрос не портит содержимое РОН (даже для передачи параметров) - поскольку защищает используемые регистры Стеком.
	; Пример вызова:  SAFE_REMOVE_TIMER tasknumber
	.MACRO 	SAFE_REMOVE_TIMER
		PUSH	TaskCode				; Здесь, сберегаем только параметровые регистры!
		REMOVE_TIMER	@0
		POP	TaskCode
	.ENDM
	.endif



.endif	//RTOS_FEATURE_PLANTIMER

.ifdef	RTOS_FEATURE_PLANWAITER
;---------------------------------------------------------------------------
; Диспетчеризация Задач по состоянию бита в Управляющем Регистре/Порте или в ячейке SRAM (Флаговая автоматизация: запуск, если бит в байте установлен?  если бит в байте снят?)



	; Установить (добавить или обновить) Выжидатель в пуле RTOS_WaitersPool, для условного запуска Задачи
	; Памятка: под параметры и временные переменные - портит содержимое всех регистров, упомянутых в секции "Subroutine Register Variables", соответствующего системного метода, и в зависимости от текущего режима оптимизации...
	; Пример вызова:  PLAN_WAITER tasknumber,address,bit,state
	.MACRO	PLAN_WAITER
		.if @0==$00
		.ERROR "system <Task_Idle> is forbidden to use!"
		.endif
		.if @1<0 || @1>0xFFF
		.ERROR "'address' parameter must be in [0..0xFFF]!"
		.endif
		.if @2<0 || @2>7
		.ERROR "'bit' parameter must be in [0..7]!"
		.endif
		.if @3!=0 && @3!=1
		.ERROR "'state' parameter must be [0 or 1]!"
		.endif
		LDI	TaskCode,		@0		; Заполняем параметры
		LDI	PoolParameter2byte,	Low(@1)
		LDI	PoolParameter3byte,	(@3<<WAITER_BIT_STATE)|(@2<<WAITER_BIT_POSITION)|(High(@1)<<WAITER_ADDRESS_HIGH)
		RCALL	RTOS_METHOD_AddWaiter			; Вызываем системный метод
	.ENDM



	.ifndef	RTOS_OPTIMIZE_FOR_SMALL_MEMORY
	; Установить (добавить или обновить) Выжидатель в пуле RTOS_WaitersPool, для условного запуска Задачи	(БЕЗОПАСНАЯ ВЕРСИЯ)
	; Памятка: данный макрос не портит содержимое РОН (даже для передачи параметров) - поскольку защищает используемые регистры Стеком.
	; Пример вызова:  SAFE_PLAN_WAITER tasknumber,address,bit,state
	.MACRO	SAFE_PLAN_WAITER
		PUSH	TaskCode				; Здесь, сберегаем только параметровые регистры!
		PUSH	PoolParameter2byte
		PUSH	PoolParameter3byte
		PLAN_WAITER	@0,@1,@2,@3
		POP	PoolParameter3byte
		POP	PoolParameter2byte
		POP	TaskCode
	.ENDM
	.endif



	.ifndef	RTOS_OPTIMIZE_FOR_SMALL_MEMORY
	; Убрать (удалить) Выжидатель из пула RTOS_WaitersPool, для отмены условного запуска Задачи
	; Памятка: под параметры и временные переменные - портит содержимое всех регистров, упомянутых в секции "Subroutine Register Variables", соответствующего системного метода, и в зависимости от текущего режима оптимизации...
	; Пример вызова:  REMOVE_WAITER tasknumber
	.MACRO 	REMOVE_WAITER
		.if @0==$00
		.ERROR "system <Task_Idle> is forbidden to use!"
		.endif
		LDI	TaskCode,	@0			; Заполняем параметры
		RCALL	RTOS_METHOD_RemoveWaiter		; Вызываем системный метод
	.ENDM
	.endif



	.ifndef	RTOS_OPTIMIZE_FOR_SMALL_MEMORY
	; Убрать (удалить) Выжидатель из пула RTOS_WaitersPool, для отмены условного запуска Задачи	(БЕЗОПАСНАЯ ВЕРСИЯ)
	; Памятка: данный макрос не портит содержимое РОН (даже для передачи параметров) - поскольку защищает используемые регистры Стеком.
	; Пример вызова:  SAFE_REMOVE_WAITER tasknumber
	.MACRO 	SAFE_REMOVE_WAITER
		PUSH	TaskCode				; Здесь, сберегаем только параметровые регистры!
		REMOVE_WAITER	@0
		POP	TaskCode
	.ENDM
	.endif



.endif	//RTOS_FEATURE_PLANWAITER

;***************************************************************************
;*
;*  Инициализация RTOS
;*
;***************************************************************************


	; Инициализация RTOS
	; (Внимание: этот блок должен располагаться в коде инициализации микроконтроллера, запускаться единожды после "Reset"!)
	; Памятка: портит содержимое регистра TEMP1.

	.MACRO	RTOS_INIT
	.if defined(RTOS_FEATURE_PLANTASK)		; (без поддержки "Базовой диспетчеризации задач" все функции RTOS отключены!)


	.ifndef	RTOS_OPTIMIZE_FOR_SMALL_MEMORY
		.ifdef	RTOS_FEATURE_PLANTASK
		RCALL	RTOS_METHOD_ClearTaskQueue	; Очистить очередь Задач RTOS
		.endif
		
		.ifdef	RTOS_FEATURE_PLANTIMER
		RCALL	RTOS_METHOD_ClearTimers		; Очистить список Таймеров RTOS
		.endif

		.ifdef	RTOS_FEATURE_PLANWAITER
		RCALL	RTOS_METHOD_ClearWaiters	; Очистить список Выжидателей RTOS
		.endif
	.endif	//RTOS_OPTIMIZE_FOR_SMALL_MEMORY


		.MESSAGE "Attention: check the main RTOS Timer setting! Are constant set correspond to CPU clock? Choose one free hardware Timer... Are Register bits correct?"


		; Вспомогательные Константы для настройки основного таймера ядра RTOS:
		;
		; Инструкция: 
		; 	Установите фактическую частоту МК, выберите Предделитель -> 
		; 	получите итоговый "Делитель тактовой частоты" так, 
		; 	чтобы событие таймера/счётчика Overflow/Compare срабатывало каждую 1ms.
		; Ограничения: 
		; 	Значения Пределителя можно использовать только поддерживаемые выбранным аппаратным Таймером (не забудьте, затем, корректно выставить ему соответствующие биты конфигурации!)
		; 	Значение Делителя должно получиться целым числом! Но как можно меньше - чтобы помещаться в разрядную сетку таймера: для 8-bit <=255 / для 16-bit <=65535.

		.equ CMainClock		= 8000000				; тактовая частота "CPU Clock" (Hz)
		.equ CTimerPrescaler	= 64					; Предделитель аппаратного таймера: 1, 8, [32,] 64, [128,] 256, 1024
		.equ CTimerDivider	= CMainClock/CTimerPrescaler/1000	; итоговый "делитель тактовой частоты", т.е. коэффициент её преобразования во время ~ 1ms



		; Самопроверка:
		.IF FRAC(CMainClock/CTimerPrescaler/1000.0)>0
		.WARNING "CTimerDivider is fractional! Timings could lose accuracy..."
		.ENDIF

		; Самопроверка (только для 8-bit таймеров):
		.IF CTimerDivider>255
		.WARNING "CTimerDivider is incorrect (over byte capacity)! Use bigger CTimerPrescaler..."
		.ENDIF



		; Выберите, для нужд RTOS, один из доступных аппаратных Таймеров/Счётчиков.
		;
		; Т.к. моделей МК очень много, в большинстве несколько таймеров, и у каждого свои регистры/биты настройки, 
		; кроме того прикладная задача накладывает ещё свои ограничения (некоторые таймеры могут быть заняты), 
		; то выбор и конфигурация конкретного аппаратного Таймера - обычно, производятся самим программистом, вручную!
		; Но для облегчения работы, был разработан код автоматического способа, покрывающий большинство случаев: см. режим RTOS_USE_TIMER0_OVERFLOW


	.ifndef	RTOS_USE_TIMER0_OVERFLOW
	.ifdef	RTOS_USE_TIMER2_COMPARE

		; Замечу, что по "методу предложенному DI HALT'ом", вам требуется Таймер/Счётчик обязательно с "режимом компарации" - чтоб навесить "службу таймеров" на его прерывание "Timer/CounterX Compare Match".
		; Идея здесь состоит в том, чтобы отсчитывать CTimerDivider тиков таймера и АВТОМАТИЧЕСКИ вызывать прерывание - точно с периодичностью 1ms.
		; Это позволяет прецизионно тактировать "службу таймеров RTOS", даже не используя специальных "часовых кварцев" (со специально подобранными частотами, кратными ёмкости счётного регистра таймера).
		; Данный приём работает на любой исходной "тактовой частоте" (CPU clock). И весьма точно! За исключением тех случаев, когда соотношение "тактовой частоты" CMainClock и доступных значений "предделителей" CTimerPrescaler не дозволяет добиться целого значения "делителя" CTimerDivider - в таких случаях, появляется НЕБОЛЬШАЯ погрешность счёта...


		; Настройка аппаратного 8-bit Timer/Counter2:
		; (Supported: ATmega8/ATmega16)
		
		OUTI	TCCR2,	1<<CTC2|4<<CS20		; Установить режим CTC (автосброс после достижения регистра сравнения) и Предделитель = clk/64
		OUTI	OCR2,	Low(CTimerDivider)	; Установить значение в регистр сравнения
		OUTI	TCNT2,	0			; Установить начальное значение счётчика
		SETB	TIMSK,	OCIE2			; Разрешаем прерывание: Timer/Counter2 Output Compare Match (по достижению регистра сравнения)

	.else	//end of RTOS_USE_TIMER2_COMPARE

		; (*** здесь вы можете вставить свой особый код инициализации таймера ***)

	.endif
	.else	//begin of RTOS_USE_TIMER0_OVERFLOW

		; Если только получилось значение Делителя CTimerDivider == 256 или 512, кратное ёмкости счётного регистра 
		; (что случается редко и только при использовании специальных "часовых кварцев") - 
		; тогда вы можете использовать любой простой Таймер/Счётчик, и его базовое прерывание "по переполнению" ("Timer/CounterX Overflow").

		; Не имея возможность ограничить значение в счётчике - можно только дождаться заполнения его полностью, переполнения, и вызова прерывания "Timer/CounterX Overflow".
		; Замечу, что "режим компарации" присутствует только в самых навороченных и ценных Таймер/Счётчиках, обладающих также другими полезными функциями (ШИМ, тактирование от отдельного кварца 32kHz, и др.) - жалко отдавать такой под RTOS!
		; Поэтому, чтобы убить всех зайцев, хочу предложить вам другой способ: использования простого счётчика, с только базовым прерыванием "по переполнению", позволяющий адаптироваться к любому значению Делителя CTimerDivider:
		; 	чтобы ограничить диапазон счёта - будем ограничивать значение в счётном регистре не "сверху", а "снизу"!
		; 	Задействуем аппаратный Timer/Counter0 - который есть во всех МК, и обычно, самый простой по функционалу (не жалко).
		; 	Службу таймеров RTOS_TIMER_SERVICE навесим на его прерывание "по переполнению" ("Timer/Counter0 Overflow").
		; 	В этом же прерывании, по переполнении счётчика и сбросе счётного регистра в ноль, будем каждый раз преинициализировать счётчик в "дополнение до переполнения = требуемому Делителю":	OUTI TCNT0, 256-Low(CTimerDivider)

		; Примечание: в некоторых младших моделях ATtiny присутствует единственный таймер (16-bit Timer/Counter0), но он 16-битный! 
		; 	код преинициализации счётчика тогда будет:	OUTI TCNT0, (65536-CTimerDivider)
		; 	где "делитель тактовой частоты" допустим:	CTimerDivider, для 16-bit, <=65535.
		; Поэтому код инициализации таймера запрограммирован универсальным, через директивы условной компиляции (проверено на ATtiny10).



		; В некоторых МК, Timer/Counter0 столь наворочен, что у него не один "Control Register TCCR0", а несколько... Причём, Источник и Предделитель обычно настраиваются через TCCR0B - вот и определим его через псевдоним, для универсализации кода.
		.ifndef	TCCR0
			.equ	TCCR0 = TCCR0B
		.endif
		
		; Аналогично, бывают несколько регистров "Timer/Counter Interrupt Mask"... Выбираем наиболее вероятный.
		.ifndef	TIMSK
			.equ	TIMSK = TIMSK0
		.endif

		; Автоматический выбор битов конфигурации Предделителя (эта система используется в большинстве, если не во всех, МК AVR):
		.if	CTimerPrescaler == 1
			.equ	CTimerPrescaler_CS 	= 0b001
		.elif	CTimerPrescaler == 8
			.equ	CTimerPrescaler_CS	= 0b010
		.elif	CTimerPrescaler == 64
			.equ	CTimerPrescaler_CS	= 0b011
		.elif	CTimerPrescaler == 256
			.equ	CTimerPrescaler_CS	= 0b100
		.elif	CTimerPrescaler == 1024
			.equ	CTimerPrescaler_CS	= 0b101
		.else
			.ERROR "CTimerPrescaler is incorrect! Use one of acceptable values: 1, 8, 64, 256, 1024"
		.endif


		; Настройка аппаратного 8-bit Timer/Counter0:
		; (Supported: большинство МК AVR)

		OUTI	TCCR0,	(CTimerPrescaler_CS<<CS00)	; Запустить таймер от Тактовой частоты, и Установить Предделитель = clk/64
		SETB	TIMSK,	TOIE0				; Разрешить прерывание: Overflow Interrupt Enable 

	.ifndef	TCNT0L
		; В случае 8-разрядного Timer/Counter0
		OUTI	TCNT0,	256-Low(CTimerDivider)		; преИнициализировать начальное значение счётчика
	.else
		; В случае 16-разрядного Timer/Counter0
		; Note: To do a 16-bit write, the high byte must be written before the low byte. For a 16-bit read, the low byte must be read before the high byte.
		; 	It is important to notice that accessing 16-bit registers are atomic operations. Aware interrupts, use cli/sei!
		OUTI	TCNT0H,	High(65536-CTimerDivider)
		OUTI	TCNT0L,	Low (65536-CTimerDivider)	; преИнициализировать начальное значение счётчика
	.endif

	.endif	//end of RTOS_USE_TIMER0_OVERFLOW
	.endif	//RTOS_FEATURE_PLANTASK
	.ENDM




.if defined(RTOS_FEATURE_PLANTASK) && (defined(RTOS_FEATURE_PLANTIMER) || defined(RTOS_FEATURE_PLANWAITER))	; (служба таймеров используется только в этих режимах)
; Q: Какой аппаратный таймер задействовать для "Службы Таймеров Ядра RTOS" (Main Timer Service)?
; 
; A1) Чтобы использовать простой Таймер/Счётчик0 и его базовое прерывание "по переполнению" ("Timer/Counter0 Overflow") - включите это определение.
; Данный метод рекомендуется использовать, по умолчанию!
; Универсален: подходит для большинства задач, использует минимум аппаратных ресурсов, прост в настройке.
; Самонастраиваемый: здесь, вам достаточно только установить Константы режимов, а управляющие регистры будут сконфигурированы автоматически (подходит для большинства МК AVR).
.EQU	RTOS_USE_TIMER0_OVERFLOW = 1

; A2) Если вы хотите использовать 8-битный Timer/Counter2 с "режимом компарации" - включите это определение.
;.EQU	RTOS_USE_TIMER2_COMPARE = 1

; A3) Если же вы хотите использовать свой иной метод - то просто закомментируйте оба верхних определения, 
; и пропишите свой код в соответствующую секцию "else", между директивами условной компиляции, выше...
.endif




; Q: Заметка: Что делать, если CTimerDivider никак не получается целым числом?
; A: 
; Таймер/Счётчик может сосчитать только целое число тиков! 
; В принципе, можно проигнорировать и принять Делитель как есть, просто отбросив его дробную часть...
; Как показывает расчёт, при округлении дробного Делителя - потеря точности счёта не слишком велика. 
; Погрешность растёт при понижении частоты, и зависит от выбора предделителя...
; Таким образом, если очень припрёт, то можно проигнорировать погрешность, возникающую при отбрасывании дробной части Делителя.
; Всё равно, "програмные таймеры RTOS" - не являются прецизионными! Да ещё и тактирование МК, зачастую, осуществляется не от Кварца...
;
; Например:
; 	при CPU clock=8MHz,  prescaler=64  -> divider=125tic    ~ 1ms, где 1tic=0,008ms  -> погрешность = 0
; 	при CPU clock=20MHz, prescaler=128 -> divider=156,25tic ~ 1ms, где 1tic=0,0064ms -> погрешность относительная=0.25/156=0,0016 или абсолютная=128*0.25=32clk, т.е. 16us за 1ms	(т.е. уходит быстрее на 0,0016сек за 1сек)
; 	при CPU clock=1MHz,  prescaler=32  -> divider=31,25tic  ~ 1ms, где 1tic=0,032ms  -> погрешность относительная=0.25/31=0,008 							(т.е. уходит быстрее на 0,008сек  за 1сек)
; 	при CPU clock=1MHz,  prescaler=8   -> divider=125tic    ~ 1ms, где 1tic=0,008ms  -> погрешность = 0
;
; Рекомендация:
; 	Как видите, например, при divider=31,25tic - погрешность 0.25 уже на два порядка меньше всего числа 31.0...
; 	Следовательно, при дробном divider, просто не допускайте значения divider<10, а лучше пусть будет divider>100. Тогда погрешность от дробной части будет минимальной...



;---------------------------------------------------------------------------


	; Инициализация USART
	; (Примечание: USART совершенно не требуется для RTOS! Данный макрос предоставляется исключительно как "довесок" - 
	;              можете включить его в секцию инициализации МК, если вы используете это периферийное устройство...)
	; Памятка: портит содержимое регистра TEMP1.

	.MACRO	USART_INIT

		.MESSAGE "Attention: check USART setting! Are constant set correspond to CPU clock? Are Register bits correct?"


		; Вспомогательные Константы для настройки скорости USART:
		.equ CXTAL		= 8000000				; тактовая частота "CPU Clock" (Hz)
		.equ CBaudRate		= 9600					; скорость передачи, опционально: 19200, 28800...
		.equ CBaudDivider	= CXTAL/(16*CBaudRate)-1		; формула для "asynchronous normal mode" (U2X=0) (without "double speeding")


		OUTI 	UBRRL,	Low (CBaudDivider)				; "Делитель" позволяет подогнать CPU clock (от которого все устройства тактируются, в т.ч. USART) 
		OUTI 	UBRRH,	High(CBaudDivider)				; к одной из стандартных "частот передачи" через последовательный порт (CBaudRate).
		OUTI 	UCSRA,	0						; (zero event flags - just cleaning)
		OUTI 	UCSRB,	(1<<RXEN)|(1<<TXEN)|(1<<RXCIE)|(1<<TXCIE)	; (enable receiver and transmitter, enable it interrupts)

	.ifdef	URSEL
		; Примечание: В ATmega8/16/32 используется мультиплексирование управляющих регистров UBRRH/UCSRC - практикуется, через изврат с URSEL...
		OUTI 	UCSRC,	(1<<URSEL)|(1<<UCSZ1)|(1<<UCSZ0)		; (set 8-bit character size)
	.else
		; В остальных МК - по нормальному...
		OUTI 	UCSRC,             (1<<UCSZ1)|(1<<UCSZ0)		; (set 8-bit character size)
	.endif

	.ENDM



;***************************************************************************
;*
;*  Служба таймеров RTOS
;*
;***************************************************************************


	; Служба таймеров RTOS	(располагается в прерывании аппаратного таймера и запускается каждую 1мс)
	; Памятка: данный обработчик прерывания не портит содержимое РОН - поскольку защищает используемые регистры Стеком. Также, SREG сохраняется всегда.
	
	; Внимание: данный обработчик прерывания требует/использует в Стеке  2+6=8 байт  (на адрес возврата и сбережение регистров)! 
	; 	Замечу, этот код является "самым слабым звеном", по потреблению памяти, что критично для самых младших AVR (с 32-64 байтами SRAM)...
	; 	Учитывайте также, что все другие системные методы (в режиме с оптимизацией RTOS_OPTIMIZE_FOR_SMALL_MEMORY) - явно не используют стек вообще, но каждый их вызов забирает в стеке по +2 байта (на адрес возврата).
	; 	Итого, только для работы RTOS (в режиме со включенной оптимизацией RTOS_OPTIMIZE_FOR_SMALL_MEMORY) - в стеке требуется гарантированно оставить 10 байт для RTOS!

	.MACRO	RTOS_TIMER_SERVICE
	.if defined(RTOS_FEATURE_PLANTASK) && (defined(RTOS_FEATURE_PLANTIMER) || defined(RTOS_FEATURE_PLANWAITER))	; (служба таймеров используется только в этих режимах)

		PUSHF								; сохранить temp1 и SREG в стеке
		PUSH	YL							; cохранить и другие регистры, используемые в данном обработчике...
		PUSH	YH
		PUSH	ZL	
		PUSH	ZH


.ifdef	RTOS_FEATURE_PLANTIMER

		;== Обработать Таймеры
		; см. описание идеи http://easyelectronics.ru/avr-uchebnyj-kurs-operacionnaya-sistema-tajmernaya-sluzhba.html

		LDI	ZL,	Low (RTOS_TimersPool)				; Загрузить адрес начала "пула Таймеров" -> в регистр Z
		LDI	ZH,	High(RTOS_TimersPool)

	LoopTimer__RTOS_TIMER_SERVICE:
		LD	temp1,	Z						; прочитать "Идентификатор Задачи" очередного таймера -> temp1

		CPI	temp1,	$00						; проверка: если "Нет Задачи"?
		BREQ	SkipTimer__RTOS_TIMER_SERVICE				; то переход к следующему элементу...


		; (состояние: текущий элемент непустой - надо декрементировать его счётчик)
		LDD	YL,	Z+1						; 
		SUBI	YL,	Low(1) 						; Уменьшить мл.байт "счётчика таймера"
		STD	Z+1,	YL						;

		LDD	YH,	Z+2						;
		SBCI	YH,	High(1) 					; Уменьшить ст.байт "счётчика таймера"	(если был заём разряда "carry bit")
		STD	Z+2,	YH						;

		OR	YL,	YH						; проверить: досчитали "счётчик таймера" до нуля?	(YL==0 and YH==0 ?)
		BRNE 	SkipTimer__RTOS_TIMER_SERVICE				; не ноль, счёт ещё не окончен...


		; (состояние: "счётчик таймера" досчитал до нуля - запустить связанную Задачу)
		LDI	YL,	Low (RTOS_TaskQueue)				; Сканируем очередь задач сначала до конца, ищем первую пустую ячейку...
		LDI	YH,	High(RTOS_TaskQueue)
	LoopTaskForTimer__RTOS_TIMER_SERVICE:
		LD	temp1,	Y+
		CPI	temp1,	$00						; проверка: ячейка пустая?
		BREQ	FoundTaskForTimer__RTOS_TIMER_SERVICE			; если пустая - прерываем сканирование на этой позиции...
		CPI	YL,	Low (RTOS_TaskQueue + RTOS_TaskQueueSize)	; условие цикла:
		BRNE	LoopTaskForTimer__RTOS_TIMER_SERVICE			; 	ПОКА адрес в Y не дойдёт до конца данных...
		CPI	YH,	High(RTOS_TaskQueue + RTOS_TaskQueueSize)	; (трюк: замечу, здесь не используется традиционная переменная-счётчик, чтобы сэкономить регистр)
		BRNE	LoopTaskForTimer__RTOS_TIMER_SERVICE
		RJMP	ClearTimer__RTOS_TIMER_SERVICE				; Если достигли конца очереди, но так и не нашли пустой ячейки - игнор...
	FoundTaskForTimer__RTOS_TIMER_SERVICE:
		LD	temp1,	Z						; Если нашли пустое место (фактически, это текущий "хвост" очереди) - то дополняем очередь требуемой Задачей...
		ST	-Y,	temp1
	ClearTimer__RTOS_TIMER_SERVICE:
		LDI	temp1,	$00
		ST	Z,	temp1						; Задача обработана - Таймер самоудаляется...


	SkipTimer__RTOS_TIMER_SERVICE:
		SUBI	ZL,	Low (-3)					; Переходим к следующему таймеру:
		SBCI	ZH,	High(-3)					; 	адрес Z+=3
		CPI	ZL,	Low (RTOS_TimersPool + RTOS_TimersPoolSize*3)	; условие цикла:
		BRNE	LoopTimer__RTOS_TIMER_SERVICE				; 	ПОКА адрес в Z не дойдёт до конца данных...
		CPI	ZH,	High(RTOS_TimersPool + RTOS_TimersPoolSize*3)	; (трюк: замечу, здесь не используется традиционная переменная-счётчик, чтобы сэкономить регистр)
		BRNE	LoopTimer__RTOS_TIMER_SERVICE

.endif	//RTOS_FEATURE_PLANTIMER



.ifdef	RTOS_FEATURE_PLANWAITER

		;== Обработать Выжидатели

		LDI	ZL,	Low (RTOS_WaitersPool)				; Загрузить адрес начала "пула Выжидателей" -> в регистр Z
		LDI	ZH,	High(RTOS_WaitersPool)

	LoopWaiter__RTOS_TIMER_SERVICE:
		LD	temp1,	Z						; прочитать "Идентификатор Задачи" очередного выжидателя -> temp1

		CPI	temp1,	$00						; проверка: если "Нет Задачи"?
		BREQ	SkipWaiter__RTOS_TIMER_SERVICE				; то переход к следующему элементу...


		; (состояние: текущий элемент непустой - надо проверить его Условие)
		LDD	YL,	Z+1						; Устанавливаем "12-битный адрес тестируемой ячейки памяти" -> Y
		LDD	YH,	Z+2
		ANDI	YH,	(0b1111<<WAITER_ADDRESS_HIGH)
		LD	YL,	Y						; Вычитываем "содержимое тестируемого байта" -> YL (во временную переменную)

		LDD	YH,	Z+2
		BST	YH,	WAITER_BIT_STATE				; "Ожидаемое состояние бита" -> T
		ANDI	YH,	(0b111<<WAITER_BIT_POSITION)
		SWAP	YH							; "Номер позиции [0..7] тестируемого бита" -> YH (во временную переменную)

		; Замечу: инструкции INC/DEC не переписывают "Carry Bit" (используемый арифметическими инструкциями)!
		; Manual: The C flag in SREG is not affected by the operation, thus allowing the INC/DEC instruction to be used on a loop counter in multiple-precision computations.
		ROL	YL							; (предварительная коррекция)
		INC	YH
	ShiftBitForWaiter__RTOS_TIMER_SERVICE:
		ROR	YL							; сдвинуть вправо (с использованием "Carry Bit")
		DEC	YH							; переменная цикла (декрементация)
		BRNE	ShiftBitForWaiter__RTOS_TIMER_SERVICE

		ANDI	YL,	(1<<0)						; "Тестируемый бит" -> в нулевой разряд YL
		BLD	YH,	0						; "Ожидаемое состояние бита" -> в нулевой разряд YH	(замечу, после цикла, YH=0)
		EOR	YL,	YH						; Операция "исключающего или" такова: Если биты совпадают, то YL станет нулевым.
		BRNE 	SkipWaiter__RTOS_TIMER_SERVICE				; не ноль, Условие не сработало...


		; (состояние: Условие сработало - запустить связанную Задачу)
		LDI	YL,	Low (RTOS_TaskQueue)				; Сканируем очередь задач сначала до конца, ищем первую пустую ячейку...
		LDI	YH,	High(RTOS_TaskQueue)
	LoopTaskForWaiter__RTOS_TIMER_SERVICE:
		LD	temp1,	Y+
		CPI	temp1,	$00						; проверка: ячейка пустая?
		BREQ	FoundTaskForWaiter__RTOS_TIMER_SERVICE			; если пустая - прерываем сканирование на этой позиции...
		CPI	YL,	Low (RTOS_TaskQueue + RTOS_TaskQueueSize)	; условие цикла:
		BRNE	LoopTaskForWaiter__RTOS_TIMER_SERVICE			; 	ПОКА адрес в Y не дойдёт до конца данных...
		CPI	YH,	High(RTOS_TaskQueue + RTOS_TaskQueueSize)	; (трюк: замечу, здесь не используется традиционная переменная-счётчик, чтобы сэкономить регистр)
		BRNE	LoopTaskForWaiter__RTOS_TIMER_SERVICE
		RJMP	ClearWaiter__RTOS_TIMER_SERVICE				; Если достигли конца очереди, но так и не нашли пустой ячейки - игнор...
	FoundTaskForWaiter__RTOS_TIMER_SERVICE:
		LD	temp1,	Z						; Если нашли пустое место (фактически, это текущий "хвост" очереди) - то дополняем очередь требуемой Задачей...
		ST	-Y,	temp1
	ClearWaiter__RTOS_TIMER_SERVICE:
		LDI	temp1,	$00
		ST	Z,	temp1						; Задача обработана - Выжидатель самоудаляется...


	SkipWaiter__RTOS_TIMER_SERVICE:
		SUBI	ZL,	Low (-3)					; Переходим к следующему таймеру:
		SBCI	ZH,	High(-3)					; 	адрес Z+=3
		CPI	ZL,	Low (RTOS_WaitersPool + RTOS_WaitersPoolSize*3)	; условие цикла:
		BRNE	LoopWaiter__RTOS_TIMER_SERVICE				; 	ПОКА адрес в Z не дойдёт до конца данных...
		CPI	ZH,	High(RTOS_WaitersPool + RTOS_WaitersPoolSize*3)	; (трюк: замечу, здесь не используется традиционная переменная-счётчик, чтобы сэкономить регистр)
		BRNE	LoopWaiter__RTOS_TIMER_SERVICE

.endif	//RTOS_FEATURE_PLANWAITER


		POP	ZH							; восстановить регистры
		POP	ZL
		POP	YH
		POP	YL
		POPF
	.endif	//RTOS_FEATURE_PLANTASK
	.ENDM



;***************************************************************************
;*
;*  Old interface aliases, for backward compatibility.	
;*
;*  (Deprecated: Do not use it in new code!)
;*
;***************************************************************************


	.MACRO	SetTimerTask 
		PLAN_TIMER	@0,@1
	.ENDM


	.MACRO	SetTask
		PLAN_TASK		@0
	.ENDM


	.MACRO	TimerService
		RTOS_TIMER_SERVICE
	.ENDM


	.MACRO	INIT_RTOS
		RTOS_INIT
	.ENDM



;***************************************************************************
;*
;*  Some DI HALT's macro, distributed earlier with RTOS...
;*  Not recomended to usage, because makes code more obfuscated, without significant profit.
;*  Provided here for backward compatibility only!
;*
;*  (Note: It have not used in RTOS code, and it have not any relations to RTOS!)
;*
;***************************************************************************


;---------------------------------------------------------------------------
; SRAM memory read/write macros for Devices with AVR8L-based CPU: ATtiny10, ATtiny20, ATtiny40.
; Maked and used by DI HALT, but deprecated by Celeron.


	.MACRO	LDR
		.MESSAGE "Deprecated: macros LDR was improper! Use macros INR (macrobaselib.inc) instead!"
		;PUSH	ZL
		;PUSH	ZH
		;
		;LDI	ZL, Low(@1)
		;LDI	ZH, High(@1)
		;
		;LD	@0, Z
		;
		;POP	ZH
		;POP	ZL
		INR	@0, @1
	.ENDM


	.MACRO	STR
		.MESSAGE "Deprecated: macros STR was improper! Use macros OUTR (macrobaselib.inc) instead!"
		;PUSH	ZL
		;PUSH	ZH
		;
		;LDI	ZL, Low(@0)
		;LDI	ZH, High(@0)
		;
		;ST	Z,  @1
		;
		;POP	ZH
		;POP	ZL
		OUTR	@0, @1
	.ENDM


;---------------------------------------------------------------------------
; FLASH memory read/write helpers.


	; Read byte (unfortunately, only first in word) from Program Flash Memory, at particular Address @1 (in words), to particular Register @0.
	; Usage:  LDF register,label
	.MACRO	LDF
		.MESSAGE "Deprecated: macros LDF is clumsy! Do not use it at all!"
		PUSH	ZL
		PUSH	ZH

		LDI	ZL, Low(@1*2)
		LDI	ZH, High(@1*2)

		LPM	@0, Z

		POP	ZH
		POP	ZL
	.ENDM


	; Just load Program Label Address @0 to Z-register. (Supposedly, preparing to LPM instruction?)
	.MACRO	LDP
		LDI	ZL, Low(@0*2)
		LDI	ZH, High(@0*2)
	.ENDM

	; Synonym...
	.MACRO	LDPA
		LDP
	.ENDM


;---------------------------------------------------------------------------
; SRAM memory read/write helpers.


	; Just load SRAM Address @0 to X-register.
	.MACRO	LDX
		LDI	XL, Low(@0)
		LDI	XH, High(@0)
	.ENDM


	; Just load SRAM Address @0 to Y-register.
	.MACRO	LDY
		LDI	YL, Low(@0)
		LDI	YH, High(@0)
	.ENDM


	; Just load SRAM Address @0 to Z-register.
	.MACRO	LDZ
		LDI	ZL, Low(@0)
		LDI	ZH, High(@0)
	.ENDM



;=== END "RTOS-macro.inc" ==================================================
; coded by (c) DI HALT, 2008 @ http://easyelectronics.ru/
; coded by (c) Celeron, 2014 @ http://we.easyelectronics.ru/my/Celeron/
.ENDIF
