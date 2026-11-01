/* flags: -mm -c -O -G */
/*
 * Worlds of Ultima: The Savage Empire - SAVAGE.EXE
 * the OSI library: the colors a surface uses
 */
#include <stdlib.h>

struct tSurface;
extern void (pascal *D_2BD6)(struct tSurface *s, char *used);/*driver: used[c] = 1 for each color c in s's window*/

/*list in out the colors (of n) used in s's window: their number (0 without memory)*/
/*C_08A9_000B*/int pascal PAL_colorsUsed(struct tSurface *s, unsigned char *out, int n) {
	register int count;
	register char *used;
	int i;

	count = 0;
	if((used = calloc(n, 1)) == 0)
		return 0;
	(*D_2BD6)(s, used);
	for(i = 0; i < n; i++)
		if(used[i])
			out[count++] = i;
	free(used);
	return count;
}
