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
; Используем HAL Микроконтроллера "ATtiny13(A)"

			.ORG	INT0addr		; External Interrupt 0
			RETI
			.ORG	PCI0addr		; External Interrupt Request 0
			RETI

			.ORG	OVF0addr		; Timer/Counter0 Overflow
		.ifdef	RTOS_USE_TIMER0_OVERFLOW
			RJMP	RTOS_TIMER_HANDLER
		.else
			RETI
		.endif

			.ORG	ERDYaddr		; EEPROM Ready
			RETI
			.ORG	ACIaddr			; Analog Comparator
			RETI
			.ORG	OC0Aaddr		; Timer/Counter Compare Match A
			RETI
			.ORG	OC0Baddr		; Timer/Counter Compare Match B
			RETI
			.ORG	WDTaddr			; Watchdog Time-out
			RETI
			.ORG	ADCCaddr		; ADC Conversion Complete
			RETI


			.ORG	INT_VECTORS_SIZE	; Конец таблицы прерываний


;=== END "ivectors.inc" ====================================================
