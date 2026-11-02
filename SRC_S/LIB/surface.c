/* flags: -mm -c -O -G */
/*
 * SAVAGE.EXE: allocating a drawing surface for the video mode (vidMode: 4 CGA, 7 Hercules,
 * 9 Tandy, 13 EGA, 0x13 VGA/MCGA)
 */
#include <dos.h>
#include <stdlib.h>

/*a drawing surface*/
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

extern unsigned pascal PAL_alloc(unsigned n);/*EGA video memory: n blocks of 256 bytes (0xFF: none)*/
extern void far *MEM_alloc(long size, int flags);/*allocate far memory (or EMS: flags)*/
extern void MEM_free(void far *p);/*free what MEM_alloc allocated*/

/*the bytes of the pixels of s*/
/*C_0D6B_0009*/unsigned pascal SurfaceSize(struct tSurface *s) {
	long size;
	unsigned h, w;
	unsigned bpl;/*bytes per line*/

	h = s->bottom - s->top + 1;
	w = s->right - s->left + 1;
	if(vidMode != 0x13) {
		switch(vidMode) {
			case 13:
				bpl = w >> 3;
				if((s->left & 7) || (w & 7))
					bpl += 2;
			break;
			case 4: case 7:
				bpl = (w >> 2) + 2;
			break;
			case 9:
				bpl = (w >> 1) + 2;
			break;
		}
	}
	switch(vidMode) {
		case 0x13:
			size = (long)h * w;
		break;
		case 13:
			size = ((bpl * h + 0x100) >> 8) << 8;
		break;
		case 4: case 7: case 9:
			size = (long)h * bpl;
		break;
	}
	return size;
}

/*allocate the pixels and the line offsets of s (its window set), in EGA memory in mode 13;
  x: handed to the driver (*D_3516) unless -1 (always -1 but in VGA/MCGA); flags: for the
  far allocation; 0: out of memory*/
/*C_0D6B_00F9*/int pascal AllocSurface(struct tSurface *s, int x, int flags) {
	char huge *p;
	long size;
	unsigned bpl;/*bytes per line*/
	unsigned i, h, w, left, top;
	unsigned ofs;

	h = s->bottom - s->top + 1;
	w = s->right - s->left + 1;
	left = s->left;
	top = s->top;
	if(vidMode != 0x13) {
		switch(vidMode) {
			case 13:
				bpl = w >> 3;
				if((s->left & 7) || (w & 7))
					bpl += 2;
			break;
			case 4: case 7:
				bpl = (w >> 2) + 2;
			break;
			case 9:
				bpl = (w >> 1) + 2;
			break;
		}
	}
	switch(vidMode) {
		case 13:
			i = PAL_alloc((bpl * h + 0x100) >> 8) << 8;
			if(i == 0xffff)
				return 0;
			p = (char huge *)0xa0000000L;
			p += i;
			s->pixels = p;
		break;
		case 4: case 7: case 9:
			size = (long)h * bpl;
			s->pixels = MEM_alloc(size, flags + 2);
			p = s->pixels;
			if(p == 0)
				return 0;
		break;
		default:
			size = (long)h * w;
			s->pixels = MEM_alloc(size, flags + 2);
			p = s->pixels;
			if(p == 0)
				return 0;
	}
	s->seg = FP_SEG(p);
	ofs = FP_OFF(p);
	s->lines = malloc(h * 2 + top * 2);
	if(s->lines == 0) {
		if(vidMode != 0x13)
			MEM_free((void far *)s->pixels);
		return 0;
	}
	for(i = 0; i < h; i ++) {
		switch(vidMode) {
			case 13:
				*(s->lines + i + top) = ofs - (left >> 3);
				ofs += bpl;
			break;
			case 4: case 7:
				*(s->lines + i + top) = ofs - (left >> 2);
				ofs += bpl;
			break;
			case 9:
				*(s->lines + i + top) = ofs - (left >> 1);
				ofs += bpl;
			break;
			default:
				if(vidMode != 0x13)
					x = -1;
				*(s->lines + i + top) = ofs - left;
				ofs += w;
		}
	}
	if(x != -1)
		(*D_3516)(s, x);
	return 1;
}
