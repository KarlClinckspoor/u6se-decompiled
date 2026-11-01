;----------------------------------------
;OSI library (SAVAGE.EXE): the mouse's variables (a module with data only)
;Which module of Origin's library held these is not known: their relocation (D_20A0)
;sits between 0892's and 0896's in the .EXE relocation table, so they belong to 0892
;or to a module linked between 0892 and 0896 (this one). D_2052-209D before them are
;07D9's.
;----------------------------------------
INCLUDE OSI.INC

DSegStart
PUBLIC D_209E,D_20A2,D_20A4,D_20BE,D_20C0,D_20C2,D_20C4,D_20C6

D_209E	LABEL	DWORD	;D_209E (not used by SAVAGE.EXE)
	FarNull
D_20A2	dw 0	;D_20A2 struct tMouse *: the mouse
D_20A4	LABEL	BYTE	;D_20A4 struct tMouse: the mouse 07D9 initialises
	tMouse	<0,0,0,0,0,0,0,0,0,0,0,0>
D_20BE	dw 0	;D_20BE (07D9) how far the mouse moved: x
D_20C0	dw 0	;D_20C0 where the pointer was drawn: x
D_20C2	dw 0	;D_20C2   y
D_20C4	dw 0	;D_20C4 (07D9) how far the mouse moved: y
D_20C6	db 0	;D_20C6 (07D9) 1: the mouse moved
DSegEnd

END
