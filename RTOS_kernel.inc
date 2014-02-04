.MESSAGE "<-- RTOS ver.1.9.1 alpha -->"
;=== BEGIN "RTOS-kernel.inc" ===============================================

; ���� RTOS
; (��������������: ���� ��� ������ �� �����, ���������������, ��� ������ �� ������������ � ���������� ����!)


.ifdef	RTOS_FEATURE_PLANTASK
;***************************************************************************
;*
;*  ������� ��������������� �����
;*
;***************************************************************************


.ifndef	RTOS_OPTIMIZE_FOR_SMALL_MEMORY	; ����������: ������ ����� ������������� ��������� ��� ������������� RTOS, ���� ������������ RAMFLUSH (macrobaselib.inc) ��� ������������� ����������������.
;---------------------------------------------------------------------------
; 
; �������� "������ �����" RTOS_TaskQueue (�������������������)
;
; ������ ����� ������� �������� ������ ���, �� ����� ������������� ��, ����� ������� �������� ������ ��� �� ������.
; �����, �� ����� ����������, ��� ���������� �������������� ����������, ������ ������� ����� ������������� �������� ��� ������� �����. (�� ������������� � �������������!)
; 
;---------------------------------------------------------------------------

;----- Subroutine Register Variables

; ��� ����������.

; �������: ������ ����� �� ������ ���������� ��� - ��������� �������� ������������ �������� ������.
; �������: SREG �����������.

;----- Code


RTOS_METHOD_ClearTaskQueue:

		PUSHF						; ��������� temp1 � SREG � �����	(������: ������, ����������� ������� ��������� "����� ����������")
		PUSH	temp2
		PUSH	ZH
		PUSH	ZL

		LDI	ZL,	Low (RTOS_TaskQueue)
		LDI	ZH,	High(RTOS_TaskQueue)
		LDI	temp1,	$00				; = ����.��� "��� ������"
		LDI	temp2,	RTOS_TaskQueueSize		; ���������� �����

		;=== ������ ����������� ������
		CLI						; ��������� ����������
	Loop__RTOS_METHOD_ClearTaskQueue:
		ST	Z+,	temp1
		DEC	temp2
		BRNE	Loop__RTOS_METHOD_ClearTaskQueue

		POP	ZL
		POP	ZH
		POP	temp2
		POPF						; Restore global interrupt flag
		;=== ����� ����������� ������
		RET



.endif	//RTOS_OPTIMIZE_FOR_SMALL_MEMORY

;---------------------------------------------------------------------------
;
; �������� ������ � ������� RTOS_TaskQueue, 
; ������� �� ����������
;
;---------------------------------------------------------------------------

;----- Subroutine Register Variables

;.def	TaskCode	= (��.����)	; (�������, �� ����������)	�����-������ ������ � ������� RTOS_TaskProcs

; �������: � ������ ��� ����������� RTOS_OPTIMIZE_FOR_SMALL_MEMORY: ����� ������������, ������ ��������� �� ������ - ��������� �������� ������������ �������� ������.
; �������: � ������  � ������������ RTOS_OPTIMIZE_FOR_SMALL_MEMORY: ������ ������ ��������� - ����������/������ ���������� ��������� TEMP1, TEMP2, TEMP3, ZH:ZL.
; �������: SREG ����������� ������.

;----- Code


RTOS_METHOD_AddTask:

	.ifndef	RTOS_OPTIMIZE_FOR_SMALL_MEMORY
		PUSH	temp1
		PUSH	temp2
		PUSH	temp3
		PUSH	ZL
		PUSH	ZH
	.endif

		;=== ������ ����������� ������			; (����������: ������+������ ����������� ������ - ������ ���� � ������ ����������!)
		INR	temp3,	SREG				; Save global interrupt flag
		CLI						; ��������� ����������

		; ��������� ������� ������� �� �����, ���� ������ ������ ������:
		LDI	ZL,	Low (RTOS_TaskQueue)
		LDI	ZH,	High(RTOS_TaskQueue)
		LDI	temp2,	RTOS_TaskQueueSize		; ���������� ����� (�������������)
	Loop__RTOS_METHOD_AddTask:
		LD	temp1,	Z+
		CPI	temp1,	$00				; ��������: ������ ������?
		BREQ	Found__RTOS_METHOD_AddTask		; ���� ������ - ��������� ������������ �� ���� �������...
		DEC	temp2					; ���������� ����� (�������������)
		BRNE	Loop__RTOS_METHOD_AddTask

	.ifndef	RTOS_OPTIMIZE_FOR_SMALL_MEMORY
		; ���� �������� ����� �������, �� ��� � �� ����� ������ ������ - ������!
		LDI	ErrorCode,	10			; ���: 10 - "������� �����" �����������, �� ���� �������� ����� ������!
		RCALL	RTOS_METHOD_ErrorHandler
	.endif
		RJMP	End__RTOS_METHOD_AddTask

		; ���� ����� ������ ����� (����������, ��� ������� "�����" �������) - �� ��������� ������� ��������� �������:
	Found__RTOS_METHOD_AddTask:
		ST	-Z,	TaskCode

	End__RTOS_METHOD_AddTask:
		OUTR	SREG,	temp3				; Restore global interrupt flag
		;=== ����� ����������� ������

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
; ���������� ������� ����� (���������)
;
; ��. �������� ���� http://easyelectronics.ru/avr-uchebnyj-kurs-operacionnaya-sistema-dispetcher-zadach.html
; 
;---------------------------------------------------------------------------

;----- Subroutine Register Variables

; ��� ����������.

; �������: � ������ ��� ����������� RTOS_OPTIMIZE_FOR_SMALL_MEMORY: ����� ZH:ZL (���������� ����������� ��� "Indirect Jump" � ������), ������ ��������� �� ������ - ��������� �������� ������������ �������� ������.
; �������: � ������  � ������������ RTOS_OPTIMIZE_FOR_SMALL_MEMORY: ������ ������ ��������� - ����������/������ ���������� ��������� TEMP1, TEMP2, TEMP3, TEMP4 � ZH:ZL.
; ����������: ����, � ������ ������, ������������� ��������� ��������, �.�. ������ ����� ���������� �� ���������� "MAIN", ����� ��������� ���������� ����� - ����� ���������� ���������, ������, �� ���������.
; �������: SREG ����������� ������.

;----- Code


RTOS_METHOD_ProcessTaskQueue:

	.ifndef	RTOS_OPTIMIZE_FOR_SMALL_MEMORY			; �����������! ���� ������-��, ��������� �������� - �������, � ������ (�� �������������) ������...
		PUSH	temp1
		PUSH	temp2
		PUSH	temp3
		PUSH	temp4
	.endif

		;=== ������ ����������� ������			; (����������: ������+������ ����������� ������ - ������ ���� � ������ ����������!)
		INR	temp4,	SREG				; Save global interrupt flag
		CLI						; ��������� ����������

		; ��������� ��� ������� ������ � ����������:
		; ����������: ��� ������� ���� � ������ �������, ����� �������� ��� ������� ������ == 0 (����.��� "��� ������") - � ����� ������, �����, ����� ���������� ������ ��������� ������ "�������� ����" (Task_Idle)...
		LDI	ZL,	Low (RTOS_TaskQueue)		; ��������� ����� ������ "������� �����" -> � ������� Z
		LDI	ZH,	High(RTOS_TaskQueue)
		LD	temp3,	Z				; ��������� ������ ���� �� ������� RTOS_TaskQueue[0] -> temp3

		; �������� "������� �����" �� ���� ������� �����:
		LDI	temp2,	RTOS_TaskQueueSize-1		; ���������� ����� (�������������)
	Loop__RTOS_METHOD_ProcessTaskQueue:
		LDD	temp1,	Z+1 				; RTOS_TaskQueue[i+1] -> temp1
		ST	Z+,	temp1				; temp1 -> RTOS_TaskQueue[i++]
		CPI	temp1,	$00				; ���������� RTOS_TaskQueue[i] �� ����.����� "��� ������"
		BREQ	EndShift__RTOS_METHOD_ProcessTaskQueue	; ���� �����? ������, ����� ������� ���� - ���������� ��� �����...
		DEC	temp2					; ���������� ����� (�������������)
		BRNE	Loop__RTOS_METHOD_ProcessTaskQueue
		ST	Z,	temp2				; � ��������� ������� ������� ��������� ����.��� "��� ������":	RTOS_TaskQueue[RTOS_TaskQueueSize]=0	(������, ��� ������, temp2==0)
	EndShift__RTOS_METHOD_ProcessTaskQueue:
	
		OUTR	SREG,	temp4				; Restore global interrupt flag
		;=== ����� ����������� ������


		; �������� �������� ����� ����� ����� � ������������ ������:
		; ����������: �������� ���������� �� "��������� ���������" - ��. ���������� ����� � "AVR. ������� ����. ��������� �� ��������� ���������" (�) http://easyelectronics.ru/avr-uchebnyj-kurs-vetvleniya.html
		; (������� ���������: temp3 == ��� ������� ������ � ����������)

	.ifdef	MAPPED_FLASH_START
		; ������� ���� ��� AVR8L Instruction Set: ��� ��� ��� R0:R15, � ��� ���������� LPM (������ �� LD).
		; see recipe: http://www.avrfreaks.net/index.php?module=PNphpBB2&file=viewtopic&t=82385&highlight=
		; ���������: ��� �������� �� ATtiny10!
		
		MOV	ZL,	temp3				; ������ ������� ������ -> ZL
		CLR	ZH
		LSL	ZL					; temp1 *= 2
		ADC	ZH,	ZH				; saving carry bit	(������: � ����������, � ���� ������������ �������-������� ZH ����� � ��� ������ �������-��������. ��� � ��� - ������ ����. � ����� ��� ��������� �������������, ������ ��� �� �������� =0, � ���� �����.)
		SUBI	ZL,	Low (-(RTOS_TaskProcs*2 + MAPPED_FLASH_START))
		SBCI	ZH,	High(-(RTOS_TaskProcs*2 + MAPPED_FLASH_START))	; Add
		
		LD	temp1,	Z+				; ��������� �� "��������� �������" ��.���� ������ �������� -> temp1
		LD	temp2,	Z				; ��������� �� "��������� �������" ��.���� ������ �������� -> temp2
		
	.else	//������� ���� ��� ��������� AVR

		MOV	ZL,	temp3				; ������ ������� ������ -> ZL
		CLR	ZH
		LSL	ZL					; temp1 *= 2
		ADC	ZH,	ZH				; saving carry bit	(������: � ����������, � ���� ������������ �������-������� ZH ����� � ��� ������ �������-��������. ��� � ��� - ������ ����. � ����� ��� ��������� �������������, ������ ��� �� �������� =0, � ���� �����.)
		SUBI	ZL,	Low (-RTOS_TaskProcs*2)
		SBCI	ZH,	High(-RTOS_TaskProcs*2)		; Add
	
		LPM	temp1,	Z+				; ��������� �� "��������� �������" ��.���� ������ �������� -> temp1
		LPM	temp2,	Z				; ��������� �� "��������� �������" ��.���� ������ �������� -> temp2
	.endif

		; ������� � ���������� ������:

		MOV	ZL,	temp1				; ��������� ����� ����� ����� � ������ -> � ������� Z
		MOV	ZH,	temp2

		OUTR	SREG,	temp4				; Restore SREG also (to be transparent)
	.ifndef	RTOS_OPTIMIZE_FOR_SMALL_MEMORY
		POP	temp4
		POP	temp3
		POP	temp2
		POP	temp1
	.endif

		; ����: ����� ������������, � ��������������� "������ ������" �� ��.
		; ����: ������ ������������ RCALL, �� ������ � ������������ ������� RJMP, �������������� ��������� � ���� ����� �������� "����� ������".
		; ��������� �����: ��������� ����� ��������� ������������ �������������� ����� ����� ������� RET �� ���� �������� ���������. ����� �������, CALL + RET === JMP

		IJMP 						; ������ ������!!!
								; ������, ��� ����, ���������������� ������������� �����:
								; 	������ �������� - ���� �� ������ �� RCALL �� �������� �����.
								; 	������, � ����� � ��� ����� ����� ��������...
								; 	� ������ � ������ �� IJMP, ������� ���� �� ������.
								; 	�� ��� �� ������� - ���� �� ������ �� ������ �� RET!

.endif	//RTOS_FEATURE_PLANTASK

.ifdef	RTOS_FEATURE_PLANTASKTIMED
;***************************************************************************
;*
;*  ��������������� ����� �� �������
;*
;***************************************************************************


.ifndef	RTOS_OPTIMIZE_FOR_SMALL_MEMORY	; ����������: ������ ����� ������������� ��������� ��� ������������� RTOS, ���� ������������ RAMFLUSH (macrobaselib.inc) ��� ������������� ����������������.
;---------------------------------------------------------------------------
; 
; �������� "��� ��������" RTOS_TimersPool (�������������������)
; 
; ������ ����� ������� �������� ������ ���, �� ����� ������������� ��, ����� ������� �������� ������ ��� �� ������.
; �����, �� ����� ����������, ��� ���������� �������������� ����������, ������ ������� ����� ������������� ���������/�������� ��� �������. (�� ������������� � �������������!)
;
;---------------------------------------------------------------------------

;----- Subroutine Register Variables

; ��� ����������.

; �������: ������ ����� �� ������ ���������� ��� - ��������� �������� ������������ �������� ������.
; �������: SREG �����������.

;----- Code


RTOS_METHOD_ClearTimers:

		PUSHF						; ��������� temp1 � SREG � �����	(������: ������, ����������� ������� ��������� "����� ����������")
		PUSH	temp2
		PUSH	ZH
		PUSH	ZL

		LDI	ZL,	Low (RTOS_TimersPool)
		LDI	ZH,	High(RTOS_TimersPool)
		LDI	temp1,	$00				; = ����.��� "��� ������"
		LDI	temp2,	RTOS_TimersPoolSize		; ���������� �����

		;=== ������ ����������� ������
		CLI						; ��������� ����������
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
		;=== ����� ����������� ������
		RET



.endif	//RTOS_OPTIMIZE_FOR_SMALL_MEMORY

;---------------------------------------------------------------------------
;
; ���������� (�������� ��� ��������) ������ � ���� RTOS_TimersPool, 
; ��� ����������� ������� ������
; 
;---------------------------------------------------------------------------

;----- Subroutine Register Variables

;.def	TaskCode	= (��.����)	; (�������, �� ����������)	�����-������ ������ � ������� RTOS_TaskProcs
;.def	TimeDelayLow	= (��.����)	; (�������, �� ����������)	����� ��������, � ������������ (��.����)
;.def	TimeDelayHigh	= (��.����)	; (�������, �� ����������)	����� ��������, � ������������ (��.����)

; �������: � ������ ��� ����������� RTOS_OPTIMIZE_FOR_SMALL_MEMORY: ����� ������������, ������ ��������� �� ������ - ��������� �������� ������������ �������� ������.
; �������: � ������  � ������������ RTOS_OPTIMIZE_FOR_SMALL_MEMORY: ������ ������ ��������� - ����������/������ ���������� ��������� TEMP1, TEMP2, TEMP3, TEMP4, ZH:ZL.
; �������: SREG ����������� ������.

;----- Code


RTOS_METHOD_AddTimer:

	.ifndef	RTOS_OPTIMIZE_FOR_SMALL_MEMORY
		PUSH	temp1
		PUSH	temp2
		PUSH	temp3
		PUSH	temp4
		PUSH	ZL
		PUSH	ZH
	.endif

		;=== ������ ����������� ������			; (����������: ������+������ ����������� ������ - ������ ���� � ������ ����������!)
		INR	temp2,	SREG				; Save global interrupt flag
		CLI						; ��������� ����������

		; ����: ���� ������������ ������ (��� �������), � ������, ���� ������ ������ (��� ������)
		LDI	ZL,	Low (RTOS_TimersPool)		; ��������� ����� ������ "���� ��������" -> � ������� Z
		LDI	ZH,	High(RTOS_TimersPool)
		LDI	temp3,	0				; ���������� ������� "��� �� �����" ��������� �� ������ ������ � ����, ��� ������ �������	(temp3=temp4=0)
		LDI	temp4,	0
	Loop__RTOS_METHOD_AddTimer:
		LD	temp1,	Z				; ��������� "������������� ������" ���������� �������

		CP	temp1,	TaskCode			; ���������: ��������� �� � ������� �������?
		BREQ	UpdateExisting__RTOS_METHOD_AddTimer	; ���� ��������� - ��� ��������� ���� ������...

		CPI	temp1,	$00				; ��������: ������ ������?
		BRNE	NotEmpty__RTOS_METHOD_AddTask
		MOV	temp3,	ZL				; ���� ����� ������ ������ - ���������� � �����, �� �������...
		MOV	temp4,	ZH	
	NotEmpty__RTOS_METHOD_AddTask:

		SUBI	ZL,	Low (-3)			; ��������� � ���������� �������:
		SBCI	ZH,	High(-3)			; 	����� Z+=3
		CPI	ZL,	Low (RTOS_TimersPool + RTOS_TimersPoolSize*3)	; ������� �����:
		BRNE	Loop__RTOS_METHOD_AddTimer				; 	���� ����� � Z �� ����� �� ����� ������...
		CPI	ZH,	High(RTOS_TimersPool + RTOS_TimersPoolSize*3)	; (����: ������, ����� �� ������������ ������������ ����������-�������, ����� ���������� �������)
		BRNE	Loop__RTOS_METHOD_AddTimer

		; (���������: ������������ ������ �� ������ - ����� �������� �����...)
		MOV	ZL,	temp3				; ��������� ��������� Z �� ��������� ������ ������, ��������� �����	(����������: ����������, ��� ��� ����� ����������� � �����, �� ��� �� �����)
		MOV	ZH,	temp4
		OR	temp3,	temp4				; ��������� ������: ���� "�� �����" ��������� �� ������ ������ � ����?	(temp3==0 and temp4==0 ?)
		BRNE	FillNew__RTOS_METHOD_AddTimer		; ���� ���� ����� - ������� � ���������� ������ �������...

	.ifndef	RTOS_OPTIMIZE_FOR_SMALL_MEMORY
		; ���� ������ ���� ���, �� ��� � �� ����� ������ ������ - ������!
		LDI	ErrorCode,	20			; ���: 20 - "��� ��������" ����������, �� ���� ���������������� ����� ������!
		RCALL	RTOS_METHOD_ErrorHandler
	.endif
		RJMP	End__RTOS_METHOD_AddTimer

		; ����������������� ������
	FillNew__RTOS_METHOD_AddTimer:
		ST	Z,	TaskCode
	UpdateExisting__RTOS_METHOD_AddTimer:
		STD	Z+1,	TimeDelayLow
		STD	Z+2,	TimeDelayHigh

	End__RTOS_METHOD_AddTimer:
		OUTR	SREG,	temp2				; Restore global interrupt flag
		;=== ����� ����������� ������

	.ifndef	RTOS_OPTIMIZE_FOR_SMALL_MEMORY
		POP	ZH
		POP	ZL
		POP	temp4
		POP	temp3
		POP	temp2
		POP	temp1
	.endif
		RET



.ifndef	RTOS_OPTIMIZE_FOR_SMALL_MEMORY	; ����������: ������ ����� �������� ��������������, ��� ���� ����� ��������.
;---------------------------------------------------------------------------
;
; ������ (�������) ������ �� ���� RTOS_TimersPool, 
; ��� ������ ����������� ������� ������
; 
;---------------------------------------------------------------------------

;----- Subroutine Register Variables

;.def	TaskCode	= (��.����)	; (�������, �� ����������)	�����-������ ������ � ������� RTOS_TaskProcs

; �������: ����� ������������, ������ ��������� �� ������ - ��������� �������� ������������ �������� ������.
; �������: SREG �����������.

;----- Code


RTOS_METHOD_RemoveTimer:

		PUSHF						; ��������� temp1 � SREG � �����	(������: ������, ����������� ������� ��������� "����� ����������")
		PUSH	temp2
		PUSH	ZH
		PUSH	ZL

		LDI	ZL,	Low (RTOS_TimersPool)
		LDI	ZH,	High(RTOS_TimersPool)
		LDI	temp2,	RTOS_TimersPoolSize		; ���������� �����

		;=== ������ ����������� ������
		CLI						; ��������� ����������
	Loop__RTOS_METHOD_RemoveTimer:
		LD	temp1,	Z				; ��������� "������������� ������" ���������� �������
		
		CP	temp1,	TaskCode			; ���������: ��������� �� � ������� �������?
		BRNE	NotFound__RTOS_METHOD_RemoveTimer
		LDI	temp1,	$00				; = ����.��� "��� ������"
		ST	Z,	temp1				; ��������� ������
		RJMP	End__RTOS_METHOD_RemoveTimer		; � ����� ��� �� �����, �.�. ������ � ����� ��������������� ����� ���� ������ ����...
	NotFound__RTOS_METHOD_RemoveTimer:

		SUBI	ZL,	Low(-3)				; ��������� � ���������� �������:
		SBCI	ZH,	High(-3)			; 	����� Z+=2
		DEC	temp2					; 	���������� ����� (�������������)
		BRNE	Loop__RTOS_METHOD_RemoveTimer
		; (���������: �������� ��� �������, �� ����� �������� - �����, ��� �� �����...)
	
	End__RTOS_METHOD_RemoveTimer:
		POP	ZL
		POP	ZH
		POP	temp2
		POPF						; Restore global interrupt flag
		;=== ����� ����������� ������
		RET



.endif	//RTOS_OPTIMIZE_FOR_SMALL_MEMORY

.endif	//RTOS_FEATURE_PLANTASKTIMED

.ifdef	RTOS_FEATURE_PLANTASKWAITER
;***************************************************************************
;*
;*  �������� �������������
;*
;***************************************************************************


; TODO:
;RTOS_METHOD_ClearWaiterQueue
;RTOS_METHOD_AddWaiter
;RTOS_METHOD_RemoveWaiter



.endif	//RTOS_FEATURE_PLANTASKWAITER

;***************************************************************************
;*
;*  ����� ���������
;*
;***************************************************************************


.ifndef	RTOS_OPTIMIZE_FOR_SMALL_MEMORY	; ����������: ������ ����� �������� ��������������, ��� ���� ����� ��������.
;---------------------------------------------------------------------------
; 
; ���� ��� ���������� ���� ���� RTOS ��������� ��������� ������ (kernel panic)? 
; (��������: ������� ����� ��� ���������, � ���������� ��� ������ RTOS_METHOD_AddTask ?)
; 
; ������� ��������, � ����� �������, �����������, �������� ���������/������������ ����������� ��������� ������.
; � �����, ���� ErrorHandler-����� ���������� �� ���� ����, ��������� ��� ��� ������.
; ����� �����, �����-�� �������, ������������������ �� ������ ������������, �������� � ���, ��� ������ (��-���������)...
; 
;---------------------------------------------------------------------------

;----- Subroutine Register Variables

;.def	ErrorCode	= (��.����)	; (�������, �� ����������)			��� ��������� ������
;.def	TaskCode	= (��.����)	; [�������, �� ������������, �� ����������]	�����-������ ������, ��� ������������ � ������� ��������� ������

; �������: ����� ����������/������ ���������� ���������...

;----- Code

	; ������ ������������ ����� ������ (��������� �������� ErrorCode):
	; 	 0 - ��� ������
	; 	10 - "������� �����" �����������, �� ���� �������� ����� ������!
	; 	20 - "��� ��������"   ����������, �� ���� ���������������� ����� ������!
	; 	30 - "��� ����������" ����������, �� ���� ���������������� ������ �����������!

RTOS_METHOD_ErrorHandler:
		RET

.endif	//RTOS_OPTIMIZE_FOR_SMALL_MEMORY




;=== END "RTOS-kernel.inc" =================================================
; coded by (c) DI HALT, 2008 @ http://easyelectronics.ru/
; coded by (c) Celeron, 2014 @ http://we.easyelectronics.ru/my/Celeron/