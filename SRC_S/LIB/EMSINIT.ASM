;----------------------------------------
;EMS memory: finding the expanded memory manager, allocating all its free
;pages, giving them back (GAME.EXE 2D17). The EMS pages are a heap of
;"virtual" far pointers: segment C000h-FFFFh, offset; C_0AD2_0024 maps the
;16 KB pages of one into the page frame. Each block of the heap has a
;4-byte header: its length (bit 0 set: in use).
;----------------------------------------

DGROUP	GROUP _DATA

_DATA	SEGMENT WORD PUBLIC 'DATA'
PUBLIC _D_23C1,_D_23C3,_D_23C5,_D_23C7,_D_23C9,_D_23CD,_D_23CF,_D_23D1,_D_23D4
D_23B8	db 'EMMXXXX0',0		;D_23B8 the name of the EMM device driver
_D_23C1	dw 0			;D_23C1 EMS version (BCD, 30h or more)
_D_23C3	dw 0			;D_23C3 segment of the page frame
_D_23C5	dw 0			;D_23C5 total EMS pages
_D_23C7	dw 0			;D_23C7 EMS pages allocated for the heap
_D_23C9	dd 0			;D_23C9 the virtual pointer whose pages are mapped in
_D_23CD	dw 0			;D_23CD EMM handle of the heap's pages
_D_23CF	dw 0			;D_23CF EMS pages to allocate apart (set before C_0AA0_0002)
_D_23D1	dw 0			;D_23D1 EMM handle of those
	db 0			;D_23D3
_D_23D4	dw 0			;D_23D4 1: EMS in use
_DATA	ENDS

EXTRN _C_0AD2_0024:FAR

EMSINIT_TEXT	SEGMENT WORD PUBLIC 'CODE'
	ASSUME CS:EMSINIT_TEXT,DS:DGROUP

;int C_0AA0_0002(void): find an EMM (version 3.0 or later), allocate D_23CF
;pages apart, then all the free pages for the heap; returns 1 (and D_23D4 = 1)
;if EMS can be used, 0 otherwise
PUBLIC _C_0AA0_0002
_C_0AA0_0002	PROC FAR	;0AA0:0002
	PUSH	SI
	PUSH	DI
	MOV	AX,3567h	;get the vector of int 67h
	INT	21h
	MOV	DI,0Ah		;the device driver's name
	MOV	SI,OFFSET DGROUP:D_23B8
	MOV	CX,8
	CLD
	REPZ	CMPSB
	JNZ	L_0090
	MOV	AH,40h		;get status
	INT	67h
	OR	AH,AH
	JNZ	L_0090
	MOV	AH,46h		;get version
	INT	67h
	XOR	AH,AH
	CMP	AL,30h
	MOV	_D_23C1,AX
	JB	L_0090
	MOV	AH,41h		;get page frame segment
	INT	67h
	OR	AH,AH
	JNZ	L_0090
	MOV	_D_23C3,BX
	MOV	BX,_D_23CF
	CMP	BX,0
	JZ	L_004D
	MOV	AH,43h		;allocate pages
	INT	67h
	OR	AH,AH
	JNZ	L_0090
	MOV	_D_23D1,DX
L_004D:
	MOV	AH,42h		;get number of pages
	INT	67h
	OR	AH,AH
	JNZ	L_0081
	MOV	_D_23C5,DX
	MOV	_D_23C7,BX
	OR	BX,BX
	JZ	L_0081
	MOV	AH,43h		;allocate pages
	INT	67h
	OR	AH,AH
	JNZ	L_0081
	MOV	_D_23CD,DX
	MOV	_D_23D4,1
	PUSH	CS
	CALL	NEAR PTR _C_0AA0_00A5
	NOP
	OR	AH,AH
	JNZ	L_0081
	MOV	AX,1
	JMP	SHORT L_009F
L_0081:
	CMP	_D_23CF,0
	JZ	L_0090
	MOV	AH,45h		;deallocate pages
	MOV	DX,_D_23D1
	INT	67h
L_0090:
	MOV	_D_23CF,0
	MOV	_D_23D1,0
	MOV	AX,0
L_009F:
	MOV	_D_23D4,AX
	POP	DI
	POP	SI
	RET
_C_0AA0_0002	ENDP

;void C_0AA0_00A5(void): empty the heap: one free block over all its pages
PUBLIC _C_0AA0_00A5
_C_0AA0_00A5	PROC FAR	;0AA0:00A5
	PUSH	BP
	MOV	BP,SP
	SUB	SP,4
	MOV	AH,1
	CMP	_D_23D4,0
	JZ	L_00FA
	LES	AX,_D_23C9
	MOV	[BP-04],AX
	MOV	[BP-02],ES
	MOV	AX,4400h	;map page 0 to physical page 0
	MOV	DX,_D_23CD
	XOR	BX,BX
	INT	67h
	OR	AH,AH
	JNZ	L_00EC
	MOV	DX,_D_23C7	;DX:AX = pages * 10000h (AX = 0 here), / 4: pages * 16 KB
	SHR	DX,1
	RCR	AX,1
	SHR	DX,1
	RCR	AX,1
	SUB	AX,4
	SBB	DX,0
	MOV	ES,_D_23C3
	MOV	ES:[0000],AX
	MOV	ES:[0002],DX
L_00EC:
	PUSH	WORD PTR [BP-02]
	PUSH	WORD PTR [BP-04]
	CALL	_C_0AD2_0024	;map back what was mapped
	ADD	SP,4
L_00FA:
	MOV	SP,BP
	POP	BP
	RET
_C_0AA0_00A5	ENDP

;void C_0AA0_00FE(void): give back the EMS pages
PUBLIC _C_0AA0_00FE
_C_0AA0_00FE	PROC FAR	;0AA0:00FE
	CMP	_D_23D4,0
	JZ	L_0128
	MOV	_D_23D4,0
	MOV	AH,45h		;deallocate pages
	MOV	DX,_D_23CD
	INT	67h
	CMP	_D_23CF,0
	JZ	L_0128
	MOV	_D_23CF,0
	MOV	AH,45h		;deallocate pages
	MOV	DX,_D_23D1
	INT	67h
L_0128:
	RET
_C_0AA0_00FE	ENDP

EMSINIT_TEXT	ENDS
END
