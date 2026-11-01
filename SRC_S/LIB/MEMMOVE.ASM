;----------------------------------------
;SAVAGE.EXE (and GAME.EXE 2F04): copying between far buffers that may overlap
;----------------------------------------

MEMMOVE_TEXT	SEGMENT PUBLIC WORD 'CODE'
	ASSUME CS:MEMMOVE_TEXT

backward	DB	0	;0CAC:000A	1 while copying downwards

;void pascal MemMove(void far *dst, void far *src, int n): copy n bytes from src to dst,
;downwards when dst is above src (the pointers are normalised first)
PUBLIC C_0CAC_000B
C_0CAC_000B	PROC FAR	;0CAC:000B
	PUSH	BP
	MOV	BP,SP
	PUSH	SI
	PUSH	DI
	PUSH	DS
	PUSHF
	MOV	CS:backward,0
	LDS	SI,[BP+8]	;src
	MOV	AX,SI
	MOV	BX,DS
	MOV	CL,4
	SHR	AX,CL
	ADD	BX,AX
	MOV	DS,BX
	db	83h,0E6h,0Fh	;AND SI,0Fh (MASM 3.0 would not sign-extend)
	MOV	DX,BX
	LES	DI,[BP+0Ch]	;dst
	MOV	AX,DI
	MOV	BX,ES
	MOV	CL,4
	SHR	AX,CL
	ADD	BX,AX
	MOV	ES,BX
	db	83h,0E7h,0Fh	;AND DI,0Fh
	CLD
	MOV	CX,[BP+6]	;n
	CMP	CX,1
	JZ	C_0081
	JL	C_008D
	CMP	DX,BX
	JG	C_0071
	JL	C_0061
	MOV	AX,SI
	SUB	AX,DI
	CMP	AX,-1
	JZ	C_0084
	CMP	AX,1
	JZ	C_008B
	CMP	SI,DI
	JG	C_0071
C_0061:	;dst above src: downwards, by words from the last one
	ADD	SI,CX
	ADD	DI,CX
	SUB	SI,2
	SUB	DI,2
	STD
	INC	CS:backward
C_0071:
	SHR	CX,1
	REP	MOVSW
	JNB	C_008D
	CMP	CS:backward,0
	JZ	C_0081
	INC	SI
	INC	DI
C_0081:
	MOVSB
	JMP	SHORT C_008D
C_0084:	;dst one byte above src: downwards, by bytes
	STD
	ADD	SI,CX
	ADD	DI,CX
	DEC	SI
	DEC	DI
C_008B:	;dst one byte below src: upwards, by bytes
	REP	MOVSB
C_008D:
	POPF
	POP	DS
	POP	DI
	POP	SI
	POP	BP
	RET	0Ah
C_0CAC_000B	ENDP

MEMMOVE_TEXT	ENDS

END
