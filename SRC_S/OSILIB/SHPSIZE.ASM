;----------------------------------------
;OSI library (SAVAGE.EXE): shape files, the size of the buffer that saves what is under
;a shape
;----------------------------------------
INCLUDE OSI.INC

EXTRN SHP_rect:FAR	;SHP_rect

CSegStart	SHPSIZE_TEXT

;pascal long SHP_size(char far *shapes, int index): its width (+3 when under 12) times
;its height
FProc	SHP_size	;0742:0006
	PUSH	BP
	MOV	BP,SP
	SUB	SP,8
	PUSH	DS
	PUSH	DI
	PUSH	SI
	PUSH	SS
	LEA	AX,[BP-08]
	PUSH	AX
	XOR	AX,AX
	PUSH	AX
	PUSH	AX
	PUSH	WORD PTR [BP+0Ah]
	PUSH	WORD PTR [BP+08]
	PUSH	WORD PTR [BP+06]
	CALL	SHP_rect
	MOV	AX,[BP-04]
	SUB	AX,[BP-08]
	INC	AX
	CMP	AX,12
	JNB	C_0035
	ADD	AX,3
C_0035:
	MOV	DX,AX
	MOV	AX,[BP-02]
	SUB	AX,[BP-06]
	INC	AX
	MUL	DX
	POP	SI
	POP	DI
	POP	DS
	MOV	SP,BP
	POP	BP
	RET	6
SHP_size	ENDP

CSegEnd	SHPSIZE_TEXT

END
