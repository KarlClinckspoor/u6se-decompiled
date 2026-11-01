;----------------------------------------
;OSI library (SAVAGE.EXE): events, taking the next one from the queue
;(the queue and its variables are 07D9's)
;----------------------------------------
INCLUDE OSI.INC

DSegStart
EXTRN D_2064:WORD	;the number of events in the queue
EXTRN D_2096:WORD	;where the next one is read
EXTRN D_209A:WORD	;the queue's first entry
EXTRN D_209C:WORD	;its last entry
DSegEnd

EXTRN C_07D9_07A7:FAR	;(07D9) update the queue

CSegStart	MOUSEVT_TEXT

;pascal EVT_get(char *e): copy the next event (16 bytes, its first word its type) to e
;and take it from the queue; events of type -1 are skipped; with none, e's type is 0
FProc	EVT_get	;0869:000E
	PUSH	BP
	MOV	BP,SP
	PUSH	ES
	PUSH	SI
	PUSH	DI
C_0014:
	CALL	C_07D9_07A7
	PUSH	DS
	POP	ES
	MOV	AX,D_2064
	OR	AX,AX
	JNZ	C_002A
	MOV	DI,[BP+06]
	MOV	ES:[DI],AX
	JZ	C_004F
C_002A:
	MOV	SI,D_2096
	MOV	AX,[SI]
	MOV	DI,[BP+06]
	MOV	CX,10h
	REP	MOVSB
	CMP	SI,D_209C
	JLE	C_0042
	MOV	SI,D_209A
C_0042:
	MOV	D_2096,SI
	DEC	D_2064
	CMP	AX,0FFFFh
	JZ	C_0014
C_004F:
	POP	DI
	POP	SI
	POP	ES
	POP	BP
	RET	2
EVT_get	ENDP

CSegEnd	MOUSEVT_TEXT

END
