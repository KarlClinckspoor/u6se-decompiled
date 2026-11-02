;----------------------------------------
;OSI library (SAVAGE.EXE): text window, scrolling up
;----------------------------------------
INCLUDE OSI.INC

DSegStart
EXTRN _pText:WORD	;struct tText *: the current text window
DSegEnd

EXTRN TXT_copyLine:FAR	;TXT_copyLine
EXTRN TXT_clearLine:FAR	;TXT_clearLine

CSegStart	TXTSCRL_TEXT

;pascal TXT_scrollUp(int n): scroll the window up by n lines of text
FProc	TXT_scrollUp	;07B8:0000
	PUSH	BP
	MOV	BP,SP
	PUSH	SI
	PUSH	DI
	PUSH	DS
	PUSH	ES
	MOV	CX,[BP+06]
C_000A:
	PUSH	CX
	PUSH	CS
	CALL	NEAR PTR TXT_scrollUp1
	POP	CX
	LOOP	C_000A
	POP	ES
	POP	DS
	POP	DI
	POP	SI
	POP	BP
	RET	2
TXT_scrollUp	ENDP

;scroll up by one line of text, and clear the line at the bottom
FProc	TXT_scrollUp1	;07B8:001A
	MOV	BX,_pText
	MOV	SI,[BX].surface
	PUSH	SI
	LES	DI,[BX].font
	MOV	AX,[SI].top
	MOV	CX,ES:[DI]	;font height
	PUSH	CX
	ADD	AX,CX
	MOV	DX,[SI].bottom
C_0030:
	CMP	AX,DX
	JG	C_0043
	MOV	BX,AX
	SUB	BX,ES:[DI]
	PUSH	AX
	CALL	TXT_copyLine
	POP	AX
	INC	AX
	JMP	SHORT C_0030
C_0043:
	MOV	AX,DX
	MOV	BX,_pText
	POP	CX
	SUB	AX,CX
	INC	AX
	POP	SI
	MOV	DX,[SI].left
	CALL	TXT_clearLine
	RET
TXT_scrollUp1	ENDP

CSegEnd	TXTSCRL_TEXT

END
