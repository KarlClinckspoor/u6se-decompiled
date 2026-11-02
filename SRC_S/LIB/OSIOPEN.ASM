;----------------------------------------
;OSI library: opening and closing a file (pascal), and the file error handler
;(GAME.EXE 2E79). On a DOS error the routines put the error code in
;osiError and call osiErrorHandler: if it clears osiError, the
;call is retried; otherwise they return -1 (open) with CF set.
;----------------------------------------

DGROUP	GROUP _DATA

_DATA	SEGMENT WORD PUBLIC 'DATA'
PUBLIC _osiError,_osiErrorHandler
_osiError	dw 0			;D_240C error code of the last file routine (0: none)
_osiErrorHandler	dd DOS_defaultHandler		;D_240E error handler (far, called with DS = DGROUP)
_DATA	ENDS

OSIOPEN_TEXT	SEGMENT WORD PUBLIC 'CODE'
	ASSUME CS:OSIOPEN_TEXT,DS:DGROUP

;int pascal DOS_open(char far *name): open the file for reading and writing,
;returns its handle (-1, CF set, on an error not retried)
PUBLIC DOS_open
DOS_open	PROC FAR	;0C02:0000
	PUSH	BP
	MOV	BP,SP
	MOV	_osiError,0
L_0009:
	PUSH	DS
	LDS	DX,[BP+06]
	MOV	AX,3D02h	;open file, read/write
	INT	21h
	POP	DS
	JNB	L_0028
	MOV	_osiError,AX
	CALL	_osiErrorHandler
	TEST	_osiError,0FFFFh
	JZ	L_0009
	MOV	AX,0FFFFh
	STC
L_0028:
	POP	BP
	RET	4
DOS_open	ENDP

;the default error handler: does nothing (the error stays: no retry)
PUBLIC DOS_defaultHandler
DOS_defaultHandler	PROC FAR	;0C02:002C
	RET
DOS_defaultHandler	ENDP

;void pascal DOS_close(int handle): close the file
PUBLIC DOS_close
DOS_close	PROC FAR	;0C02:002D
	PUSH	BP
	MOV	BP,SP
L_0030:
	MOV	BX,[BP+06]
	MOV	AH,3Eh		;close file
	INT	21h
	JNB	L_0048
	MOV	_osiError,AX
	CALL	_osiErrorHandler
	TEST	_osiError,0FFFFh
	JZ	L_0030
L_0048:
	POP	BP
	RET	2
DOS_close	ENDP

OSIOPEN_TEXT	ENDS
END
