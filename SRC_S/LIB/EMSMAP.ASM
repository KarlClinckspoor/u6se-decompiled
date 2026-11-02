;----------------------------------------
;EMS memory: mapping a virtual pointer to a real one; saving and restoring
;the page map (GAME.EXE 2D49)
;----------------------------------------

DGROUP	GROUP _DATA

_DATA	SEGMENT WORD PUBLIC 'DATA'
emsSavedMapped	dd 0			;D_23D6 emsMapped when the page map was saved
_DATA	ENDS

EXTRN _emsFrameSeg:WORD,_emsHeapPages:WORD,_emsMapped:DWORD,_emsHandle:WORD,_emsInUse:WORD

EMSMAP_TEXT	SEGMENT WORD PUBLIC 'CODE'
	ASSUME CS:EMSMAP_TEXT,DS:DGROUP

;int EMS_isVirtual(void far *p): 1 if p is a virtual (EMS) pointer: segment
;C000h or more
PUBLIC _EMS_isVirtual
_EMS_isVirtual	PROC FAR	;0AD2:0008
	PUSH	BP
	MOV	BP,SP
	MOV	DX,[BP+08]
	MOV	AX,[BP+06]
	MOV	BH,DH
	AND	BH,0C0h
	CMP	BH,0C0h
	JNZ	L_0020
	MOV	AX,1
	POP	BP
	RET
L_0020:
	XOR	AX,AX
	POP	BP
	RET
_EMS_isVirtual	ENDP

;void far *EMS_map(void far *p): a pointer that can be used: p itself if
;it is not a virtual pointer; otherwise map logical pages (segment - C000h)
;to (segment - C000h) + 3 to the 4 physical pages and return the page frame
;:offset (NULL if it fails or EMS is not in use). emsMapped remembers the last
;pointer mapped.
PUBLIC _EMS_map
_EMS_map	PROC FAR	;0AD2:0024
	PUSH	BP
	MOV	BP,SP
	SUB	SP,4
	PUSH	SI
	PUSH	DI
	MOV	DX,[BP+08]
	MOV	AX,[BP+06]
	MOV	BH,DH
	AND	BH,0C0h
	CMP	BH,0C0h
	JNZ	L_0091
	CMP	_emsInUse,0
	JZ	L_0097
	CMP	DX,WORD PTR _emsMapped+2
	JNZ	L_004F
	CMP	AX,WORD PTR _emsMapped
	JZ	L_008A
L_004F:
	MOV	WORD PTR _emsMapped,AX
	MOV	WORD PTR _emsMapped+2,DX
	AND	DH,3Fh
	CMP	DX,_emsHeapPages
	JNB	L_0097
	MOV	[BP-04],DX
	MOV	BYTE PTR [BP-02],0
L_0066:
	MOV	AH,44h		;map a logical page to a physical page
	MOV	AL,[BP-02]
	MOV	BX,[BP-04]
	CMP	BX,_emsHeapPages
	JNB	L_008A
	MOV	DX,_emsHandle
	INT	67h
	OR	AH,AH
	JNZ	L_0097
	INC	BYTE PTR [BP-02]
	INC	WORD PTR [BP-04]
	CMP	BYTE PTR [BP-02],4
	JB	L_0066
L_008A:
	MOV	DX,_emsFrameSeg
	MOV	AX,[BP+06]
L_0091:
	POP	DI
	POP	SI
	MOV	SP,BP
	POP	BP
	RET
L_0097:
	XOR	AX,AX
	XOR	DX,DX
	MOV	WORD PTR _emsMapped,AX
	MOV	WORD PTR _emsMapped+2,AX
	POP	DI
	POP	SI
	MOV	SP,BP
	POP	BP
	RET
_EMS_map	ENDP

;int EMS_saveMap(void): save the page map (EMM function 47h); 1 if done
PUBLIC _EMS_saveMap
_EMS_saveMap	PROC FAR	;0AD2:00A7
	PUSH	SI
	PUSH	DI
	PUSH	DS
	PUSH	BP
	CMP	_emsInUse,0
	JZ	L_00D6
	MOV	AH,47h		;save page map
	MOV	DX,_emsHandle
	INT	67h
	OR	AH,AH
	JNZ	L_00D6
	MOV	AX,1
	MOV	BX,WORD PTR _emsMapped
	MOV	WORD PTR emsSavedMapped,BX
	MOV	BX,WORD PTR _emsMapped+2
	MOV	WORD PTR emsSavedMapped+2,BX
	POP	BP
	POP	DS
	POP	DI
	POP	SI
	RET
L_00D6:
	XOR	AX,AX
	POP	BP
	POP	DS
	POP	DI
	POP	SI
	RET
_EMS_saveMap	ENDP

;int EMS_restoreMap(void): restore the page map (EMM function 48h); 1 if done
PUBLIC _EMS_restoreMap
_EMS_restoreMap	PROC FAR	;0AD2:00DD
	PUSH	SI
	PUSH	DI
	PUSH	DS
	PUSH	BP
	CMP	_emsInUse,0
	JZ	L_010C
	MOV	AH,48h		;restore page map
	MOV	DX,_emsHandle
	INT	67h
	OR	AH,AH
	JNZ	L_010C
	MOV	AX,1
	MOV	BX,WORD PTR emsSavedMapped
	MOV	WORD PTR _emsMapped,BX
	MOV	BX,WORD PTR emsSavedMapped+2
	MOV	WORD PTR _emsMapped+2,BX
	POP	BP
	POP	DS
	POP	DI
	POP	SI
	RET
L_010C:
	XOR	AX,AX
	POP	BP
	POP	DS
	POP	DI
	POP	SI
	RET
_EMS_restoreMap	ENDP

EMSMAP_TEXT	ENDS
END
