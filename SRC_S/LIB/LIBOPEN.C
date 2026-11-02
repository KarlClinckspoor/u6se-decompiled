/* flags: -mm -c -O -G */
/* OSI library: opening one entry of a library file (GAME.EXE 2E94). A
   library starts with its length (a long), then the offset of each entry (a
   long: the high byte is the entry's type, 1: LZW compressed); the first
   offset tells how many there are. */

/* an open library entry */
#pragma inline	/*compiled through the assembler: the .EXE has its relocations in ascending order, as an assembler writes them*/
struct libfile {
	int handle;	/* +00 */
	int last;	/* +02 1: the last entry (it ends at the end of the file) */
	int count;	/* +04 entries in the library */
	int type;	/* +06 1: LZW compressed (see LZC.C) */
	long start;	/* +08 offset of the entry in the file */
	long pos;	/* +0C position in the entry */
	long length;	/* +10 */
};

extern int osiError;	/* error code of the file routines */

extern int pascal OSI_open(char far *name);
extern void pascal OSI_close(int handle);
extern long pascal OSI_read(int handle, long pos, long size, void far *buf);
extern long pascal OSI_seek(int handle, long pos, int whence);

/* open entry index of the library name into f; 1 if done (0: osiError = 3 if
   there is no such entry) */
/*C_0C27_000A*/int pascal LIB_open(char far *name, int index, struct libfile *f)
{
	int count;
	int type;
	long filesize;
	long first;
	long pos;
	long end;
	long start;
	int handle;

	handle = OSI_open(name);
	if (handle == -1)
		goto fail;
	if (!OSI_read(handle, 0L, 4L, (void far *)&filesize))
		goto fail;
	if (!OSI_read(handle, 4L, 4L, (void far *)&first))
		goto fail;
	count = first / 4 - 1;
	if (index >= count) {
		osiError = 3;
		goto fail;
	}
	pos = (index + 1) * 4U;
	if (!OSI_read(handle, pos, 4L, (void far *)&start))
		goto fail;
	/* the type is the offset's high byte (written with the pseudo-registers:
	   xor ah,ah, and AH stored back as the 0) */
	_AL = ((unsigned char *)&start)[3];
	_AH ^= _AH;
	type = _AX;
	((unsigned char *)&start)[3] = _AH;
	f->last = 0;
	if (count - 1 == index) {
		end = filesize;
		f->last = 1;
	} else {
		pos += 4;
		if (!OSI_read(handle, pos, 4L, (void far *)&end))
			goto fail;
		((unsigned char *)&end)[3] = 0;
	}
	filesize = end - start;
	if (!OSI_seek(handle, start, 0))
		goto fail;
	f->handle = handle;
	f->count = count;
	f->type = type;
	f->start = start;
	f->pos = 0;
	f->length = filesize;
	return 1;
fail:
	OSI_close(handle);
	return 0;
}

/* close the library entry */
/*C_0C27_0176*/void pascal LIB_close(struct libfile *f)
{
	OSI_close(f->handle);
}
