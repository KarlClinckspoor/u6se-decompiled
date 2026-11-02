;----------------------------------------
;EMS memory: finding the expanded memory manager, allocating all its free
;pages, giving them back (GAME.EXE 2D17). The EMS pages are a heap of
;"virtual" far pointers: segment C000h-FFFFh, offset; EMS_map maps the
;16 KB pages of one into the page frame. Each block of the heap has a
;4-byte header: its length (bit 0 set: in use).
;----------------------------------------

DGROUP	GROUP _DATA

_DATA	SEGMENT WORD PUBLIC 'DATA'
PUBLIC _emsVersion,_emsFrameSeg,_emsTotalPages,_emsHeapPages
PUBLIC _emsMapped,_emsHandle,_emsReservePages,_emsReserveHandle,_emsInUse
emmDevName	db 'EMMXXXX0',0		;D_23B8 the name of the EMM device driver
_emsVersion	dw 0			;D_23C1 EMS version (BCD, 30h or more)
_emsFrameSeg	dw 0			;D_23C3 segment of the page frame
_emsTotalPages	dw 0			;D_23C5 total EMS pages
_emsHeapPages	dw 0			;D_23C7 EMS pages allocated for the heap
_emsMapped	dd 0			;D_23C9 the virtual pointer whose pages are mapped in
_emsHandle	dw 0			;D_23CD EMM handle of the heap's pages
_emsReservePages	dw 0			;D_23CF EMS pages to allocate apart (set before EMS_init)
_emsReserveHandle	dw 0			;D_23D1 EMM handle of those
	db 0			;D_23D3
_emsInUse	dw 0			;D_23D4 1: EMS in use
_DATA	ENDS

EXTRN _EMS_map:FAR

EMSINIT_TEXT	SEGMENT WORD PUBLIC 'CODE'
	ASSUME CS:EMSINIT_TEXT,DS:DGROUP

;int EMS_init(void): find an EMM (version 3.0 or later), allocate emsReservePages
;pages apart, then all the free pages for the heap; returns 1 (and emsInUse = 1)
;if EMS can be used, 0 otherwise
PUBLIC _EMS_init
_EMS_init	PROC FAR	;0AA0:0002
	PUSH	SI
	PUSH	DI
	MOV	AX,3567h	;get the vector of int 67h
	INT	21h
	MOV	DI,0Ah		;the device driver's name
	MOV	SI,OFFSET DGROUP:emmDevName
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
	MOV	_emsVersion,AX
	JB	L_0090
	MOV	AH,41h		;get page frame segment
	INT	67h
	OR	AH,AH
	JNZ	L_0090
	MOV	_emsFrameSeg,BX
	MOV	BX,_emsReservePages
	CMP	BX,0
	JZ	L_004D
	MOV	AH,43h		;allocate pages
	INT	67h
	OR	AH,AH
	JNZ	L_0090
	MOV	_emsReserveHandle,DX
L_004D:
	MOV	AH,42h		;get number of pages
	INT	67h
	OR	AH,AH
	JNZ	L_0081
	MOV	_emsTotalPages,DX
	MOV	_emsHeapPages,BX
	OR	BX,BX
	JZ	L_0081
	MOV	AH,43h		;allocate pages
	INT	67h
	OR	AH,AH
	JNZ	L_0081
	MOV	_emsHandle,DX
	MOV	_emsInUse,1
	PUSH	CS
	CALL	NEAR PTR _EMS_reset
	NOP
	OR	AH,AH
	JNZ	L_0081
	MOV	AX,1
	JMP	SHORT L_009F
L_0081:
	CMP	_emsReservePages,0
	JZ	L_0090
	MOV	AH,45h		;deallocate pages
	MOV	DX,_emsReserveHandle
	INT	67h
L_0090:
	MOV	_emsReservePages,0
	MOV	_emsReserveHandle,0
	MOV	AX,0
L_009F:
	MOV	_emsInUse,AX
	POP	DI
	POP	SI
	RET
_EMS_init	ENDP

;void EMS_reset(void): empty the heap: one free block over all its pages
PUBLIC _EMS_reset
_EMS_reset	PROC FAR	;0AA0:00A5
	PUSH	BP
	MOV	BP,SP
	SUB	SP,4
	MOV	AH,1
	CMP	_emsInUse,0
	JZ	L_00FA
	LES	AX,_emsMapped
	MOV	[BP-04],AX
	MOV	[BP-02],ES
	MOV	AX,4400h	;map page 0 to physical page 0
	MOV	DX,_emsHandle
	XOR	BX,BX
	INT	67h
	OR	AH,AH
	JNZ	L_00EC
	MOV	DX,_emsHeapPages	;DX:AX = pages * 10000h (AX = 0 here), / 4: pages * 16 KB
	SHR	DX,1
	RCR	AX,1
	SHR	DX,1
	RCR	AX,1
	SUB	AX,4
	SBB	DX,0
	MOV	ES,_emsFrameSeg
	MOV	ES:[0000],AX
	MOV	ES:[0002],DX
L_00EC:
	PUSH	WORD PTR [BP-02]
	PUSH	WORD PTR [BP-04]
	CALL	_EMS_map	;map back what was mapped
	ADD	SP,4
L_00FA:
	MOV	SP,BP
	POP	BP
	RET
_EMS_reset	ENDP

;void EMS_shutdown(void): give back the EMS pages
PUBLIC _EMS_shutdown
_EMS_shutdown	PROC FAR	;0AA0:00FE
	CMP	_emsInUse,0
	JZ	L_0128
	MOV	_emsInUse,0
	MOV	AH,45h		;deallocate pages
	MOV	DX,_emsHandle
	INT	67h
	CMP	_emsReservePages,0
	JZ	L_0128
	MOV	_emsReservePages,0
	MOV	AH,45h		;deallocate pages
	MOV	DX,_emsReserveHandle
	INT	67h
L_0128:
	RET
_EMS_shutdown	ENDP

EMSINIT_TEXT	ENDS
END
