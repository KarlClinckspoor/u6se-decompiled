;----------------------------------------
;OSI library (SAVAGE.EXE): setting the whole VGA palette
;----------------------------------------
INCLUDE OSI.INC

CSegStart	PALSET_TEXT

;pascal PAL_setAll(unsigned char far *pal): pal (256 colors, 8 bits a component) is
;turned into 6 bits a component, in place, and set (int 10h AX 1012h)
FProc	PAL_setAll	;08E2:000A
	PUSH	BP
	MOV	BP,SP
	PUSH	DS
	PUSH	SI
	LDS	SI,[BP+06]
	MOV	CX,300h
C_0015:
	SHR	BYTE PTR [SI],1
	SHR	BYTE PTR [SI],1
	INC	SI
	LOOP	C_0015
	XOR	BX,BX
	MOV	CX,100h
	LES	DX,[BP+06]
	MOV	AX,1012h	;set a block of DAC registers
	INT	10h
	POP	SI
	POP	DS
	POP	BP
	RET	4
PAL_setAll	ENDP

CSegEnd	PALSET_TEXT

END
