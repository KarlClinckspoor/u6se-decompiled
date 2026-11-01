;----------------------------------------
;OSI library (SAVAGE.EXE): text window, printing a character
;(U.EXE's TXTCHAR, newer: no variables, far calls within the module made
;PUSH CS / CALL near)
;----------------------------------------
INCLUDE OSI.INC

DSegStart
EXTRN D_201E:WORD	;struct tText *: the current text window
EXTRN D_2028:DWORD	;driver: draw a character
EXTRN D_2030:WORD	;added to the cursor's x to see whether a character fits
DSegEnd

EXTRN C_07B8_001A:FAR	;TXT_scrollUp1

CSegStart	TXTCHAR_TEXT

;pascal TXT_putc(int c): print a character
FProc	TXT_putc	;0784:0002
	PUSH	BP
	MOV	BP,SP
	MOV	AX,[BP+06]
	PUSH	SI
	PUSH	DI
	PUSH	DS
	PUSH	CS
	CALL	NEAR PTR TXT_putcAL
	POP	DS
	POP	DI
	POP	SI
	POP	BP
	RET	2
TXT_putc	ENDP

;print the character in AL at the cursor (13: carriage return, 10: new line), going to
;a new line when it does not fit
FProc	TXT_putcAL	;0784:0016
	XOR	AH,AH
	CMP	AL,0Dh
	JNZ	C_001E
	JMP	SHORT C_006C
C_001E:
	CMP	AL,0Ah
	JNZ	C_0024
	JMP	SHORT C_0079
C_0024:
	PUSH	AX
	MOV	BX,D_201E
	LES	SI,[BX].font
	MOV	BP,AX
	MOV	CX,ES:[SI]	;height
	MOV	DL,ES:[BP+SI+04]	;width
	XOR	DH,DH
	CLD
C_0038:
	MOV	BX,D_201E
	LES	SI,[BX].font
	MOV	AX,[BX].curX
	ADD	AX,D_2030
	DEC	AX
	MOV	SI,[BX].surface
	CMP	AX,[SI].right
	JLE	C_0054
	PUSH	CS
	CALL	NEAR PTR TXT_newLine
	JMP	SHORT C_0038
C_0054:
	MOV	AX,[BX].curY
	ADD	AX,CX
	CMP	AX,[SI].bottom
	JG	C_006A
	SUB	AX,CX
	SHL	AX,1
	MOV	SI,AX
	POP	AX
	CALL	D_2028
	PUSH	AX
C_006A:
	POP	AX
	RET
TXT_putcAL	ENDP

;carriage return
FProc	TXT_cr	;0784:006C
C_006C:
	MOV	BX,D_201E
	MOV	SI,[BX].surface
	MOV	AX,[SI].left
	MOV	[BX].curX,AX
	RET
TXT_cr	ENDP

;new line, scrolling the window up at its bottom
FProc	TXT_newLine	;0784:0079
C_0079:
	PUSH	ES
	PUSH	DI
	PUSH	SI
	PUSH	BX
	PUSH	AX
	PUSH	CS
	CALL	NEAR PTR TXT_cr
	LES	DI,[BX].font
	MOV	AX,[BX].curY
	ADD	AX,ES:[DI]
	CMP	AX,[SI].bottom
	JL	C_0097
	CALL	C_07B8_001A
	JMP	SHORT C_009A
C_0097:
	MOV	[BX].curY,AX
C_009A:
	POP	AX
	POP	BX
	POP	SI
	POP	DI
	POP	ES
	RET
TXT_newLine	ENDP

CSegEnd	TXTCHAR_TEXT

END
