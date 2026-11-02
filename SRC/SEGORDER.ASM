;GAME.EXE only: the order of the start-up code's segments, linked before C0M.OBJ.
;In GAME.EXE C0M's _BSSEND (edata@) comes before _BSS: bdata@ and edata@ are both
;DS 3268, so the start-up code clears no uninitialised data (TLINK /i gives zeros in the
;file anyway), and the near heap would start there. TLINK orders segments by the class
;and the segment first named in the link; this module names C0M's segments in its
;order but for _BSSEND before _BSS. (How Origin got this layout is not known.)

_TEXT	SEGMENT BYTE PUBLIC 'CODE'
_TEXT	ENDS
_FARDATA	SEGMENT PARA PUBLIC 'FAR_DATA'
_FARDATA	ENDS
_OVERLAY_	SEGMENT PARA PUBLIC 'OVRINFO'
_OVERLAY_	ENDS
_INIT_	SEGMENT WORD PUBLIC 'INITDATA'
_INIT_	ENDS
_INITEND_	SEGMENT BYTE PUBLIC 'INITDATA'
_INITEND_	ENDS
_EXIT_	SEGMENT WORD PUBLIC 'EXITDATA'
_EXIT_	ENDS
_EXITEND_	SEGMENT BYTE PUBLIC 'EXITDATA'
_EXITEND_	ENDS
_1STUB_	SEGMENT PARA PUBLIC 'STUBSEG'
_1STUB_	ENDS
_DATA	SEGMENT PARA PUBLIC 'DATA'
_DATA	ENDS
_CVTSEG	SEGMENT WORD PUBLIC 'DATA'
_CVTSEG	ENDS
_SCNSEG	SEGMENT WORD PUBLIC 'DATA'
_SCNSEG	ENDS
_BSSEND	SEGMENT BYTE PUBLIC 'ENDBSS'
_BSSEND	ENDS
_BSS	SEGMENT WORD PUBLIC 'BSS'
_BSS	ENDS
DGROUP	GROUP	_DATA,_CVTSEG,_SCNSEG,_BSSEND,_BSS
	END
