;----------------------------------------
;OSI library (SAVAGE.EXE): waiting for the beginning of a vertical retrace
;----------------------------------------
INCLUDE OSI.INC

DSegStart
EXTRN D_2032:WORD	;the CRT status port
DSegEnd

CSegStart	VSYNC_TEXT

;wait for the beginning of a vertical retrace
FProc	_VID_waitRetrace	;07D7:0008
	MOV	DX,D_2032
C_000C:
	IN	AL,DX
	TEST	AL,8
	JNZ	C_000C
C_0011:
	IN	AL,DX
	TEST	AL,8
	JZ	C_0011
	RET
_VID_waitRetrace	ENDP

CSegEnd	VSYNC_TEXT

END
