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
EXTRN _timerDivisor:WORD	;the timer chip divisor (PIT.ASM)
EXTRN _D_2B8C:DWORD	;(far *)(void far *, long, int): a TM.DRV routine (header +1A)

_DATA	ENDS

EXTRN _TimerDelay:FAR	;TimerDelay(unsigned long steps)

CALIBDRV_TEXT	SEGMENT PUBLIC WORD 'CODE'
	ASSUME CS:CALIBDRV_TEXT,DS:DGROUP

;void cdecl DrvCountLoop(unsigned long n): draw on the screen with a TM.DRV routine n times
;(the timer handler may cut it short)
PUBLIC _DrvCountLoop
_DrvCountLoop	PROC FAR	;0C97:0002
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
	MOV	_drvCalCountHi,AX
	MOV	CX,[BP+6]
	MOV	_drvCalCount,CX
	MOV	BYTE PTR _drvCalNSamples,0
	INC	BYTE PTR _drvCalCounting
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
	CMP	BYTE PTR _drvCalCounting,0
	JZ	C_006A
	DEC	WORD PTR _drvCalCount
	JNZ	C_0036
C_0059:
	CMP	WORD PTR _drvCalCountHi,0
	JZ	C_0066
	DEC	WORD PTR _drvCalCountHi
	JMP	C_0036
C_0066:
	DEC	BYTE PTR _drvCalCounting
C_006A:
	MOV	SP,BP
	POP	BP
	RET
_DrvCountLoop	ENDP

;the timer interrupt while calibrating: the loop counter at each tick; after 10 ticks the
;loop is stopped
DrvCalibTimerInt	PROC FAR	;0C97:006E
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
	CMP	BYTE PTR _drvCalCounting,0
	JZ	C_00AD
	MOV	BL,_drvCalNSamples
	XOR	BH,BH
	CMP	BL,0Ah
	JB	C_0097
	MOV	BYTE PTR _drvCalCounting,0
	JMP	SHORT C_00AD
C_0097:
	SHL	BX,1
	SHL	BX,1
	MOV	AX,_drvCalCount
	MOV	WORD PTR _drvCalSamples[BX],AX
	MOV	AX,_drvCalCountHi
	MOV	WORD PTR _drvCalSamples[BX+2],AX
	INC	BYTE PTR _drvCalNSamples
C_00AD:
	PUSHF
	CALL	DWORD PTR _drvCalOldInt8
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
DrvCalibTimerInt	ENDP

;int DrvCalibrate(void): count the TM.DRV calls in a timer tick (once), the timer at
;18.2 Hz meanwhile; returns the count / 256
PUBLIC _DrvCalibrate
_DrvCalibrate	PROC FAR	;0C97:00BE
	PUSH	SI
	PUSH	DI
	PUSH	DS
	CMP	BYTE PTR _drvCalDone,0
	JZ	C_00CB
	JMPN	C_0153
C_00CB:
	MOV	BYTE PTR _drvCalDone,1
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
	CALL	_TimerDelay
	ADD	SP,4
	MOV	AX,3508h
	INT	21h
	MOV	WORD PTR _drvCalOldInt8,BX
	MOV	WORD PTR _drvCalOldInt8+2,ES
	LDS	DX,_drvCalHandler
	MOV	AX,2508h
	INT	21h
	POP	DS
	PUSH	DS
	MOV	AX,0FFFFh
	PUSH	AX
	PUSH	AX
	PUSH	CS
	CALL	NEAR PTR _DrvCountLoop
	ADD	SP,4
	LDS	DX,_drvCalOldInt8
	MOV	AX,2508h
	INT	21h
	POP	DS
	PUSH	DS
	MOV	CX,9
	MOV	BX,1
C_0122:	;(SI is not advanced: the first two samples, nine times)
	MOV	SI,OFFSET DGROUP:_drvCalSamples
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
	MOV	_drvCallsPerTick,BX
	MOV	BX,_timerDivisor	;the timer back at its rate
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
	MOV	AX,_drvCallsPerTick
	POP	DS
	POP	DI
	POP	SI
	RET
_DrvCalibrate	ENDP

CALIBDRV_TEXT	ENDS

;(after the code: TASM wrote the data record after the code's)
_DATA	SEGMENT
PUBLIC _drvCalCounting,_drvCalNSamples,_drvCalSamples
PUBLIC _drvCalCount,_drvCalCountHi,_drvCalOldInt8,_drvCallsPerTick,_drvCalHandler,_drvCalDone
_drvCalCounting	db	0	;D_2418	1 while DrvCountLoop counts
_drvCalNSamples	db	0	;D_2419	samples taken
_drvCalSamples	dd	11 dup(0)	;D_241A	the loop counter at each timer tick (10 used)
_drvCalCount	dw	0	;D_2446	the loop counter (low word)
_drvCalCountHi	dw	0	;D_2448	(high word)
_drvCalOldInt8	dd	0	;D_244A	the timer interrupt (int 8) before
_drvCallsPerTick	dw	1	;D_244E	loops in a timer tick / 256
_drvCalHandler	dd	CALIBDRV_TEXT:DrvCalibTimerInt	;D_2450	the sampling interrupt handler
_drvCalDone	db	0	;D_2454	calibrated
_DATA	ENDS

END
