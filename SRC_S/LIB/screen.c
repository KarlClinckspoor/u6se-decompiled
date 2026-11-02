/* flags: -mm -c -O -G */
/*
 * SAVAGE.EXE: the drawing surfaces of the screen and of the back buffer, for each video
 * mode (vidMode: 4 CGA, 7 Hercules, 9 Tandy, 13 EGA, 0x13 VGA/MCGA)
 * (inline assembly: compiled through the assembler, its fixups in TASM's order)
 */

/*a drawing surface (see surface.c)*/
struct tSurface {
	unsigned seg;/*+00 segment of the pixels*/
	unsigned *lines;/*+02 offset of each line (from line top)*/
	int left;/*+04 clipping window (inclusive)*/
	int top;/*+06*/
	int right;/*+08*/
	int bottom;/*+0A*/
	char huge *pixels;/*+0C the memory allocated for it*/
};

extern int vidMode;/*the video mode*/
extern void (far *D_3516)(struct tSurface *, int);/*a TM.DRV routine (header +18)*/
extern unsigned D_407C[200];/*the screen's line offsets*/
extern struct tSurface *D_420C;/*the screen*/
extern struct tSurface *D_420E;/*the back buffer*/

extern int pascal AllocSurface(struct tSurface *s, int x, int n);/*AllocSurface*/

/*make s the screen (EGA: its second page); x: handed to the driver (*D_3516) unless -1
  (always -1 but in VGA/MCGA); 0: out of memory*/
/*C_0D28_000B*/int pascal ScreenSurface(register struct tSurface *s, register int x) {
	int i;

	D_420C = s;
	s->left = 0;
	s->top = 0;
	s->right = 319;
	s->bottom = 199;
	switch(vidMode) {
		case 13:
			if(AllocSurface(s, x, 0) == 0)
				return 0;
			i = s->seg;
			/*the CRT controller's start address (high byte): the surface*/
			asm mov dx,3d4h
			asm mov ax,i
			asm shl ax,1
			asm shl ax,1
			asm shl ax,1
			asm shl ax,1
			asm mov al,0ch
			asm out dx,ax
		break;
		case 4:
			s->seg = 0xb800;
			s->lines = D_407C;
			D_407C[0] = 0;
			D_407C[1] = 0x2000;
			for(i = 2; i < 200; i ++)
				D_407C[i] = D_407C[i - 2] + 80;
		break;
		case 7:
			s->seg = 0xb000;
			s->lines = D_407C;
			D_407C[0] = 0x032f;
			D_407C[1] = 0x232f;
			D_407C[2] = 0x632f;
			D_407C[3] = 0x0389;
			D_407C[4] = 0x4389;
			D_407C[5] = 0x6389;
			D_407C[6] = 0x23e3;
			D_407C[7] = 0x43e3;
			for(i = 8; i < 200; i ++)
				D_407C[i] = D_407C[i - 8] + 270;
			/*break: Turbo C makes it a near JMP in its assembly output (-S) and TASM
			  shortened it; MASM 3.0 would pad the short JMP with a NOP*/
			asm jmp short done

		case 9:
			s->seg = 0xb800;
			s->lines = D_407C;
			D_407C[0] = 0;
			D_407C[1] = 0x2000;
			D_407C[2] = 0x4000;
			D_407C[3] = 0x6000;
			for(i = 4; i < 200; i ++)
				D_407C[i] = D_407C[i - 4] + 160;
		break;
		default:
			if(vidMode != 0x13)
				x = -1;
			s->seg = 0xa000;
			s->lines = D_407C;
			for(i = 0; i < 200; i ++)
				D_407C[i] = i * 320;
	}
done:
	if(x != -1)
		(*D_3516)(s, x);
	return 1;
}

/*make s the back buffer, 320x200, allocated*/
/*C_0D28_01BC*/int pascal BufferSurface(struct tSurface *s, int x) {
	D_420E = s;
	s->left = 0;
	s->right = 319;
	s->top = 0;
	s->bottom = 199;
	return AllocSurface(s, x, 0);
}
