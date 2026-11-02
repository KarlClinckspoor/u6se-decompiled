INCLUDE OSI.INC

EXTRN CpuSpeed:WORD	;computer speed

DSegStart	
	
D_2BDA	dw 0
D_2BDC	dw 0
D_2BDE	dw 0,0	;original int 1Ch

DSegEnd


CSegStart	_DELAY_TEXT

ProcStart	OSI_delay	;2B4F:000A
	MOV	AX,[BP+06]
	MOV	D_2BDC,AX
C_000E:	;--
	MOV	AX,D_2BDA
	CMP	AX,D_2BDC
	JB	C_000E
	;--
	MOV	D_2BDA,0
ProcEndN	OSI_delay,2


NEW_INT_1C:	;2B4F:002C
	STI
	PUSH	DS
	PUSH	AX
	PUSH	SI
	;--
	MOV	AX,DGROUP
	MOV	DS,AX
	MOV	SI,OFFSET DGROUP:D_2BDA
	INC	WORD PTR [SI]
	;--
	POP	SI
	POP	AX
	POP	DS
	IRET

PUBLIC OSI_delay_init	;2B4F:003E
OSI_delay_init	PROC FAR
	PUSH	SI
	PUSH	DI
	PUSH	DS
	;--
	MOV	D_2BDA,0
	;--
	PUSH	ES
	MOV	AL,1Ch
	MOV	AH,35h
	INT	21h
	MOV	AX,ES
	POP	ES
	MOV	D_2BDE,BX
	MOV	D_2BDE+2,AX
	;--
	PUSH	DS
	PUSH	CS
	POP	DS
	LEA	DX,NEW_INT_1C
	MOV	AL,1Ch
	MOV	AH,25h
	INT	21h
	POP	DS
	;--
	POP	DS
	POP	DI
	POP	SI
	RET
OSI_delay_init	ENDP

PUBLIC OSI_delay_clean	;2B4F:006A
OSI_delay_clean	PROC FAR
	PUSH	SI
	PUSH	DI
	PUSH	DS
	;--
	MOV	AX,D_2BDE+2
	MOV	DX,D_2BDE
	PUSH	DS
	MOV	DS,AX
	MOV	AL,1Ch
	MOV	AH,25h
	INT	21h
	POP	DS
	;--
	POP	DS
	POP	DI
	POP	SI
	RET
OSI_delay_clean	ENDP

CSegEnd	_DELAY_TEXT

END
