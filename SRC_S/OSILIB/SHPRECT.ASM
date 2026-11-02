;----------------------------------------
;OSI library (SAVAGE.EXE): shape files, the rectangle a shape covers
;(see SHPCOUNT.ASM for the file)
;----------------------------------------
INCLUDE OSI.INC

EXTRN _EMS_map:FAR	;cdecl: the far pointer, normalised (returned in DX:AX)

CSegStart	SHPRECT_TEXT

;pascal int SHP_rect(int far *rect, int x, int y, char far *shapes, int index):
;the rectangle {left, top, right, bottom} that shape index covers when its hot spot is
;at (x, y); returns -1, or 0 (rect unchanged) when the file has no such shape
FProc	SHP_rect	;0746:000A
	PUSH	BP
	MOV	BP,SP
	PUSH	DS
	PUSH	ES
	PUSH	SI
	PUSH	DI
	PUSH	WORD PTR [BP+0Ah]
	PUSH	WORD PTR [BP+08]
	CALL	_EMS_map
	ADD	SP,4
	MOV	[BP+0Ah],DX
	MOV	[BP+08],AX
	LDS	SI,[BP+08]
	MOV	BX,[BP+06]
	;-- the file's linear address
	MOV	AX,DS
	XOR	DX,DX
	SHL	AX,1
	RCL	DX,1
	SHL	AX,1
	RCL	DX,1
	SHL	AX,1
	RCL	DX,1
	SHL	AX,1
	RCL	DX,1
	ADD	AX,SI
	ADC	DX,0
	SHL	BX,1
	SHL	BX,1
	CMP	BX,[SI+04]
	JL	C_0051
	XOR	AX,AX
	JMP	SHORT C_009E
C_0051:	;-- + the shape's offset: DS:SI the shape
	ADD	BX,4
	ADD	AX,[BX+SI]
	ADC	DX,[BX+SI+02]
	MOV	SI,AX
	db	83h,0E6h,0Fh	;AND SI,0Fh (MASM: 81 E6 0F 00)
	SHR	DX,1
	RCR	AX,1
	SHR	DX,1
	RCR	AX,1
	SHR	DX,1
	RCR	AX,1
	SHR	DX,1
	RCR	AX,1
	MOV	DS,AX
	LES	DI,[BP+10h]
	MOV	CX,[BP+0Eh]
	MOV	DX,[BP+0Ch]
	MOV	AX,[SI]
	ADD	AX,CX
	MOV	ES:[DI+04],AX	;right
	MOV	AX,CX
	SUB	AX,[SI+02]
	MOV	ES:[DI],AX	;left
	MOV	AX,DX
	SUB	AX,[SI+04]
	MOV	ES:[DI+02],AX	;top
	MOV	AX,[SI+06]
	ADD	AX,DX
	MOV	ES:[DI+06],AX	;bottom
	MOV	AX,0FFFFh
C_009E:
	POP	DI
	POP	SI
	POP	ES
	POP	DS
	POP	BP
	RET	0Eh
SHP_rect	ENDP

CSegEnd	SHPRECT_TEXT

END
