;----------------------------------------
;OSI library (SAVAGE.EXE): shape files, the number of shapes
;A shape file: a table of 32-bit offsets (from the start of the file), one per shape,
;from +4; the first offset is also the size of the table. Each shape starts with its
;extents around its hot spot: right, left, up, down (words).
;----------------------------------------
INCLUDE OSI.INC

EXTRN C_0AD2_0024:FAR	;cdecl: the far pointer, normalised (returned in DX:AX)

CSegStart	SHPCOUNT_TEXT

;pascal int SHP_count(char far *shapes): the number of shapes in the file
FProc	SHP_count	;0750:0006
	PUSH	BP
	MOV	BP,SP
	PUSH	SI
	PUSH	DS
	PUSH	WORD PTR [BP+08]
	PUSH	WORD PTR [BP+06]
	CALL	C_0AD2_0024
	ADD	SP,4
	MOV	[BP+08],DX
	MOV	[BP+06],AX
	LDS	SI,[BP+06]
	MOV	AX,[SI+04]
	SHR	AX,1
	SHR	AX,1
	DEC	AX
	POP	DS
	POP	SI
	POP	BP
	RET	4
SHP_count	ENDP

CSegEnd	SHPCOUNT_TEXT

END
