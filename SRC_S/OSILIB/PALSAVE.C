/* flags: -mm -c -O -G */
/*
 * Worlds of Ultima: The Savage Empire - SAVAGE.EXE
 * the OSI library: save and restore the whole palette
 */
struct tRGB { int r, g, b; };

extern void (pascal *D_2B84)(int index, struct tRGB *c);/*driver: get a palette entry*/
extern void (pascal *D_2B88)(int index, struct tRGB *c);/*driver: set a palette entry*/
extern struct tRGB D_36E0[256];/*the saved palette*/
extern void far C_07D7_0008(void);/*VID_waitRetrace*/

/*save the 256 colors (waiting for the retrace every 16)*/
/*C_0737_0001*/PAL_save() {
	register int i;

	for(i = 0; i < 256; i++) {
		if(!(i & 15))
			C_07D7_0008();
		(*D_2B84)(i, &D_36E0[i]);
	}
}

/*and restore them*/
/*C_0737_002E*/PAL_restore() {
	register int i;

	for(i = 0; i < 256; i++) {
		if(!(i & 15))
			C_07D7_0008();
		(*D_2B88)(i, &D_36E0[i]);
	}
}
