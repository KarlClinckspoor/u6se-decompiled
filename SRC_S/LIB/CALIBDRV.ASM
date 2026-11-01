;----------------------------------------
;SAVAGE.EXE: how many calls of a TM.DRV routine fit in a timer tick (a variant of
;CALIB.ASM, 0D11)
;----------------------------------------
;TASM fixup order: 0078 000B 003C   (TASM wrote these fixups in this order: tools/se/tasmorder.py)

JMPN	MACRO target	;;near JMP without MASM's self-relative fixup
	db	0E9h
	dw	target-$-2
ENDM

_DATA	SEGMENT PUBLIC WORD 'DATA'
_DATA	ENDS
DGROUP	GROUP _DATA

_DATA	SEGMENT
EXTRN _D_2B1A:BYTE	;struct tSurface (16 bytes): the screen (see screen.c)
EXTRN _D_2B34:DWORD	;int (far *)(void far *, long): a TM.DRV routine (header +1C, tmdrv.c)
EXTRN _D_2456:WORD	;the timer chip divisor (PIT.ASM)
EXTRN _D_2B8C:DWORD	;(far *)(void far *, long, int): a TM.DRV routine (header +1A)

_DATA	ENDS

EXTRN _C_0CED_0106:FAR	;TimerDelay(unsigned long steps)

CALIBDRV_TEXT	SEGMENT PUBLIC WORD 'CODE'
	ASSUME CS:CALIBDRV_TEXT,DS:DGROUP

;void cdecl DrvCountLoop(unsigned long n): draw on the screen with a TM.DRV routine n times
;(the timer handler may cut it short)
PUBLIC _C_0C97_0002
_C_0C97_0002	PROC FAR	;0C97:0002
	PUSH	BP
	MOV	BP,SP
	SUB	SP,2
	XOR	AX,AX
	PUSH	AX
	PUSH	AX
	MOV	AX,DGROUP
	PUSH	AX
	MOV	AX,OFFSET DGROUP:_D_2B1A
	PUSH	AX
	CALL	_D_2B34
	ADD	SP,8
	MOV	[BP-2],AX
	MOV	AX,[BP+8]
	MOV	_D_2448,AX
	MOV	CX,[BP+6]
	MOV	_D_2446,CX
	MOV	BYTE PTR _D_2419,0
	INC	BYTE PTR _D_2418
	JCXZ	C_0059
C_0036:
	PUSH	WORD PTR [BP-2]
	XOR	AX,AX
	PUSH	AX
	PUSH	AX
	MOV	AX,DGROUP
	PUSH	AX
	MOV	AX,OFFSET DGROUP:_D_2B1A
	PUSH	AX
	CALL	_D_2B8C
	ADD	SP,0Ah
	CMP	BYTE PTR _D_2418,0
	JZ	C_006A
	DEC	WORD PTR _D_2446
	JNZ	C_0036
C_0059:
	CMP	WORD PTR _D_2448,0
	JZ	C_0066
	DEC	WORD PTR _D_2448
	JMP	C_0036
C_0066:
	DEC	BYTE PTR _D_2418
C_006A:
	MOV	SP,BP
	POP	BP
	RET
_C_0C97_0002	ENDP

;the timer interrupt while calibrating: the loop counter at each tick; after 10 ticks the
;loop is stopped
C_0C97_006E	PROC FAR	;0C97:006E
	PUSH	AX
	PUSH	BX
	PUSH	CX
	PUSH	DX
	PUSH	ES
	PUSH	DS
	PUSH	SI
	PUSH	DI
	PUSH	BP
	MOV	BP,SP
	MOV	AX,DGROUP
	MOV	DS,AX
	CMP	BYTE PTR _D_2418,0
	JZ	C_00AD
	MOV	BL,_D_2419
	XOR	BH,BH
	CMP	BL,0Ah
	JB	C_0097
	MOV	BYTE PTR _D_2418,0
	JMP	SHORT C_00AD
C_0097:
	SHL	BX,1
	SHL	BX,1
	MOV	AX,_D_2446
	MOV	WORD PTR _D_241A[BX],AX
	MOV	AX,_D_2448
	MOV	WORD PTR _D_241A[BX+2],AX
	INC	BYTE PTR _D_2419
C_00AD:
	PUSHF
	CALL	DWORD PTR _D_244A
	MOV	SP,BP
	POP	BP
	POP	DI
	POP	SI
	POP	DS
	POP	ES
	POP	DX
	POP	CX
	POP	BX
	POP	AX
	IRET
C_0C97_006E	ENDP

;int DrvCalibrate(void): count the TM.DRV calls in a timer tick (once), the timer at
;18.2 Hz meanwhile; returns the count / 256
PUBLIC _C_0C97_00BE
_C_0C97_00BE	PROC FAR	;0C97:00BE
	PUSH	SI
	PUSH	DI
	PUSH	DS
	CMP	BYTE PTR _D_2454,0
	JZ	C_00CB
	JMPN	C_0153
C_00CB:
	MOV	BYTE PTR _D_2454,1
	PUSHF
	CLI
	MOV	AL,36h	;the timer at 18.2 Hz
	OUT	43h,AL
	XOR	AL,AL
	OUT	40h,AL
	OUT	40h,AL
	POPF
	MOV	AX,12h	;wait 0x124000 steps (one second)
	PUSH	AX
	MOV	AX,4000h
	PUSH	AX
	CALL	_C_0CED_0106
	ADD	SP,4
	MOV	AX,3508h
	INT	21h
	MOV	WORD PTR _D_244A,BX
	MOV	WORD PTR _D_244A+2,ES
	LDS	DX,_D_2450
	MOV	AX,2508h
	INT	21h
	POP	DS
	PUSH	DS
	MOV	AX,0FFFFh
	PUSH	AX
	PUSH	AX
	PUSH	CS
	CALL	NEAR PTR _C_0C97_0002
	ADD	SP,4
	LDS	DX,_D_244A
	MOV	AX,2508h
	INT	21h
	POP	DS
	PUSH	DS
	MOV	CX,9
	MOV	BX,1
C_0122:	;(SI is not advanced: the first two samples, nine times)
	MOV	SI,OFFSET DGROUP:_D_241A
	MOV	AX,[SI]
	MOV	DX,[SI+2]
	SUB	AX,[SI+4]
	SBB	DX,[SI+6]
	MOV	AL,AH
	MOV	AH,DL
	CMP	BX,AX
	JNB	C_013A
	MOV	BX,AX
C_013A:
	LOOP	C_0122
	MOV	_D_244E,BX
	MOV	BX,_D_2456	;the timer back at its rate
	PUSHF
	CLI
	MOV	AL,36h
	OUT	43h,AL
	MOV	AL,BL
	OUT	40h,AL
	MOV	AL,BH
	OUT	40h,AL
	POPF
C_0153:
	MOV	AX,_D_244E
	POP	DS
	POP	DI
	POP	SI
	RET
_C_0C97_00BE	ENDP

CALIBDRV_TEXT	ENDS

;(after the code: TASM wrote the data record after the code's)
_DATA	SEGMENT
PUBLIC _D_2418,_D_2419,_D_241A,_D_2446,_D_2448,_D_244A,_D_244E,_D_2450,_D_2454
_D_2418	db	0	;D_2418	1 while C_0C97_0002 counts
_D_2419	db	0	;D_2419	samples taken
_D_241A	dd	11 dup(0)	;D_241A	the loop counter at each timer tick (10 used)
_D_2446	dw	0	;D_2446	the loop counter (low word)
_D_2448	dw	0	;D_2448	(high word)
_D_244A	dd	0	;D_244A	the timer interrupt (int 8) before
_D_244E	dw	1	;D_244E	loops in a timer tick / 256
_D_2450	dd	CALIBDRV_TEXT:C_0C97_006E	;D_2450	the sampling interrupt handler
_D_2454	db	0	;D_2454	calibrated
_DATA	ENDS

END
