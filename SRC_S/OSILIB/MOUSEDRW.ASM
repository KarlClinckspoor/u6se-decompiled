;----------------------------------------
;OSI library (SAVAGE.EXE): mouse, drawing the pointer
;----------------------------------------
INCLUDE OSI.INC

DSegStart
EXTRN D_20A2:WORD	;struct tMouse *: the mouse
EXTRN D_20C0:WORD	;where the pointer is drawn: x
EXTRN D_20C2:WORD	;  y
EXTRN D_2BC6:DWORD	;driver: draw a shape (struct tSurface *s, int x, int y, void far *shapes, int index)
DSegEnd

EXTRN C_088A_003B:FAR	;MOUSE_clamp

CSegStart	MOUSEDRW_TEXT

;draw the pointer at the mouse position (kept in the surface)
FProc	MOUSE_draw	;0892:0008
	CALL	C_088A_003B
	MOV	BX,D_20A2
	MOV	CX,[BX].posX
	MOV	DX,[BX].posY
	CMP	[BX].installed,0
	JZ	C_0025
	MOV	AX,4	;set the position
	SHL	CX,1
	INT	33h
	SHR	CX,1
C_0025:
	PUSH	[BX].shapeIndex
	PUSH	WORD PTR [BX].shapes+2
	PUSH	WORD PTR [BX].shapes
	MOV	D_20C2,DX
	PUSH	DX
	MOV	D_20C0,CX
	PUSH	CX
	PUSH	[BX].mSurface
	CALL	D_2BC6
	ADD	SP,0Ch
	RET
MOUSE_draw	ENDP

CSegEnd	MOUSEDRW_TEXT

END
