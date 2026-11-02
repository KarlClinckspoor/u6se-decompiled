/* flags: -mm -c -O -G */
/* OSI library: moving in a library entry (GAME.EXE 2EB7) */

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

extern long pascal DOS_seek(int handle, long pos, int whence);

/* move to offset from the start (whence 0), the current position (1) or
   the end (2) of the entry f, not before its start nor after its end
   (unless it is the library's last entry); returns the new position in the
   entry, -1L on an error */
/*C_0C49_000E*/long pascal LIB_seek(struct libfile *f, long offset, int whence)
{
	long end;
	long newpos;

	end = f->start + f->length;
	switch (whence) {
	case 0:
		newpos = f->start;
		break;
	case 1:
		newpos = f->start + f->pos;
		break;
	case 2:
		newpos = end;
		break;
	}
	newpos += offset;
	if (newpos < f->start)
		newpos = f->start;
	if (newpos > end && !f->last)
		newpos = end;
	newpos = DOS_seek(f->handle, newpos, 0);
	if (newpos != -1L) {
		f->pos = newpos - f->start;
		return f->pos;
	}
	return newpos;
}
