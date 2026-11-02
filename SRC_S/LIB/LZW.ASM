;----------------------------------------
;LZW decompression (GAME.EXE 2E5D), Origin's LZW (as U6's LZW.ASM: 9 to 12
;bit codes, 256 clear, 257 end), decoding from a file through an input
;buffer to far memory. LZC_decompress sets it up: lzwFile the file, lzwIn the
;input buffer of lzwInSize bytes, lzwOut where the data goes, lzwDictSeg the segment
;of the dictionary (3 bytes per code).
;----------------------------------------

DGROUP	GROUP _DATA

_DATA	SEGMENT WORD PUBLIC 'DATA'
PUBLIC _lzwNoRefill,_lzwFile,_lzwDictSeg,_lzwOut,_lzwIn,_lzwInSize,_lzwBitPos
_lzwNoRefill	dw 0		;D_23E2 nonzero: stop when the input buffer runs out
lzwStackCount	dw 0		;D_23E4 stack count: the bytes of a string, pushed backwards
lzwCodeBits	dw 0		;D_23E6 bits per code (9-12)
lzwCodeLimit	dw 0		;D_23E8 the first code that needs one more bit
_lzwFile	dw 0		;D_23EA handle of the file
lzwNextCode	dw 0		;D_23EC the next free code
_lzwDictSeg	dw 0		;D_23EE segment of the dictionary
lzwCode	dw 0		;D_23F0 the code just read
lzwFirstByte	db 0		;D_23F2 the first byte of the last string
lzwPrevCode	dw 0		;D_23F3 the previous code
_lzwOut	dw 0,DGROUP	;D_23F5 where the next byte goes (far pointer)
_lzwIn	dw 0,DGROUP	;D_23F9 input: the byte of the next code (far pointer)
_lzwInSize	dw 0		;D_23FD length of the input buffer
lzwInStart	dw 0		;D_23FF offset of the input buffer
_lzwBitPos	dw 0		;D_2401 bit offset of the next code in its byte
lzwMasks	dw 1FFh,3FFh,7FFh,0FFFh	;D_2403 the masks of 9 to 12 bits
_DATA	ENDS

LZW_TEXT	SEGMENT WORD PUBLIC 'CODE'
	ASSUME CS:LZW_TEXT,DS:DGROUP

;read the next code: AX. When the buffer runs low, what is left moves to
;its start and the rest is read from the file (or, if lzwNoRefillCS is set, the
;decompression stops)
lzwReadCode	PROC NEAR	;0BE6:0004
	PUSH	BP
	MOV	BP,_lzwInSize
	MOV	BX,BP
	SUB	BX,3
	ADD	BX,lzwInStart
	MOV	DX,_lzwFile
	MOV	AX,_lzwBitPos
	MOV	CX,AX
	SHR	AX,1
	SHR	AX,1
	SHR	AX,1
	ADD	AX,WORD PTR _lzwIn
	MOV	WORD PTR _lzwIn,AX
	MOV	AX,CX
	AND	AX,7
	MOV	CX,AX
	ADD	AX,lzwCodeBits
	MOV	_lzwBitPos,AX
	PUSH	CX
	PUSH	DS
	LDS	AX,DWORD PTR _lzwIn
	MOV	CX,AX
	CMP	AX,BX
	JL	L_0080
	MOV	SI,DS
	POP	DS
	MOV	AX,CS:lzwInStartCS
	MOV	WORD PTR _lzwIn,AX
	MOV	lzwInStart,AX
	PUSH	DS
	TEST	CS:lzwNoRefillCS,0FFFFh
	JZ	L_0060
	ADD	SP,6
	POP	SI
	POP	DI
	POP	BP
	RET
L_0060:
	MOV	DS,SI
	PUSH	ES
	PUSH	AX
	PUSH	DS
	POP	ES
	MOV	DI,AX
	MOV	SI,CX
	SUB	CX,BP
	SUB	AX,CX
	MOV	CX,AX
	REP	MOVSB
	SUB	BP,AX
	MOV	CX,BP
	MOV	BX,DX
	MOV	DX,DI
	MOV	AH,3Fh		;read file
	INT	21h
	POP	AX
	POP	ES
L_0080:
	MOV	SI,AX
	LODSW
	MOV	BX,AX
	LODSB
	POP	DS
	POP	CX
	JCXZ	L_0090
L_008A:
	SHR	AL,1
	RCR	BX,1
	LOOP	L_008A
L_0090:
	MOV	AX,BX
	MOV	BX,lzwCodeBits
	SHL	BX,1
	AND	AX,lzwMasks[BX-18]
	POP	BP
	RET
lzwReadCode	ENDP

lzwInStartCS	dw 0		;0BE6:009E offset of the input buffer
lzwNoRefillCS	dw 0		;0BE6:00A0 lzwNoRefill

;void LZW_decompress(void): decompress until the end code
PUBLIC _LZW_decompress
_LZW_decompress	PROC FAR	;0BE6:00A2
	PUSH	BP
	PUSH	DI
	PUSH	SI
	MOV	AX,WORD PTR _lzwIn
	MOV	lzwInStartCS,AX
	MOV	lzwInStart,AX
	MOV	AX,_lzwNoRefill
	MOV	lzwNoRefillCS,AX
	CLD
	MOV	lzwCodeBits,9
	MOV	lzwCodeLimit,200h
	MOV	lzwNextCode,102h
	MOV	lzwStackCount,0
	MOV	ES,_lzwDictSeg
L_00D3:
	CALL	lzwReadCode
	CMP	AX,101h		;end
	JNZ	L_00DF
	POP	SI
	POP	DI
	POP	BP
	RET
L_00DF:
	CMP	AX,100h		;clear
	JZ	L_00E7
	JMP	SHORT L_0115
	NOP
L_00E7:
	MOV	lzwCodeBits,9
	MOV	lzwCodeLimit,200h
	MOV	lzwNextCode,102h
	CALL	lzwReadCode
	MOV	lzwPrevCode,AX
	MOV	lzwFirstByte,AL
	PUSH	ES
	LES	DI,DWORD PTR _lzwOut
	STOSB
	POP	ES
	INC	WORD PTR _lzwOut
	JNZ	L_0113
	INC	WORD PTR _lzwOut+2
L_0113:
	JMP	SHORT L_00D3
L_0115:
	MOV	lzwCode,AX
	MOV	BX,AX
	SHL	BX,1
	ADD	BX,AX
	CMP	AX,lzwNextCode
	JL	L_0138
	MOV	AL,lzwFirstByte
	PUSH	AX
	INC	lzwStackCount
	MOV	ES:[BX+02],AL
	MOV	AX,lzwPrevCode
	MOV	ES:[BX],AX
	JMP	SHORT L_0149
L_0138:
	CMP	AX,0FFh
	JLE	L_0151
	MOV	AL,ES:[BX+02]
	PUSH	AX
	INC	lzwStackCount
	MOV	AX,ES:[BX]
L_0149:
	MOV	BX,AX
	SHL	BX,1
	ADD	BX,AX
	JMP	SHORT L_0138
L_0151:
	MOV	lzwFirstByte,AL
	PUSH	AX
	INC	lzwStackCount
	MOV	DX,ES
	LES	DI,DWORD PTR _lzwOut
	MOV	CX,lzwStackCount
	JCXZ	L_017D
	MOV	BX,ES
L_0167:
	POP	AX
	STOSB
	OR	DI,DI
	JNZ	L_0173
	ADD	BX,1000h
	MOV	ES,BX
L_0173:
	LOOP	L_0167
	MOV	WORD PTR _lzwOut+2,BX
	MOV	WORD PTR _lzwOut,DI
L_017D:
	MOV	lzwStackCount,CX
	MOV	ES,DX
	MOV	AX,lzwNextCode
	MOV	BX,AX
	SHL	AX,1
	ADD	BX,AX
	MOV	AL,lzwFirstByte
	MOV	ES:[BX+02],AL
	MOV	AX,lzwPrevCode
	MOV	ES:[BX],AX
	INC	lzwNextCode
	MOV	AX,lzwCode
	MOV	lzwPrevCode,AX
	MOV	BX,lzwNextCode
	CMP	BX,lzwCodeLimit
	JL	L_01BC
	CMP	lzwCodeBits,0Ch
	JZ	L_01BC
	INC	lzwCodeBits
	SHL	lzwCodeLimit,1
L_01BC:
	JMP	L_00D3
_LZW_decompress	ENDP

LZW_TEXT	ENDS
END
