/* flags: -mm -c -O -G */
/*
 * Worlds of Ultima: The Savage Empire - SAVAGE.EXE
 * the OSI library: setting the video mode, and the allocation of the palette's colors
 */
#pragma inline
#include <stdio.h>

extern int D_3CE0[256];/*colors allocated: at each color of a run, the run's length (0: free)*/
extern void far C_07D6_0004(void);/*VID_initPort: the CRT status port*/

/*204A*/int vidMode = -1;/*the video mode last set (a graphics mode)*/

/*set a video mode, through the BIOS (the BIOS equipment word says monochrome for mode 7,
  color otherwise); a graphics mode frees all the palette's colors*/
/*C_07C3_0005*/void pascal VID_setMode(int mode) {
	register int i;

	if(mode > 3 && mode != 7) {
		vidMode = mode;
		for(i = 0; i < 256; i++)
			D_3CE0[i] = 0;
	}
	asm mov ax,mode
	asm push ds
	asm mov cx,40h
	asm mov ds,cx
	asm mov bl,20h
	asm mov ah,al
	asm and ah,7
	asm cmp ah,7
	asm jnz color
	asm mov bl,30h
color:
	asm and byte ptr ds:[10h],0CFh
	asm or ds:[10h],bl
	asm xor ah,ah
	asm push bp
	asm int 10h
	asm pop bp
	asm pop ds
	C_07D6_0004();
}

/*allocate a run of n free colors: its first color, or -1*/
/*C_07C3_0061*/int pascal PAL_alloc(register int n) {
	register int i;

	i = _CX = _DX = 0;
	for(; i < 256; i++) {
		if(D_3CE0[i]) {
			_CX = 0;
			_DX = i + 1;
		} else
			_CX++;
		if(_CX == n) {
			for(i = 0; i < n; i++)
				D_3CE0[i + _DX] = n;
			return _DX;
		}
	}
	return -1;
}

/*free the run of colors that starts at start*/
/*C_07C3_00B3*/void pascal PAL_free(int start) {
	register int i;
	register int n;

	n = D_3CE0[_AX = start];
	for(i = 0; i < n; i++)
		D_3CE0[i + _AX] = 0;
}

/*print the colors allocated ('.') and free ('_'), 64 a line*/
/*C_07C3_00DE*/PAL_dump() {
	int row;
	register int col;
	register int k;

	for(row = k = 0; row < 4; row++) {
		for(col = 0; col < 64; col++)
			printf("%c", D_3CE0[k++] ? '.' : '_');
		printf("\n");
	}
}
