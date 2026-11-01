/* flags: -mm -c -O -G */
/* The conventional heap (see HEAP.C): how much of it is free (GAME.EXE 2E29) */

/* an entry of the heap's block table (the table grows down from the top of
   the heap, D_4074 is its lowest entry) */
struct heapent {
	long addr;	/* +0 linear address of the block */
	long info;	/* +4 length (bits 0-19), 40000000h fixed, 80000000h in use */
};

extern long D_4068;	/* linear address of the heap */
extern long D_4070;	/* its length */
extern long D_4074;	/* linear address of the lowest table entry */

extern struct heapent far * pascal C_0D09_000E(long linear);

/* the total length of the free blocks */
/*C_0BB1_000F*/long C_0BB1_000F(void)
{
	struct heapent far *e;
	long a;
	long total;

	total = 0;
	for (a = D_4068 + D_4070 - 8; a >= D_4074; a -= 8) {
		e = C_0D09_000E(a);
		if (!(e->info & 0x80000000L))
			total += e->info & 0xFFFFFL;
	}
	return total;
}

/* the length of the largest free block */
/*C_0BB1_009E*/long C_0BB1_009E(void)
{
	struct heapent far *e;
	long a;
	long best;

	best = 0;
	for (a = D_4068 + D_4070 - 8; a >= D_4074; a -= 8) {
		e = C_0D09_000E(a);
		if (!(e->info & 0x80000000L) && (e->info & 0xFFFFFL) > best)
			best = e->info & 0xFFFFFL;
	}
	return best;
}
