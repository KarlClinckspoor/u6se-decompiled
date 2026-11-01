;----------------------------------------
;OSI library (SAVAGE.EXE): mouse, showing the pointer
;----------------------------------------
INCLUDE OSI.INC

DSegStart
EXTRN D_2061:WORD	;(07D9) the pointer is shown when 0, hidden when below
EXTRN D_201A:DWORD	;int far *: while not 0 the pointer is left alone
EXTRN D_34F2:DWORD	;int far *: the same
DSegEnd

EXTRN C_088A_000C:FAR	;MOUSE_saveBG
EXTRN C_0892_0008:FAR	;MOUSE_draw

CSegStart	MOUSESHO_TEXT

busy	db 0	;0880:0004 1 while drawing

;show the pointer (once as many times as it was hidden)
FProc	MOUSE_show	;0880:0005
	PUSH	DI
	PUSH	ES
	INC	D_2061
	JNZ	C_003F
	CMP	busy,0
	JNZ	C_003F
	INC	busy
	LES	DI,D_201A
	MOV	AX,ES:[DI]
	OR	AX,AX
	JNZ	C_003A
	LES	DI,D_34F2
	MOV	AX,ES:[DI]
	OR	AX,AX
	JNZ	C_003A
	CALL	C_088A_000C
	CALL	C_0892_0008
C_003A:
	DEC	busy
C_003F:
	POP	ES
	POP	DI
	RET
MOUSE_show	ENDP

CSegEnd	MOUSESHO_TEXT

END
