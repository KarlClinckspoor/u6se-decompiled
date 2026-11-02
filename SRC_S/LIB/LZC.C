/* flags: -mm -c -O -G */
/* OSI library: loading a library entry, LZW compressed (type 1, see LZW.ASM)
   or not (GAME.EXE 2EC5). A compressed entry starts with the length of its
   data (a long). The decompression needs a work area of 3020h bytes: the
   dictionary (3000h) and an input buffer (400h, which may overlap the end of
   it); lzcWorkArea, if set, is one to use. */

#include <dos.h>
#include <stdlib.h>

/* an open library entry (LIBOPEN.C) */
struct libfile {
	int handle;	/* +00 */
	int last;	/* +02 1: the last entry (it ends at the end of the file) */
	int count;	/* +04 entries in the library */
	int type;	/* +06 1: LZW compressed */
	long start;	/* +08 offset of the entry in the file */
	long pos;	/* +0C position in the entry */
	long length;	/* +10 */
};

extern int osiError;		/* error code of the file routines */
extern int lzwNoRefill;		/* LZW.ASM: stop when the input runs out */
extern int lzwFile;		/* LZW.ASM: the file */
extern unsigned lzwDictSeg;		/* LZW.ASM: segment of the dictionary */
extern char far *lzwOut;	/* LZW.ASM: output */
extern char far *lzwIn;	/* LZW.ASM: input buffer */
extern unsigned lzwInSize;		/* LZW.ASM: its length */
extern int lzwBitPos;		/* LZW.ASM: bit offset */
extern void far *D_4078;	/* the data loaded (uninitialised) */

extern void far *EMS_map(void far *p);
extern void LZW_decompress(void);
extern void far *MEM_alloc(long size, int flags);
extern void MEM_free(void far *p);
extern int pascal LIB_open(char far *name, int index, struct libfile *f);
extern void pascal LIB_close(struct libfile *f);
extern int pascal LIB_read(struct libfile *f, void far *buf, long n);
extern long pascal LIB_seek(struct libfile *f, long offset, int whence);
extern char far * pascal NormalizePtr(char far *p);

/*2412*/char far *lzcWorkArea = 0;	/* a work area to use (3020h bytes) */
/*2416*/unsigned lzcWorkInSize = 0;	/* the length of the input buffer in it (0: 400h) */

/* decompress the open entry f (type 1) to out, or, if out is NULL, to memory
   allocated with flags memflags (MEM_alloc); work: a work area or NULL.
   Returns the data, NULL on an error (osiError: 6 not compressed, 1 and 2 no
   memory for the work area, 5 seek error, 4 no memory for the data) */
/*C_0C58_0005*/void far * pascal LZC_decompress(register struct libfile *f, void far *out, register int memflags, char far *work)
{
	char far *tbl;
	char far *buf1;
	char far *buf2;
	char allocated;
	long len;
	void far *result;
	int unused;

	allocated = 0;
	result = 0;
	unused = 0;
	D_4078 = 0;
	if (f->type != 1) {
		osiError = 6;
		return 0;
	}
	if (work == 0 && lzcWorkArea != 0) {
		work = lzcWorkArea;
		lzcWorkInSize = 0;
	}
	if (work == 0) {
		/* near memory first (tested and freed as near pointers) */
		buf1 = (char *)malloc(0x3020);
		buf2 = (char *)malloc(0x410);
		if (FP_OFF(buf1) == 0 || FP_OFF(buf2) == 0) {
			if (FP_OFF(buf1) != 0)
				free((char *)buf1);
			if (FP_OFF(buf2) != 0)
				free((char *)buf2);
			allocated = 1;
			buf1 = MEM_alloc(0x3000L, 0x22);
			if (buf1 == 0) {
				osiError = 1;
				return 0;
			}
			buf2 = MEM_alloc(0x400L, 0x22);
			if (buf2 == 0) {
				MEM_free(buf1);
				osiError = 2;
				return 0;
			}
		}
		tbl = NormalizePtr(buf1);
		lzwIn = NormalizePtr(buf2);
		lzwInSize = 0x400;
	} else {
		tbl = NormalizePtr(work);
		lzwInSize = 0x400;
		len = 0x3020;
		lzwIn = work + len;
		lzwIn = NormalizePtr(lzwIn);
		if (lzcWorkInSize)
			lzwInSize = lzcWorkInSize;
	}
	lzwFile = f->handle;
	lzwNoRefill = 0;
	lzwBitPos = 0;
	lzwDictSeg = FP_SEG(tbl);
	if (FP_OFF(tbl))
		lzwDictSeg++;
	if (LIB_seek(f, 0L, 0) == -1L) {
		osiError = 5;
		return 0;
	}
	if (out == 0) {
		if (LIB_read(f, (void far *)&len, 4L)) {
			D_4078 = MEM_alloc(len, memflags);
			if (D_4078 == 0)
				osiError = 4;
		}
	} else {
		if (LIB_seek(f, 4L, 0) == -1L) {
			osiError = 5;
			return 0;
		}
		D_4078 = out;
	}
	result = D_4078;
	if (!LIB_read(f, lzwIn, lzwInSize))
		result = 0;
	lzwOut = EMS_map(result);
	if (result)
		LZW_decompress();
	if (work == 0) {
		if (allocated) {
			MEM_free(buf2);
			MEM_free(buf1);
		} else {
			free((char *)buf2);
			free((char *)buf1);
		}
	}
	return result;
}

/* load entry index of the library name to out, or, if out is NULL, to
   memory allocated with flags memflags; work: a work area for an LZW
   compressed entry, or NULL. Returns the data, NULL on an error (osiError: 8
   the entry is empty, 4 no memory, or as LIB_open, LZC_decompress) */
/*C_0C58_0305*/void far * pascal LZC_load(char far *name, int index, void far *out, int memflags, char far *work)
{
	void far *p;
	struct libfile f;

	p = 0;
	if (LIB_open(name, index, &f)) {
		switch (f.type) {
		case 1:
			p = LZC_decompress(&f, out, memflags, work);
			break;
		default:
			if (f.length == 0) {
				p = 0;
				osiError = 8;
				break;
			}
			p = out;
			if (p == 0) {
				p = D_4078 = MEM_alloc(f.length, memflags);
				if (p == 0)
					osiError = 4;
			}
			if (p != 0) {
				if (!LIB_read(&f, p, f.length))
					p = 0;
			}
		}
		LIB_close(&f);
	}
	return p;
}
