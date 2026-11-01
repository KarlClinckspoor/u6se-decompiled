;----------------------------------------
;OSI library: reading a file (pascal) into memory that may be EMS (a far
;pointer with segment C000h-FFFFh is mapped in first, C_0AD2_0024).
;(GAME.EXE 2E7E.) Assembled by TASM with JUMPS: a conditional jump that
;turned out short is followed by three NOPs, written out here.
;----------------------------------------

DGROUP	GROUP _DATA
_DATA	SEGMENT WORD PUBLIC 'DATA'
_DATA	ENDS

EXTRN _D_240C:WORD,_D_240E:DWORD
EXTRN _C_0AD2_0024:FAR

OSIREAD_TEXT	SEGMENT WORD PUBLIC 'CODE'
	ASSUME CS:OSIREAD_TEXT,DS:DGROUP

;int pascal C_0C06_000C(int handle, long pos, long size, void far *buf):
;seek to pos (unless pos's high word is -1), read size bytes to buf;
;returns 1, or 0 with CF set on an error not retried (_D_240C has the error)
PUBLIC C_0C06_000C
C_0C06_000C	PROC FAR	;0C06:000C
	PUSH	BP
	MOV	BP,SP
	SUB	SP,4
	PUSH	SI
	PUSH	DI
L_0014:
	XOR	AX,AX
	CMP	WORD PTR [BP+08],0
	JNZ	L_001F
	JMP	L_009E
L_001F:
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
	JZ	L_004E
	NOP
	NOP
	NOP
	MOV	AX,4200h	;move file pointer
	INT	21h
	JB	L_009E
	NOP
	NOP
	NOP
L_004E:
	MOV	DI,[BP+0Ch]
	MOV	SI,[BP+0Ah]
L_0054:
	OR	DI,DI
	JNZ	L_0063
	NOP
	NOP
	NOP
	CMP	SI,-10h
	JBE	L_008E
	NOP
	NOP
	NOP
L_0063:
	MOV	CX,0FFF0h
	PUSH	DS
	LDS	DX,[BP-04]
	MOV	AH,3Fh		;read file
	INT	21h
	POP	DS
	JB	L_009E
	NOP
	NOP
	NOP
	CMP	AX,0
	JZ	L_00B5
	NOP
	NOP
	NOP
	SUB	SI,AX
	SBB	DI,0
	SHR	AX,1
	SHR	AX,1
	SHR	AX,1
	SHR	AX,1
	ADD	[BP-02],AX
	JMP	SHORT L_0054
L_008E:
	MOV	CX,SI
	PUSH	DS
	LDS	DX,[BP-04]
	MOV	AH,3Fh		;read file
	INT	21h
	POP	DS
	JNB	L_00B5
	NOP
	NOP
	NOP
L_009E:
	MOV	_D_240C,AX
	CALL	_D_240E
	TEST	_D_240C,0FFFFh
	JNZ	L_00B0
	JMP	L_0014
L_00B0:
	XOR	AX,AX
	STC
	JMP	SHORT L_00B8
L_00B5:
	MOV	AX,1
L_00B8:
	POP	DI
	POP	SI
	MOV	SP,BP
	POP	BP
	RET	0Eh
C_0C06_000C	ENDP

OSIREAD_TEXT	ENDS
END
