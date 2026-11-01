;----------------------------------------
;OSI library (SAVAGE.EXE): mouse, restoring the background under the pointer
;----------------------------------------
INCLUDE OSI.INC

DSegStart
EXTRN D_20A2:WORD	;struct tMouse *: the mouse
EXTRN D_20C0:WORD	;where the pointer was drawn: x
EXTRN D_20C2:WORD	;  y
EXTRN D_2B3A:DWORD	;driver: restore the background (struct tSurface *s, void far *buf, int x, int y, void far *shapes, int index)
DSegEnd

CSegStart	MOUSERST_TEXT

;put back what the pointer covered
FProc	MOUSE_restoreBG	;0888:0006
	MOV	BX,D_20A2
	PUSH	[BX].shapeIndex
	PUSH	WORD PTR [BX].shapes+2
	PUSH	WORD PTR [BX].shapes
	PUSH	D_20C2
	PUSH	D_20C0
	PUSH	WORD PTR [BX].saveBuf+2
	PUSH	WORD PTR [BX].saveBuf
	PUSH	[BX].mSurface
	CALL	D_2B3A
	ADD	SP,10h
	RET
MOUSE_restoreBG	ENDP

CSegEnd	MOUSERST_TEXT

END
