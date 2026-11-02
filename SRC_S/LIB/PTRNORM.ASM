;----------------------------------------
;SAVAGE.EXE (and GAME.EXE 2F2D): normalise a far pointer
;----------------------------------------

PTRNORM_TEXT	SEGMENT PUBLIC WORD 'CODE'
	ASSUME CS:PTRNORM_TEXT

;void far * pascal NormalizePtr(void far *p): the same address with an offset below 16
PUBLIC NormalizePtr
NormalizePtr	PROC FAR	;0D0E:0004
	PUSH	BP
	MOV	BP,SP
	MOV	DX,[BP+8]	;segment
	XOR	AX,AX
	SHL	DX,1
	RCL	AL,1
	SHL	DX,1
	RCL	AL,1
	SHL	DX,1
	RCL	AL,1
	SHL	DX,1
	RCL	AL,1
	ADD	DX,[BP+6]	;offset: AL:DX is the linear address
	ADC	AL,AH
	MOV	BL,DL
	SHR	AL,1
	RCR	DX,1
	SHR	AL,1
	RCR	DX,1
	SHR	AL,1
	RCR	DX,1
	SHR	AL,1
	RCR	DX,1
	MOV	AL,BL
	AND	AX,0Fh
	POP	BP
	RET	4
NormalizePtr	ENDP

PTRNORM_TEXT	ENDS

END
