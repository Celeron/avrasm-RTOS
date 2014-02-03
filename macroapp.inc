.IFNDEF	_MACROAPP__INCLUDED_
.EQU	_MACROAPP__INCLUDED_ = 1
.MESSAGE "Note: <macroapp.inc> have included, only once!" 
;=== BEGIN "macroapp.inc" ==================================================
; Прикладные Макроопределения, используемые при реализации логики приложения.
; (Примечание: эти макросы - "ПриложениеЗависимы"!)



;***** BEGIN "RTOS Tasks" section ******************************************

; Псевдонимы для номеров-индексов Задач (используются как параметры макросов PLAN_TASK_*):
; Указание: определяйте их как вам угодно - они используются только для удобства программирования прикладного кода, и не используются кодом ядра RTOS...
.equ	TS_Send		= 1		; 
.equ	TS_Led_Blink	= 2		; 
.equ	TS_TextTime	= 3		; 
.equ	TS_Task4	= 4		; 
.equ	TS_Task5	= 5		; 
.equ	TS_Task6	= 6		; 
.equ	TS_Task7	= 7		; 
.equ	TS_Task8	= 8		; 
.equ	TS_Task9	= 9		; 

;***** END "RTOS Tasks" section



	.MACRO 	LED_OFF
		CBI		LED_P,LED
	.ENDM


	.MACRO 	LED_ON
		SBI		LED_P,LED
	.ENDM


	.MACRO 	ENABLE_ON
		SBI		ENABLE_P,ENABLE
	.ENDM


	.MACRO 	ENABLE_OFF
		CBI		ENABLE_P,ENABLE
	.ENDM



;=== END "macroapp.inc" ====================================================
.ENDIF
