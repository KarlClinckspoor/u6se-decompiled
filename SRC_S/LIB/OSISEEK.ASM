;----------------------------------------
;OSI library: moving a file's pointer (pascal) (GAME.EXE 2E89)
;----------------------------------------

DGROUP	GROUP _DATA
_DATA	SEGMENT WORD PUBLIC 'DATA'
_DATA	ENDS

EXTRN _osiError:WORD,_osiErrorHandler:DWORD

OSISEEK_TEXT	SEGMENT WORD PUBLIC 'CODE'
	ASSUME CS:OSISEEK_TEXT,DS:DGROUP

;long pascal OSI_seek(int handle, long pos, int whence): move the file
;pointer (whence: 0 start, 1 current, 2 end), returns the new position
;(-1L, CF set, on an error not retried)
PUBLIC OSI_seek
OSI_seek	PROC FAR	;0C1C:0002
	PUSH	BP
	MOV	BP,SP
	PUSH	SI
	PUSH	DI
	MOV	_osiError,0
L_000D:
	MOV	BX,[BP+0Ch]
	MOV	AX,[BP+06]
	MOV	DX,[BP+08]
	MOV	CX,[BP+0Ah]
	MOV	AH,42h		;move file pointer
	INT	21h
	JNB	L_0034
	MOV	_osiError,AX
	CALL	_osiErrorHandler
	TEST	_osiError,0FFFFh
	JZ	L_000D
	MOV	AX,0FFFFh
	MOV	DX,AX
	STC
L_0034:
	POP	DI
	POP	SI
	POP	BP
	RET	8
OSI_seek	ENDP

OSISEEK_TEXT	ENDS
END
