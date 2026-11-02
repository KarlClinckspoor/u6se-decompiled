;----------------------------------------
;SAVAGE.EXE (like GAME.EXE 2B57): random numbers
;----------------------------------------

_DATA	SEGMENT PUBLIC WORD 'DATA'
_DATA	ENDS
DGROUP	GROUP _DATA

_DATA	SEGMENT
PUBLIC _randomSeed
_randomSeed	dd	87654321h	;D_2486	the random seed
_DATA	ENDS

RANDOM_TEXT	SEGMENT PUBLIC WORD 'CODE'
	ASSUME CS:RANDOM_TEXT,DS:DGROUP

;the next seed: seed * 3 + (low word << 16), mod 2^32
RandomStep	PROC FAR	;0D23:000A
	MOV	CX,WORD PTR _randomSeed
	MOV	BX,CX
	MOV	AX,WORD PTR _randomSeed+2
	MOV	DX,AX
	ADD	CX,CX
	ADC	AX,AX
	ADD	CX,BX
	ADC	AX,DX
	ADD	AX,BX
	MOV	WORD PTR _randomSeed,CX
	MOV	WORD PTR _randomSeed+2,AX
	RET
RandomStep	ENDP

;unsigned pascal Random(unsigned a, unsigned b): a random number from min(a,b) to max(a,b)
PUBLIC Random
Random	PROC FAR	;0D23:0027
	PUSH	BP
	MOV	BP,SP
	PUSH	CS
	CALL	NEAR PTR RandomStep
	MOV	BX,[BP+6]	;b
	MOV	CX,[BP+8]	;a
	CMP	BX,CX
	JNB	C_003A
	XCHG	BX,CX
C_003A:
	XOR	DX,DX
	SUB	BX,CX
	INC	BX
	JZ	C_0043	;the whole range
	DIV	BX
C_0043:
	ADD	DX,CX
	MOV	AX,DX
	POP	BP
	RET	4
Random	ENDP

;void pascal SeedRandom(unsigned seed)
PUBLIC SeedRandom
SeedRandom	PROC FAR	;0D23:004B
	PUSH	BP
	MOV	BP,SP
	MOV	AX,[BP+6]
	OR	AX,1
	MOV	WORD PTR _randomSeed,AX
	POP	BP
	RET	2
SeedRandom	ENDP

RANDOM_TEXT	ENDS

END
