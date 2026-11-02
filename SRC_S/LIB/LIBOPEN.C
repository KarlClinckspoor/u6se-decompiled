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

extern int D_240C;	/* error code of the file routines */

extern int pascal C_0C02_0000(char far *name);
extern void pascal C_0C02_002D(int handle);
extern long pascal C_0C06_000C(int handle, long pos, long size, void far *buf);
extern long pascal C_0C1C_0002(int handle, long pos, int whence);

/* open entry index of the library name into f; 1 if done (0: D_240C = 3 if
   there is no such entry) */
/*C_0C27_000A*/int pascal C_0C27_000A(char far *name, int index, struct libfile *f)
{
	int count;
	int type;
	long filesize;
	long first;
	long pos;
	long end;
	long start;
	int handle;

	handle = C_0C02_0000(name);
	if (handle == -1)
		goto fail;
	if (!C_0C06_000C(handle, 0L, 4L, (void far *)&filesize))
		goto fail;
	if (!C_0C06_000C(handle, 4L, 4L, (void far *)&first))
		goto fail;
	count = first / 4 - 1;
	if (index >= count) {
		D_240C = 3;
		goto fail;
	}
	pos = (index + 1) * 4U;
	if (!C_0C06_000C(handle, pos, 4L, (void far *)&start))
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
		if (!C_0C06_000C(handle, pos, 4L, (void far *)&end))
			goto fail;
		((unsigned char *)&end)[3] = 0;
	}
	filesize = end - start;
	if (!C_0C1C_0002(handle, start, 0))
		goto fail;
	f->handle = handle;
	f->count = count;
	f->type = type;
	f->start = start;
	f->pos = 0;
	f->length = filesize;
	return 1;
fail:
	C_0C02_002D(handle);
	return 0;
}

/* close the library entry */
/*C_0C27_0176*/void pascal C_0C27_0176(struct libfile *f)
{
	C_0C02_002D(f->handle);
}
