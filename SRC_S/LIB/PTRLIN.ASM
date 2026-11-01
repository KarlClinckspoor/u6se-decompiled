;----------------------------------------
;SAVAGE.EXE (and GAME.EXE 2F2B): a far pointer to its linear address
;----------------------------------------

PTRLIN_TEXT	SEGMENT PUBLIC WORD 'CODE'
	ASSUME CS:PTRLIN_TEXT

;long pascal PtrToLinear(void far *p): segment*16 + offset (20 bits, in DX:AX)
PUBLIC C_0D0C_0002
C_0D0C_0002	PROC FAR	;0D0C:0002
	PUSH	BP
	MOV	BP,SP
	MOV	AX,[BP+8]	;segment
	XOR	DX,DX
	SHL	AX,1
	RCL	DL,1
	SHL	AX,1
	RCL	DL,1
	SHL	AX,1
	RCL	DL,1
	SHL	AX,1
	RCL	DL,1
	ADD	AX,[BP+6]	;offset
	ADC	DL,DH
	POP	BP
	RET	4
C_0D0C_0002	ENDP

PTRLIN_TEXT	ENDS

END
