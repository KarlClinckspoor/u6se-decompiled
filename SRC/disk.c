/*
 * The Savage Empire (Worlds of Ultima) - GAME.EXE
 * Drive/floppy management module, from Ultima VI's disk.c.
 *
 * The game came on 3 floppy disks ("DISK 1".."DISK 3"); DiskNames gives the
 * floppy of each of the 8 disk numbers the game asks for (DISK_x, Ultima VI's
 * numbering kept). Each floppy is identified by a file "SE.<n>" in its root
 * directory. DISK_init() finds the drives (DOS IOCTL / BIOS equipment list)
 * and assumes the current one holds disk 1. DiskDrive[] remembers on which
 * drive each disk was found (1 = A:, 0 = unknown), so on a hard disk install
 * every disk resolves to the same drive and no prompt ever appears.
 * DISK_confirm(n) must be called before accessing the files of disk n: it
 * selects the right drive, or asks the player to insert the disk and searches
 * all the removable drives for it.
 * DISK_full() is called by the file error handler on a short write.
 */
#include "se.h"

/*text window (Ultima VI's)*/
struct tClipInfo/*size:9*/ {
	unsigned char originX, originY, cols, rows;
	unsigned char curX, curY;
	unsigned char fgColor, bgColor;
	unsigned char attrFlags;
};

/*the mouse state, filled by MOUSE_SERVE (Ultima VI's)*/
struct tSurface;
struct tMouse {
	unsigned char buttonL, buttonR;
	int16 posX, posY;
	uint16 unused06;
	void far *shapes;
	uint16 shapeIndex;
	struct tSurface *surface;
	void far *saveBuf;
	void (far *onMove)();
	unsigned char installed;
};

/*OSI file routines (asm)*/
extern union tHandler D_01F6;/*OSI_ErrorHandler: far callback invoked by the OSI file functions on error*/
extern C_0000_0244();/*OSI_callback: an error handler that does nothing*/
extern int16 C_0000_0245(char *path);/*OSI_open*/
extern int16 C_0000_0284(int16 handle);/*OSI_close*/
extern C_0268_0128();/*FileErrorHandler (main)*/
extern C_2AF5_001B(int16 driveLetter);/*OSI_selectDrive*/
extern int16 C_2AFB_0006();/*OSI_getDrive: the current drive's letter*/
extern void (*D_2B84)();/*DriveErrorCallback: called by OSI_selectDrive when selection fails*/
extern cdecl C_2ABF_00A7();/*MOUSE_SERVE*/

/*console*/
extern int16 C_05D0_0E2D();/*CON_prompt: a key if one is waiting (animating meanwhile), else 0*/
extern int16 C_05D0_22C8();/*CON_getch*/
extern C_05D0_25AD(struct tClipInfo *);/*CON_setClip*/
extern C_05D0_264F(int16);/*CON_putch0*/
extern cdecl C_05D0_2DB9(char far *, ...);/*CON_printf*/
extern char far C_2F65_033C[];/*"\nPlease insert %s." (far messages, like C_ names: outside DGROUP)*/

extern unsigned char D_00D8;/*MouseOn*/
extern int16 D_0208;/*MapAnimEnabled*/
extern char D_03AE;/*AnimateAtPrompt: run animations while waiting for a key*/
extern int16 D_03C7;/*MousePress*/
extern int16 D_03C9;/*MouseTrigger*/
extern struct tClipInfo *D_03EC;/*CurClip: current text window*/
extern unsigned char D_24D8;/*text color of the disk prompt*/
extern unsigned char D_24E6;/*normal text color of the message window*/
extern struct tMouse D_3E9C;/*Mouse*/
extern struct tClipInfo D_B644[4];/*TextWindows: [3] message scroll*/
extern unsigned char D_ED1E, D_ED1F;/*DosVerMajor, DosVerMinor*/

/*floppy of each disk number*/
/*2A64*/char *DiskNames[] = {
	/*2AA4*/"",
	/*2AA5*/"DISK 1",
	/*2AAC*/"DISK 2",
	/*2AB3*/"DISK 3",
	"DISK 2",
	"DISK 1",
	"DISK 1",
	"DISK 2",
	"DISK 3"
};
/*2A76*/int16 CurrentDisk = 0;/*current disk (DISK_x)*/
/*2A78*/int16 CurrentDrive = 1;/*current drive (1 = A:)*/
/*2A7A*/int16 DiskFullChoice = ' ';/*"disk full" answer: 'S'ave/'R'estore, ' ' none*/
/*2A7C*/unsigned char DiskDrive[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};/*drive of each disk (1 = A:, 0 = unknown)*/
/*2A85*/unsigned char DriveType[26] = {0};/*drive types A:..Z: 0 none, 1 fixed, 2 removable*/
/*2A9F*/char DiskIdFile[] = "se.1";/*disk identification file, [3] = disk number*/

/*default "drive selection failed" callback (DriveErrorCallback): do nothing*/
/*C_2A81_0003*/static DISK_NullCallback() {
	return 0;
}

/*is disk "diskNum" in the current drive? (does "SE.<diskNum>" exist)
  The file errors are silenced while probing*/
/*C_2A81_000A*/static DISK_IsInDrive(int16 diskNum) {
	int16 fileHandle;

	D_01F6.f = C_0000_0244;
	DiskIdFile[3] = diskNum + '0';
	fileHandle = C_0000_0245(DiskIdFile);
	if(fileHandle != -1)
		C_0000_0284(fileHandle);
	D_01F6.f = C_0268_0128;
	return (fileHandle != -1);
}

/*select the drive of disk "diskNum" and register this drive for every
  other disk whose identification file is also there*/
/*C_2A81_0055*/static DISK_ScanDrive(int16 diskNum) {
	int16 drive;
	int16 disk;

	CurrentDrive =
	drive = DiskDrive[diskNum];
	C_2AF5_001B(drive + 'A' - 1);
	for(disk = 1; disk < 9; disk++)
		if(DISK_IsInDrive(disk))
			DiskDrive[disk] = drive;
}

/*make sure disk "diskNum" is available and its drive selected; prompts
  the player to insert it when its drive is not known. 100, 101 and 102
  stand for disks 3, 3 and 4*/
/*C_2A81_0098*/DISK_confirm(register int16 diskNum) {
	register int16 keyOrIndex;
	int16 foundDrive;
	int16 savedAnimate;
	struct tClipInfo *savedClip;

	if(diskNum == 100)
		diskNum = 3;
	else if(diskNum == 101)
		diskNum = 3;
	else if(diskNum == 102)
		diskNum = 4;
	savedAnimate = D_03AE;
	if(CurrentDisk != diskNum) {
		while(CurrentDisk != diskNum && !DiskDrive[diskNum]) {
			savedClip = D_03EC;
			C_05D0_25AD(&(D_B644[3]));
			C_05D0_264F(-1);
			D_03EC->fgColor = D_24D8;
			C_05D0_2DB9(C_2F65_033C, DiskNames[diskNum]);
			C_05D0_264F(-1);
			D_03AE = 0;/*no animations while waiting*/
			/*wait for a key or a mouse click (= ' ')*/
			keyOrIndex = 0;
			while(!keyOrIndex) {
				if(D_00D8) {
					C_2ABF_00A7();
					if(D_3E9C.buttonL || D_3E9C.buttonR) {
						if(!D_03C7) {
							D_03C7 =
							D_03C9 = 1;
							keyOrIndex = ' ';
						} else {
							D_03C9 = 0;
						}
					} else {
						D_03C7 =
						D_03C9 = 0;
					}
				}
				if(!keyOrIndex)
					keyOrIndex = C_05D0_0E2D();
			}
			C_05D0_264F('\n');
			C_05D0_25AD(savedClip);
			D_03EC->fgColor = D_24E6;
			D_03AE = savedAnimate;
			/*<CTRL+C>: accept the current drive as is.
			  bug?: CurrentDrive is a drive number here, not a letter*/
			if(keyOrIndex == 3) {
				CurrentDisk = diskNum;
				DiskDrive[diskNum] = CurrentDrive - ('A' - 1);
				C_2AF5_001B(CurrentDrive);
				return;
			}
			/*search the disk in all the removable drives*/
			foundDrive = 0;
			for(keyOrIndex = 0; keyOrIndex < 26; keyOrIndex++) {
				if(DriveType[keyOrIndex] != 2)
					continue;
				C_2AF5_001B(keyOrIndex + 'A');
				if(DISK_IsInDrive(diskNum)) {
					foundDrive = keyOrIndex + 1;
					DiskDrive[diskNum] = foundDrive;
					/*the other disks are not in this drive anymore*/
					for(keyOrIndex = 1; keyOrIndex < 9; keyOrIndex++) {
						if(keyOrIndex != diskNum && DiskDrive[keyOrIndex] == foundDrive)
							DiskDrive[keyOrIndex] = 0;
					}
					DISK_ScanDrive(diskNum);
					break;
				}
			}
		}
		if(DiskDrive[diskNum] != CurrentDrive) {
			CurrentDrive = DiskDrive[diskNum];
			C_2AF5_001B(CurrentDrive + 'A' - 1);
		}
		CurrentDisk = diskNum;
	}
}

/*forget the drive of the current disk and ask for it again*/
/*C_2A81_0242*/DISK_Reconfirm() {
	DiskDrive[CurrentDisk] = 0;
	DISK_confirm(CurrentDisk);
}

/*"disk full" while writing: ask whether to save the game (S) or restore
  the last saved game (R); the answer is stored in DiskFullChoice*/
/*C_2A81_0255*/DISK_full() {
	int16 answer;

	D_0208 = 0;
	C_05D0_2DB9("Disk full.  Do you want to Save the game and continue, ");
	C_05D0_2DB9("or Restore your last saved game? ");
	do {
		answer = C_05D0_22C8();
	} while(answer != 'S' && answer != 'R');
	DiskFullChoice = answer;
	if(answer == 'S')
		C_05D0_2DB9("S\n\nSaving...\n");
	else
		C_05D0_2DB9("R\n\nRestoring...\n");
}

/*find the drives and their types, assume the current drive holds disk 1*/
/*C_2A81_02A6*/DISK_init() {
	int16 drive;
	union REGS regs;

	D_2B84 = DISK_NullCallback;
	/*get dos version*/
	regs.h.ah = 0x30;
	intdos(&regs, &regs);
	D_ED1E = regs.h.al;
	D_ED1F = regs.h.ah;

	if(D_ED1E >= 3) {
		for(drive = 0; drive < 26; drive++) {
			/*IOCTL 4408h: is drive removable? (ax: 0 yes, 1 no)*/
			regs.h.ah = 0x44;
			regs.h.al = 8;
			regs.h.bl = drive + 1;
			intdos(&regs, &regs);
			if(regs.x.cflag != 0)
				continue;
			if(regs.x.ax)
				DriveType[drive] = 1;
			else
				DriveType[drive] = 2;
			/*dos 3.20 and above: IOCTL 440Eh get logical drive map;
			  a drive which is an alias of another one (e.g. B: on
			  single floppy systems) is ignored*/
			if(D_ED1E >= 4 || D_ED1F >= 20) {
				regs.h.ah = 0x44;
				regs.h.al = 0xe;
				regs.h.bl = drive + 1;
				intdos(&regs, &regs);
				if(regs.h.al && regs.h.al != drive + 1)
					DriveType[drive] = 0;
			}
		}
	}
	/*below dos 3.20: BIOS equipment list (int 11h): bit 0 floppy
	  installed, bits 6-7 number of floppies - 1; the current drive is
	  considered fixed if it is not A: or B:*/
	if(D_ED1E < 3 || (D_ED1E == 3 && D_ED1F < 20)) {
		int86(0x11, &regs, &regs);
		if(regs.x.ax & 1) {
			DriveType[0] = 2;
			if(regs.x.ax & 0xc0)
				DriveType[1] = 2;
		} else {
			DriveType[0] =
			DriveType[1] = 0;
		}
		drive = C_2AFB_0006();
		if(drive > 'B')
			DriveType[drive - 'A'] = 1;
	}

	CurrentDisk = 1;
	DiskDrive[1] = C_2AFB_0006() + 1 - 'A';
	DISK_ScanDrive(1);
}
