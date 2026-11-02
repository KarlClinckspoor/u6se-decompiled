/* flags: -mm -c -O -G */
/*
 * Worlds of Ultima: The Savage Empire - SAVAGE.EXE
 * the OSI library: palette fades, fading a picture out (U.EXE's FADE_out, through the
 * driver; only in mode 13h)
 */
#include <stdlib.h>

struct tSurface;
struct tRGB { int r, g, b; };

extern int vidMode;/*vidMode: the video mode last set*/
extern void (pascal *D_2B84)(int index, struct tRGB *c);/*driver: get a palette entry*/
extern void (pascal *D_2B88)(int index, struct tRGB *c);/*driver: set a palette entry*/
extern void far VID_waitRetrace(void);/*VID_waitRetrace*/
extern int pascal PAL_colorsUsed(struct tSurface *s, unsigned char *out, int n);/*PAL_colorsUsed*/
extern int pascal FADE_step(int *cur, int *target, int n);/*FADE_step*/

/*fade the colors used in s's window to the palette's color "color", waiting delay
  retraces between the steps*/
/*C_0896_0004*/void pascal FADE_out(struct tSurface *s, int color, int delay) {
	int count;
	struct tRGB *cur;
	struct tRGB *target;
	struct tRGB rgb;
	register int i;
	register unsigned char *list;

	if(vidMode == 0x13) {
		if((list = calloc(256, 1)) == 0)
			return;
		count = PAL_colorsUsed(s, list, 256);
		if((cur = calloc(count, sizeof(struct tRGB))) == 0) {
			free(list);
			return;
		}
		if((target = calloc(count, sizeof(struct tRGB))) == 0) {
			free(list);
			free(cur);
			return;
		}
		(*D_2B84)(color, &rgb);
		for(i = 0; i < count; i++) {
			(*D_2B84)(list[i], &cur[i]);
			target[i] = rgb;
		}
		while(FADE_step((int *)cur, (int *)target, count * 3)) {
			if(delay)
				for(i = delay; i--; )
					VID_waitRetrace();
			for(i = 0; i < count; i++)
				(*D_2B88)(list[i], &cur[i]);
		}
		free(target);
		free(cur);
		free(list);
	}
}
