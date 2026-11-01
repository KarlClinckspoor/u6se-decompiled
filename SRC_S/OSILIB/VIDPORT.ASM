;----------------------------------------
;OSI library (SAVAGE.EXE): the CRT status port
;----------------------------------------
INCLUDE OSI.INC

DSegStart
EXTRN D_2032:WORD	;the CRT status port
DSegEnd

CSegStart	VIDPORT_TEXT

;D_2032 = the display's input status register 1 (3BAh or 3DAh)
FProc	VID_initPort	;07D6:0004
	PUSH	ES
	MOV	AX,40h
	MOV	ES,AX
	MOV	DX,ES:[63h]	;0040:0063 video controller base I/O port address
	ADD	DL,6	;input status register 1
	MOV	D_2032,DX
	POP	ES
	RET
VID_initPort	ENDP

CSegEnd	VIDPORT_TEXT

END
