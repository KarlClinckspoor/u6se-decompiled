;----------------------------------------
;OSI library: opening and closing a file (pascal), and the file error handler
;(GAME.EXE 2E79). On a DOS error the routines put the error code in
;OSI_ErrorCode and call OSI_ErrorHandler: if it clears OSI_ErrorCode, the
;call is retried; otherwise they return -1 (open) with CF set.
;----------------------------------------

DGROUP	GROUP _DATA

_DATA	SEGMENT WORD PUBLIC 'DATA'
PUBLIC _D_240C,_D_240E
_D_240C	dw 0			;D_240C error code of the last file routine (0: none)
_D_240E	dd C_0C02_002C		;D_240E error handler (far, called with DS = DGROUP)
_DATA	ENDS

OSIOPEN_TEXT	SEGMENT WORD PUBLIC 'CODE'
	ASSUME CS:OSIOPEN_TEXT,DS:DGROUP

;int pascal C_0C02_0000(char far *name): open the file for reading and writing,
;returns its handle (-1, CF set, on an error not retried)
PUBLIC C_0C02_0000
C_0C02_0000	PROC FAR	;0C02:0000
	PUSH	BP
	MOV	BP,SP
	MOV	_D_240C,0
L_0009:
	PUSH	DS
	LDS	DX,[BP+06]
	MOV	AX,3D02h	;open file, read/write
	INT	21h
	POP	DS
	JNB	L_0028
	MOV	_D_240C,AX
	CALL	_D_240E
	TEST	_D_240C,0FFFFh
	JZ	L_0009
	MOV	AX,0FFFFh
	STC
L_0028:
	POP	BP
	RET	4
C_0C02_0000	ENDP

;the default error handler: does nothing (the error stays: no retry)
PUBLIC C_0C02_002C
C_0C02_002C	PROC FAR	;0C02:002C
	RET
C_0C02_002C	ENDP

;void pascal C_0C02_002D(int handle): close the file
PUBLIC C_0C02_002D
C_0C02_002D	PROC FAR	;0C02:002D
	PUSH	BP
	MOV	BP,SP
L_0030:
	MOV	BX,[BP+06]
	MOV	AH,3Eh		;close file
	INT	21h
	JNB	L_0048
	MOV	_D_240C,AX
	CALL	_D_240E
	TEST	_D_240C,0FFFFh
	JZ	L_0030
L_0048:
	POP	BP
	RET	2
C_0C02_002D	ENDP

OSIOPEN_TEXT	ENDS
END
