;----------------------------------------
;OSI library: writing a file (pascal) from memory that may be EMS (a far
;pointer with segment C000h-FFFFh is mapped in first, C_0AD2_0024).
;(GAME.EXE's 2E7E is the read routine's twin; this one has no JUMPS NOPs.)
;----------------------------------------

DGROUP	GROUP _DATA
_DATA	SEGMENT WORD PUBLIC 'DATA'
_DATA	ENDS

EXTRN _D_240C:WORD,_D_240E:DWORD
EXTRN _C_0AD2_0024:FAR

OSIWRITE_TEXT	SEGMENT WORD PUBLIC 'CODE'
	ASSUME CS:OSIWRITE_TEXT,DS:DGROUP

;int pascal C_0C12_0000(int handle, long pos, long size, void far *buf):
;seek to pos (unless pos's high word is -1), write size bytes from buf;
;returns 1, or 0 with CF set on an error not retried (a short write is
;error -1)
PUBLIC C_0C12_0000
C_0C12_0000	PROC FAR	;0C12:0000
	PUSH	BP
	MOV	BP,SP
	SUB	SP,4
	PUSH	SI
	PUSH	DI
L_0008:
	XOR	AX,AX
	CMP	WORD PTR [BP+08],0
	JZ	L_0080
	PUSH	WORD PTR [BP+08]
	PUSH	WORD PTR [BP+06]
	CALL	_C_0AD2_0024		;map an EMS pointer
	ADD	SP,4
	MOV	[BP-02],DX
	MOV	[BP-04],AX
	MOV	BX,[BP+12h]
	MOV	DX,[BP+0Eh]
	MOV	CX,[BP+10h]
	CMP	CX,-1
	JZ	L_0039
	MOV	AX,4200h	;move file pointer
	INT	21h
	JB	L_0080
L_0039:
	MOV	DI,[BP+0Ch]
	MOV	SI,[BP+0Ah]
L_003F:
	OR	DI,DI
	JNZ	L_0048
	CMP	SI,-10h
	JBE	L_006C
L_0048:
	MOV	CX,0FFF0h
	PUSH	DS
	LDS	DX,[BP-04]
	MOV	AH,40h		;write file
	INT	21h
	POP	DS
	JB	L_0080
	CMP	AX,CX
	JB	L_007D
	SUB	SI,AX
	SBB	DI,0
	SHR	AX,1
	SHR	AX,1
	SHR	AX,1
	SHR	AX,1
	ADD	[BP-02],AX
	JMP	SHORT L_003F
L_006C:
	MOV	CX,SI
	PUSH	DS
	LDS	DX,[BP-04]
	MOV	AH,40h		;write file
	INT	21h
	POP	DS
	JB	L_0080
	CMP	AX,CX
	JNB	L_0097
L_007D:
	MOV	AX,0FFFFh
L_0080:
	MOV	_D_240C,AX
	CALL	_D_240E
	TEST	_D_240C,0FFFFh
	JNZ	L_0092
	JMP	L_0008
L_0092:
	XOR	AX,AX
	STC
	JMP	SHORT L_009A
L_0097:
	MOV	AX,1
L_009A:
	POP	DI
	POP	SI
	MOV	SP,BP
	POP	BP
	RET	0Eh
C_0C12_0000	ENDP

OSIWRITE_TEXT	ENDS
END
