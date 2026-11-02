;----------------------------------------
;SAVAGE.EXE (and GAME.EXE 2F30): a delay loop calibrated on the timer interrupt
;----------------------------------------

_DATA	SEGMENT PUBLIC WORD 'DATA'
_DATA	ENDS
DGROUP	GROUP _DATA


CALIB_TEXT	SEGMENT PUBLIC WORD 'CODE'
	ASSUME CS:CALIB_TEXT,DS:DGROUP

;void cdecl CountLoop(unsigned long n): count n down (the timer handler may cut it short)
PUBLIC _CountLoop
_CountLoop	PROC FAR	;0D11:000C
	PUSH	BP
	MOV	BP,SP
	MOV	AX,[BP+8]
	MOV	_calCountHi,AX
	MOV	CX,[BP+6]
	MOV	_calCount,CX
	MOV	BYTE PTR _calNSamples,0
	INC	BYTE PTR _calCounting
	JCXZ	C_002D
C_0027:
	DEC	WORD PTR _calCount
	JNZ	C_0027
C_002D:
	CMP	WORD PTR _calCountHi,0
	JZ	C_003A
	DEC	WORD PTR _calCountHi
	JMP	C_0027
C_003A:
	DEC	BYTE PTR _calCounting
	POP	BP
	RET
_CountLoop	ENDP

;the timer interrupt while calibrating: the loop counter at each tick; after 4 ticks the
;loop is stopped
CalibTimerInt	PROC FAR	;0D11:0040
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
	CMP	BYTE PTR _calCounting,0
	JZ	C_008B
	MOV	BL,_calNSamples
	XOR	BH,BH
	CMP	BL,4
	JB	C_0075
	MOV	WORD PTR _calCount,1
	MOV	WORD PTR _calCountHi,1
	MOV	BYTE PTR _calCounting,0
	JMP	SHORT C_008B
C_0075:
	SHL	BX,1
	SHL	BX,1
	MOV	AX,_calCount
	MOV	WORD PTR _calSamples[BX],AX
	MOV	AX,_calCountHi
	MOV	WORD PTR _calSamples[BX+2],AX
	INC	BYTE PTR _calNSamples
C_008B:
	PUSHF
	CALL	DWORD PTR _calOldInt8
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
CalibTimerInt	ENDP

;void CalibrateLoop(void): count the loops in a timer tick (once)
PUBLIC _CalibrateLoop
_CalibrateLoop	PROC FAR	;0D11:009C
	PUSH	SI
	PUSH	DI
	PUSH	DS
	CMP	BYTE PTR _calDone,0
	JNZ	C_00FE
	MOV	BYTE PTR _calDone,1
	MOV	AX,3508h
	INT	21h
	MOV	WORD PTR _calOldInt8,BX
	MOV	WORD PTR _calOldInt8+2,ES
	LDS	DX,_calHandler
	MOV	AX,2508h
	INT	21h
	POP	DS
	PUSH	DS
	MOV	AX,0FFFFh
	PUSH	AX
	PUSH	AX
	PUSH	CS
	CALL	NEAR PTR _CountLoop
	ADD	SP,4
	LDS	DX,_calOldInt8
	MOV	AX,2508h
	INT	21h
	POP	DS
	PUSH	DS
	MOV	CX,3
	MOV	BX,1
C_00E0:	;(SI is not advanced: the first two samples, three times)
	MOV	SI,OFFSET DGROUP:_calSamples
	MOV	AX,[SI]
	MOV	DX,[SI+2]
	SUB	AX,[SI+4]
	SBB	DX,[SI+6]
	MOV	AL,AH
	MOV	AH,DL
	CMP	BX,AX
	JNB	C_00F8
	MOV	BX,AX
C_00F8:
	LOOP	C_00E0
	MOV	_loopsPerTick,BX
C_00FE:
	POP	DS
	POP	DI
	POP	SI
	RET
_CalibrateLoop	ENDP

;void cdecl LoopDelay(unsigned long t): CountLoop(t * loopsPerTick / 256)
PUBLIC _LoopDelay
_LoopDelay	PROC FAR	;0D11:0102
	PUSH	BP
	MOV	BP,SP
	MOV	AX,_loopsPerTick
	MUL	WORD PTR [BP+6]
	MOV	BL,AH
	MOV	BH,DL
	MOV	CL,DH
	XOR	CH,CH
	MOV	AX,_loopsPerTick
	MUL	WORD PTR [BP+8]
	ADD	BH,AL
	ADC	CL,AH
	ADC	CH,DL
	PUSH	CX
	PUSH	BX
	PUSH	CS
	CALL	NEAR PTR _CountLoop
	ADD	SP,4
	POP	BP
	RET
_LoopDelay	ENDP

CALIB_TEXT	ENDS

;(after the code: TASM wrote the data record after the code's)
_DATA	SEGMENT
PUBLIC _calCounting,_calNSamples,_calSamples
PUBLIC _calCount,_calCountHi,_calOldInt8,_loopsPerTick,_calHandler,_calDone
_calCounting	db	0	;D_2464	1 while CountLoop counts
_calNSamples	db	0	;D_2465	samples taken
_calSamples	dd	4 dup(0)	;D_2466	the loop counter at each timer tick
_calCount	dw	0	;D_2476	the loop counter (low word)
_calCountHi	dw	0	;D_2478	(high word)
_calOldInt8	dd	0	;D_247A	the timer interrupt (int 8) before
_loopsPerTick	dw	1	;D_247E	loops in a timer tick / 256
_calHandler	dd	CALIB_TEXT:CalibTimerInt	;D_2480	the sampling interrupt handler
_calDone	db	0	;D_2484	calibrated
_DATA	ENDS

END
