/* flags: -mm -c -O -G */
/* The conventional heap (GAME.EXE 2D70): one block of DOS memory, taken
   with farmalloc() at the start, given out in blocks. Addresses are linear
   (longs; C_0D09_000E makes a far pointer of one, C_0D0C_0002 the other
   way). The block table is at the top of the heap and grows down: one
   8-byte entry per block, in the order of the blocks (the lowest entry,
   D_4074, is the lowest block); D_23DC entries are kept for it.
   On a machine with 624 KB (BIOS: 270h KB) the heap stops at 98000h and
   what was above it is moved up to 9C000h (and back at the end). */

#include <alloc.h>

struct heapent {
	long addr;	/* +0 linear address of the block */
	long info;	/* +4 length (bits 0-19), 40000000h fixed, 80000000h in use */
};

extern long D_4068;		/* linear address of the heap */
extern void far *D_406C;	/* the heap, as farmalloc() gave it */
extern long D_4070;		/* its length */
extern long D_4074;		/* linear address of the lowest table entry */

extern struct heapent far * pascal C_0D09_000E(long linear);
extern long pascal C_0D0C_0002(void far *p);
extern void pascal C_0CAC_000B(void far *dest, void far *src, unsigned n);

/*23DA*/int D_23DA = 0;		/* 1: the heap is set up */
/*23DC*/int D_23DC = 0x80;	/* table entries kept at the top */
/*23DE*/long D_23DE = 0;	/* bytes moved from below 9C000h (624 KB machines) */

/* merge the free block of entry a with the next one (entry a + 8) if that is
   free too; the entries below a move up. 1 if merged */
/*C_0AF9_0008*/int C_0AF9_0008(long a)
{
	struct heapent far *e;

	e = C_0D09_000E(a);
	if (!(e->info & 0x80000000L) && !(e[1].info & 0x80000000L)
	    && e->addr + (e->info & 0xFFFFFL) == e[1].addr) {
		e[1].addr = e->addr;
		e[1].info += e->info & 0xFFFFFL;
		while (a > D_4074) {
			a -= 8;
			e = C_0D09_000E(a);
			e[1].addr = e->addr;
			e[1].info = e->info;
		}
		D_4074 += 8;
		return 1;
	}
	return 0;
}

/* free the block of entry a and merge it with free neighbours; returns its
   entry's address now */
/*C_0AF9_010A*/long C_0AF9_010A(long a)
{
	struct heapent far *e;

	e = C_0D09_000E(a);
	e->info = e->info & 0x7FFFFFFFL;
	if (a + 8 < D_4068 + D_4070) {
		if (C_0AF9_0008(a))
			a += 8;
	}
	if (a > D_4074)
		C_0AF9_0008(a - 8);
	return a;
}

/* empty the heap: with flags 10h, one free block over all of it; otherwise
   free every block that is not fixed */
/*C_0AF9_01AD*/void C_0AF9_01AD(int flags)
{
	long a;
	struct heapent far *e;

	if (D_23DA) {
		if (flags & 0x10) {
			D_4074 = D_4068 + D_4070 - 8;
			e = C_0D09_000E(D_4074);
			e->addr = D_4068;
			e->info = D_4070 - D_23DC * 8;
		} else {
			for (a = D_4068 + D_4070 - 8; a >= D_4074; a -= 8) {
				e = C_0D09_000E(a);
				if (!(e->info & 0x40000000L))
					a = C_0AF9_010A(a);
			}
		}
	}
}

/* set up the heap: all the DOS memory left; returns D_23DA (1 if done) */
/*C_0AF9_029D*/int C_0AF9_029D(void)
{
	if (!D_23DA) {
		D_4070 = farcoreleft();
		D_406C = 0;
		if (D_23DC * 8 < D_4070)
			D_406C = farmalloc(D_4070);
		if (D_406C) {
			D_23DA++;
			D_4068 = C_0D0C_0002(D_406C);
			if (*(int far *)0x00400013L == 0x270) {
				D_23DE = 0x9C000L - D_4068 - D_4070;
				D_4070 = 0x98000L - D_4068 - D_23DE;
				if (D_23DC * 8 < D_4070) {
					C_0CAC_000B(C_0D09_000E(D_4068 + D_4070),
						C_0D09_000E(0x9C000L - D_23DE), D_23DE);
				} else {
					farfree(D_406C);
					D_23DA = 0;
				}
			}
			if (D_23DA)
				C_0AF9_01AD(0x10);
		}
	}
	return D_23DA;
}

/* give the heap back (moving back what was moved) */
/*C_0AF9_03DE*/void C_0AF9_03DE(void)
{
	if (D_23DE)
		C_0CAC_000B(C_0D09_000E(0x9C000L - D_23DE),
			C_0D09_000E(D_4068 + D_4070), D_23DE);
	if (D_23DA)
		farfree(D_406C);
	D_23DA = 0;
}

/* allocate size bytes at the top of the heap (the last free block that is
   large enough); see C_0AF9_078E */
/*C_0AF9_0441*/void far *C_0AF9_0441(long size, int flags)
{
	long a;
	long result;
	long len;
	long b;
	long flagbits;
	struct heapent far *e;
	struct heapent far *e2;

	if (size <= 0)
		return 0;
	if ((flags & 3) == 1)
		size += 1;
	else if ((flags & 3) == 2)
		size += 15;
	flagbits = 0x80000000L;
	if (flags & 0x10)
		flagbits |= 0x40000000L;
	result = 0;
	for (a = D_4068 + D_4070 - 8; a >= D_4074; a -= 8) {
		e = C_0D09_000E(a);
		len = e->info & 0xFFFFFL;
		if (!(e->info & 0x80000000L) && size <= len) {
			if (size < len) {
				if (D_4068 + D_4070 - D_23DC * 8 >= D_4074) {
					e2 = C_0D09_000E(D_4068 + D_4070 - 8);
					if (!(e2->info & 0x80000000L)
					    && e2->addr + (e2->info & 0xFFFFFL) == D_4074
					    && (e2->info & 0xFFFFFL) > 8) {
						e2->info -= 8;
						D_23DC++;
					}
				}
				if (D_4068 + D_4070 - D_23DC * 8 < D_4074) {
					D_4074 -= 8;
					for (b = D_4074; b < a; b += 8) {
						e2 = C_0D09_000E(b);
						e2->addr = e2[1].addr;
						e2->info = e2[1].info;
					}
					a -= 8;
					e = C_0D09_000E(a);
					e->info -= size;
					e[1].addr += e->info & 0xFFFFFFL;
					e[1].info = size + flagbits;
					result = e[1].addr;
					break;
				}
			} else {
				e->info |= flagbits;
				result = e->addr;
				break;
			}
		}
	}
	if (result) {
		if ((flags & 3) == 1)
			result = (result + 1) & 0xFFFFFFFEL;
		else if ((flags & 3) == 2)
			result = (result + 15) & 0xFFFFFFF0L;
	}
	return C_0D09_000E(result);
}

/* allocate size bytes (the first free block that is large enough); flags:
   1 word aligned, 2 paragraph aligned, 10h fixed (C_0AF9_01AD leaves it),
   20h at the top of the heap. NULL if there is no room */
/*C_0AF9_078E*/void far *C_0AF9_078E(long size, int flags)
{
	long a;
	long result;
	long len;
	long b;
	long flagbits;
	struct heapent far *e;
	struct heapent far *e2;

	if (flags & 0x20)
		return C_0AF9_0441(size, flags);
	if (size <= 0)
		return 0;
	if ((flags & 3) == 1)
		size += 1;
	else if ((flags & 3) == 2)
		size += 15;
	flagbits = 0x80000000L;
	if (flags & 0x10)
		flagbits |= 0x40000000L;
	result = 0;
	for (a = D_4074; a < D_4068 + D_4070; a += 8) {
		e = C_0D09_000E(a);
		len = e->info & 0xFFFFFL;
		if (!(e->info & 0x80000000L) && size <= len) {
			if (size < len) {
				if (D_4068 + D_4070 - D_23DC * 8 >= D_4074) {
					e2 = C_0D09_000E(D_4068 + D_4070 - 8);
					if (!(e2->info & 0x80000000L)
					    && e2->addr + (e2->info & 0xFFFFFL) == D_4074
					    && (e2->info & 0xFFFFFL) > 8) {
						e2->info -= 8;
						D_23DC++;
					}
				}
				if (D_4068 + D_4070 - D_23DC * 8 < D_4074) {
					D_4074 -= 8;
					for (b = D_4074; b < a; b += 8) {
						e2 = C_0D09_000E(b);
						e2->addr = e2[1].addr;
						e2->info = e2[1].info;
					}
					a -= 8;
					e = C_0D09_000E(a);
					e->info = size + flagbits;
					e[1].addr += size;
					e[1].info -= size;
					result = e->addr;
					break;
				}
			} else {
				e->info |= flagbits;
				result = e->addr;
				break;
			}
		}
	}
	if (result) {
		if ((flags & 3) == 1)
			result = (result + 1) & 0xFFFFFFFEL;
		else if ((flags & 3) == 2)
			result = (result + 15) & 0xFFFFFFF0L;
	}
	return C_0D09_000E(result);
}

/* free the block holding p */
/*C_0AF9_0ADB*/void C_0AF9_0ADB(void far *p)
{
	long a;
	long lin;
	struct heapent far *e;

	lin = C_0D0C_0002(p);
	for (a = D_4068 + D_4070 - 8; a >= D_4074; a -= 8) {
		e = C_0D09_000E(a);
		if (e->addr <= lin && e->addr + (e->info & 0xFFFFFL) > lin) {
			C_0AF9_010A(a);
			break;
		}
	}
}
