/* flags: -mm -c -O -G */
/*
 * SAVAGE.EXE: loading the drivers TM.DRV and SUPERTM.DRV and taking their entry points
 * (far pointers made of the driver's segment and the offsets in its header)
 */

#include <dos.h>

typedef void (far *tDrvFunc)();

/*the entry points: driver segment and an offset of its header*/
#define ENTRY(seg, off) ((tDrvFunc)(((long)(seg) << 16) | (off)))

extern char tmDrvName[];/*"TM.DRV"*/
extern char superTmDrvName[];/*"SUPERTM.DRV"*/

/*C_0C58_0305: load a file (into buf, or allocated when buf is 0)*/
extern void far * pascal C_0C58_0305(char far *name, int kind, void far *buf, int flags, long x);
extern void far *C_0AD2_0024(void far *p);/*map an EMS pointer: handed to the drivers (header +2E / +0C)*/

extern unsigned far *D_201A;/*TM.DRV: its header (an offset per entry point)*/
extern tDrvFunc D_2020;/*TM.DRV entry points, by header offset: +04*/
extern tDrvFunc D_2024;/*+06*/
extern tDrvFunc drvPutChar;/*+02*/
extern tDrvFunc D_2B34;/*+1C*/
extern tDrvFunc D_2B3A;/*+12*/
extern tDrvFunc D_2B62;/*+1E*/
extern tDrvFunc D_2B84;/*+0A*/
extern tDrvFunc D_2B88;/*+0C*/
extern tDrvFunc D_2B8C;/*+1A*/
extern tDrvFunc D_2B90;/*+24*/
extern tDrvFunc D_2BA8;/*+28*/
extern tDrvFunc D_2BB2;/*+2A*/
extern tDrvFunc D_2BBA;/*+16*/
extern tDrvFunc D_2BBE;/*+32*/
extern tDrvFunc D_2BC6;/*+0E*/
extern tDrvFunc D_2BD6;/*+08*/
extern tDrvFunc D_34EE;/*+26*/
extern tDrvFunc D_34FE;/*+2C*/
extern tDrvFunc D_3516;/*+18*/
extern tDrvFunc D_352C;/*+22*/
extern tDrvFunc D_3530;/*+10*/
extern tDrvFunc D_3534;/*+14*/
extern tDrvFunc D_353A;/*+20*/

extern unsigned far *D_34F2;/*SUPERTM.DRV: its header*/
extern tDrvFunc D_2B30;/*SUPERTM.DRV entry points: +08*/
extern tDrvFunc D_2B6A;/*+06*/
extern tDrvFunc D_2B94;/*+0A*/
extern tDrvFunc D_2BA4;/*+04*/
extern tDrvFunc D_2BDC;/*+02*/

/*load TM.DRV (kind 4 loads kind 2's) and take its entry points; 0: not loaded*/
/*C_0D46_000C*/int LoadTM(int kind) {
	register int k;
	register int seg;

	if(kind == 4)
		k = 2;
	else
		k = kind;
	D_201A = C_0C58_0305(tmDrvName, k, 0L, 0x12, 0L);
	if(D_201A) {
		seg = FP_SEG(D_201A);
		drvPutChar = ENTRY(seg, D_201A[0x02 / 2]);
		D_2020 = ENTRY(seg, D_201A[0x04 / 2]);
		D_2024 = ENTRY(seg, D_201A[0x06 / 2]);
		D_2BD6 = ENTRY(seg, D_201A[0x08 / 2]);
		D_2B84 = ENTRY(seg, D_201A[0x0A / 2]);
		D_2B88 = ENTRY(seg, D_201A[0x0C / 2]);
		D_2BC6 = ENTRY(seg, D_201A[0x0E / 2]);
		D_3530 = ENTRY(seg, D_201A[0x10 / 2]);
		D_2B3A = ENTRY(seg, D_201A[0x12 / 2]);
		D_3534 = ENTRY(seg, D_201A[0x14 / 2]);
		D_2BBA = ENTRY(seg, D_201A[0x16 / 2]);
		D_3516 = ENTRY(seg, D_201A[0x18 / 2]);
		D_2B8C = ENTRY(seg, D_201A[0x1A / 2]);
		D_2B34 = ENTRY(seg, D_201A[0x1C / 2]);
		D_2B62 = ENTRY(seg, D_201A[0x1E / 2]);
		D_353A = ENTRY(seg, D_201A[0x20 / 2]);
		D_352C = ENTRY(seg, D_201A[0x22 / 2]);
		D_2B90 = ENTRY(seg, D_201A[0x24 / 2]);
		D_34EE = ENTRY(seg, D_201A[0x26 / 2]);
		D_2BA8 = ENTRY(seg, D_201A[0x28 / 2]);
		D_2BB2 = ENTRY(seg, D_201A[0x2A / 2]);
		D_34FE = ENTRY(seg, D_201A[0x2C / 2]);
		D_2BBE = ENTRY(seg, D_201A[0x32 / 2]);
		*(tDrvFunc far *)(D_201A + 0x2E / 2) = (tDrvFunc)C_0AD2_0024;
		return 1;
	}
	return 0;
}

/*load TM.DRV, then SUPERTM.DRV, and take its entry points; 0: not loaded*/
/*C_0D46_01AA*/int LoadSuperTM(int kind) {
	register int k;
	register int seg;

	if(LoadTM(kind)) {
		if(kind == 4)
			k = 2;
		else
			k = kind;
		D_34F2 = C_0C58_0305(superTmDrvName, k, 0L, 0x12, 0L);
		if(D_34F2) {
			seg = FP_SEG(D_34F2);
			D_2BDC = ENTRY(seg, D_34F2[0x02 / 2]);
			D_2BA4 = ENTRY(seg, D_34F2[0x04 / 2]);
			D_2B6A = ENTRY(seg, D_34F2[0x06 / 2]);
			D_2B30 = ENTRY(seg, D_34F2[0x08 / 2]);
			D_2B94 = ENTRY(seg, D_34F2[0x0A / 2]);
			*(tDrvFunc far *)(D_34F2 + 0x0C / 2) = (tDrvFunc)C_0AD2_0024;
			return 1;
		}
	}
	return 0;
}
