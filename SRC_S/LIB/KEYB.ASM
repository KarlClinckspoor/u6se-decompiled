;----------------------------------------
;SAVAGE.EXE: the keyboard interrupt (int 9): which keys are down, a callback for each key
;----------------------------------------

JMPN	MACRO target	;;near JMP without MASM's self-relative fixup
	db	0E9h
	dw	target-$-2
ENDM

KEYB_TEXT	SEGMENT PUBLIC WORD 'CODE'
	ASSUME CS:KEYB_TEXT

keyCode	dw	0	;0CB5:0006	the key (translated scan code) handed to the callback, in AX too
keepBios	dw	0	;0CB5:0008	1: keys passed on to the BIOS stay in its buffer
installed	db	0	;0CB5:000A
kbMode	db	0	;0CB5:000B	1: keys only passed on; 2: seen, then passed on; else seen only
lastScan	db	0	;0CB5:000C	the previous byte from the keyboard (E0: an extended key)
oldInt9	dd	0	;0CB5:000D
callback	dd	0	;0CB5:0011	void far (*)(void): called at each key (AX: the key)
tandy	dw	0	;0CB5:0015	1: a Tandy 1000 keyboard (scan codes translated by xlat)
rightAlt	db	0	;0CB5:0017
leftAlt	db	0	;0CB5:0018
keyHit	db	0	;0CB5:0019	set at each key pressed
nInts	dw	0	;0CB5:001A	keyboard interrupts counted
xlat	LABEL BYTE	;0CB5:001C	scan code translation: a Tandy 1000's to the standard ones
	db	000h,001h,002h,003h,004h,005h,006h,007h,008h,009h,00Ah,00Bh,00Ch,00Dh,00Eh,00Fh	;00
	db	010h,011h,012h,013h,014h,015h,016h,017h,018h,019h,01Ah,01Bh,01Ch,01Dh,01Eh,01Fh	;10
	db	020h,021h,022h,023h,024h,025h,026h,027h,028h,048h,02Ah,04Bh,02Ch,02Dh,02Eh,02Fh	;20
	db	030h,031h,032h,033h,034h,035h,036h,037h,038h,039h,03Ah,03Bh,03Ch,03Dh,03Eh,03Fh	;30
	db	040h,041h,042h,043h,044h,045h,046h,047h,048h,049h,050h,04Bh,04Ch,04Dh,04Dh,04Fh	;40
	db	050h,051h,052h,053h,054h,055h,056h,057h,058h,057h,058h,05Bh,05Ch,05Dh,05Eh,05Fh	;50
	db	060h,061h,062h,063h,064h,065h,066h,067h,068h,069h,06Ah,06Bh,06Ch,06Dh,06Eh,06Fh	;60
	db	070h,071h,072h,073h,074h,075h,076h,077h,078h,079h,07Ah,07Bh,07Ch,07Dh,07Eh,07Fh	;70
	db	080h,081h,082h,083h,084h,085h,086h,087h,088h,089h,08Ah,08Bh,08Ch,08Dh,08Eh,08Fh	;80
	db	090h,091h,092h,093h,094h,095h,096h,097h,098h,099h,09Ah,09Bh,09Ch,09Dh,09Eh,09Fh	;90
	db	0A0h,0A1h,0A2h,0A3h,0A4h,0A5h,0A6h,0A7h,0A8h,0A8h,0AAh,0ABh,0ACh,0ADh,0AEh,0AFh	;A0
	db	0B0h,0B1h,0B2h,0B3h,0B4h,0B5h,0B6h,0B7h,0B8h,0B9h,0BAh,0BBh,0BCh,0BDh,0BEh,0BFh	;B0
	db	0C0h,0C1h,0C2h,0C3h,0C4h,0C5h,0C6h,0C7h,0C8h,0C9h,0C0h,0CBh,0CCh,0CDh,0CDh,0CFh	;C0
	db	0D0h,0D1h,0D2h,0D3h,0D4h,0D5h,0D6h,0D7h,0D8h,0D7h,0D8h,0DBh,0DCh,0DDh,0DEh,0DFh	;D0
	db	0E0h,0E1h,0E2h,0E3h,0E4h,0E5h,0E6h,0E7h,0E8h,0E9h,0EAh,0EBh,0ECh,0EDh,0EEh,0EFh	;E0
	db	0F0h,0F1h,0F2h,0F3h,0F4h,0F5h,0F6h,0F7h,0F8h,0F9h,0FAh,0FBh,0FCh,0FDh,0FEh,0FFh	;F0
	PUBLIC	C_0CB5_011C	;(the key table: events.asm reads it)
C_0CB5_011C	LABEL	BYTE
keys	db	128 dup(0)	;0CB5:011C	1 for each key down (by scan code)

;empty the BIOS keyboard buffer
flushBios	PROC NEAR	;0CB5:019C
	PUSH	AX
C_019D:
	MOV	AH,1
	INT	16h
	JZ	C_01A9
	XOR	AH,AH
	INT	16h
	JMP	C_019D
C_01A9:
	POP	AX
	RET
flushBios	ENDP

;void KeyboardInstall(void): take over int 9, look for a Tandy 1000 keyboard
PUBLIC _C_0CB5_01AB
_C_0CB5_01AB	PROC FAR	;0CB5:01AB
	PUSH	AX
	PUSH	BX
	PUSH	CX
	PUSH	DX
	PUSH	DS
	PUSH	ES
	PUSH	DI
	MOV	CS:tandy,1
	MOV	CS:kbMode,0
	MOV	AX,3509h
	INT	21h
	MOV	WORD PTR CS:oldInt9,BX
	MOV	WORD PTR CS:oldInt9+2,ES
	PUSH	DS
	MOV	AX,KEYB_TEXT
	MOV	WORD PTR CS:callback+2,AX
	MOV	AX,OFFSET nullCallback
	MOV	WORD PTR CS:callback,AX
	MOV	AX,KEYB_TEXT
	MOV	DX,OFFSET keybInt
	MOV	DS,AX
	MOV	AX,2509h
	INT	21h
	POP	DS
	MOV	AX,0FFFFh	;the BIOS model byte at FFFF:000E
	MOV	ES,AX
	MOV	DI,0Eh
	MOV	AL,ES:[DI]
	CMP	AL,0FFh
	JNZ	C_020F
	MOV	AX,0FC00h
	MOV	ES,AX
	XOR	DI,DI
	MOV	AL,ES:[DI]
	CMP	AL,21h	;FC00:0000 = 21h: a Tandy
	JNZ	C_020F
	MOV	AX,0C000h	;BIOS configuration (fails on a Tandy 1000)
	INT	15h
	JB	C_0224
C_020F:	;not a Tandy: no translation
	MOV	CS:tandy,0
	PUSH	CS
	POP	ES
	MOV	DI,OFFSET xlat
	MOV	CX,100h
	XOR	AX,AX
C_0220:
	STOSB
	INC	AX
	LOOPNZ	C_0220
C_0224:
	MOV	CS:installed,1
	POP	DI
	POP	ES
	POP	DS
	POP	DX
	POP	CX
	POP	BX
	POP	AX
	RET
_C_0CB5_01AB	ENDP

;void KeyboardClear(void): no key down, but Alt and Ctrl as they are
PUBLIC _C_0CB5_0232
_C_0CB5_0232	PROC FAR	;0CB5:0232
	PUSH	AX
	PUSH	CX
	PUSH	ES
	PUSH	DI
	MOV	AX,KEYB_TEXT
	MOV	ES,AX
	MOV	DI,OFFSET keys
	XOR	AX,AX
	MOV	AL,ES:[DI+38h]	;Alt
	PUSH	AX
	MOV	AL,ES:[DI+1Dh]	;Ctrl
	PUSH	AX
	CLD
	XOR	AX,AX
	MOV	CX,80h
	REP	STOSB
	MOV	DI,OFFSET keys
	POP	AX
	MOV	ES:[DI+1Dh],AL
	POP	AX
	MOV	ES:[DI+38h],AL
	POP	DI
	POP	ES
	POP	CX
	POP	AX
	RET
_C_0CB5_0232	ENDP

;void KeyboardClearAll(void): no key down at all
PUBLIC _C_0CB5_0264
_C_0CB5_0264	PROC FAR	;0CB5:0264
	PUSH	ES
	PUSH	DI
	PUSH	AX
	MOV	AX,KEYB_TEXT
	MOV	ES,AX
	MOV	DI,OFFSET keys
	XOR	AX,AX
	MOV	ES:[DI+38h],AL
	MOV	ES:[DI+1Dh],AL
	PUSH	CS
	CALL	NEAR PTR _C_0CB5_0232
	POP	AX
	POP	DI
	POP	ES
	RET
_C_0CB5_0264	ENDP

;void KeyboardRemove(void): give int 9 back, empty the BIOS buffer
PUBLIC _C_0CB5_0281
_C_0CB5_0281	PROC FAR	;0CB5:0281
	PUSH	AX
	PUSH	BX
	PUSH	CX
	PUSH	DX
	PUSH	DS
	PUSH	ES
	MOV	AX,WORD PTR CS:oldInt9+2
	MOV	DX,WORD PTR CS:oldInt9
	MOV	DS,AX
	MOV	AX,2509h
	INT	21h
	CALL	flushBios
	MOV	CS:installed,0
	POP	ES
	POP	DS
	POP	DX
	POP	CX
	POP	BX
	POP	AX
	RET
_C_0CB5_0281	ENDP

;void cdecl KeyboardSetCallback(void (far *f)(void)): f is called at each key, with the
;key in AX (and keyCode)
PUBLIC _C_0CB5_02A7
_C_0CB5_02A7	PROC FAR	;0CB5:02A7
	PUSH	BP
	MOV	BP,SP
	PUSH	AX
	PUSHF
	CLI
	MOV	AX,[BP+6]
	MOV	WORD PTR CS:callback,AX
	MOV	AX,[BP+8]
	MOV	WORD PTR CS:callback+2,AX
	POPF
	POP	AX
	POP	BP
	RET
_C_0CB5_02A7	ENDP

;the callback until one is set
nullCallback	PROC FAR	;0CB5:02BF
	RET
nullCallback	ENDP

;the keyboard interrupt
keybInt	PROC FAR	;0CB5:02C0
	PUSHF
	PUSH	DS
	PUSH	AX
	CMP	CS:kbMode,1
	JNZ	C_02CE
	JMPN	C_0377
C_02CE:
	INC	CS:nInts
	PUSH	CX
	PUSH	BX
	XOR	CX,CX
	IN	AL,60h	;the scan code
	MOV	BL,AL
	IN	AL,61h	;acknowledge it
	MOV	AH,AL
	OR	AL,80h
	OUT	61h,AL
	XCHG	AH,AL
	OUT	61h,AL
	MOV	AL,BL
	CMP	AL,0E0h
	JZ	C_035E
	NOP		;(TASM's long conditional jumps)
	NOP
	NOP
	XOR	AH,AH
	MOV	BX,AX
	MOV	AL,CS:xlat[BX]
	RCL	BL,1
	ADC	CL,CH
	XOR	CL,1	;CL: 1 pressed, 0 released
	SHR	BL,1
	CMP	BL,2Ah	;left Shift
	JNZ	C_0315
	NOP
	NOP
	NOP
	CMP	CS:lastScan,0E0h	;the Shift an extended key adds: ignored
	JZ	C_035E
	NOP
	NOP
	NOP
C_0315:
	OR	CS:keyHit,CL
	CMP	BL,38h	;Alt
	JNZ	C_0350
	NOP
	NOP
	NOP
	CMP	CS:lastScan,0E0h
	JNZ	C_0337
	NOP
	NOP
	NOP
	MOV	CS:rightAlt,CL
	MOV	BX,1
	JMP	SHORT C_033E
C_0337:
	MOV	CS:leftAlt,CL
	XOR	BX,BX
C_033E:
	MOV	AL,CS:leftAlt
	OR	AL,CS:rightAlt
	MOV	CS:keys+38h,AL
	MOV	AX,38h
	JMP	SHORT C_0355
C_0350:
	MOV	CS:keys[BX],CL
C_0355:
	MOV	CS:keyCode,AX
	CALL	CS:callback
C_035E:
	MOV	CS:lastScan,AL
	POP	BX
	POP	CX
	CMP	CS:kbMode,2
	JZ	C_0377
	NOP
	NOP
	NOP
	MOV	AL,20h	;end of interrupt
	OUT	20h,AL
	POP	AX
	POP	DS
	POPF
	IRET
C_0377:	;on to the BIOS
	CMP	CS:keepBios,1
	JZ	C_0385
	NOP
	NOP
	NOP
	CALL	flushBios
C_0385:
	POP	AX
	POP	DS
	POPF
	JMP	CS:oldInt9
keybInt	ENDP

KEYB_TEXT	ENDS

END
