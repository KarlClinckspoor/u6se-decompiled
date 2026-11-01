/* flags: -mm -c -O -G */
/* OSI library: loading a library entry, LZW compressed (type 1, see LZW.ASM)
   or not (GAME.EXE 2EC5). A compressed entry starts with the length of its
   data (a long). The decompression needs a work area of 3020h bytes: the
   dictionary (3000h) and an input buffer (400h, which may overlap the end of
   it); D_2412, if set, is one to use. */

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

extern int D_240C;		/* error code of the file routines */
extern int D_23E2;		/* LZW.ASM: stop when the input runs out */
extern int D_23EA;		/* LZW.ASM: the file */
extern unsigned D_23EE;		/* LZW.ASM: segment of the dictionary */
extern char far *D_23F5;	/* LZW.ASM: output */
extern char far *D_23F9;	/* LZW.ASM: input buffer */
extern unsigned D_23FD;		/* LZW.ASM: its length */
extern int D_2401;		/* LZW.ASM: bit offset */
extern void far *D_4078;	/* the data loaded (uninitialised) */

extern void far *C_0AD2_0024(void far *p);
extern void C_0BE6_00A2(void);
extern void far *C_0BC5_00AD(long size, int flags);
extern void C_0BC5_0126(void far *p);
extern int pascal C_0C27_000A(char far *name, int index, struct libfile *f);
extern void pascal C_0C27_0176(struct libfile *f);
extern int pascal C_0C3F_0007(struct libfile *f, void far *buf, long n);
extern long pascal C_0C49_000E(struct libfile *f, long offset, int whence);
extern char far * pascal C_0D0E_0004(char far *p);

/*2412*/char far *D_2412 = 0;	/* a work area to use (3020h bytes) */
/*2416*/unsigned D_2416 = 0;	/* the length of the input buffer in it (0: 400h) */

/* decompress the open entry f (type 1) to out, or, if out is NULL, to memory
   allocated with flags memflags (C_0BC5_00AD); work: a work area or NULL.
   Returns the data, NULL on an error (D_240C: 6 not compressed, 1 and 2 no
   memory for the work area, 5 seek error, 4 no memory for the data) */
/*C_0C58_0005*/void far * pascal C_0C58_0005(register struct libfile *f, void far *out, register int memflags, char far *work)
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
		D_240C = 6;
		return 0;
	}
	if (work == 0 && D_2412 != 0) {
		work = D_2412;
		D_2416 = 0;
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
			buf1 = C_0BC5_00AD(0x3000L, 0x22);
			if (buf1 == 0) {
				D_240C = 1;
				return 0;
			}
			buf2 = C_0BC5_00AD(0x400L, 0x22);
			if (buf2 == 0) {
				C_0BC5_0126(buf1);
				D_240C = 2;
				return 0;
			}
		}
		tbl = C_0D0E_0004(buf1);
		D_23F9 = C_0D0E_0004(buf2);
		D_23FD = 0x400;
	} else {
		tbl = C_0D0E_0004(work);
		D_23FD = 0x400;
		len = 0x3020;
		D_23F9 = work + len;
		D_23F9 = C_0D0E_0004(D_23F9);
		if (D_2416)
			D_23FD = D_2416;
	}
	D_23EA = f->handle;
	D_23E2 = 0;
	D_2401 = 0;
	D_23EE = FP_SEG(tbl);
	if (FP_OFF(tbl))
		D_23EE++;
	if (C_0C49_000E(f, 0L, 0) == -1L) {
		D_240C = 5;
		return 0;
	}
	if (out == 0) {
		if (C_0C3F_0007(f, (void far *)&len, 4L)) {
			D_4078 = C_0BC5_00AD(len, memflags);
			if (D_4078 == 0)
				D_240C = 4;
		}
	} else {
		if (C_0C49_000E(f, 4L, 0) == -1L) {
			D_240C = 5;
			return 0;
		}
		D_4078 = out;
	}
	result = D_4078;
	if (!C_0C3F_0007(f, D_23F9, D_23FD))
		result = 0;
	D_23F5 = C_0AD2_0024(result);
	if (result)
		C_0BE6_00A2();
	if (work == 0) {
		if (allocated) {
			C_0BC5_0126(buf2);
			C_0BC5_0126(buf1);
		} else {
			free((char *)buf2);
			free((char *)buf1);
		}
	}
	return result;
}

/* load entry index of the library name to out, or, if out is NULL, to
   memory allocated with flags memflags; work: a work area for an LZW
   compressed entry, or NULL. Returns the data, NULL on an error (D_240C: 8
   the entry is empty, 4 no memory, or as C_0C27_000A, C_0C58_0005) */
/*C_0C58_0305*/void far * pascal C_0C58_0305(char far *name, int index, void far *out, int memflags, char far *work)
{
	void far *p;
	struct libfile f;

	p = 0;
	if (C_0C27_000A(name, index, &f)) {
		switch (f.type) {
		case 1:
			p = C_0C58_0005(&f, out, memflags, work);
			break;
		default:
			if (f.length == 0) {
				p = 0;
				D_240C = 8;
				break;
			}
			p = out;
			if (p == 0) {
				p = D_4078 = C_0BC5_00AD(f.length, memflags);
				if (p == 0)
					D_240C = 4;
			}
			if (p != 0) {
				if (!C_0C3F_0007(&f, p, f.length))
					p = 0;
			}
		}
		C_0C27_0176(&f);
	}
	return p;
}
