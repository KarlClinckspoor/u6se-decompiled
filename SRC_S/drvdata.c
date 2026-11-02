/* flags: -mm -c -O */
/*
 * The Savage Empire: SAVAGE.EXE
 * data only: the video modes and the names of the graphics drivers. Linked first, so its
 * data comes right after C0M's (DS:00A8), while the code that uses it (LIB/tmdrv.c, frame
 * 0D46) is near the end.
 */

/*00A8*/unsigned char videoModes[] = {0x13, 0x0D, 4, 9, 7};/*MCGA/VGA, EGA, CGA, Tandy, Hercules*/
/*00AD*/char tmDrvName[] = "TM.DRV";
/*00B4*/char superTmDrvName[] = "SUPERTM.DRV";
