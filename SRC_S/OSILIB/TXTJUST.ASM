;----------------------------------------
;OSI library (SAVAGE.EXE): text window, printing a string word-wrapped and justified
;(U.EXE's TXTJUST, made pascal; the line routines are near)
;The variables are defined after the code: the relocation of pStr comes after the
;code's in the .EXE relocation table (MASM writes the records in the order of the source)
;----------------------------------------
INCLUDE OSI.INC

DSegStart
EXTRN _pText:WORD	;struct tText *: the current text window
DSegEnd

EXTRN TXT_putcAL:FAR	;TXT_putcAL: print the character AL
EXTRN TXT_newLine:FAR	;TXT_newLine: new line

CSegStart	TXTJUST_TEXT

;print a string: pascal TXT_puts(char far *s)
FProc	TXT_puts	;0756:0002
	PUSH	BP
	MOV	BP,SP
	PUSH	DS
	PUSH	SI
	PUSH	DI
	LES	DI,[BP+06]
	CALL	TXT_putsES
	POP	DI
	POP	SI
	POP	DS
	POP	BP
	RET	4
TXT_puts	ENDP

;print the string at ES:DI, a line at a time: each line gets the words that fit in
;the window, and is printed in the window's justification
TXT_putsES	PROC NEAR	;0756:0015
	MOV	WORD PTR pStr,DI
	MOV	WORD PTR pStr+2,ES
	MOV	strPos,0
	MOV	BP,_pText
	MOV	AL,DS:[BP].justify
	MOV	mode,AL
	MOV	BX,DS:[BP].surface
	MOV	AX,[BX].right
	MOV	winRight,AX
	MOV	AX,[BX].left
	MOV	winLeft,AX
	LES	BX,pStr
C_0042:	;-- the first word always goes on the line
	MOV	SI,strPos
	MOV	lineStart,SI
	CMP	BYTE PTR ES:[BX+SI],0
	JZ	C_0061
C_0050:
	MOV	AL,ES:[BX+SI]
	INC	SI
	CMP	AL,0
	JZ	C_0061
	CMP	AL,0Ah
	JZ	C_0061
	CMP	AL,' '
	JNZ	C_0050
	DEC	SI
C_0061:
	MOV	lineEnd,SI
	;-- then the words while they fit
	MOV	CX,DS:[BP].curX
	MOV	SI,strPos
C_006D:
	MOV	AL,ES:[BX+SI]
	CMP	AL,0
	JZ	C_009B
	CMP	AL,0Ah
	JZ	C_009B
	CMP	AL,' '
	JNZ	C_0080
	MOV	lineEnd,SI
C_0080:
	INC	SI
	XOR	AH,AH
	MOV	BX,AX
	LES	DI,DS:[BP].font
	ADD	CL,ES:[BX+DI+04]	;+ width of the character
	LES	BX,pStr
	ADC	CH,AH
	CMP	CX,winRight
	JB	C_006D
	JMP	SHORT C_00A0
C_009B:	;-- the end of the string (or of a line) fits
	INC	SI
	MOV	lineEnd,SI
C_00A0:	;-- the line's width
	MOV	AX,winRight
	SUB	AX,DS:[BP].curX
	INC	AX
	MOV	lineRoom,AX
	XOR	CX,CX
	MOV	SI,lineStart
C_00B1:
	CMP	SI,lineEnd
	JNB	C_00CF
	MOV	AL,ES:[BX+SI]
	XOR	AH,AH
	MOV	BX,AX
	LES	DI,DS:[BP].font
	ADD	CL,ES:[BX+DI+04]
	LES	BX,pStr
	ADC	CH,AH
	INC	SI
	JMP	SHORT C_00B1
C_00CF:
	MOV	lineWidth,CX
	;-- print it
	MOV	AL,mode
	CMP	AL,0
	JZ	C_00F1
	CMP	AL,1
	JZ	C_00EC
	CMP	AL,2
	JZ	C_00E7
	CALL	TXT_lineFull
	JMP	SHORT C_00F4
C_00E7:
	CALL	TXT_lineCenter
	JMP	SHORT C_00F4
C_00EC:
	CALL	TXT_lineRight
	JMP	SHORT C_00F4
C_00F1:
	CALL	TXT_lineLeft
C_00F4:	;-- the next line
	MOV	SI,lineEnd
	LES	BX,pStr
	MOV	BP,_pText
	MOV	strPos,SI
	MOV	AL,ES:[BX+SI-01]
	CMP	AL,0
	JZ	C_012C
	CMP	AL,0Ah
	JZ	C_012C
	PUSH	AX
	CALL	TXT_newLine
	LES	BX,pStr
	MOV	SI,strPos
C_011E:	;-- skip the spaces
	CMP	BYTE PTR ES:[BX+SI],' '
	JNZ	C_0127
	INC	SI
	JMP	SHORT C_011E
C_0127:
	MOV	strPos,SI
	POP	AX
C_012C:
	CMP	AL,0
	JZ	C_0133
	JMPN	C_0042
C_0133:
	RET
TXT_putsES	ENDP

;centered line
TXT_lineCenter	PROC NEAR	;0756:0134
	MOV	SI,lineStart
	CMP	BYTE PTR ES:[BX+SI],0
	JZ	C_017B
	MOV	AX,lineWidth
	CMP	AX,lineRoom
	JA	C_017B
	MOV	AX,lineRoom
	SUB	AX,lineWidth
	INC	AX
	SHR	AX,1
	ADD	DS:[BP].curX,AX
	JMP	SHORT C_017B
TXT_lineCenter	ENDP

;line justified right
TXT_lineRight	PROC NEAR	;0756:0157
	MOV	SI,lineStart
	CMP	BYTE PTR ES:[BX+SI],0
	JZ	C_0174
	MOV	AX,lineWidth
	CMP	AX,lineRoom
	JA	C_0174
	MOV	AX,winRight
	SUB	AX,lineWidth
	INC	AX
	JMP	SHORT C_0177
C_0174:
	MOV	AX,winLeft
C_0177:
	MOV	DS:[BP].curX,AX
TXT_lineRight	ENDP

;line justified left: print the characters from lineStart to lineEnd
TXT_lineLeft	PROC NEAR	;0756:017B
C_017B:
	MOV	SI,lineStart
C_017F:
	LES	BX,pStr
	CMP	SI,lineEnd
	JNB	C_019A
	MOV	AL,ES:[BX+SI]
	CMP	AL,0
	JZ	C_019A
	PUSH	SI
	CALL	TXT_putcAL
	POP	SI
	INC	SI
	JMP	SHORT C_017F
C_019A:
	RET
TXT_lineLeft	ENDP

;fully justified line: the room left is shared between the spaces (not the last line
;of a paragraph)
TXT_lineFull	PROC NEAR	;0756:019B
	MOV	AX,lineWidth
	CMP	AX,lineRoom
	JNB	C_017B
	MOV	SI,lineEnd
	CMP	BYTE PTR ES:[BX+SI-01],0
	JZ	C_017B
	CMP	BYTE PTR ES:[BX+SI-01],0Ah
	JZ	C_017B
C_01B6:	;-- the leading spaces as they are
	MOV	SI,lineStart
	CMP	BYTE PTR ES:[BX+SI],' '
	JNZ	C_01E3
	MOV	AL,' '
	CALL	TXT_putcAL
	MOV	BP,_pText
	LES	DI,DS:[BP].font
	MOV	AL,ES:[DI+04+' ']
	LES	BX,pStr
	XOR	AH,AH
	SUB	lineWidth,AX
	INC	lineStart
	JMP	SHORT C_01B6
C_01E3:	;-- the first word
	MOV	SI,lineStart
	MOV	AL,ES:[BX+SI]
	CMP	AL,' '
	JZ	C_0213
	PUSH	AX
	CALL	TXT_putcAL
	POP	BX
	XOR	BH,BH
	MOV	BP,_pText
	LES	DI,DS:[BP].font
	MOV	AL,ES:[BX+DI+04]
	LES	BX,pStr
	XOR	AH,AH
	SUB	lineWidth,AX
	INC	lineStart
	JMP	SHORT C_01E3
C_0213:	;-- the room left, and the spaces to share it
	MOV	DX,winRight
	SUB	DX,DS:[BP].curX
	INC	DX
	SUB	DX,lineWidth
	XOR	CX,CX
	MOV	SI,lineStart
C_0226:
	CMP	SI,lineEnd
	JNB	C_0236
	CMP	BYTE PTR ES:[BX+SI],' '
	JNZ	C_0233
	INC	CX
C_0233:
	INC	SI
	JMP	SHORT C_0226
C_0236:
	CMP	CX,0
	JNZ	C_023E
	JMPN	C_017B
C_023E:
	MOV	AX,DX
	DIV	CL
	MOV	spaceRem,AH
	MOV	spaceAdd,AL
	MOV	SI,lineStart
C_024D:
	LES	BX,pStr
	CMP	SI,lineEnd
	JNB	C_028A
	MOV	AL,ES:[BX+SI]
	CMP	AL,0
	JZ	C_028A
	PUSH	SI
	PUSH	AX
	CALL	TXT_putcAL
	POP	AX
	CMP	AL,' '
	JNZ	C_0286
	MOV	BP,_pText
	MOV	AL,spaceAdd
	XOR	AH,AH
	ADD	DS:[BP].curX,AX
	CMP	spaceRem,0
	JZ	C_0286
	DEC	spaceRem
	INC	DS:[BP].curX
C_0286:
	POP	SI
	INC	SI
	JMP	SHORT C_024D
C_028A:
	RET
TXT_lineFull	ENDP

CSegEnd	TXTJUST_TEXT

DSegStart
pStr	LABEL	DWORD	;D_2034 the string
	FarNull
strPos	dw 0	;D_2038 index of the next line
lineStart	dw 0	;D_203A index of the line's first character
lineEnd	dw 0	;D_203C index after the line's last word
winLeft	dw 0	;D_203E the window's left
winRight	dw 0	;D_2040 the window's right
mode	db 0	;D_2042 the justification (tText.justify)
spaceRem	db 0	;D_2043 full justification: one more pixel to the first spaceRem spaces
spaceAdd	db 0	;D_2044   and pixels added to each space
lineRoom	dw 0	;D_2045 pixels from the cursor to the right of the window
lineWidth	dw 0	;D_2047 width in pixels of the line
DSegEnd

END
