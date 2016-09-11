.IFNDEF	_MACROBASELIB__INCLUDED_
.EQU	_MACROBASELIB__INCLUDED_ = 1
.MESSAGE "Note: <macrobaselib.inc> (ver.1.0.3 beta) have included, only once!" 
;=== BEGIN "macrobaselib.inc" ==============================================

; Библиотека базовых Макроопределений, 
; расширяющая стандартный набор ассемблерных инструкций микроконтроллеров Atmel AVR (8-bit AVR Instruction Set), 
; и рекомендующая парадигму программирования: с хранением "модели прикладных данных" в ОЗУ (SRAM) и использованием нескольких регистров (РОН) под "временные переменные"...



;---------------------------------------------------------------------------

; Рекомендации по использованию "индексных регистров" в коде:
; 	* РОН из диапазона R26..R31 лучше не использовать для иных нужд, чем как "индексные регистры" (т.е. не рекомендуется использовать их в качестве "временных переменных").
; 	При реализации алгоритма функции, в коде, часто удобно использовать инструкции "непрямой пересылки данных": LD/ST, LDD/STD.
; 	Возникает вопрос выбора: какие при этом лучше использовать "индексные регистры": X, Y, или Z ?
; 	* Регистр Z лучше не использовать для арифметических преобразований, особенно для "параметров функций" - потому что он требуется для инструкций "непрямого перехода" ICALL/IJMP (дальнобойностью до 64K слов), т.е. Z требуется для вызова самой функции. 	(Напомню, что в семействе МК "ATTiny*" нет инструкций "дальнего перехода" CALL/JMP - поэтому, в них, инструкции ICALL/IJMP становятся единственной возможностью!)
; 	* А выбирая между X и Y - в первую очередь, лучше использовать Y, т.к. он более функционален: поддерживает также инструкции LDD/STD. 	(Кроме того, Y - чуть ближе к концу файла РОН, что "концептуальнее": меньше вероятность задействования его, в качестве "временного регистра".)

;---------------------------------------------------------------------------

; Основные положения о "временных переменных" (переменных общего назначения):
;	В коде данной библиотеки, и при работе с ней, регистры R16,R17,R18,R19
;	рекомендуется использовать как "временные переменные" - поэтому, 
;	в вашем прикладном коде, не используйте их для хранения постоянных данных!
; Как сказал DI HALT: 
;	При вычислениях, регистры можно предварительно заталкивать в стек... 
;	но я дам тебе лучше другой совет:
;	когда пишешь программу, продумывай алгоритм так, чтобы использовать регистры 
;	как сплошной TEMP, данные которого актуальны только здесь и сейчас.
;	И что с ними будет в следующей процедуре - уже не важно.
;	А все перманентные данные - следует сохранять в оперативке (SRAM).



; Совет: вы также можете использовать эти псевдонимы регистров в своей программе:
.def	temp	= R16
.def	temp1	= R16
.def	temp2	= R17
.def	temp3	= R18	; пока не используется данной библиотекой
.def	temp4	= R19	; пока не используется данной библиотекой



;***************************************************************************
;*
;*  Начальная Инициализация МК
;*
;***************************************************************************



; Инициализация Стека
	.MACRO	STACKINIT
	.if	RAMEND>0x0000			; Только для МК имеющих SRAM...

		LDI	temp,	Low(RAMEND)	; адрес вершины стека = конец памяти (мл.байт)
		.ifdef	SPL
		OUT	SPL,	temp
		.else
		OUT	SP,	temp
		.endif

		.ifdef	SPH
		LDI	temp,	High(RAMEND)	; адрес вершины стека = конец памяти (ст.байт)
		OUT	SPH,	temp
		.endif

	.else
	.WARNING "This device have not SRAM. Can not initialize Stack!"
	.endif	//RAMEND
	.ENDM



; Очистка памяти
	.MACRO	RAMFLUSH
		LDI	ZL, Low (SRAM_START)	; Адрес начала ОЗУ в индекс
		LDI	ZH, High(SRAM_START)
		CLR	temp			; Очищаем R16

		LOOP__RAMFLUSH:
		ST	Z+, temp		; Сохраняем 0 в ячейку памяти
		CPI	ZL, Low (RAMEND+1)	; Достигли конца оперативки?
		BRNE	LOOP__RAMFLUSH		; Нет? Крутимся дальше!

		CPI	ZH, High(RAMEND+1)	; А старший байт достиг конца?
		BRNE	LOOP__RAMFLUSH
	.ENDM



; Очистка РОН (Регистров Общего Назначения)
	.MACRO	GPRFLUSH
		LDI	ZL, 30	; +-----------------------+
		CLR	ZH	; | Очистка РОН (R00-R31) |
		DEC	ZL	; |                       |
		ST	Z, ZH	; | [всего 10 байт кода!] |
		BRNE	PC-2	; +-----------------------+
	.ENDM



; Disable Watchdog timer permanently (ensure)
; (Supported: ATTiny25/45/85)
; Note:	If the watchdog timer is not going to be used in the application (fuse must be: "WDTON"=Unprogrammed, "Safety Level 1"), 
;	it is important to go through a watchdog disable procedure  in the initialization of the device. 
;	If the Watchdog is accidentally enabled, for example by a runaway pointer or brown-out condition, the device will be reset, which in turn will lead to a new watchdog reset. 
;	To avoid this situation, the application software should always clear the WDRF flag and the WDE control bit in the initialization routine. (c) Atmel datasheet
	.MACRO	WDTOFF
		wdr
		; Clear WDRF in MCUSR
		ldi	temp,	(0<<WDRF)
		out	MCUSR,	temp
		; Write logical one to WDCE and WDE
		; Keep old prescaler setting to prevent unintentional Watchdog Reset
		in	temp,	WDTCR
		ori	temp,	(1<<WDCE)|(1<<WDE)
		out	WDTCR,	temp
		; Turn off WDT
		ldi	temp,	(0<<WDE)
		out	WDTCR,	temp
	.ENDM



;***************************************************************************
;*
;*  Поддержка ВВОДа / ВЫВОДа
;*
;***************************************************************************
; Где: РВВ - Регистры Ввода Вывода (I/O registers).



; Универсальный вывод в РВВ (порты): стандартного диапазона адресации (0x00-0x3F), и сверх диапазона ("MEMORY MAPPED": >= 0x60),
; в т.ч. в любую ячейку оперативной памяти (Location < 64K bytes).


; Выгрузка числа @1 в порт @0 (OUTput "Immediate")
; Пример вызова:  OUTI port,byte
; Памятка: портит содержимое регистра TEMP.
	.MACRO	OUTI
		LDI	temp, @1
	.if	@0 < 0x40
		OUT	@0, temp
	.else
		STS	@0, temp
	.endif
	.ENDMACRO


; Выгрузка [содержимого] регистра @1 в порт @0 (OUTput from "Register")
; Пример вызова:  OUTR port,register
	.MACRO	OUTR
	.if	@0 < 0x40
		OUT	@0, @1
	.else
		STS	@0, @1
	.endif
	.ENDMACRO


; Загрузка [содержимого] порта @1 в регистр @0 (INput to "Register")
; Пример вызова:  INR register,port
	.MACRO	INR
	.if	@1 < 0x40		; IO
		IN	@0, @1
	.else				; Memory
		LDS	@0, @1
	.endif
	.ENDM



;---------------------------------------------------------------------------


; Сохранение/Восстановление регистров SREG и TEMP в Стеке.
; (данные макросы используются парно, обычно, в обработчиках прерываний)


	.MACRO PUSHF
		PUSH	temp
		IN	temp, SREG
		PUSH	temp
	.ENDM

; Примечание: портит содержимое регистра TEMP на период обработки прерывания (между PUSHF..POPF)!
; Однако же, также и восстанавливает потом (после POPF) - что делает регистр TEMP: "защищённой" доступной "временной переменной", внутри обработчика прерывания.

	.MACRO POPF
		POP	temp
		OUT	SREG, temp
		POP	temp
	.ENDM


;---------------------------------------------------------------------------


; Универсальная манипуляция битами в РВВ (портах): 
; с младшими (0x00-0x1F), старшими (0x20-0x3F) адресами, 
; и сверх диапазона ("MEMORY MAPPED": >= 0x60)



; SET BIT via REG
; Пример вызова:  SETB byte,bit
; Памятка: портит содержимое регистра TEMP... но при работе с битами младших РВВ (0x00-0x1F) - вспомогательный регистр TEMP не используется (сохраняется).
	.MACRO	SETB
	.if	@0 < 0x20		; Low IO
		SBI	@0,	@1
	.elif	@0 < 0x40		; High IO
		IN	temp,	@0
		ORI	temp,	1<<@1
		OUT	@0,	temp
	.else				; Memory
		LDS	temp,	@0
		ORI	temp,	1<<@1
		STS	@0,	temp
	.endif
	.ENDM


; CLEAR BIT via REG
; Пример вызова:  CLRB byte,bit
; Памятка: портит содержимое регистра TEMP... но при работе с битами младших РВВ (0x00-0x1F) - вспомогательный регистр TEMP не используется (сохраняется).
	.MACRO	CLRB
	.if	@0 < 0x20		; Low IO
		CBI	@0,	@1
	.elif	@0 < 0x40		; High IO
		IN	temp,	@0
		ANDI	temp,	~(1<<@1)
		OUT	@0,	temp
	.else				; Memory
		LDS	temp,	@0
		ANDI	temp,	~(1<<@1)
		STS	@0,	temp
	.endif
	.ENDM


; INVERT BIT via REG
; Пример вызова:  INVB byte,bit
; Памятка: портит содержимое регистров TEMP1,TEMP2.
	.MACRO	INVB
	.if	@0 < 0x40		; IO
		IN	temp1,	@0
		LDI	temp2,	1<<@1
		EOR	temp1,	temp2
		OUT	@0,	temp1
	.else				; Memory
		LDS	temp1,	@0
		LDI	temp2,	1<<@1
		EOR	temp1,	temp2
		STS	@0,	temp1
	.endif
	.ENDM


; Примечание: "Transfer bit" позволяет Тестировать значение бита и делать Условные переходы (BRTS/BRTC).


; STORE BIT to "Transfer bit" [T <- Location(bit)] via REG
; Пример вызова:  STOREB byte,bit
; Памятка: портит содержимое регистра TEMP.
	.MACRO	STOREB
	.if	@0 < 0x40		; IO
		IN	temp,	@0
		BST	temp,	@1
	.else				; Memory
		LDS	temp,	@0
		BST	temp,	@1
	.endif
	.ENDM


; LOAD BIT from "Transfer bit" [Location(bit) <- T] via REG
; Пример вызова:  LOADB byte,bit
; Памятка: портит содержимое регистра TEMP.
	.MACRO	LOADB
	.if	@0 < 0x40		; IO
		IN	temp,	@0
		BLD	temp,	@1
		OUT	@0,	temp
	.else				; Memory
		LDS	temp,	@0
		BLD	temp,	@1
		STS	@0,	temp
	.endif
	.ENDM


;---------------------------------------------------------------------------


; Примечание: макросы этой группы - "безопасные", 
; не требуют "вспомогательных регистров" TEMPx (резервируют их самостоятельно), 
; но работают дольше и используют оперативную память (Стек).



; SET BIT via STACK/MEMORY
; Пример вызова:  SETBM byte,bit
	.MACRO	SETBM 
	.if	@0 < 0x20		; Low IO
		SBI	@0,	@1
	.elif	@0 < 0x40		; High IO
		PUSH	R17
		IN	R17,	@0
		ORI	R17,	1<<@1
		OUT	@0,	R17
		POP	R17
	.else				; Memory
		PUSH	R17
		LDS	R17,	@0
		ORI	R17,	1<<@1
		STS	@0,	R17
		POP	R17
	.endif
	.ENDM


; CLEAR BIT via STACK/MEMORY
; Пример вызова:  CLRBM byte,bit
	.MACRO	CLRBM
	.if	@0 < 0x20		; Low IO
		CBI	@0,	@1
	.elif	@0 < 0x40		; High IO
		PUSH	R17
		IN	R17,	@0
		ANDI	R17,	~(1<<@1)
		OUT	@0,	R17
		POP	R17
	.else				; Memory
		PUSH	R17
		LDS	R17,	@0
		ANDI	R17,	~(1<<@1)
		STS	@0,	R17
		POP	R17
	.endif
	.ENDM


; INVERT BIT via STACK/MEMORY
; Пример вызова:  INVBM byte,bit
	.MACRO	INVBM
	.if	@0 < 0x40		; IO
		PUSH	R16
		PUSH	R17
		IN	R16,	@0
		LDI	R17,	1<<@1
		EOR	R17,	R16
		OUT	@0,	R17
		POP	R17
		POP	R16
	.else				; Memory
		PUSH	R16
		PUSH	R17
		LDS	R16,	@0
		LDI	R17,	1<<@1
		EOR	R17,	R16
		STS	@0,	R17
		POP	R17
		POP	R16
	.endif
	.ENDM



; STORE BIT to "Transfer bit" [T <- Location(bit)] via STACK/MEMORY
; Пример вызова:  STOREBM byte,bit
	.MACRO	STOREBM
		PUSH	R16
	.if	@0 < 0x40		; IO
		IN	R16,	@0
		BST	R16,	@1
	.else				; Memory
		LDS	R16,	@0
		BST	R16,	@1
	.endif
		POP	R16
	.ENDM


; LOAD BIT from "Transfer bit" [Location(bit) <- T] via STACK/MEMORY
; Пример вызова:  LOADBM byte,bit
	.MACRO	LOADBM
		PUSH	R16
	.if	@0 < 0x40		; IO
		IN	R16,	@0
		BLD	R16,	@1
		OUT	@0,	R16
	.else				; Memory
		LDS	R16,	@0
		BLD	R16,	@1
		STS	@0,	R16
	.endif
		POP	R16
	.ENDM



;***************************************************************************
;*
;*  Арифметические операции: 8-битные
;*
;***************************************************************************


; Инкрементация (+1) однойбайтной переменной @0 в памяти
; Памятка: портит содержимое регистра TEMP.
	.MACRO	INC8M
		LDS	temp,	@0
		SUBI	temp,	(-1)
		STS	@0,	temp
	.ENDM


; Декрементация (-1) однойбайтной переменной @0 в памяти
; Памятка: портит содержимое регистра TEMP.
	.MACRO	DEC8M
		LDS	temp,	@0
		SUBI	temp,	(1)
		STS	@0,	temp
	.ENDM


; Обнуление однобайтной переменной @0 в памяти
; Памятка: портит содержимое регистра TEMP.
	.MACRO	CLR8M
		CLR	temp		; Тут нам нужен ноль...
		STS	@0,	temp	; Ноль в первый байт счетчика в RAM
	.ENDM



;***************************************************************************
;*
;*  Арифметические операции: 16-битные
;*
;***************************************************************************


; Инкрементация (+1) двухбайтной переменной @0 в памяти (по адресу @0 - мл.байт, little-endian)
; Памятка: портит содержимое регистра TEMP.
	.MACRO	INC16M
		LDS	temp,	@0
		SUBI	temp,	(-1)
		STS	@0,	temp

		LDS	temp,	@0+1
		SBCI	temp,	(-1)
		STS	@0+1,	temp
	.ENDM


; Декрементация (-1) двухбайтной переменной @0 в памяти (по адресу @0 - мл.байт, little-endian)
; Памятка: портит содержимое регистра TEMP.
	.MACRO	DEC16M
		LDS	temp,	@0
		SUBI	temp,	(1)
		STS	@0,	temp

		LDS	temp,	@0+1
		SBCI	temp,	(0)
		STS	@0+1,	temp
	.ENDM


; Обнуление двухбайтной переменной @0 в памяти
; Памятка: портит содержимое регистра TEMP.
	.MACRO	CLR16M
		CLR	temp		; Тут нам нужен ноль...
		STS	@0,	temp	; Ноль в первый байт счетчика в RAM
		STS	@0+1,	temp	; Ноль в второй байт счетчика в RAM
	.ENDM


;---------------------------------------------------------------------------


; Арифметическое вычитание 16-битной константы @2 из регистровой пары @0:@1
; (Это эмуляция стандартной 16-битной инструкции, которая есть не во всех МК:  SBIW  YH:YL,0x1234 )
; Пример вызова:  SUBI16  YH,YL,0x1234
	.MACRO	SUBI16			; Start macro definition 
		SUBI	@1, Low(@2)	; Subtract low byte 
		SBCI	@0, High(@2)	; Subtract high byte 
	.ENDM				; End macro definition



;***************************************************************************
;*
;*  Арифметические операции: 32-битные
;*
;***************************************************************************


; Инкрементация (+1) четырехбайтной переменной @0 в памяти (по адресу @0 - мл.байт, little-endian)
; Памятка: портит содержимое регистра TEMP.
	.MACRO	INC32M
		LDS	temp,	@0
		SUBI	temp,	(-1)
		STS	@0,	temp

		LDS	temp,	@0+1
		SBCI	temp,	(-1)
		STS	@0+1,	temp

		LDS	temp,	@0+2
		SBCI	temp,	(-1)
		STS	@0+2,	temp

		LDS	temp,	@0+3
		SBCI	temp,	(-1)
		STS	@0+3,	temp
	.ENDM


; Декрементация (-1) четырехбайтной переменной @0 в памяти (по адресу @0 - мл.байт, little-endian)
; Памятка: портит содержимое регистра TEMP.
	.MACRO	DEC32M
		LDS	temp,	@0
		SUBI	temp,	(1)
		STS	@0,	temp

		LDS	temp,	@0+1
		SBCI	temp,	(0)
		STS	@0+1,	temp

		LDS	temp,	@0+2
		SBCI	temp,	(0)
		STS	@0+2,	temp

		LDS	temp,	@0+3
		SBCI	temp,	(0)
		STS	@0+3,	temp
	.ENDM


; Обнуление четырехбайтной переменной @0 в памяти
; Памятка: портит содержимое регистра TEMP.
	.MACRO	CLR32M
		CLR	temp		; Тут нам нужен ноль...
		STS	@0,	temp	; Ноль в первый байт счетчика в RAM
		STS	@0+1,	temp	; Ноль в второй байт счетчика в RAM
		STS	@0+2,	temp	; Ноль в третий байт счетчика в RAM
		STS	@0+3,	temp	; Ноль в четвёртый байт счетчика в RAM
	.ENDM




; Для примера: Версия DI HALT - более наглядная, но требует аж 4 временных регистра! (неоптимально)
; Памятка: портит содержимое регистров: R16,R17,R18,R19.
;	.MACRO	INC32M
;		LDS	R16,@0
;		LDS	R17,@0+1
;		LDS	R18,@0+2
;		LDS	R19,@0+3
;
;		SUBI	R16,(-1)	; инкремент четырехбайтного числа R19:R18:R17:R16
;		SBCI	R17,(-1)
;		SBCI	R18,(-1)
;		SBCI	R19,(-1)
;
;		STS	@0,R16
;		STS	@0+1,R17
;		STS	@0+2,R18
;		STS	@0+3,R19	
;	.ENDM



;=== END "macrobaselib.inc" ================================================
; coded by (c) DI HALT, 2008  http://easyelectronics.ru/
; coded by (c) Celeron, 2013  http://inventproject.info/
.ENDIF
