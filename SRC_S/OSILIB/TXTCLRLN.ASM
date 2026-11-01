;----------------------------------------
;OSI library (SAVAGE.EXE): text window, clearing a line of text
;----------------------------------------
INCLUDE OSI.INC

DSegStart
EXTRN D_201E:WORD	;struct tText *: the current text window
EXTRN D_2024:DWORD	;driver: fill a line of pixels
EXTRN D_202C:WORD	;the line of pixels being filled
EXTRN D_202E:WORD	;lines of pixels left
DSegEnd

CSegStart	TXTCLRLN_TEXT

;clear a line of text (font height lines of pixels) from (DX,AX) to the right of the
;window, in the background color
FProc	TXT_clearLine	;07BD:0008
	PUSH	BP
	PUSH	BX
	CLD
	PUSH	DX
	MOV	D_202C,AX
	MOV	SI,D_201E
	MOV	DI,[SI].surface
	MOV	CX,[DI].right
	MOV	BP,CX
	MOV	AL,[SI].bgColor
	LES	BX,[SI].font
	MOV	DX,ES:[BX]
	MOV	D_202E,DX
	MOV	SI,[DI].scanlines
	MOV	ES,[DI].segment_
	POP	DX
C_002D:
	MOV	CX,BP
	MOV	BX,D_202C
	CALL	D_2024
	INC	D_202C
	DEC	D_202E
	JNZ	C_002D
	POP	BX
	POP	BP
	RET
TXT_clearLine	ENDP

CSegEnd	TXTCLRLN_TEXT

END
