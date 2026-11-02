;----------------------------------------
;OSI library (SAVAGE.EXE): the library's shared variables (a module with data only)
;Which module of Origin's library held these is not known: they come after the shape
;routines (0753) and before TXTJUST (0756) in the data segment, and their relocations
;sit between those two modules' in the .EXE relocation table, so this module is linked
;between them. The far pointers start as DGROUP:0000.
;(DS:2014-2015, a zero word, belongs either to this module or to the one before 0731)
;----------------------------------------
INCLUDE OSI.INC

DSegStart
PUBLIC D_2016,_D_201A,_pText,_D_2020,_D_2024,_drvPutChar,D_202C,D_202E,D_2030,D_2032

D_2014	dw 0	;D_2014 (not used by SAVAGE.EXE; it could as well be the last of 071C's data)
D_2016	LABEL	DWORD	;D_2016 (not used by SAVAGE.EXE)
	FarNull
_D_201A	LABEL	DWORD	;D_201A int far *: while *D_201A is not 0 the mouse pointer is not redrawn
	FarNull
_pText	dw 0	;D_201E struct tText *: the current text window
_D_2020	LABEL	DWORD	;D_2020 driver: copy a line of pixels (ES: segment, SI: scanlines, BX: from y, DI: to y, DX: left, CX: right)
	FarNull
_D_2024	LABEL	DWORD	;D_2024 driver: fill a line of pixels (ES: segment, SI: scanlines, BX: y, DX: left, CX: right, AL: color)
	FarNull
_drvPutChar	LABEL	DWORD	;D_2028 driver: draw the character AX (BX: struct tText *, CX: height, DX: width, SI: 2*y)
	FarNull
D_202C	dw 0	;D_202C TXT_clearLine: the line of pixels being filled
D_202E	dw 0	;D_202E TXT_clearLine: lines of pixels left
D_2030	dw 0	;D_2030 added to the cursor's x to test whether a character fits (never set: 0)
D_2032	dw 0	;D_2032 the CRT status port (3BAh or 3DAh), set by VID_initPort
DSegEnd

END
