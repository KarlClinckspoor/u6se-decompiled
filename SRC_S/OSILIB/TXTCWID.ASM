;----------------------------------------
;OSI library (SAVAGE.EXE): text window, the width of a character
;----------------------------------------
INCLUDE OSI.INC

DSegStart
EXTRN D_201E:WORD	;struct tText *: the current text window
DSegEnd

CSegStart	TXTCWID_TEXT

;pascal int TXT_charWidth(int c): its width in the window's font
FProc	TXT_charWidth	;0780:0004
	PUSH	BP
	MOV	BP,SP
	PUSH	DS
	PUSH	SI
	MOV	BX,D_201E
	LDS	SI,[BX].font
	MOV	BX,[BP+06]
	XOR	BH,BH
	MOV	AL,[BX+SI+04]
	XOR	AH,AH
	POP	SI
	POP	DS
	POP	BP
	RET	2
TXT_charWidth	ENDP

CSegEnd	TXTCWID_TEXT

END
