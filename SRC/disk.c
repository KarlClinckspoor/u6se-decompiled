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
extern union tHandler OSI_ErrorHandler;/*OSI_ErrorHandler: far callback invoked by the OSI file functions on error*/
extern OSI_callback();/*OSI_callback: an error handler that does nothing*/
extern int16 OSI_open(char *path);/*OSI_open*/
extern int16 OSI_close(int16 handle);/*OSI_close*/
extern FileErrorHandler();/*FileErrorHandler (main)*/
extern OSI_selectDrive(int16 driveLetter);/*OSI_selectDrive*/
extern int16 OSI_getDrive();/*OSI_getDrive: the current drive's letter*/
extern void (*DriveErrorCallback)();/*DriveErrorCallback: called by OSI_selectDrive when selection fails*/
extern cdecl MOUSE_SERVE();/*MOUSE_SERVE*/

/*console*/
extern int16 CON_prompt();/*CON_prompt: a key if one is waiting (animating meanwhile), else 0*/
extern int16 CON_getch();/*CON_getch*/
extern CON_setClip(struct tClipInfo *);/*CON_setClip*/
extern CON_putch0(int16);/*CON_putch0*/
extern cdecl CON_printf(char far *, ...);/*CON_printf*/
extern char far InsertDiskMsg[];/*"\nPlease insert %s." (far messages, like C_ names: outside DGROUP)*/

extern unsigned char MouseOn;/*MouseOn*/
extern int16 MapAnimEnabled;/*MapAnimEnabled*/
extern char AnimateAtPrompt;/*AnimateAtPrompt: run animations while waiting for a key*/
extern int16 MousePress;/*MousePress*/
extern int16 MouseTrigger;/*MouseTrigger*/
extern struct tClipInfo *CurClip;/*CurClip: current text window*/
extern unsigned char ColorRed;/*text color of the disk prompt*/
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

	OSI_ErrorHandler.f = OSI_callback;
	DiskIdFile[3] = diskNum + '0';
	fileHandle = OSI_open(DiskIdFile);
	if(fileHandle != -1)
		OSI_close(fileHandle);
	OSI_ErrorHandler.f = FileErrorHandler;
	return (fileHandle != -1);
}

/*select the drive of disk "diskNum" and register this drive for every
  other disk whose identification file is also there*/
/*C_2A81_0055*/static DISK_ScanDrive(int16 diskNum) {
	int16 drive;
	int16 disk;

	CurrentDrive =
	drive = DiskDrive[diskNum];
	OSI_selectDrive(drive + 'A' - 1);
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
	savedAnimate = AnimateAtPrompt;
	if(CurrentDisk != diskNum) {
		while(CurrentDisk != diskNum && !DiskDrive[diskNum]) {
			savedClip = CurClip;
			CON_setClip(&(D_B644[3]));
			CON_putch0(-1);
			CurClip->fgColor = ColorRed;
			CON_printf(InsertDiskMsg, DiskNames[diskNum]);
			CON_putch0(-1);
			AnimateAtPrompt = 0;/*no animations while waiting*/
			/*wait for a key or a mouse click (= ' ')*/
			keyOrIndex = 0;
			while(!keyOrIndex) {
				if(MouseOn) {
					MOUSE_SERVE();
					if(D_3E9C.buttonL || D_3E9C.buttonR) {
						if(!MousePress) {
							MousePress =
							MouseTrigger = 1;
							keyOrIndex = ' ';
						} else {
							MouseTrigger = 0;
						}
					} else {
						MousePress =
						MouseTrigger = 0;
					}
				}
				if(!keyOrIndex)
					keyOrIndex = CON_prompt();
			}
			CON_putch0('\n');
			CON_setClip(savedClip);
			CurClip->fgColor = D_24E6;
			AnimateAtPrompt = savedAnimate;
			/*<CTRL+C>: accept the current drive as is.
			  bug?: CurrentDrive is a drive number here, not a letter*/
			if(keyOrIndex == 3) {
				CurrentDisk = diskNum;
				DiskDrive[diskNum] = CurrentDrive - ('A' - 1);
				OSI_selectDrive(CurrentDrive);
				return;
			}
			/*search the disk in all the removable drives*/
			foundDrive = 0;
			for(keyOrIndex = 0; keyOrIndex < 26; keyOrIndex++) {
				if(DriveType[keyOrIndex] != 2)
					continue;
				OSI_selectDrive(keyOrIndex + 'A');
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
			OSI_selectDrive(CurrentDrive + 'A' - 1);
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

	MapAnimEnabled = 0;
	CON_printf("Disk full.  Do you want to Save the game and continue, ");
	CON_printf("or Restore your last saved game? ");
	do {
		answer = CON_getch();
	} while(answer != 'S' && answer != 'R');
	DiskFullChoice = answer;
	if(answer == 'S')
		CON_printf("S\n\nSaving...\n");
	else
		CON_printf("R\n\nRestoring...\n");
}

/*find the drives and their types, assume the current drive holds disk 1*/
/*C_2A81_02A6*/DISK_init() {
	int16 drive;
	union REGS regs;

	DriveErrorCallback = DISK_NullCallback;
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
		drive = OSI_getDrive();
		if(drive > 'B')
			DriveType[drive - 'A'] = 1;
	}

	CurrentDisk = 1;
	DiskDrive[1] = OSI_getDrive() + 1 - 'A';
	DISK_ScanDrive(1);
}
