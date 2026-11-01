/* flags: -mm -c -O -G */
/*
 * Worlds of Ultima: The Savage Empire - SAVAGE.EXE
 * the OSI library: shape files, the height of a file's shapes
 */
extern int pascal C_0750_0006(char far *shapes);/*SHP_count*/
extern int pascal C_0746_000A(int far *rect, int x, int y, char far *shapes, int index);/*SHP_rect*/

/*meant to be the tallest of the shapes' heights; returns the last shape's height*/
/*C_0731_000C*/int pascal SHP_height(char far *shapes) {
	int n;
	int max;
	int rect[4];
	register int i;
	register int h;

	max = 0;
	n = C_0750_0006(shapes);
	for(i = 0; i < n; i++) {
		C_0746_000A(rect, 0, 0, shapes, i);
		if((h = rect[3] - rect[1] + 1) > max)
			max = h;
	}
	return h;
}
