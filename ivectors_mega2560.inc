;=== BEGIN "ivectors.inc" ==================================================

; Тут располагается "таблица векторов на обработчики прерываний" - важнейшая вещь! 
; Если про нее забыть, то можно огрести такие неразрешимые глюки, что просто капец. Так что не забываем...
; (Вынесено из общего кода в отдельный файл - для облегчения портирования программы на другой камень...)
;
; КАК СДЕЛАТЬ: 
; Скопировать секцию "INTERRUPT VECTORS" из HAL-файла Микроконтроллера (например, "tn85def.inc"),
; Заменить все ".equ" на ".org"... и дописать ко всем точкам входа команду RETI.
; (Это на тот случай, если Прерывание случайно вылезет - чтобы вернуть управление туда, откуда прервалось.)


; ***** INTERRUPT VECTORS ************************************************
; Используем HAL Микроконтроллера "ATmega2560"

			.ORG	INT0addr	; External Interrupt Request 0
			RETI
			.ORG	INT1addr	; External Interrupt Request 1
			RETI
			.ORG	INT2addr	; External Interrupt Request 2
			RETI
			.ORG	INT3addr	; External Interrupt Request 3
			RETI
			.ORG	INT4addr	; External Interrupt Request 4
			RETI
			.ORG	INT5addr	; External Interrupt Request 5
			RETI
			.ORG	INT6addr	; External Interrupt Request 6
			RETI
			.ORG	INT7addr	; External Interrupt Request 7
			RETI
			.ORG	PCI0addr	; Pin Change Interrupt Request 0
			RETI
			.ORG	PCI1addr	; Pin Change Interrupt Request 1
			RETI
			.ORG	PCI2addr	; Pin Change Interrupt Request 2
			RETI
			.ORG	WDTaddr		; Watchdog Time-out Interrupt
			RETI
			.ORG	OC2Aaddr	; Timer/Counter2 Compare Match A
			RETI
			.ORG	OC2Baddr	; Timer/Counter2 Compare Match B
			RETI
			.ORG	OVF2addr	; Timer/Counter2 Overflow
			RETI
			.ORG	ICP1addr	; Timer/Counter1 Capture Event
			RETI
			.ORG	OC1Aaddr	; Timer/Counter1 Compare Match A
			RETI
			.ORG	OC1Baddr	; Timer/Counter1 Compare Match B
			RETI
			.ORG	OC1Caddr	; Timer/Counter1 Compare Match C
			RETI
			.ORG	OVF1addr	; Timer/Counter1 Overflow
			RETI
			.ORG	OC0Aaddr	; Timer/Counter0 Compare Match A
			RETI
			.ORG	OC0Baddr	; Timer/Counter0 Compare Match B
			RETI

			.ORG	OVF0addr	; Timer/Counter0 Overflow
		.ifdef	RTOS_USE_TIMER0_OVERFLOW
			RJMP	RTOS_TIMER_HANDLER
		.else
			RETI
		.endif

			.ORG	SPIaddr		; SPI Serial Transfer Complete
			RETI
			.ORG	URXC0addr	; USART0, Rx Complete
			RETI
			.ORG	UDRE0addr	; USART0 Data register Empty
			RETI
			.ORG	UTXC0addr	; USART0, Tx Complete
			RETI
			.ORG	ACIaddr		; Analog Comparator
			RETI
			.ORG	ADCCaddr	; ADC Conversion Complete
			RETI
			.ORG	ERDYaddr	; EEPROM Ready
			RETI
			.ORG	ICP3addr	; Timer/Counter3 Capture Event
			RETI
			.ORG	OC3Aaddr	; Timer/Counter3 Compare Match A
			RETI
			.ORG	OC3Baddr	; Timer/Counter3 Compare Match B
			RETI
			.ORG	OC3Caddr	; Timer/Counter3 Compare Match C
			RETI
			.ORG	OVF3addr	; Timer/Counter3 Overflow
			RETI
			.ORG	URXC1addr	; USART1, Rx Complete
			RETI
			.ORG	UDRE1addr	; USART1 Data register Empty
			RETI
			.ORG	UTXC1addr	; USART1, Tx Complete
			RETI
			.ORG	TWIaddr		; 2-wire Serial Interface
			RETI
			.ORG	SPMRaddr	; Store Program Memory Read
			RETI
			.ORG	ICP4addr	; Timer/Counter4 Capture Event
			RETI
			.ORG	OC4Aaddr	; Timer/Counter4 Compare Match A
			RETI
			.ORG	OC4Baddr	; Timer/Counter4 Compare Match B
			RETI
			.ORG	OC4Caddr	; Timer/Counter4 Compare Match C
			RETI
			.ORG	OVF4addr	; Timer/Counter4 Overflow
			RETI
			.ORG	ICP5addr	; Timer/Counter5 Capture Event
			RETI
			.ORG	OC5Aaddr	; Timer/Counter5 Compare Match A
			RETI
			.ORG	OC5Baddr	; Timer/Counter5 Compare Match B
			RETI
			.ORG	OC5Caddr	; Timer/Counter5 Compare Match C
			RETI
			.ORG	OVF5addr	; Timer/Counter5 Overflow
			RETI
			.ORG	URXC2addr	; USART2, Rx Complete
			RETI
			.ORG	UDRE2addr	; USART2 Data register Empty
			RETI
			.ORG	UTXC2addr	; USART2, Tx Complete
			RETI
			.ORG	URXC3addr	; USART3, Rx Complete
			RETI
			.ORG	UDRE3addr	; USART3 Data register Empty
			RETI
			.ORG	UTXC3addr	; USART3, Tx Complete
			RETI


			.ORG	INT_VECTORS_SIZE	; Конец таблицы прерываний


;=== END "ivectors.inc" ====================================================
