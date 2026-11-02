/* flags: -mm -c -O -G */
/*
 * Worlds of Ultima: The Savage Empire - SAVAGE.EXE
 * the OSI library: palette fades, fading a picture in (U.EXE's FADE_in, through the
 * driver; in other modes than 13h the picture is just shown)
 */
#include <stdlib.h>

/*a drawing surface (see OSI.INC)*/
struct tSurface {
	unsigned segment;/*+00 segment of the pixels*/
	unsigned *scanlines;/*+02 offset of each line*/
	int left, top, right, bottom;/*+04 the clipping window (inclusive)*/
};
struct tRGB { int r, g, b; };

extern int vidMode;/*vidMode: the video mode last set*/
extern int (*D_2B34)(struct tSurface *s, int x, int y);/*driver: the color of a pixel*/
extern void (*D_2BBA)(struct tSurface *src, struct tSurface *dst);/*driver: copy a surface's window*/
extern void (pascal *D_2B84)(int index, struct tRGB *c);/*driver: get a palette entry*/
extern void (pascal *D_2B88)(int index, struct tRGB *c);/*driver: set a palette entry*/
extern void far VID_waitRetrace(void);/*VID_waitRetrace*/
extern int pascal PAL_colorsUsed(struct tSurface *s, unsigned char *out, int n);/*PAL_colorsUsed*/
extern int pascal FADE_step(int *cur, int *target, int n);/*FADE_step*/

/*show the picture src on dst, its colors fading in from the color of dst's top left
  pixel*/
/*C_08CA_0006*/void pascal FADE_in(struct tSurface *src, struct tSurface *dst) {
	int count;
	struct tRGB *cur;
	struct tRGB *target;
	struct tRGB rgb;
	register int i;
	register unsigned char *list;

	if(vidMode == 0x13) {
		if((list = calloc(256, 1)) == 0)
			return;
		count = PAL_colorsUsed(src, list, 256);
		cur = calloc(count, sizeof(struct tRGB));
		target = calloc(count, sizeof(struct tRGB));
		if(!cur || !target) {
			free(list);
			if(cur)
				free(cur);
			if(target)
				free(target);
			return;
		}
		(*D_2B84)((*D_2B34)(dst, dst->left, dst->top), &rgb);
		VID_waitRetrace();
		for(i = 0; i < count; i++) {
			(*D_2B84)(list[i], &cur[i]);
			(*D_2B88)(list[i], &rgb);
			target[i] = rgb;
		}
		(*D_2BBA)(src, dst);
		while(FADE_step((int *)target, (int *)cur, count * 3)) {
			VID_waitRetrace();
			for(i = 0; i < count; i++)
				(*D_2B88)(list[i], &target[i]);
		}
		free(target);
		free(cur);
		free(list);
	} else
		(*D_2BBA)(src, dst);
}
