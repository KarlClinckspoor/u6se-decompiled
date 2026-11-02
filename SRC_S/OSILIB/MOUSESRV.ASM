;----------------------------------------
;OSI library (SAVAGE.EXE): mouse, following its moves
;----------------------------------------
INCLUDE OSI.INC

DSegStart
EXTRN _D_201A:DWORD	;int far *: while not 0 the pointer is left alone
EXTRN _D_34F2:DWORD	;int far *: the same
EXTRN D_205A:WORD	;(07D9) the mouse state at the last update: x
EXTRN D_205C:WORD	;  y
EXTRN D_205E:BYTE	;  buttons (left: bit 0)
EXTRN D_205F:BYTE	;  (right: bit 1)
EXTRN D_2060:BYTE	;  (middle: bit 2)
EXTRN D_20A2:WORD	;struct tMouse *: the mouse
DSegEnd

EXTRN MOUSE_hide:FAR	;MOUSE_hide
EXTRN MOUSE_show:FAR	;MOUSE_show

CSegStart	MOUSESRV_TEXT

;update the mouse state, and call onMove (the pointer hidden) when it changed;
;returns 1 when it changed
FProc	MOUSE_serve	;0876:000E
	PUSH	SI
	PUSH	DI
	XOR	AX,AX
	MOV	BX,D_20A2
	CMP	[BX].installed,AL
	JNZ	C_001E
	JMPN	C_00A0
C_001E:
	MOV	AX,3	;get the position and the buttons
	INT	33h
	SHR	CX,1
	MOV	AL,D_205E
	XOR	AL,BL
	AND	AL,1
	JNZ	C_004C
	MOV	AL,D_205F
	XOR	AL,BL
	AND	AL,2
	JNZ	C_004C
	MOV	AL,D_2060
	XOR	AL,BL
	AND	AL,4
	JNZ	C_004C
	CMP	D_205A,CX
	JNZ	C_004C
	CMP	D_205C,DX
	JZ	C_00A0
C_004C:
	MOV	D_205A,CX
	MOV	D_205C,DX
	MOV	D_205E,BL
	MOV	D_205F,BL
	MOV	D_2060,BL
	MOV	SI,D_20A2
	MOV	[SI].posX,CX
	MOV	[SI].posY,DX
	MOV	[SI].buttonM,AL
	MOV	[SI].buttonL,BL
	AND	[SI].buttonL,1
	MOV	[SI].buttonR,BL
	AND	[SI].buttonR,2
	LES	DI,_D_201A
	MOV	AX,ES:[DI]
	OR	AX,AX
	JNZ	C_009D
	LES	DI,_D_34F2
	MOV	AX,ES:[DI]
	OR	AX,AX
	JNZ	C_009D
	CALL	MOUSE_hide
	CALL	[SI].onMove
	CALL	MOUSE_show
C_009D:
	MOV	AX,1
C_00A0:
	POP	DI
	POP	SI
	RET
MOUSE_serve	ENDP

CSegEnd	MOUSESRV_TEXT

END
