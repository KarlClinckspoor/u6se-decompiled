/* flags: -mm -c -O -G */
/*
 * Worlds of Ultima: The Savage Empire - SAVAGE.EXE
 * the OSI library: shape files, one edge of the rectangle a shape covers
 */
extern int pascal C_0746_000A(int far *rect, int x, int y, char far *shapes, int index);/*SHP_rect*/

/*which: 0 left, 1 top, 2 right, 3 bottom*/
/*C_0753_0000*/int pascal SHP_edge(int x, int y, char far *shapes, int index, int which) {
	int rect[4];

	C_0746_000A(rect, x, y, shapes, index);
	return rect[which];
}
