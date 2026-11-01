/* flags: -mm -c -O -G */
/* OSI library: the length of a library entry's data (GAME.EXE 2E8C) */

/* an open library entry (LIBOPEN.C) */
struct libfile {
	int handle;	/* +00 */
	int last;	/* +02 1: the last entry (it ends at the end of the file) */
	int count;	/* +04 entries in the library */
	int type;	/* +06 1: LZW compressed (see LZC.C) */
	long start;	/* +08 offset of the entry in the file */
	long pos;	/* +0C position in the entry */
	long length;	/* +10 */
};

extern int D_240C;	/* error code of the file routines */

extern int pascal C_0C27_000A(char far *name, int index, struct libfile *f);
extern void pascal C_0C27_0176(struct libfile *f);
extern int pascal C_0C3F_0007(struct libfile *f, void far *buf, long n);

/* the length of entry index of the library name, uncompressed (an LZW
   compressed entry starts with it); -1L on an error */
/*C_0C1F_000A*/long pascal C_0C1F_000A(char far *name, int index)
{
	long size;
	struct libfile f;

	size = -1;
	if (C_0C27_000A(name, index, &f)) {
		switch (f.type) {
		case 1:
			C_0C3F_0007(&f, (void far *)&size, 4L);
			if (D_240C)
				size = -1;
			break;
		case 2:
			size = f.length;
			break;
		default:
			size = f.length;
		}
		C_0C27_0176(&f);
	}
	return size;
}
