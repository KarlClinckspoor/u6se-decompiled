;----------------------------------------
;OSI library (SAVAGE.EXE): mouse, initialisation
;----------------------------------------
INCLUDE OSI.INC

DSegStart
EXTRN D_2458:WORD	;(saved and given back to C_0CED_00A5)
EXTRN D_205A:WORD	;(07D9) the mouse's x at the last update
EXTRN D_20A2:WORD	;struct tMouse *: the mouse
DSegEnd

EXTRN C_0CED_00A5:FAR	;cdecl (int): sets D_2458
EXTRN C_07D9_002A:FAR	;(07D9) the default onMove: does nothing
EXTRN C_088A_000C:FAR	;MOUSE_saveBG
EXTRN C_07D6_0004:FAR	;VID_initPort
EXTRN C_0876_000E:FAR	;MOUSE_serve

CSegStart	MOUSEINI_TEXT

;pascal int MOUSE_init(struct tMouse *m, int serve): initialise the mouse (position 0,0);
;serve 1: update it at once. Returns the number of buttons (garbage without a mouse)
FProc	MOUSE_init	;086E:0006
	PUSH	BP
	MOV	BP,SP
	SUB	SP,2
	PUSH	SI
	PUSH	DI
	MOV	AX,D_2458
	MOV	[BP-02],AX
	XOR	AX,AX
	PUSH	AX
	CALL	C_0CED_00A5
	POP	AX
	MOV	BX,[BP+08]
	XOR	AX,AX
	MOV	[BX].buttonL,AL
	MOV	[BX].buttonR,AL
	MOV	[BX].buttonM,AL
	MOV	[BX].posX,AX
	MOV	[BX].posY,AX
	MOV	[BX].installed,AL
	MOV	WORD PTR [BX].onMove,OFFSET C_07D9_002A
	MOV	WORD PTR [BX].onMove+2,SEG C_07D9_002A
	MOV	CX,BX
	MOV	D_20A2,BX
	INT	33h	;AX 0: reset, get the number of buttons
	OR	AX,AX
	JZ	C_007B
	MOV	DX,BX
	MOV	BX,CX
	MOV	[BX].status,AX
	MOV	[BX].installed,DL
	PUSH	DX
	MOV	CX,0
	MOV	DX,0
	MOV	AX,4	;set the position
	INT	33h
	CALL	C_088A_000C
	CALL	C_07D6_0004
	CMP	WORD PTR [BP+06],1
	JNZ	C_007A
	MOV	D_205A,0FFFFh
	CALL	C_0876_000E
C_007A:
	POP	AX
C_007B:
	MOV	AX,[BP-02]
	PUSH	AX
	CALL	C_0CED_00A5
	POP	AX
	POP	DI
	POP	SI
	MOV	SP,BP
	POP	BP
	RET	4
MOUSE_init	ENDP

CSegEnd	MOUSEINI_TEXT

END
