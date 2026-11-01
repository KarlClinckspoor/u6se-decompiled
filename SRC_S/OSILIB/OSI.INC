;OSI library (SAVAGE.EXE version): common macros and structures
;(this version's procedures save their registers in no fixed order: each module
;writes its own prologue)

DSegStart	MACRO
_DATA	SEGMENT PUBLIC WORD 'DATA'
ENDM

DSegEnd	MACRO
_DATA	ENDS
ENDM

DGROUP	GROUP _DATA

DSegStart
DSegEnd


CSegStart	MACRO segName
segName	SEGMENT PUBLIC WORD 'CODE'
	ASSUME CS:segName,DS:DGROUP
ENDM

CSegEnd	MACRO segName
segName	ENDS
ENDM

;far procedure (the prologue is written out)
FProc	MACRO procName
PUBLIC procName
procName	PROC FAR
ENDM

;near procedure
NProc	MACRO procName
procName	PROC NEAR
ENDM

;near JMP to a label of the same segment: MASM 3.0 leaves a self-relative fixup for a
;forward near jump; JMPN emits the jump without one
JMPN	MACRO target
	db	0E9h
	dw	target-$-2
ENDM

;far JMP to a procedure of the same segment
JMPF	MACRO target
	db	0EAh
	dd	target
ENDM

;a far pointer whose initial value is DGROUP:0000 (the original's far pointers in
;_DATA all start so: offset 0, segment relocated to DGROUP)
FarNull	MACRO
	dw	0
	dw	DGROUP
ENDM

;a drawing surface: a bitmap in memory (or the screen) with its clipping window
;(this version: a near scanline table, and left/top before right/bottom)
tSurface	STRUC
	segment_	dw ?	;+00 segment of the pixels
	scanlines	dw ?	;+02 offsets of the scanlines (one word per line)
	left	dw ?	;+04 clipping window (inclusive)
	top	dw ?	;+06
	right	dw ?	;+08
	bottom	dw ?	;+0A
tSurface	ENDS

;a text window: where and how the text routines print
tText	STRUC
	surface	dw ?	;+00 struct tSurface *
	curX	dw ?	;+02 cursor
	curY	dw ?	;+04
	font	dd ?	;+06 the font: height (word), ..., width of each character at +4
	fgColor	db ?	;+0A
	bgColor	db ?	;+0B
	buffer	dw ?	;+0C string buffer of the formatted output
	bufPtr	dw ?	;+0E where the next character goes
	justify	db ?	;+10 0: left, 1: right, 2: centered, 3: full
tText	ENDS

;the mouse (this version)
tMouse	STRUC
	posX	dw ?	;+00
	posY	dw ?	;+02
	buttonL	db ?	;+04 left button down (bit 0)
	buttonR	db ?	;+05 right button down (bit 1)
	buttonM	db ?	;+06 middle button down (bit 2)
	status	dw ?	;+07 what the driver's reset returned (0: no mouse)
	shapes	dd ?	;+09 the pointer shapes
	shapeIndex	dw ?	;+0D the shape shown
	saveBuf	dd ?	;+0F the background under it
	mSurface	dw ?	;+13 struct tSurface * it is drawn on
	onMove	dd ?	;+15 called when it moves or a button changes
	installed	db ?	;+19 the number of buttons (0: no mouse)
tMouse	ENDS
