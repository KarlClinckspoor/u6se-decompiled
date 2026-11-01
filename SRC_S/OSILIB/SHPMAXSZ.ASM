;----------------------------------------
;OSI library (SAVAGE.EXE): shape files, the largest buffer any of a file's shapes needs
;----------------------------------------
INCLUDE OSI.INC

EXTRN C_0750_0006:FAR	;SHP_count
EXTRN C_0742_0006:FAR	;SHP_size

CSegStart	SHPMAXSZ_TEXT

;pascal long SHP_maxSize(char far *shapes): the largest SHP_size of its shapes
FProc	SHP_maxSize	;073C:000C
	PUSH	BP
	MOV	BP,SP
	SUB	SP,6
	PUSH	DS
	PUSH	SI
	PUSH	DI
	MOV	AX,[BP+08]
	PUSH	AX
	MOV	AX,[BP+06]
	PUSH	AX
	CALL	C_0750_0006
	MOV	[BP-02],AX	;index
	XOR	AX,AX
	MOV	[BP-06],AX	;the largest
	MOV	[BP-04],AX
	JMP	SHORT C_004F
C_002F:
	MOV	BX,[BP+08]
	PUSH	BX
	MOV	BX,[BP+06]
	PUSH	BX
	PUSH	AX
	CALL	C_0742_0006
	CMP	DX,[BP-04]
	JNZ	C_0047
	CMP	AX,[BP-06]
	JZ	C_004F
C_0047:
	JB	C_004F
	MOV	[BP-06],AX
	MOV	[BP-04],DX
C_004F:
	DEC	WORD PTR [BP-02]
	MOV	AX,[BP-02]
	JNS	C_002F
	MOV	AX,[BP-06]
	MOV	DX,[BP-04]
	POP	DI
	POP	SI
	POP	DS
	MOV	SP,BP
	POP	BP
	RET	4
SHP_maxSize	ENDP

CSegEnd	SHPMAXSZ_TEXT

END
