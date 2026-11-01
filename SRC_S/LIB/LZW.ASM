;----------------------------------------
;LZW decompression (GAME.EXE 2E5D), Origin's LZW (as U6's LZW.ASM: 9 to 12
;bit codes, 256 clear, 257 end), decoding from a file through an input
;buffer to far memory. C_0C58_0005 sets it up: D_23EA the file, D_23F9 the
;input buffer of D_23FD bytes, D_23F5 where the data goes, D_23EE the segment
;of the dictionary (3 bytes per code).
;----------------------------------------

DGROUP	GROUP _DATA

_DATA	SEGMENT WORD PUBLIC 'DATA'
PUBLIC _D_23E2,_D_23EA,_D_23EE,_D_23F5,_D_23F9,_D_23FD,_D_2401
_D_23E2	dw 0		;D_23E2 nonzero: stop when the input buffer runs out
D_23E4	dw 0		;D_23E4 stack count: the bytes of a string, pushed backwards
D_23E6	dw 0		;D_23E6 bits per code (9-12)
D_23E8	dw 0		;D_23E8 the first code that needs one more bit
_D_23EA	dw 0		;D_23EA handle of the file
D_23EC	dw 0		;D_23EC the next free code
_D_23EE	dw 0		;D_23EE segment of the dictionary
D_23F0	dw 0		;D_23F0 the code just read
D_23F2	db 0		;D_23F2 the first byte of the last string
D_23F3	dw 0		;D_23F3 the previous code
_D_23F5	dw 0,DGROUP	;D_23F5 where the next byte goes (far pointer)
_D_23F9	dw 0,DGROUP	;D_23F9 input: the byte of the next code (far pointer)
_D_23FD	dw 0		;D_23FD length of the input buffer
D_23FF	dw 0		;D_23FF offset of the input buffer
_D_2401	dw 0		;D_2401 bit offset of the next code in its byte
masks	dw 1FFh,3FFh,7FFh,0FFFh	;D_2403 the masks of 9 to 12 bits
_DATA	ENDS

LZW_TEXT	SEGMENT WORD PUBLIC 'CODE'
	ASSUME CS:LZW_TEXT,DS:DGROUP

;read the next code: AX. When the buffer runs low, what is left moves to
;its start and the rest is read from the file (or, if L_00A0 is set, the
;decompression stops)
read_code	PROC NEAR	;0BE6:0004
	PUSH	BP
	MOV	BP,_D_23FD
	MOV	BX,BP
	SUB	BX,3
	ADD	BX,D_23FF
	MOV	DX,_D_23EA
	MOV	AX,_D_2401
	MOV	CX,AX
	SHR	AX,1
	SHR	AX,1
	SHR	AX,1
	ADD	AX,WORD PTR _D_23F9
	MOV	WORD PTR _D_23F9,AX
	MOV	AX,CX
	AND	AX,7
	MOV	CX,AX
	ADD	AX,D_23E6
	MOV	_D_2401,AX
	PUSH	CX
	PUSH	DS
	LDS	AX,DWORD PTR _D_23F9
	MOV	CX,AX
	CMP	AX,BX
	JL	L_0080
	MOV	SI,DS
	POP	DS
	MOV	AX,CS:L_009E
	MOV	WORD PTR _D_23F9,AX
	MOV	D_23FF,AX
	PUSH	DS
	TEST	CS:L_00A0,0FFFFh
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
	MOV	BX,D_23E6
	SHL	BX,1
	AND	AX,masks[BX-18]
	POP	BP
	RET
read_code	ENDP

L_009E	dw 0		;0BE6:009E offset of the input buffer
L_00A0	dw 0		;0BE6:00A0 D_23E2

;void C_0BE6_00A2(void): decompress until the end code
PUBLIC _C_0BE6_00A2
_C_0BE6_00A2	PROC FAR	;0BE6:00A2
	PUSH	BP
	PUSH	DI
	PUSH	SI
	MOV	AX,WORD PTR _D_23F9
	MOV	L_009E,AX
	MOV	D_23FF,AX
	MOV	AX,_D_23E2
	MOV	L_00A0,AX
	CLD
	MOV	D_23E6,9
	MOV	D_23E8,200h
	MOV	D_23EC,102h
	MOV	D_23E4,0
	MOV	ES,_D_23EE
L_00D3:
	CALL	read_code
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
	MOV	D_23E6,9
	MOV	D_23E8,200h
	MOV	D_23EC,102h
	CALL	read_code
	MOV	D_23F3,AX
	MOV	D_23F2,AL
	PUSH	ES
	LES	DI,DWORD PTR _D_23F5
	STOSB
	POP	ES
	INC	WORD PTR _D_23F5
	JNZ	L_0113
	INC	WORD PTR _D_23F5+2
L_0113:
	JMP	SHORT L_00D3
L_0115:
	MOV	D_23F0,AX
	MOV	BX,AX
	SHL	BX,1
	ADD	BX,AX
	CMP	AX,D_23EC
	JL	L_0138
	MOV	AL,D_23F2
	PUSH	AX
	INC	D_23E4
	MOV	ES:[BX+02],AL
	MOV	AX,D_23F3
	MOV	ES:[BX],AX
	JMP	SHORT L_0149
L_0138:
	CMP	AX,0FFh
	JLE	L_0151
	MOV	AL,ES:[BX+02]
	PUSH	AX
	INC	D_23E4
	MOV	AX,ES:[BX]
L_0149:
	MOV	BX,AX
	SHL	BX,1
	ADD	BX,AX
	JMP	SHORT L_0138
L_0151:
	MOV	D_23F2,AL
	PUSH	AX
	INC	D_23E4
	MOV	DX,ES
	LES	DI,DWORD PTR _D_23F5
	MOV	CX,D_23E4
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
	MOV	WORD PTR _D_23F5+2,BX
	MOV	WORD PTR _D_23F5,DI
L_017D:
	MOV	D_23E4,CX
	MOV	ES,DX
	MOV	AX,D_23EC
	MOV	BX,AX
	SHL	AX,1
	ADD	BX,AX
	MOV	AL,D_23F2
	MOV	ES:[BX+02],AL
	MOV	AX,D_23F3
	MOV	ES:[BX],AX
	INC	D_23EC
	MOV	AX,D_23F0
	MOV	D_23F3,AX
	MOV	BX,D_23EC
	CMP	BX,D_23E8
	JL	L_01BC
	CMP	D_23E6,0Ch
	JZ	L_01BC
	INC	D_23E6
	SHL	D_23E8,1
L_01BC:
	JMP	L_00D3
_C_0BE6_00A2	ENDP

LZW_TEXT	ENDS
END
