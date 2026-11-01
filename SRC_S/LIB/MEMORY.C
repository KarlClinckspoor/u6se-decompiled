/* flags: -mm -c -O -G */
/* Memory: the two heaps together, the EMS heap (C_0AA0_*, C_0AB2_*, C_0AE3_*)
   and the conventional one (C_0AF9_*, C_0BB1_*) (GAME.EXE 2E3C).
   The flags of each call say which: 4 the EMS heap first (else the
   conventional one), 8 the other one too if the first fails; 10h is passed
   on to the conventional heap. */

extern int C_0AA0_0002(void);
extern void C_0AA0_00A5(void);
extern void C_0AA0_00FE(void);
extern void far *C_0AB2_000A(long size);
extern void C_0AB2_01B8(void far *p);
extern long C_0AE3_0004(void);
extern long C_0AE3_00A9(void);
extern void C_0AF9_01AD(int flags);
extern int C_0AF9_029D(void);
extern void C_0AF9_03DE(void);
extern void far *C_0AF9_078E(long size, int flags);
extern void C_0AF9_0ADB(void far *p);
extern long C_0BB1_000F(void);
extern long C_0BB1_009E(void);

/* empty the heap(s): everything allocated is freed */
/*C_0BC5_000B*/void C_0BC5_000B(int flags)
{
	if (flags & 4)
		C_0AA0_00A5();
	else
		C_0AF9_01AD(flags);
	if (flags & 8) {
		if (flags & 4)
			C_0AF9_01AD(flags);
		else
			C_0AA0_00A5();
	}
}

/* set up the heap(s) */
/*C_0BC5_0045*/void C_0BC5_0045(int flags)
{
	if (flags & 4)
		C_0AA0_0002();
	else
		C_0AF9_029D();
	if (flags & 8) {
		if (flags & 4)
			C_0AF9_029D();
		else
			C_0AA0_0002();
	}
}

/* give the heap(s) back */
/*C_0BC5_0079*/void C_0BC5_0079(int flags)
{
	if (flags & 4)
		C_0AA0_00FE();
	else
		C_0AF9_03DE();
	if (flags & 8) {
		if (flags & 4)
			C_0AF9_03DE();
		else
			C_0AA0_00FE();
	}
}

/* allocate size bytes; NULL if there is not enough memory */
/*C_0BC5_00AD*/void far *C_0BC5_00AD(long size, int flags)
{
	void far *p;

	if (flags & 4)
		p = C_0AB2_000A(size);
	else
		p = C_0AF9_078E(size, flags);
	if (p == 0 && (flags & 8)) {
		if (flags & 4)
			p = C_0AF9_078E(size, flags);
		else
			p = C_0AB2_000A(size);
	}
	return p;
}

/* free what C_0BC5_00AD allocated (an EMS pointer has segment C000h-FFFFh) */
/*C_0BC5_0126*/void C_0BC5_0126(void far *p)
{
	if ((((unsigned)((unsigned long)p >> 16)) & 0xC000) == 0xC000)
		C_0AB2_01B8(p);
	else
		C_0AF9_0ADB(p);
}

/* the total free memory */
/*C_0BC5_0151*/long C_0BC5_0151(int flags)
{
	long n;

	n = 0;
	if (flags & 4)
		n += C_0AE3_0004();
	else
		n += C_0BB1_000F();
	if (flags & 8) {
		if (flags & 4)
			n += C_0BB1_000F();
		else
			n += C_0AE3_0004();
	}
	return n;
}

/* the largest block that can be allocated */
/*C_0BC5_01A6*/long C_0BC5_01A6(int flags)
{
	long ems;
	long conv;

	ems = conv = 0;
	if ((flags & 4) || (flags & 8))
		ems = C_0AE3_00A9();
	if (!(flags & 4) || (flags & 8))
		conv = C_0BB1_009E();
	return ems > conv ? ems : conv;
}
