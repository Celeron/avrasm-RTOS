.IFNDEF	_MACROAPP__INCLUDED_
.EQU	_MACROAPP__INCLUDED_ = 1
.MESSAGE "Note: <macroapp.inc> have included, only once!" 
;=== BEGIN "macroapp.inc" ==================================================
; ���������� ����������������, ������������ ��� ���������� ������ ����������.
; (����������: ��� ������� - "������������������"!)



;***** BEGIN "RTOS Tasks" section ******************************************

; ���������� ��� �������-�������� ����� (������������ ��� ��������� �������� PLAN_TASK_*):
; ��������: ����������� �� ��� ��� ������ - ��� ������������ ������ ��� �������� ���������������� ����������� ����, � �� ������������ ����� ���� RTOS...
.equ	TS_Task1	= 1		; 
.equ	TS_Task2	= 2		; 
.equ	TS_Task3	= 3		; 
.equ	TS_Task4	= 4		; 
.equ	TS_Task5	= 5		; 
.equ	TS_Task6	= 6		; 
.equ	TS_Task7	= 7		; 
.equ	TS_Task8	= 8		; 
.equ	TS_Task9	= 9		; 

;***** END "RTOS Tasks" section



;=== END "macroapp.inc" ====================================================
.ENDIF
