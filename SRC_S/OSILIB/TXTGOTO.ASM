;----------------------------------------
;OSI library (SAVAGE.EXE): text window, moving the cursor
;----------------------------------------
INCLUDE OSI.INC

DSegStart
EXTRN _pText:WORD	;struct tText *: the current text window
DSegEnd

CSegStart	TXTGOTO_TEXT

;pascal TXT_gotoxy(int x, int y)
FProc	TXT_gotoxy	;077E:000C
	PUSH	BP
	MOV	BP,SP
	MOV	BX,_pText
	MOV	AX,[BP+08]
	MOV	[BX].curX,AX
	MOV	AX,[BP+06]
	MOV	[BX].curY,AX
	POP	BP
	RET	4
TXT_gotoxy	ENDP

CSegEnd	TXTGOTO_TEXT

END
