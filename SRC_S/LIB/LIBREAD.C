/* flags: -mm -c -O -G */
/* OSI library: reading from a library entry (GAME.EXE 2EAC) */

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

extern long pascal DOS_read(int handle, long pos, long size, void far *buf);

/* read n bytes (-1: all the rest) of the entry f to buf, no further than
   its end; 1 if done */
/*C_0C3F_0007*/int pascal LIB_read(struct libfile *f, void far *buf, long n)
{
	long cur;
	long end;

	cur = f->start + f->pos;
	end = f->start + f->length;
	if (buf == 0)
		return 0;
	if (cur + n > end || n == -1L)
		n = end - cur;
	if (!DOS_read(f->handle, cur, n, buf))
		return 0;
	f->pos += n;
	return 1;
}
