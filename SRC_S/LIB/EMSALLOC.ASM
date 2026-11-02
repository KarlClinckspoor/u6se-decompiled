;----------------------------------------
;EMS memory: allocating and freeing a block of the EMS heap (GAME.EXE 2D29).
;A block is a 4-byte header, its length (a long, bit 0 set: in use), then
;the data; blocks never cross a 16 KB page (the header of a block that would
;is cut short at the page's end: 3FFCh). A virtual pointer is
;(C000h + logical page):offset (see EMS_map).
;----------------------------------------

DGROUP	GROUP _DATA
_DATA	SEGMENT WORD PUBLIC 'DATA'
_DATA	ENDS

EXTRN _emsMapped:DWORD,_emsInUse:WORD
EXTRN _EMS_map:FAR

EMSALLOC_TEXT	SEGMENT WORD PUBLIC 'CODE'
	ASSUME CS:EMSALLOC_TEXT,DS:DGROUP

;void far *EMS_alloc(unsigned long size): allocate size (less than 64 KB - 4)
;bytes of the EMS heap, first fit; returns the block's virtual pointer, or NULL
PUBLIC _EMS_alloc
_EMS_alloc	PROC FAR	;0AB2:000A
	PUSH	BP
	MOV	BP,SP
	SUB	SP,14h
	PUSH	SI
	PUSH	DI
	CMP	_emsInUse,0
	JNZ	L_001C
	JMP	L_016D
L_001C:
	CMP	WORD PTR [BP+08],0
	JZ	L_0025
	JMP	L_016D
L_0025:
	CMP	WORD PTR [BP+06],-4
	JBE	L_002E
	JMP	L_016D
L_002E:
	ADD	WORD PTR [BP+06],3	;round up to 4
	db 83h,66h,06h,0FCh	;AND WORD PTR [BP+06],-4
	LES	AX,_emsMapped
	MOV	[BP-0Ch],AX
	MOV	[BP-0Ah],ES
	MOV	AX,0C000h
	MOV	[BP-02],AX
	PUSH	AX
	XOR	AX,AX
	MOV	[BP-04],AX
	PUSH	AX
	CALL	_EMS_map
	ADD	SP,4
	MOV	ES,DX
	MOV	SI,AX
	MOV	AX,ES:[SI]
	MOV	DX,ES:[SI+02]
	ADD	SI,4
L_0063:
	TEST	AX,1
	JZ	L_0070
L_0068:
	CALL	emsNextBlock
	JNZ	L_0063
	JMP	L_016D
L_0070:
	MOV	BX,[BP+06]
	XOR	CX,CX
	ADD	SI,BX
	JNB	L_0084
	SUB	SI,BX
	SUB	BX,SI
	ADD	BX,4004h
	ADC	CX,0
L_0084:
	MOV	[BP-10h],BX
	MOV	[BP-0Eh],CX
	MOV	[BP-14h],AX
	MOV	[BP-12h],DX
	MOV	CX,[BP-04]
	MOV	[BP-08],CX
	MOV	CX,[BP-02]
	MOV	[BP-06],CX
L_009C:
	TEST	AX,1
	JNZ	L_0068
	MOV	BX,[BP-12h]
	CMP	BX,[BP-0Eh]
	JA	L_00CB
	JB	L_00B3
	MOV	BX,[BP-14h]
	CMP	BX,[BP-10h]
	JNB	L_00CB
L_00B3:
	CALL	emsNextBlock
	JNZ	L_00BB
	JMP	L_016D
L_00BB:
	ADD	[BP-14h],AX
	ADC	[BP-12h],DX
	ADD	WORD PTR [BP-14h],4
	ADC	WORD PTR [BP-12h],0
	JMP	SHORT L_009C
L_00CB:
	MOV	DX,[BP-08]
	MOV	[BP-04],DX
	MOV	DX,[BP-06]
	MOV	[BP-02],DX
	PUSH	WORD PTR [BP-06]
	PUSH	WORD PTR [BP-08]
	CALL	_EMS_map
	ADD	SP,4
	MOV	ES,DX
	MOV	SI,AX
	MOV	BX,[BP+06]
	ADD	AX,4
	ADD	AX,BX
	JNB	L_011D
	MOV	AX,3FFCh
	SUB	AX,SI
	MOV	ES:[SI],AX
	XOR	DX,DX
	MOV	ES:[SI+02],DX
	ADD	SI,4
	SUB	[BP-14h],AX
	SBB	[BP-12h],DX
	SUB	WORD PTR [BP-14h],4
	SBB	WORD PTR [BP-12h],0
	CALL	emsNextBlock
	JZ	L_016D
	MOV	BX,[BP+06]
	SUB	SI,4
L_011D:
	SUB	[BP-14h],BX
	SBB	WORD PTR [BP-12h],0
	INC	BX
	MOV	ES:[SI],BX
	MOV	WORD PTR ES:[SI+02],0
	MOV	AX,[BP-14h]
	OR	AX,[BP-12h]
	JZ	L_0150
	MOV	AX,BX
	XOR	DX,DX
	CALL	emsNextBlock
	JZ	L_016D
	MOV	AX,[BP-14h]
	SUB	AX,4
	MOV	ES:[SI-04],AX
	MOV	AX,[BP-12h]
	MOV	ES:[SI-02],AX
L_0150:
	PUSH	WORD PTR [BP-0Ah]
	PUSH	WORD PTR [BP-0Ch]
	CALL	_EMS_map
	ADD	SP,4
	MOV	AX,[BP-08]
	ADD	AX,4
	MOV	DX,[BP-06]
	POP	DI
	POP	SI
	MOV	SP,BP
	POP	BP
	RET
L_016D:
	XOR	AX,AX
	XOR	DX,DX
	POP	DI
	POP	SI
	MOV	SP,BP
	POP	BP
	RET
_EMS_alloc	ENDP

;the next block: DX:AX the length of the block at [BP-04] (virtual pointer
;[BP-04],[BP-02]); moves there, maps it in (ES:SI after its header) and
;returns its length in DX:AX; ZF set at the end of the heap
emsNextBlock	PROC NEAR	;0AB2:0177
	AND	AL,0FEh
	ADD	AX,4
	ADC	DX,0
	ADD	AX,[BP-04]
	ADC	DX,0
	MOV	BH,AH
	AND	AH,3Fh
	SHL	BH,1
	RCL	DX,1
	SHL	BH,1
	RCL	DX,1
	ADD	DX,[BP-02]
	MOV	[BP-02],DX
	MOV	[BP-04],AX
	PUSH	DX
	PUSH	AX
	CALL	_EMS_map
	ADD	SP,4
	MOV	ES,DX
	MOV	SI,AX
	OR	DX,DX
	JZ	L_01B7
	MOV	AX,ES:[SI]
	MOV	DX,ES:[SI+02]
	ADD	SI,4
L_01B7:
	RET
emsNextBlock	ENDP

;void EMS_free(void far *p): free a block of the EMS heap (p a virtual
;pointer, C000h or more)
PUBLIC _EMS_free
_EMS_free	PROC FAR	;0AB2:01B8
	PUSH	BP
	MOV	BP,SP
	SUB	SP,4
	PUSH	SI
	PUSH	DI
	CMP	_emsInUse,0
	JZ	L_0201
	MOV	AX,[BP+08]
	AND	AX,0C000h
	CMP	AX,0C000h
	JNZ	L_0201
	LES	AX,_emsMapped
	MOV	[BP-04],AX
	MOV	[BP-02],ES
	PUSH	WORD PTR [BP+08]
	PUSH	WORD PTR [BP+06]
	CALL	_EMS_map
	ADD	SP,4
	MOV	ES,DX
	MOV	BX,AX
	AND	BYTE PTR ES:[BX-04],0FEh
	PUSH	WORD PTR [BP-02]
	PUSH	WORD PTR [BP-04]
	CALL	_EMS_map
	ADD	SP,4
L_0201:
	POP	DI
	POP	SI
	MOV	SP,BP
	POP	BP
	RET
_EMS_free	ENDP

EMSALLOC_TEXT	ENDS
END
