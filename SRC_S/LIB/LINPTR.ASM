;----------------------------------------
;SAVAGE.EXE (and GAME.EXE 2F29): a linear address to a normalised far pointer
;----------------------------------------

LINPTR_TEXT	SEGMENT PUBLIC WORD 'CODE'
	ASSUME CS:LINPTR_TEXT

;void far * pascal LinearToPtr(long linear): segment linear/16, offset linear%16
PUBLIC C_0D09_000E
C_0D09_000E	PROC FAR	;0D09:000E
	PUSH	BP
	MOV	BP,SP
	MOV	DX,[BP+6]	;low word
	MOV	BL,DL
	MOV	AX,[BP+8]	;high word
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
C_0D09_000E	ENDP

LINPTR_TEXT	ENDS

END
