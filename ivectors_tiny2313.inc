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
; Используем HAL Микроконтроллера "ATtiny2313"

			.ORG	INT0addr		; External Interrupt Request 0
			RETI
			.ORG	INT1addr		; External Interrupt Request 1
			RETI	
			.ORG	ICP1addr		; Timer/Counter1 Capture Event
			RETI
			.ORG	OC1Aaddr		; Timer/Counter1 Compare Match A
			RETI
			.ORG	OVF1addr		; Timer/Counter1 Overflow
			RETI

			.ORG	OVF0addr		; Timer/Counter0 Overflow
		.ifdef	RTOS_USE_TIMER0_OVERFLOW
			RJMP	RTOS_TIMER_HANDLER
		.else
			RETI
		.endif

			.ORG	URXCaddr		; USART, Rx Complete
			RETI
			.ORG	UDREaddr		; USART Data Register Empty
			RETI
			.ORG	UTXCaddr		; USART, Tx Complete
			RETI
			.ORG	ACIaddr			; Analog Comparator
			RETI
			.ORG	PCIaddr			; 
			RETI
			.ORG	OC1Baddr		; 
			RETI
			.ORG	OC0Aaddr		; 
			RETI
			.ORG	OC0Baddr		; 
			RETI
			.ORG	USI_STARTaddr		; USI Start Condition
			RETI
			.ORG	USI_OVFaddr		; USI Overflow
			RETI
			.ORG	ERDYaddr		; 
			RETI
			.ORG	WDTaddr			; Watchdog Timer Overflow
			RETI


			.ORG	INT_VECTORS_SIZE	; Конец таблицы прерываний


;=== END "ivectors.inc" ====================================================
