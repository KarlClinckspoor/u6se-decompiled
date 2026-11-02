;----------------------------------------
;OSI library (SAVAGE.EXE): mouse, hiding the pointer
;----------------------------------------
INCLUDE OSI.INC

DSegStart
EXTRN D_2061:WORD	;(07D9) the pointer is shown when 0, hidden when below
EXTRN _D_201A:DWORD	;int far *: while not 0 the pointer is left alone
EXTRN _D_34F2:DWORD	;int far *: the same
DSegEnd

EXTRN VID_inRetrace:FAR	;VID_inRetrace
EXTRN MOUSE_restoreBG:FAR	;MOUSE_restoreBG

CSegStart	MOUSEHID_TEXT

busy	db 0	;0884:0002 1 while drawing

;hide the pointer
FProc	MOUSE_hide	;0884:0003
	PUSH	DI
	PUSH	ES
	DEC	D_2061
	CMP	D_2061,-1
	JNZ	C_0042
	CMP	busy,0
	JNZ	C_0042
	INC	busy
	LES	DI,_D_201A
	MOV	AX,ES:[DI]
	OR	AX,AX
	JNZ	C_003D
	LES	DI,_D_34F2
	MOV	AX,ES:[DI]
	OR	AX,AX
	JNZ	C_003D
	CALL	VID_inRetrace
	CALL	MOUSE_restoreBG
C_003D:
	DEC	busy
C_0042:
	POP	ES
	POP	DI
	RET
MOUSE_hide	ENDP

CSegEnd	MOUSEHID_TEXT

END
