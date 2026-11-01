;----------------------------------------
;OSI library (SAVAGE.EXE): text window, adding a character to its string buffer
;----------------------------------------
INCLUDE OSI.INC

DSegStart
EXTRN D_201E:WORD	;struct tText *: the current text window
DSegEnd

CSegStart	TXTBUFC_TEXT

;pascal TXT_bufc(int c): add a character to the text window's string buffer
FProc	TXT_bufc	;0782:0000
	PUSH	BP
	MOV	BP,SP
	MOV	AX,[BP+06]
	CALL	TXT_bufcAL
	POP	BP
	RET	2
TXT_bufc	ENDP

;the same with the character in AL
TXT_bufcAL	PROC NEAR	;0782:000D
	PUSH	SI
	PUSH	BX
	MOV	BX,D_201E
	MOV	SI,[BX].bufPtr
	INC	[BX].bufPtr
	MOV	[SI],AL
	INC	SI
	MOV	BYTE PTR [SI],0
	POP	BX
	POP	SI
	RET
TXT_bufcAL	ENDP

CSegEnd	TXTBUFC_TEXT

END
