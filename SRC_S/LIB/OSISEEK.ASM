;----------------------------------------
;OSI library: moving a file's pointer (pascal) (GAME.EXE 2E89)
;----------------------------------------

DGROUP	GROUP _DATA
_DATA	SEGMENT WORD PUBLIC 'DATA'
_DATA	ENDS

EXTRN _D_240C:WORD,_D_240E:DWORD

OSISEEK_TEXT	SEGMENT WORD PUBLIC 'CODE'
	ASSUME CS:OSISEEK_TEXT,DS:DGROUP

;long pascal C_0C1C_0002(int handle, long pos, int whence): move the file
;pointer (whence: 0 start, 1 current, 2 end), returns the new position
;(-1L, CF set, on an error not retried)
PUBLIC C_0C1C_0002
C_0C1C_0002	PROC FAR	;0C1C:0002
	PUSH	BP
	MOV	BP,SP
	PUSH	SI
	PUSH	DI
	MOV	_D_240C,0
L_000D:
	MOV	BX,[BP+0Ch]
	MOV	AX,[BP+06]
	MOV	DX,[BP+08]
	MOV	CX,[BP+0Ah]
	MOV	AH,42h		;move file pointer
	INT	21h
	JNB	L_0034
	MOV	_D_240C,AX
	CALL	_D_240E
	TEST	_D_240C,0FFFFh
	JZ	L_000D
	MOV	AX,0FFFFh
	MOV	DX,AX
	STC
L_0034:
	POP	DI
	POP	SI
	POP	BP
	RET	8
C_0C1C_0002	ENDP

OSISEEK_TEXT	ENDS
END
