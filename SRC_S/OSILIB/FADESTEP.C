/* flags: -mm -c -O -G */
/*
 * Worlds of Ultima: The Savage Empire - SAVAGE.EXE
 * the OSI library: palette fades, a step (U.EXE's FADE_step, made pascal; it gives up
 * when it has no memory)
 */
#include <stdlib.h>

/*20C8*/int fadeStart = 1;/*1: the next FADE_step starts a fade*/
/*this module's _BSS (uninitialised: declared only; each 2 bytes)*/
extern int D_3EE0;/*fadeSteps: the largest distance: the number of steps*/
extern int *D_3EE2;/*fadeDist: the distance of each value*/
extern int D_3EE4;/*fadeLeft: the steps left*/
extern int *D_3EE6;/*fadeDir: the direction of each value (+1/-1)*/
extern int *D_3EE8;/*fadeErr: the error term of each value*/

/*move each of the n values of cur one step towards target (all of them arrive in the
  same number of steps, as the points of Bresenham's lines): 0 when they have arrived
  (or there is no memory)*/
/*C_08AF_000A*/int pascal FADE_step(int *cur, int *target, int n) {
	register int i;
	register int d;

	if(fadeStart) {
		D_3EE8 = malloc(n * 2);
		D_3EE2 = malloc(n * 2);
		D_3EE6 = malloc(n * 2);
		if(!D_3EE8 || !D_3EE2 || !D_3EE6) {
			if(D_3EE8)
				free(D_3EE8);
			if(D_3EE2)
				free(D_3EE2);
			if(D_3EE6)
				free(D_3EE6);
			return 0;
		}
		D_3EE0 = 0;
		for(i = 0; i < n; i++) {
			d = cur[i] - target[i];
			if(d < 0) {
				d = -d;
				D_3EE6[i] = 1;
			} else {
				D_3EE6[i] = -1;
			}
			D_3EE2[i] = d;
			if(d > D_3EE0)
				D_3EE0 = d;
		}
		d = D_3EE0 / 2;
		for(i = 0; i < n; i++)
			D_3EE8[i] = d;
		D_3EE4 = D_3EE0;
		fadeStart = 0;
	}
	if(D_3EE4-- == 0) {
		free(D_3EE8);
		free(D_3EE2);
		free(D_3EE6);
		fadeStart = 1;
		return 0;
	}
	for(i = 0; i < n; i++) {
		if((D_3EE8[i] += D_3EE2[i]) > D_3EE0) {
			D_3EE8[i] -= D_3EE0;
			cur[i] += D_3EE6[i];
		}
	}
	return 1;
}
