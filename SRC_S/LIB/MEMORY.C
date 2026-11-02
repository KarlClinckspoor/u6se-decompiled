/* flags: -mm -c -O -G */
/* Memory: the two heaps together, the EMS heap (C_0AA0_*, C_0AB2_*, C_0AE3_*)
   and the conventional one (C_0AF9_*, C_0BB1_*) (GAME.EXE 2E3C).
   The flags of each call say which: 4 the EMS heap first (else the
   conventional one), 8 the other one too if the first fails; 10h is passed
   on to the conventional heap. */

extern int EMS_init(void);
extern void EMS_reset(void);
extern void EMS_shutdown(void);
extern void far *EMS_alloc(long size);
extern void EMS_free(void far *p);
extern long EMS_totalFree(void);
extern long EMS_largestFree(void);
extern void HEAP_reset(int flags);
extern int HEAP_init(void);
extern void HEAP_shutdown(void);
extern void far *HEAP_alloc(long size, int flags);
extern void HEAP_free(void far *p);
extern long HEAP_totalFree(void);
extern long HEAP_largestFree(void);

/* empty the heap(s): everything allocated is freed */
/*C_0BC5_000B*/void MEM_reset(int flags)
{
	if (flags & 4)
		EMS_reset();
	else
		HEAP_reset(flags);
	if (flags & 8) {
		if (flags & 4)
			HEAP_reset(flags);
		else
			EMS_reset();
	}
}

/* set up the heap(s) */
/*C_0BC5_0045*/void MEM_init(int flags)
{
	if (flags & 4)
		EMS_init();
	else
		HEAP_init();
	if (flags & 8) {
		if (flags & 4)
			HEAP_init();
		else
			EMS_init();
	}
}

/* give the heap(s) back */
/*C_0BC5_0079*/void MEM_shutdown(int flags)
{
	if (flags & 4)
		EMS_shutdown();
	else
		HEAP_shutdown();
	if (flags & 8) {
		if (flags & 4)
			HEAP_shutdown();
		else
			EMS_shutdown();
	}
}

/* allocate size bytes; NULL if there is not enough memory */
/*C_0BC5_00AD*/void far *MEM_alloc(long size, int flags)
{
	void far *p;

	if (flags & 4)
		p = EMS_alloc(size);
	else
		p = HEAP_alloc(size, flags);
	if (p == 0 && (flags & 8)) {
		if (flags & 4)
			p = HEAP_alloc(size, flags);
		else
			p = EMS_alloc(size);
	}
	return p;
}

/* free what MEM_alloc allocated (an EMS pointer has segment C000h-FFFFh) */
/*C_0BC5_0126*/void MEM_free(void far *p)
{
	if ((((unsigned)((unsigned long)p >> 16)) & 0xC000) == 0xC000)
		EMS_free(p);
	else
		HEAP_free(p);
}

/* the total free memory */
/*C_0BC5_0151*/long MEM_totalFree(int flags)
{
	long n;

	n = 0;
	if (flags & 4)
		n += EMS_totalFree();
	else
		n += HEAP_totalFree();
	if (flags & 8) {
		if (flags & 4)
			n += HEAP_totalFree();
		else
			n += EMS_totalFree();
	}
	return n;
}

/* the largest block that can be allocated */
/*C_0BC5_01A6*/long MEM_largestFree(int flags)
{
	long ems;
	long conv;

	ems = conv = 0;
	if ((flags & 4) || (flags & 8))
		ems = EMS_largestFree();
	if (!(flags & 4) || (flags & 8))
		conv = HEAP_largestFree();
	return ems > conv ? ems : conv;
}
