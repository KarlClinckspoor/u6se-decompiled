;----------------------------------------
;SAVAGE.EXE (and GAME.EXE 2F0D): the timer chip's rate, interrupt vectors taken over and
;given back, a delay counted on the timer chip
;----------------------------------------

_DATA	SEGMENT PUBLIC WORD 'DATA'
_DATA	ENDS
DGROUP	GROUP _DATA

;a taken-over interrupt vector (a list of them, last taken first)
tHook	STRUC
	hNext	dw ?	;+00 struct tHook *
	hIntNo	dw ?	;+02 the interrupt; FFFF: given back
	hOld	dd ?	;+04 the vector before
tHook	ENDS

_DATA	SEGMENT
PUBLIC _timerDivisor, _timerRate, _hookList, _timerHook
_timerDivisor	dw	0	;D_2456	timer chip divisor (0: 65536, 18.2 Hz)
_timerRate	dw	0	;D_2458	timer rate asked for (Hz; 0 when 18 or less)
_hookList	dw	0	;D_245A	struct tHook *: the vectors taken over
_timerHook	tHook	<0,0FFFFh,0>	;D_245C	the timer interrupt's (int 8)
_DATA	ENDS

EXTRN _malloc:FAR

PIT_TEXT	SEGMENT PUBLIC WORD 'CODE'
	ASSUME CS:PIT_TEXT,DS:DGROUP

;void far * cdecl HookVector(int intNo, void interrupt (far *handler)(), struct tHook *hook,
;  void far * far *pOld): set an interrupt vector, remember the old one in hook (malloc'ed
;  when 0) and in *pOld (when not 0); returns the old vector (0 when out of memory)
PUBLIC _HookVector
_HookVector	PROC FAR	;0CED:000E
	PUSH	BP
	MOV	BP,SP
	SUB	SP,4
	PUSH	DS
	PUSH	SI
	PUSH	DI
	MOV	SI,[BP+0Ch]	;hook
	OR	SI,SI
	JNZ	C_0029
	MOV	AL,SIZE tHook	;(AH not set)
	PUSH	AX
	CALL	_malloc
	POP	CX
	MOV	SI,AX
C_0029:
	XOR	AX,AX
	MOV	DX,AX
	OR	SI,SI
	JZ	C_0079
	PUSH	SI
	MOV	AX,[BP+6]	;intNo
	MOV	AH,35h	;get the vector
	INT	21h
	MOV	[BP-4],BX
	MOV	[BP-2],ES
	PUSH	DS
	CMP	WORD PTR [BP+10h],0	;pOld
	JZ	C_004E
	LDS	DI,[BP+0Eh]
	MOV	[DI],BX
	MOV	[DI+2],ES
C_004E:
	MOV	DX,[BP+8]	;handler
	MOV	DS,[BP+0Ah]
	MOV	AX,[BP+6]
	MOV	AH,25h	;set the vector
	INT	21h
	POP	DS
	POP	SI
	MOV	AX,[BP+6]
	MOV	[SI].hIntNo,AX
	MOV	AX,[BP-4]
	MOV	DX,[BP-2]
	MOV	WORD PTR [SI].hOld,AX
	MOV	WORD PTR [SI].hOld+2,DX
	MOV	BX,_hookList
	MOV	[SI].hNext,BX
	MOV	_hookList,SI
C_0079:
	POP	DI
	POP	SI
	POP	DS
	MOV	SP,BP
	POP	BP
	RET
_HookVector	ENDP

timerStep	dw	0	;0CED:0080	the divisor (0: no faster timer)
oldInt8	dw	0, DGROUP	;0CED:0082	the timer interrupt before (a far pointer)
timerSum	dw	0	;0CED:0086	the divisors added up: the old handler runs at each carry

;the timer interrupt (int 8) at a higher rate: the old handler is called at about 18.2 Hz
timerInt	PROC FAR	;0CED:0088
	CLI
	PUSH	AX
	MOV	AX,CS:timerStep
	OR	AX,AX
	JZ	C_009F
	ADD	CS:timerSum,AX
	JB	C_009F
	MOV	AL,20h	;end of interrupt
	OUT	20h,AL
	POP	AX
	IRET
C_009F:
	POP	AX
	JMP	DWORD PTR CS:oldInt8
timerInt	ENDP

;void cdecl SetTimerRate(int hz): make the timer interrupt hz times a second (18.2 when
;hz <= 18) and take it over
PUBLIC _SetTimerRate
_SetTimerRate	PROC FAR	;0CED:00A5
	PUSH	BP
	MOV	BP,SP
	PUSH	DS
	PUSH	SI
	PUSH	DI
	PUSHF
	MOV	AX,[BP+6]
	MOV	_timerRate,AX
	CLI
	CMP	WORD PTR [BP+6],12h
	JA	C_00C0
	XOR	AX,AX
	MOV	_timerRate,AX
	JZ	C_00CB
C_00C0:
	MOV	AX,34DEh	;1193182 Hz / hz
	MOV	DX,12h
	MOV	BX,[BP+6]
	DIV	BX
C_00CB:
	MOV	CS:timerStep,AX
	MOV	_timerDivisor,AX
	MOV	BX,AX
	MOV	AL,36h	;counter 0, mode 3, binary
	OUT	43h,AL
	MOV	AL,BL
	OUT	40h,AL
	MOV	AL,BH
	OUT	40h,AL
	CMP	_timerHook.hIntNo,-1
	JNZ	C_0100
	PUSH	CS
	MOV	AX,OFFSET oldInt8
	PUSH	AX
	MOV	AX,OFFSET DGROUP:_timerHook
	PUSH	AX
	PUSH	CS
	MOV	AX,OFFSET timerInt
	PUSH	AX
	MOV	AX,8
	PUSH	AX
	PUSH	CS
	CALL	NEAR PTR _HookVector
	ADD	SP,0Ch
C_0100:
	POPF
	POP	DI
	POP	SI
	POP	DS
	POP	BP
	RET
_SetTimerRate	ENDP

;void cdecl TimerDelay(unsigned long n): wait until the timer chip's counter 0 has counted
;n steps (1193182 a second)
PUBLIC _TimerDelay
_TimerDelay	PROC FAR	;0CED:0106
	PUSH	BP
	MOV	BP,SP
	PUSH	SI
	PUSH	DI
	PUSH	BP
	MOV	DX,[BP+8]
	MOV	BP,[BP+6]	;DX:BP = n
	XOR	SI,SI
	XOR	DI,DI	;DI:SI = steps counted
	MOV	AL,6	;latch counter 0
	OUT	43h,AL
	MOV	AL,36h
	IN	AL,40h
	MOV	BL,AL
	IN	AL,40h
	MOV	BH,AL
C_0124:
	MOV	CX,BX
	MOV	AL,6
	OUT	43h,AL
	MOV	AL,36h
	IN	AL,40h
	MOV	BL,AL
	IN	AL,40h
	MOV	BH,AL
	SUB	CX,BX
	JNB	C_013C
	ADD	CX,_timerDivisor
C_013C:
	ADD	SI,CX
	ADC	DI,0
	CMP	DI,DX
	JB	C_0124
	JA	C_014B
	CMP	SI,BP
	JB	C_0124
C_014B:
	POP	BP
	POP	DI
	POP	SI
	POP	BP
	RET
_TimerDelay	ENDP

;void cdecl UnhookVector(int intNo): give back an interrupt vector taken over
PUBLIC _UnhookVector
_UnhookVector	PROC FAR	;0CED:0150
	PUSH	BP
	MOV	BP,SP
	PUSH	DS
	PUSH	SI
	PUSH	DI
	MOV	AX,[BP+6]
	MOV	DI,OFFSET DGROUP:_hookList
	MOV	SI,[DI]
C_015E:
	OR	SI,SI
	JZ	C_0182
	CMP	AX,[SI].hIntNo
	JNZ	C_017C
	MOV	BX,[SI].hNext
	MOV	[DI],BX
	MOV	[SI].hIntNo,0FFFFh
	MOV	DX,WORD PTR [SI].hOld
	MOV	DS,WORD PTR [SI].hOld+2
	MOV	AH,25h
	INT	21h
	JMP	SHORT C_0182
C_017C:
	MOV	DI,SI
	MOV	SI,[SI].hNext
	JMP	C_015E
C_0182:
	POP	DI
	POP	SI
	POP	DS
	POP	BP
	RET
_UnhookVector	ENDP

;void cdecl UnhookAll(void): the timer back to 18.2 Hz, every vector taken over given back
PUBLIC _UnhookAll
_UnhookAll	PROC FAR	;0CED:0187
	PUSH	BP
	PUSH	DS
	PUSH	SI
	PUSH	DI
	PUSHF
	CLI
	CMP	_timerHook.hIntNo,-1
	JZ	C_019E
	MOV	AL,36h
	OUT	43h,AL
	XOR	AL,AL
	OUT	40h,AL
	OUT	40h,AL
C_019E:
	MOV	SI,_hookList
	OR	SI,SI
	JZ	C_01C7
	MOV	AX,[SI].hIntNo
	CMP	AX,0FFh
	JA	C_01C7
	MOV	[SI].hIntNo,0FFFFh
	MOV	BX,[SI].hNext
	MOV	_hookList,BX
	PUSH	DS
	MOV	DX,WORD PTR [SI].hOld
	MOV	DS,WORD PTR [SI].hOld+2
	MOV	AH,25h
	INT	21h
	POP	DS
	JMP	C_019E
C_01C7:
	POPF
	POP	DI
	POP	SI
	POP	DS
	POP	BP
	RET
_UnhookAll	ENDP

PIT_TEXT	ENDS

END
