/* flags: -mm -c -O -G -y */
/*
 * Worlds of Ultima: The Savage Empire - SAVAGE.EXE
 * segment 034C: the drives and the game disks, the critical error handler,
 * loading from .lzc archives, the configuration (CONFIG.SE and the command
 * line), main(), the title and introduction, the main menu, the story so far,
 * character creation (Intanya's questions) and saving the new character,
 * the endgame, starting GAME.EXE.
 */
#include <conio.h>
#include <ctype.h>
#include <dir.h>
#include <dos.h>
#include <process.h>
#include <stdlib.h>
#include <string.h>

/*a drawing surface of the OSI library (16 bytes)*/
struct tSurface {
	int _00, _02;
	int left, top, right, bottom;/*+04..+0A the clipping rectangle*/
	int _0c, _0e;
};

/*a text window of the OSI library (17 bytes; U.EXE's tText)*/
struct tText {
	struct tSurface *surface;/*+00 the surface it prints on*/
	int curX, curY;/*+02,+04 the cursor*/
	void far *font;/*+06*/
	unsigned char fg, bg;/*+0A,+0B the colours*/
	char *buffer;/*+0C the formatted output*/
	char *bufPtr;/*+0E where its next character goes*/
	char justify;/*+10*/
};

/*a question of Intanya's: a deed and the choice (each in one or two texts),
the virtue an answer of (a) and of (b) raise (0 strength, 1 dexterity,
2 intelligence)*/
struct tQuestion {
	char *deed, *deed2;
	char *choice, *choice2;
	int virtueA, virtueB;
};

/*other modules' initialised data*/
extern struct tText *D_201E;/*the current text window*/
extern int D_20CA;/*music card: 0 none, 1 Roland, 2 AdLib (main sets 3 first)*/
extern int D_20CE;/*how far the music has played (the intro waits on it)*/
extern int D_240C;/*the last file operation failed*/

/*uninitialised data*/
extern int D_2818;/*a saved game exists (0: run INSTALL first)*/
extern char D_281A[0x300];/*the palette being set*/
extern struct tSurface D_2B1A;/*the screen*/
extern int D_2B2E;/*endgame: months taken*/
extern int D_2B38;
extern struct tSurface D_2B3E;/*the text window's area*/
extern void far *D_2B4E;/*the font (savage.fnt)*/
extern struct tSurface D_2B52;/*the back buffer*/
extern int D_2B66;
extern int D_2B68;/*graphic adapter: 0 VGA, 1 EGA, 3 Tandy*/
extern struct tText D_2B72;/*the text window*/
extern void (far *D_2B90)();/*fill a rectangle (surface, x1, y1, x2, y2, colour)*/
extern int D_2B98;/*show the endgame (D, B or Y given)*/
extern char **D_2B9A;/*argv, passed on to GAME.EXE*/
extern int D_2B9C;/*skip the title and introduction (X)*/
extern int D_2BA2;/*frame skipping on slow machines: 2 never, else every other frame*/
extern int D_2BAC;/*mouse (M) or keyboard (K)*/
extern char D_2BB6, D_2BB7;/*DOS version: major, minor*/
extern int D_2BB8;/*endgame: days taken*/
extern void (far *D_2BBA)();/*copy a surface onto another (from, to)*/
extern void (far *D_2BC6)();/*draw a shape (surface, x, y, shapes, n)*/
extern int D_2BCA;/*endgame: years taken*/
extern char D_2BCC, D_2BCD, D_2BCE;/*the new character's strength, dexterity, intelligence*/
extern char *D_2BCF;/*the new character's name*/
extern int D_2BDA;
extern char D_2BE4[3][0x300];/*the palettes (savage.pal)*/
extern int D_34EC;/*a game is in progress*/
extern void (far *D_34FE)();/*dissolve a surface onto another (from, to, n)*/
extern struct tSurface D_3506;/*the text window's area, saved*/
extern void (far *D_3516)();/*clear a surface (surface, colour)*/
extern struct tText D_351A;/*the text window, saved*/
extern void (far *D_3534)();/*draw a shape (surface, x, y, shapes, n, m)*/
extern int D_3538;/*a character has been created*/
extern int D_353E;/*music on*/

/*the OSI library (pascal) and Origin's other routines*/
extern int pascal C_0C02_0000(char far *name);/*open a file (-1: error)*/
extern void pascal C_0C02_002D(int fd);/*close a file*/
extern long pascal C_0C06_000C(int fd, long offset, long length, void far *buf);/*read*/
extern long pascal C_0C12_0000(int fd, long offset, long length, void far *buf);/*write*/
extern long pascal C_0C1F_000A(char far *name, int n);/*size of item n of an .lzc archive*/
extern void far * pascal C_0C58_0305(char far *name, int n, void far *buf, int a, long b);/*load item n of an .lzc archive*/
extern void far *C_0BC5_00AD(long size, int flags);/*allocate far memory*/
extern void C_0BC5_0126(void far *p);/*free far memory*/
extern void C_0BC5_000B(int);
extern void C_0BC5_0045(int);
extern void C_0BC5_0079(int);
extern void pascal C_077E_000C(int x, int y);/*move the text cursor*/
extern int pascal C_0780_0004(char c);/*width of a character*/
extern void C_078E_0230(char far *fmt, ...);/*print*/
extern void C_078E_0252(char far *fmt, ...);/*print, word wrapped, %P pauses*/
extern void pascal C_07C3_0005(int mode);/*set the video mode*/
extern void C_07D7_0008(void);/*wait for the vertical retrace*/
extern void pascal C_08CA_0006(struct tSurface *from, struct tSurface *to);/*fade in*/
extern void pascal C_0896_0004(struct tSurface *, int, int);/*fade out*/
extern void pascal C_08E2_000A(char far *pal);/*set the palette*/
extern void C_08E4_1478(int);/*fade the music out*/
extern void C_08E4_164A(char far *name);/*load the timbres*/
extern void C_08E4_17BE(void);/*stop the music*/
extern void C_08E4_19FF(void far *song);/*play a song*/
extern long C_0AE3_0004(void);/*free far memory*/
extern int C_0C97_00BE(void);/*speed of the machine*/
extern void C_0CED_0106(long t);/*wait (2383 per millisecond)*/
extern int pascal C_0D23_0027(int lo, int hi);/*random number from lo to hi*/
extern void pascal C_0D23_004B(int seed);/*seed the random numbers*/
extern void pascal C_0D28_000B(struct tSurface *, int);/*make the screen surface*/
extern void pascal C_0D28_01BC(struct tSurface *, int);/*allocate a surface*/
extern int C_0D46_01AA(int adapter);/*load the graphics driver*/
extern int pascal C_0753_0000(int, int, void far *shapes, int n, int what);/*a shape's size*/
extern void C_0636_0C6D(struct tSurface *, struct tSurface *, int adapter, void far *song);/*the title*/

/*this module's functions used before they are defined*/
void ResetText(int color);
int GetKey(void);
void FlushKeys(void);
void Fatal(void);
void SkipIntro(int keep);
void FadeOut(void);
void Intro(void);
void IntroEnd(void);
void MainMenu(int fromTitle);
void Credits(void);
void CreateCharacter(void);
int Awakening(void);
int AskQuestion(int q, int last);
int RaiseVirtue(int v);
void AnimateIntanya(int n);
int SaveCharacter(void);
void CheckSavedGame(void);
int GameInProgress(void);
int CharacterCreated(void);
void Story(int unused);
void Endgame(void);

/*the name of each game disk, as "Please insert" says it*/
/*00C0*/char *diskNames[9] = {"", "DISK 1", "DISK 2", "DISK 3", "DISK 2", "DISK 1", "DISK 1", "DISK 2", "DISK 3"};
/*00D2*/char copyright[] =/*unused*/ "SAVAGE EMPIRE (c) 1990 Origin Systems, All Rights Reserved.";
/*010E*/char textBuffer[0x400] = "";/*the text window's buffer*/
/*050E*/int textColor = 7;/*the text window's background colour*/
/*0510*/void far *introShapes = 0;/*intro.lzc 1*/
/*0514*/void far *introShapes2 = 0;/*intro.lzc 2*/
/*0518*/void far *introShapes0 = 0;/*intro.lzc 0*/
/*051C*/void far *D_051C = 0;/*freed with the intro's shapes, never loaded*/
/*0520*/void far *D_0520 = 0;/*0520-052B unused*/
/*0524*/void far *D_0524 = 0;
/*0528*/void far *D_0528 = 0;
/*052C*/void far *introSong = 0;/*music.lzc 18*/
/*0530*/void far *menuSong = 0;/*music.lzc 0*/
/*0534*/void far *endShapes = 0;/*endgame.lzc 0*/
/*0538*/void far *diskShapes = 0;/*create.lzc 7: the "insert disk" box*/
/*053C*/void far *titleSong = 0;/*music.lzc 19*/
/*0540*/void far *createShapes1 = 0;/*create.lzc 1..4: Intanya*/
/*0544*/void far *createShapes2 = 0;
/*0548*/void far *createShapes3 = 0;
/*054C*/void far *createShapes4 = 0;
/*0550*/void far *menuShapes = 0;/*create.lzc 0*/
/*0554*/void far *storyShapes = 0;/*create.lzc 5*/
/*0558*/void far *creditShapes = 0;/*create.lzc 6*/
/*055C*/int exitCode = 0;/*1 file not found, 2 out of memory*/
/*055E*/int D_055E = 0;/*unused*/
/*0560*/int endingChoice = 0;/*endgame (L): 1 Aiela, -1 Tristia*/
/*0562*/int curPalette = 1;
/*0564*/struct tQuestion questions[6] = {
	{"Your chief has told you to guard the village while a battle rages nearby. Your chief fights in the battle.\n",
		0,
		"Will you (a) obey the word of your chief, or (b) join your fellow warriors in the fight? ",
		0, 2, 0},
	{"You fight a warrior you hate, and knock his spear from his hands.\n",
		"Another blow and he will be dead.\n",
		"Will you (a) let him surrender, and spare him if he does, or (b) slay him where he stands? ",
		0, 1, 0},
	{"One warrior borrows another's spear and fails to return it. Days later, he mislays his own spear and you find it.\n",
		0,
		"Do you (a) return it to him, or (b) give it to the warrior who is owed the spear? ",
		0, 1, 2},
	{"You have sworn to protect your chief at any cost. Yet you see him foully murder a warrior of his own tribe.\n",
		0,
		"The murdered man's brother asks you what you saw.\n",
		"Will you (a) keep your oath and protect your king, or (b) tell the truth of the matter? ", 0, 2},
	{"A huge, powerful warrior stands against you and demands you give him your food.\n",
		0,
		"Will you (a) throw his demand in his teeth and attack him, or (b) give him your food, since it is clear he is hungry? ",
		0, 0, 1},
	{"Your chief gives you a pouch of gems to take to another; he has not counted them.\n",
		"On the path to the other village, you find a lamed warrior who cannot hunt.\n",
		"He could trade a few of your gems for food, and they will not be missed.\n",
		"Will you (a) deliver the gems as you were charged, or (b) give the warrior a gem? ", 2, 1}
};
/*05AC*/int curDisk = 0;/*the disk in the drive*/
/*05AE*/int curDrive = 0;
/*05B0*/int D_05B0 = 0x20;/*unused*/
/*05B2*/char diskDrive[9] = {-1, -1, -1, -1, -1, -1, -1, -1, -1};/*drive of each game disk (0 = A:), -1 unknown*/
/*05BB*/char driveKind[26] = {0};/*kind of each drive: 0 none, 1 fixed, 2 removable*/
/*05D5*/char markerFile[] = "se.1";/*a disk's marker file; [3] is the disk number*/
/*05DA*/int menuLast = 3;/*the last choice of the main menu*/
/*05DC*/int menuY[4] = {0x74, 0x86, 0x98, 0xaa};/*the main menu's buttons: y*/
/*05E4*/int menuShape[4] = {5, 1, 9, 3};/*the main menu's buttons: shape*/
/*05EC*/char playerName[14] = "0123456789012";

/*is disk n in the drive (its marker file there)*/
/*C_034C_000B*/int IsDiskIn(int n) {
	int fd;

	markerFile[3] = n + '0';
	fd = C_0C02_0000(markerFile);
	if(fd != -1)
		C_0C02_002D(fd);
	return fd != -1;
}

/*make disk d's drive the current one; the disks found there are in it too*/
/*C_034C_003D*/void SelectDiskDrive(int d) {
	int drive;
	int i;

	drive = diskDrive[d];
	curDrive = drive;
	setdisk(drive);
	for(i = 1; i < 9; i++)
		if(IsDiskIn(i))
			diskDrive[i] = drive;
}

/*make sure disk d is in a drive: ask for it ("Please insert") until it is
found in one of the removable drives*/
/*C_034C_007D*/void InsertDisk(int d) {
	int drive;
	int i;

	if(curDisk != d) {
		D_351A = D_2B72;
		D_3506 = D_2B3E;
		(*D_2BBA)(&D_2B1A, &D_2B52);
		while(curDisk != d && diskDrive[d] < 0) {
			(*D_2BC6)(&D_2B1A, 0x6e, 0x50, diskShapes, curPalette);
			D_201E = &D_2B72;
			D_2B3E.left = 0x76;
			D_2B3E.top = 0x5f;
			D_2B3E.right = 0xdf;
			D_2B3E.bottom = 0x70;
			D_2B72.curX = 0x76;
			D_2B72.curY = 0x5f;
			D_2B72.justify = 0;
			if(curPalette == 0 && D_2B68 == 0)
				D_2B72.bg = 0x12;
			ResetText(D_2B72.bg);
			C_078E_0252("Please insert\n%P");
			C_078E_0252("%s%P", diskNames[d]);
			for(i = 0; !i; i = GetKey())
				FlushKeys();
			if(i == 0x1b)
				exit(1);
			drive = 0;
			for(i = 0; i < 26; i++) {
				if(driveKind[i] == 2) {
					setdisk(i);
					if(IsDiskIn(d)) {
						drive = i;
						diskDrive[d] = drive;
						for(i = 1; i <= 9; i++)
							if(i != d && diskDrive[i] == drive)
								diskDrive[i] = -1;
						SelectDiskDrive(d);
						break;
					}
				}
			}
		}
		D_2B72 = D_351A;
		D_2B3E = D_3506;
		(*D_2BBA)(&D_2B52, &D_2B1A);
		if(diskDrive[d] != curDrive)
			setdisk(curDrive = diskDrive[d]);
		curDisk = d;
	}
}

/*the critical error handler: beep, and fail the operation*/
/*C_034C_023C*/int CritErrHandler(int errval, int ax, int bp, int si) {
	sound(440);
	C_0CED_0106(1191500L);
	nosound();
	hardretn(2);
}

/*find the drives and their kinds; the current drive has disk 5*/
/*C_034C_026C*/void FindDrives() {
	int i;
	union REGS r;

	harderr(CritErrHandler);
	r.h.ah = 0x30;
	intdos(&r, &r);
	D_2BB6 = r.h.al;
	D_2BB7 = r.h.ah;
	if(D_2BB6 >= 3) {
		for(i = 0; i < 26; i++) {
			r.h.ah = 0x44;
			r.h.al = 8;
			r.h.bl = i + 1;
			intdos(&r, &r);
			if(r.x.cflag == 0) {
				if(r.x.ax != 0)
					driveKind[i] = 1;
				else
					driveKind[i] = 2;
				if(D_2BB6 >= 4 || D_2BB7 >= 20) {
					r.h.ah = 0x44;
					r.h.al = 0xe;
					r.h.bl = i + 1;
					intdos(&r, &r);
					if(r.h.al != 0 && r.h.al != i + 1)
						driveKind[i] = 0;
				}
			}
		}
	}
	if(D_2BB6 < 3 || (D_2BB6 == 3 && D_2BB7 < 20)) {
		int86(0x11, &r, &r);
		if(r.x.ax & 1) {
			driveKind[0] = 2;
			if(r.x.ax & 0xc0)
				driveKind[1] = 2;
		} else
			driveKind[0] = driveKind[1] = 0;
		i = getdisk();
		if(i > 1)
			driveKind[i] = 1;
	}
	curDisk = 5;
	diskDrive[5] = getdisk();
	SelectDiskDrive(5);
}

/*free a far block and clear its pointer*/
/*C_034C_03B4*/void FreeFar(void far **p) {
	if(*p) {
		C_0BC5_0126(*p);
		*p = 0;
	}
}

/*load item n of an .lzc archive; 0 (and exitCode) on error*/
/*C_034C_03DB*/void far *LoadLzc(char *name, int n) {
	long size;
	void far *buf;

	size = C_0C1F_000A(name, n);
	if(size < 0) {
		exitCode = 1;
		return 0;
	}
	buf = C_0BC5_00AD(size, 0);
	if(buf == 0) {
		exitCode = 2;
		return 0;
	}
	buf = C_0C58_0305(name, n, buf, 0, 0L);
	if(buf == 0) {
		exitCode = 1;
		return 0;
	}
	return buf;
}

/*the same, allocating with flag 4*/
/*C_034C_0471*/void far *LoadLzc4(char *name, int n) {
	long size;
	void far *buf;

	size = C_0C1F_000A(name, n);
	if(size < 0) {
		exitCode = 1;
		return 0;
	}
	buf = C_0BC5_00AD(size, 4);
	if(buf == 0) {
		exitCode = 2;
		return 0;
	}
	buf = C_0C58_0305(name, n, buf, 0, 0L);
	if(buf == 0) {
		exitCode = 1;
		return 0;
	}
	return buf;
}

/*quit (control-break, the main menu's ^Q)*/
/*C_034C_0508*/int Quit() {
	FadeOut();
	C_07C3_0005(3);
	C_08E4_17BE();
	C_0BC5_000B(8);
	C_0BC5_0079(8);
	exit(0);
}

/*run GAME.EXE*/
/*C_034C_0540*/void StartGame() {
	FadeOut();
	InsertDisk(1);
	C_08E4_17BE();
	C_0BC5_000B(8);
	C_0BC5_0079(8);
	C_07C3_0005(3);
	if(execv("GAME.EXE", D_2B9A) == -1)
		Fatal();
}

/*load the songs; 0 if there is no music*/
/*C_034C_0592*/int LoadMusic() {
	if(D_353E == 0 || D_20CA == 0)
		return 0;
	if(C_0AE3_0004() <= 0)
		return 0;
	InsertDisk(7);
	introSong = LoadLzc4("music.lzc", 0x12);
	menuSong = LoadLzc4("music.lzc", 0);
	titleSong = LoadLzc4("music.lzc", 0x13);
	if(introSong == 0 || menuSong == 0 || titleSong == 0)
		return 0;
	return 1;
}

/*read a key: extended keys are 0x100 + their scan code*/
/*C_034C_0627*/int GetKey() {
	int key;

	key = getch();
	if(key == 0 && kbhit())
		key = getch() + 0x100;
	return key;
}

/*empty the keyboard buffer*/
/*C_034C_064E*/void FlushKeys() {
	while(kbhit())
		GetKey();
}

/*wait t milliseconds; a key skips the introduction*/
/*C_034C_0662*/void Wait(int t) {
	int i;
	int step;

	if(t > 1000)
		step = 500;
	else
		step = t >> 1;
	for(i = 0; i <= t; i += step) {
		if(kbhit())
			SkipIntro(0);
		else
			C_0CED_0106((long)step * 2383);
	}
}

/*set palette n (VGA)*/
/*C_034C_06B9*/void SetPalette(int n) {
	int i;
	char *dst, *src;

	i = 0;
	if(D_2B68 == 0) {
		src = D_2BE4[n];
		curPalette = n;
		for(dst = D_281A; i < 0x300; i++) {
			*dst = *src;
			dst++;
			src++;
		}
		C_08E2_000A(D_281A);
	}
}

/*fatal error: back to text mode, exit with exitCode*/
/*C_034C_0704*/void Fatal() {
	C_07C3_0005(3);
	if(D_20CA)
		C_08E4_17BE();
	C_0BC5_000B(8);
	C_0BC5_0079(8);
	exit(exitCode);
}

/*a key was pressed in the introduction: end it; unless keep, go to the
main menu*/
/*C_034C_073F*/void SkipIntro(int keep) {
	FlushKeys();
	if(D_353E)
		C_08E4_1478(200);
	D_2B1A.top = 0;
	D_2B1A.bottom = 199;
	D_2B52.left = 0;
	D_2B52.right = 319;
	D_2B52.top = 0;
	D_2B52.bottom = 199;
	FadeOut();
	FreeFar(&introShapes);
	FreeFar(&introShapes2);
	FreeFar(&introShapes0);
	FreeFar(&D_051C);
	if(keep == 0) {
		FlushKeys();
		CheckSavedGame();
		MainMenu(0);
	}
}

/*fade the screen out, clear it*/
/*C_034C_07C4*/void FadeOut() {
	if(D_2B68 == 0)
		C_0896_0004(&D_2B1A, 0, 1);
	(*D_3516)(&D_2B1A, 0);
	SetPalette(curPalette);
}

/*show the back buffer (fade in on VGA)*/
/*C_034C_07F8*/void FadeIn() {
	if(D_2B68 == 0)
		C_08CA_0006(&D_2B52, &D_2B1A);
	else
		(*D_2BBA)(&D_2B52, &D_2B1A);
}

/*reset the text window (clearing it in colour, if >= 0)*/
/*C_034C_0822*/void ResetText(int color) {
	if(color >= 0)
		(*D_3516)(&D_2B3E, color);
	D_2B72.curX = D_2B3E.left;
	D_2B72.curY = D_2B3E.top;
	D_2B72.fg = 0;
	D_2B72.bg = textColor;
	D_2B72.justify = 0;
	D_2B72.buffer = D_2B72.bufPtr = textBuffer;
	textBuffer[0] = 0;
}

/*parse a configuration string (CONFIG.SE or a command line argument):
V/E/T graphic adapter, M/K mouse or keyboard, N/P/A/R music (none, none,
AdLib, Roland), X skip the introduction, LA/LT the ending, D/B/Y<number>
the days, months, years of the endgame*/
/*C_034C_0865*/void ParseConfig(char *s) {
	while(*s) {
		switch(toupper(*s++)) {
			case 'V': D_2B68 = 0; break;
			case 'E': D_2B68 = 1; break;
			case 'T': D_2B68 = 3; break;
			case 'M': D_2BAC = 1; break;
			case 'K': D_2BAC = 0; break;
			case 'N': case 'P': D_353E = 0; D_20CA = 0; break;
			case 'A': D_353E = 1; D_20CA = 2; break;
			case 'R': D_353E = 1; D_20CA = 1; break;
			case 'X': D_2B9C = 1; break;
			case 'L':
				if(toupper(*s) == 'A') {
					s++;
					endingChoice = 1;
				} else if(toupper(*s) == 'T') {
					s++;
					endingChoice = -1;
				}
			break;
			case 'D':
				D_2B98 = 1;
				for(D_2BB8 = 0; *s >= '0' && *s <= '9'; s++) {
					D_2BB8 *= 10;
					D_2BB8 += *s - '0';
				}
				s--;
			break;
			case 'B':
				D_2B98 = 1;
				for(D_2B2E = 0; *s >= '0' && *s <= '9'; s++) {
					D_2B2E *= 10;
					D_2B2E += *s - '0';
				}
				s--;
			break;
			case 'Y':
				D_2B98 = 1;
				for(D_2BCA = 0; *s >= '0' && *s <= '9'; s++) {
					D_2BCA *= 10;
					D_2BCA += *s - '0';
				}
				s--;
			break;
		}
	}
}

/*C_034C_09F7*/main(int argc, char **argv) {
	char *pal;
	char cfg[10];
	unsigned now[2];
	int i;
	int fd;

	D_2B9A = argv;
	setcbrk(0);
	ctrlbrk(Quit);
	gettime((struct time *)now);
	C_0BC5_0045(8);
	C_0D23_004B(now[1]);
	for(i = 0; i < now[1]; i++)
		C_0D23_0027(0, 2);
	D_2B38 = 0x182;
	D_2BA2 = 2;
	D_20CA = 3;
	D_2B66 = 1;
	D_2BDA = 1;
	D_2BAC = 0;
	D_353E = 0;
	D_2B9C = 0;
	D_2B98 = 0;
	FindDrives();
	fd = C_0C02_0000("config.se");
	if(fd >= 0) {
		C_0C06_000C(fd, 0L, 6L, cfg);
		cfg[6] = 0;
		ParseConfig(cfg);
		C_0C02_002D(fd);
	}
	for(i = 1; i < argc; i++)
		ParseConfig(argv[i]);
	if(C_0D46_01AA(D_2B68) == 0) {
		if(D_353E)
			C_08E4_17BE();
		C_0BC5_000B(8);
		C_0BC5_0079(8);
		exit(1);
	}
	pal = (char *)C_0C58_0305("savage.pal", 0, D_2BE4, 0, 0L);
	if(pal == 0 || pal != (char *)D_2BE4) {
		exitCode = 1;
		Fatal();
	}
	switch(D_2B68) {
		case 0: C_07C3_0005(0x13); textColor = 0x48; break;
		case 1: C_07C3_0005(0xd); textColor = 7; break;
		case 3: C_07C3_0005(9); textColor = 7; break;
	}
	C_0D28_000B(&D_2B1A, 0);
	C_0D28_01BC(&D_2B52, 0);
	D_2B3E = D_2B1A;
	D_2B4E = LoadLzc("savage.fnt", 0);
	if(D_2B4E == 0) {
		exitCode = 1;
		Fatal();
	}
	D_2B72.surface = &D_2B3E;
	D_2B72.curX = D_2B72.curY = 0;
	D_2B72.font = D_2B4E;
	D_2B72.fg = 0;
	D_2B72.bg = textColor;
	D_2B72.justify = 0;
	D_2B72.buffer = D_2B72.bufPtr = textBuffer;
	SetPalette(1);
	diskShapes = LoadLzc("create.lzc", 7);
	if(diskShapes == 0) {
		exitCode = 1;
		Fatal();
	}
	C_08E4_164A("savage.tim");
	if(LoadMusic() == 0)
		D_353E = 0;
	else
		D_353E = 1;
	if(max(1, 2 - C_0C97_00BE() / 8) != 1)
		D_2BA2 = 0;
	if(D_2B98 == 1)
		Endgame();
	else if(D_2B9C == 0) {
		InsertDisk(5);
		SetPalette(2);
		C_0636_0C6D(&D_2B52, &D_2B1A, D_2B68, D_353E ? titleSong : (void far *)0);
		SetPalette(1);
		Intro();
		FreeFar(&introShapes0);
		CheckSavedGame();
		MainMenu(1);
	} else {
		CheckSavedGame();
		MainMenu(0);
	}
}

/*the introduction: the jungle scrolling by in layers (parallax)*/
/*C_034C_0D1E*/void Intro() {
	int unused2;
	int x2;
	int x6;
	int unused8;
	int frame;
	int frame2;
	int x;
	int x3;

	unused2 = 0;
	x2 = 0;
	x3 = 0;
	x6 = 0;
	FlushKeys();
	unused2 = 0;
	x2 = 0;
	x3 = 0;
	x6 = 0;
	(*D_3516)(&D_2B1A, 0);
	frame = 1;
	unused8 = 0;
	(*D_3516)(&D_2B1A, 0);
	(*D_3516)(&D_2B52, 0);
	InsertDisk(6);
	introShapes0 = LoadLzc("intro.lzc", 0);
	if(introShapes0 == 0)
		Fatal();
	SetPalette(0);
	(*D_2BC6)(&D_2B52, 0, 0, introShapes0, 0);
	if(D_353E)
		C_08E4_19FF(introSong);
	FadeIn();
	(*D_2BC6)(&D_2B52, 0, 0, introShapes0, 1);
	FreeFar(&introShapes0);
	InsertDisk(6);
	introShapes = LoadLzc("intro.lzc", 1);
	if(introShapes == 0)
		Fatal();
	if(D_353E) {
		while(D_20CE < 1)
			if(kbhit())
				SkipIntro(0);
	} else
		Wait(1000);
	FadeOut();
	SetPalette(0);
	FadeIn();
	InsertDisk(6);
	introShapes2 = LoadLzc("intro.lzc", 2);
	if(introShapes2 == 0)
		Fatal();
	if(D_353E) {
		while(D_20CE < 2)
			if(kbhit())
				SkipIntro(0);
	} else
		Wait(1000);
	FadeOut();
	SetPalette(0);
	(*D_2BC6)(&D_2B52, 0, 0, introShapes, 0);
	(*D_2BC6)(&D_2B52, 0, 0, introShapes, 2);
	(*D_2BC6)(&D_2B52, 0, 0, introShapes, 5);
	FadeIn();
	IntroEnd();
	(*D_2BC6)(&D_2B52, 0, 0, introShapes, 0);
	(*D_2BC6)(&D_2B52, 0, 0, introShapes, 2);
	(*D_2BC6)(&D_2B52, 0, 0, introShapes, 5);
	(*D_34FE)(&D_2B52, &D_2B1A, 0);
	for(x = 0; x > -213; x--) {
		if(D_2BA2 == 1)
			D_2BA2 = 0;
		else {
			(*D_2BC6)(&D_2B52, x, 0, introShapes, 0);
			(*D_2BC6)(&D_2B52, x + 320, 0, introShapes, 1);
			(*D_2BC6)(&D_2B52, x2, 0, introShapes, 2);
			(*D_2BC6)(&D_2B52, x2 + 320, 0, introShapes, 3);
			(*D_2BC6)(&D_2B52, x2 + 640, 0, introShapes, 4);
			(*D_2BC6)(&D_2B52, x3, 0, introShapes, 5);
			(*D_2BC6)(&D_2B52, x3 + 320, 0, introShapes, 6);
			(*D_2BC6)(&D_2B52, x3 + 640, 0, introShapes, 7);
			frame = abs(x) / 35 + 1;
			frame2 = abs(x) % 35;
			if(frame2 < 15)
				(*D_2BC6)(&D_2B52, 0, 0, introShapes2, frame);
			C_07D7_0008();
			(*D_2BBA)(&D_2B52, &D_2B1A);
			if(D_2BA2 == 0)
				D_2BA2 = 1;
		}
		x2 -= 2;
		x3 -= 3;
		x6 -= 6;
		if(kbhit())
			SkipIntro(0);
	}
	x++;
	x2 += 2;
	x3 += 3;
	x6 += 6;
	(*D_2BC6)(&D_2B52, x, 0, introShapes, 0);
	(*D_2BC6)(&D_2B52, x + 320, 0, introShapes, 1);
	(*D_2BC6)(&D_2B52, x2, 0, introShapes, 2);
	(*D_2BC6)(&D_2B52, x2 + 320, 0, introShapes, 3);
	(*D_2BC6)(&D_2B52, x2 + 640, 0, introShapes, 4);
	(*D_2BC6)(&D_2B52, x6, 0, introShapes2, 0);
	(*D_2BC6)(&D_2B52, x3, 0, introShapes, 5);
	(*D_2BC6)(&D_2B52, x3 + 320, 0, introShapes, 6);
	(*D_2BC6)(&D_2B52, x3 + 640, 0, introShapes, 7);
	C_07D7_0008();
	(*D_2BBA)(&D_2B52, &D_2B1A);
	FreeFar(&introShapes2);
	FreeFar(&introShapes);
}

/*the introduction, continued*/
/*C_034C_1264*/void IntroEnd() {
	int i;

	for(i = 200; i >= 0; i -= 4) {
		if(D_2BA2 == 1)
			D_2BA2 = 0;
		else {
			(*D_2BC6)(&D_2B52, 0, 0, introShapes, 0);
			(*D_2BC6)(&D_2B52, 0, 0, introShapes, 2);
			(*D_2BC6)(&D_2B52, 0, i, introShapes2, 0);
			(*D_2BC6)(&D_2B52, 0, 0, introShapes, 5);
			C_07D7_0008();
			(*D_2BBA)(&D_2B52, &D_2B1A);
			if(D_353E && D_20CE >= 3)
				i -= 4;
			if(kbhit())
				SkipIntro(0);
		}
	}
	for(i = 0; i < 5; i++) {
		(*D_2BC6)(&D_2B52, 0, 0, introShapes, 0);
		(*D_2BC6)(&D_2B52, 0, 0, introShapes, 2);
		(*D_2BC6)(&D_2B52, 0, 0, introShapes2, 0);
		(*D_2BC6)(&D_2B52, 0, 0, introShapes, 5);
		(*D_2BC6)(&D_2B52, 0, 0, introShapes, i + 8);
		C_07D7_0008();
		(*D_2BBA)(&D_2B52, &D_2B1A);
		if(kbhit())
			SkipIntro(0);
	}
	for(i = 4; i >= 0; i--) {
		(*D_2BC6)(&D_2B52, 0, 0, introShapes, 0);
		(*D_2BC6)(&D_2B52, 0, 0, introShapes, 2);
		(*D_2BC6)(&D_2B52, 0, 0, introShapes2, 0);
		(*D_2BC6)(&D_2B52, 0, 0, introShapes, 5);
		(*D_2BC6)(&D_2B52, 0, 0, introShapes, i + 8);
		C_07D7_0008();
		(*D_2BBA)(&D_2B52, &D_2B1A);
		if(kbhit())
			SkipIntro(0);
	}
	for(i = 0; i < 7; i++) {
		if(D_353E && D_20CE >= 3)
			break;
		Wait(200);
	}
}

/*draw the main menu, choice sel highlighted (pressed: the pressed shape)*/
/*C_034C_14B8*/void DrawMenu(int sel, int pressed) {
	int i;

	(*D_2BC6)(&D_2B52, 0, 0, menuShapes, 0);
	for(i = 0; i <= menuLast; i++)
		if(sel != i)
			(*D_2BC6)(&D_2B52, 0x55, menuY[i], menuShapes, menuShape[i]);
		else
			(*D_2BC6)(&D_2B52, 0x55, menuY[i], menuShapes,
				pressed == 0 ? menuShape[i] + 1 : (menuShape[i] - 1) / 2 + 11);
}

/*wait for a key*/
/*C_034C_1537*/int WaitKey() {
	FlushKeys();
	return GetKey();
}

/*choose choice n of the main menu: show its button pressed*/
/*C_034C_1544*/int MenuPick(int *sel, int n) {
	*sel = n;
	DrawMenu(*sel, 1);
	C_07D7_0008();
	(*D_2BBA)(&D_2B52, &D_2B1A);
	return n;
}

/*the main menu: Create a character, Story so far (or Transfer), Acknowledgments,
Journey onward; fromTitle: it comes after the introduction*/
/*C_034C_1577*/void MainMenu(int fromTitle) {
	int sel;
	int choice;

	choice = -1;
	sel = 0;
	FlushKeys();
	InsertDisk(6);
	menuShapes = LoadLzc("create.lzc", 0);
	if(menuShapes == 0)
		Fatal();
	if(D_353E == -1)
		C_08E4_19FF(menuSong);
	if(D_3538 == 0)
		menuLast = 2;
	if(fromTitle == 0) {
		(*D_3516)(&D_2B1A, 0);
		SetPalette(0);
	}
	(*D_3516)(&D_2B52, 0);
	DrawMenu(sel, 0);
	if(fromTitle)
		(*D_34FE)(&D_2B52, &D_2B1A, 0);
	else
		FadeIn();
	for(;;) {
		choice = -1;
		while(choice == -1) {
			switch(WaitKey()) {
				case 0xd: case ' ':
					choice = MenuPick(&sel, sel);
				break;
				case 'C': case 'c':
					choice = MenuPick(&sel, 1);
				break;
				case 'S': case 's': case 'T': case 't':
					choice = MenuPick(&sel, 0);
				break;
				case 'J': case 'j':
					if(menuLast == 3)
						choice = MenuPick(&sel, 3);
				break;
				case 'A': case 'a':
					choice = MenuPick(&sel, 2);
				break;
				case 0x150:/*down*/
					if(++sel > menuLast)
						sel = 0;
					DrawMenu(sel, 0);
					C_07D7_0008();
					(*D_2BBA)(&D_2B52, &D_2B1A);
				break;
				case 0x148:/*up*/
					if(--sel < 0)
						sel = menuLast;
					DrawMenu(sel, 0);
					C_07D7_0008();
					(*D_2BBA)(&D_2B52, &D_2B1A);
				break;
				case 0x11: case 0x12d:/*^Q, alt-X*/
					Quit();
			}
		}
		switch(choice) {
			case 1:
				FadeOut();
				CreateCharacter();
				if(menuLast == 4)
					choice = 3;
				SetPalette(0);
				DrawMenu(choice, 0);
				FadeIn();
			break;
			case 0:
				FadeOut();
				Story(0);
				choice = 1;
				SetPalette(0);
				DrawMenu(choice, 0);
				FadeIn();
			break;
			case 2:
				Credits();
				DrawMenu(choice, 0);
				(*D_34FE)(&D_2B52, &D_2B1A, 0);
			break;
			case 3:
				StartGame();
			break;
		}
		sel = choice;
	}
}

/*the acknowledgments: the 13 shapes of create.lzc 6 scrolling up*/
/*C_034C_1829*/void Credits() {
	int y;
	int total;
	int i;
	int h;

	FlushKeys();
	(*D_2BC6)(&D_2B52, 0, 0, menuShapes, 0);
	(*D_34FE)(&D_2B52, &D_2B1A, 0);
	InsertDisk(6);
	creditShapes = LoadLzc("create.lzc", 6);
	if(creditShapes == 0)
		Fatal();
	for(i = 0; i < 13; i++)
		total += C_0753_0000(0, 0, creditShapes, i, 3);
	for(y = 197; y + total > 4; y -= 4) {
		(*D_2BC6)(&D_2B52, 0, 0, menuShapes, 0);
		D_2B52.left = 100;
		D_2B52.right = 244;
		D_2B52.top = 100;
		D_2B52.bottom = 199;
		h = 0;
		for(i = 0; i < 13; i++) {
			(*D_2BC6)(&D_2B52, 100, y + h, creditShapes, i);
			h += C_0753_0000(0, 0, creditShapes, i, 3);
		}
		D_2B52.left = 0;
		D_2B52.right = 319;
		D_2B52.top = 0;
		D_2B52.bottom = 199;
		C_07D7_0008();
		(*D_2BBA)(&D_2B52, &D_2B1A);
		if(kbhit()) {
			FlushKeys();
			break;
		}
	}
	FreeFar(&creditShapes);
}

/*wait for a key with a blinking cursor; mode 1 or more: echo it (2: and
put the cursor back); an extended key gives 0x100 + its code, in a char*/
/*C_034C_1984*/int InputKey(int mode) {
	char key;
	int fg;
	int ext;
	int x, y;

	key = 0;
	ext = 0;
	x = D_2B72.curX;
	y = D_2B72.curY;
	fg = D_2B72.fg;
	D_2B72.fg = 1;
	while(key == 0) {
		C_077E_000C(x, y);
		C_078E_0230("_");
		if(kbhit()) {
			if(!(key = getch())) {
				ext = 1;
				key = getch();
			}
		} else
			C_0CED_0106(238300L);
		(*D_2B90)(D_2B72.surface, x, y, x + 8, y + 8, D_2B72.bg);
		if(kbhit()) {
			if(!(key = getch())) {
				ext = 1;
				key = getch();
			}
		} else
			C_0CED_0106(238300L);
	}
	key = (ext << 8) + key;
	if(mode == 0)
		C_077E_000C(x, y);
	else if(mode > 1) {
		C_077E_000C(x, y);
		if(key < ' ' || key > 'z')
			C_078E_0230(" ");
		else
			C_078E_0230("%c", key);
		C_077E_000C(x, y);
	} else {
		C_077E_000C(x, y);
		if(key < ' ' || key > 'z')
			C_078E_0230(" ");
		else
			C_078E_0230("%c", key);
	}
	D_2B72.fg = fg;
	return key;
}

/*wait for a key, with a blinking ']'*/
/*C_034C_1AF6*/int WaitKeyPrompt() {
	char key;
	int fg;
	int x, y;

	key = 0;
	x = D_2B72.curX;
	y = D_2B72.curY;
	fg = D_2B72.fg;
	D_2B72.fg = 1;
	while(key == 0) {
		C_077E_000C(x, y);
		C_078E_0230("]");
		if(kbhit()) {
			if(!(key = getch()))
				key = getch();
		} else
			C_0CED_0106(238300L);
		(*D_2B90)(D_2B72.surface, x, y, x + 8, y + 8, D_2B72.bg);
		if(kbhit()) {
			if(!(key = getch()))
				key = getch();
		} else
			C_0CED_0106(238300L);
	}
	(*D_2B90)(D_2B72.surface, x, y, x + 8, y + 8, D_2B72.bg);
	C_077E_000C(x, y);
	D_2B72.fg = fg;
	return key;
}

/*character creation: Intanya's hut, his questions, saving the character*/
/*C_034C_1BEC*/void CreateCharacter() {
	int unused2 = 0;
	int unused4 = 0;
	char msg[] = "\n\nCheck your reference card for details.";
	int q;

	FlushKeys();
	D_2BCC = C_0D23_0027(0, 2) + 16;
	D_2BCD = C_0D23_0027(0, 2) + 16;
	D_2BCE = C_0D23_0027(0, 2) + 16;
	(*D_3516)(&D_2B1A, 0);
	SetPalette(1);
	(*D_3516)(&D_2B52, 0);
	InsertDisk(6);
	createShapes1 = LoadLzc("create.lzc", 1);
	if(createShapes1 == 0)
		Fatal();
	createShapes2 = LoadLzc("create.lzc", 2);
	if(createShapes2 == 0)
		Fatal();
	createShapes3 = LoadLzc("create.lzc", 3);
	if(createShapes3 == 0)
		Fatal();
	createShapes4 = LoadLzc("create.lzc", 4);
	if(createShapes4 == 0)
		Fatal();
	D_201E = &D_2B72;
	D_2B3E.left = 0xb1;
	D_2B3E.top = 0x20;
	D_2B3E.right = 0x132;
	D_2B3E.bottom = 0xa2;
	D_2B72.curX = 0xb1;
	D_2B72.curY = 0x20;
	(*D_2BC6)(&D_2B52, 0, 0, createShapes1, 2);
	(*D_2BC6)(&D_2B52, 8, 0x1c, createShapes1, 0);
	(*D_2BC6)(&D_2B52, 8, 0x1c, createShapes1, 1);
	(*D_3534)(&D_2B52, 0xaf, 0x1e, createShapes4, 2, 0);
	(*D_2BC6)(&D_2B52, 0xaa, 0x19, createShapes4, 2);
	FadeIn();
	ResetText(-1);
	if(D_2818 == 0) {
		C_078E_0252("You must first run the INSTALL program.\n\nCheck your reference card for details.%P");
		WaitKeyPrompt();
	} else if(D_34EC == -1) {
		C_078E_0252("A saved game is already in progress. To create a new character, you must first run the INSTALL program.%s%P");
		WaitKeyPrompt();
	} else if(Awakening()) {
		D_2BCF = playerName;
		q = C_0D23_0027(0, 2);
		if(AskQuestion(AskQuestion(AskQuestion(q, 0), 1), 2) != 6 && SaveCharacter()) {
			D_3538 = -1;
			menuLast = 3;
		}
	}
	ResetText(D_2B72.bg);
	FadeOut();
	FreeFar(&createShapes1);
	FreeFar(&createShapes2);
	FreeFar(&createShapes3);
	FreeFar(&createShapes4);
}

/*a character a name cannot have*/
/*C_034C_1E99*/int BadNameChar(int c) {
	if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ' ' || c == '.' || c == '-' || c == '\'')
		return 0;
	return 1;
}

/*input the name (at most 13 characters) into playerName; 0 if cancelled
or empty*/
/*C_034C_1EDC*/int InputName() {
	int y;
	int done;
	int key;
	int len;
	int x;

	len = 0;
	done = 0;
	playerName[0] = 0;
	x = D_2B72.curX;
	y = D_2B72.curY;
	while(done == 0) {
		FlushKeys();
		playerName[len + 1] = 0;
		key = InputKey(1);
		if(key == 8 || key == 0x14b) {
			(*D_2B90)(D_2B72.surface, x, y, D_2B72.surface->right, y + 8, D_2B72.bg);
			C_077E_000C(x, y);
			if(len > 0) {
				len--;
				x -= C_0780_0004(playerName[len]);
			}
			(*D_2B90)(D_2B72.surface, x, y, x + 8, y + 8, D_2B72.bg);
			C_077E_000C(x, y);
			playerName[len] = 0;
		} else if(key == 0xd) {
			done = 1;
		} else if(key == 0x1b)
			return 0;
		else if(key < ' ' || key > 'z' || BadNameChar(key)) {
			(*D_2B90)(D_2B72.surface, x, y, x + 8, y + 8, D_2B72.bg);
			C_077E_000C(x, y);
			playerName[len] = 0;
		} else if(len < 13) {
			playerName[len] = key;
			len++;
			x += C_0780_0004(key);
		} else {
			(*D_2B90)(D_2B72.surface, x, y, x + 8, y + 8, D_2B72.bg);
			C_077E_000C(x, y);
			playerName[len] = 0;
		}
	}
	if(strcmp(playerName, "") == 0)
		return 0;
	return 1;
}

/*you wake up in Intanya's hut; the name; 0 if escaped*/
/*C_034C_1FFD*/int Awakening() {
	FlushKeys();
	ResetText(D_2B72.bg);
	C_078E_0252("You are in a dirt-floored hut... and a tribesman with wise eyes stares down at you.\n%P");
	C_078E_0252("\nYou feel as though you are floating. You cannot move your limbs. It is like a dream... yet you sense it is not a dream.\n%P");
	if(WaitKeyPrompt() == 0x1b)
		return 0;
	ResetText(D_2B72.bg);
	C_078E_0252("The man speaks: \"Yes, warrior, you can hear. You see Intanya, shaman and healer. Intanya will heal you...\n%P");
	C_078E_0252("but to concoct his healing potion, he must ask you questions.&\n%P");
	if(WaitKeyPrompt() == 0x1b)
		return 0;
	ResetText(D_2B72.bg);
	C_078E_0252("\"In order to heal your spirit, Intanya must know your spirit, so you must answer with truthful words.&\n%P");
	C_078E_0252("\n\"Intanya will speak of deeds and choices; you must answer with the choices you would make.&\n%P");
	if(WaitKeyPrompt() == 0x1b)
		return 0;
	ResetText(D_2B72.bg);
	C_078E_0252("\"First, though, Intanya must know the warrior's name, for there is much power in names.\n\n\"What does the warrior call himself?&\n%P");
	if(InputName() == 0) {
		ResetText(D_2B72.bg);
		return 0;
	}
	ResetText(D_2B72.bg);
	return 1;
}

/*ask question q (the third if last); returns the next question, 6 if
escaped*/
/*C_034C_20E0*/int AskQuestion(int q, int last) {
	char key;
	int virtue;

	if(q == 6)
		return 6;
	FlushKeys();
	ResetText(D_2B72.bg);
	C_078E_0252("%s%P", questions[q].deed);
	if(questions[q].deed2) {
		C_078E_0252("\n%s%P", questions[q].deed2);
		if(questions[q].choice2) {
			WaitKeyPrompt();
			ResetText(D_2B72.bg);
			C_078E_0252("%s%P", questions[q].choice);
			C_078E_0252("\n%s%P", questions[q].choice2);
		} else {
			C_078E_0252("\n%s%P", questions[q].choice);
		}
	} else {
		C_078E_0252("\n%s%P", questions[q].choice);
		if(questions[q].choice2)
			C_078E_0252("\n%s%P", questions[q].choice2);
	}
	C_078E_0230("\n");
	key = -1;
	while(key != 0x1b && key != 'A' && key != 'B')
		key = toupper(InputKey(2));
	switch(key) {
		case 'A':
			AnimateIntanya(RaiseVirtue(virtue = questions[q].virtueA));
		break;
		case 'B':
			AnimateIntanya(RaiseVirtue(virtue = questions[q].virtueB));
		break;
		case 0x1b:
			return 6;
	}
	switch(virtue) {
		case 0:
			return last == 0 ? 2 : (q == 0 ? 4 : 3);
		break;
		case 1:
			return last == 0 ? 0 : (q == 2 ? 4 : 5);
		break;
		case 2:
			return last == 0 ? 1 : (q == 0 ? 5 : 3);
	}
}

/*raise a virtue by 3 to 5; returns the animation to play*/
/*C_034C_22F5*/int RaiseVirtue(int v) {
	switch(v) {
		case 2:
			D_2BCE += C_0D23_0027(0, 2) + 3;
			return 0;
		case 1:
			D_2BCD += C_0D23_0027(0, 2) + 3;
			return 2;
		case 0:
			D_2BCC += C_0D23_0027(0, 2) + 3;
			return 1;
	}
}

/*Intanya adds an ingredient n to the potion*/
/*C_034C_2358*/void AnimateIntanya(int n) {
	int i;

	(*D_2BBA)(&D_2B1A, &D_2B52);
	for(i = 0; i < 6; i++) {
		(*D_2BC6)(&D_2B52, 8, 0x1c, createShapes1, 0);
		(*D_2BC6)(&D_2B52, 8, 0x1c, createShapes2, i);
		(*D_2BC6)(&D_2B52, 8, 0x1c, createShapes1, 1);
		C_07D7_0008();
		(*D_2BBA)(&D_2B52, &D_2B1A);
		if(i == 0 || i == 5)
			C_0CED_0106(95320L);/*40 ms*/
		else if(i == 1 || i == 4)
			C_0CED_0106(71490L);/*30 ms*/
		else
			C_0CED_0106(47660L);/*20 ms*/
	}
	for(i = 0; i < 17; i++) {
		(*D_2BC6)(&D_2B52, 8, 0x1c, createShapes1, 0);
		(*D_2BC6)(&D_2B52, 8, 0x1c, createShapes2, 5);
		(*D_2BC6)(&D_2B52, 8, 0x1c, createShapes3, n + 17);
		(*D_2BC6)(&D_2B52, 8, 0x1c, createShapes3, i);
		(*D_2BC6)(&D_2B52, 8, 0x1c, createShapes1, 1);
		C_07D7_0008();
		(*D_2BBA)(&D_2B52, &D_2B1A);
		if(i < 5)
			C_0CED_0106((24 - i * 6) * 2383L);
		else
			C_0CED_0106(35745L);/*15 ms*/
	}
	for(i = 5; i >= 0; i--) {
		(*D_2BC6)(&D_2B52, 8, 0x1c, createShapes1, 0);
		(*D_2BC6)(&D_2B52, 8, 0x1c, createShapes2, i);
		(*D_2BC6)(&D_2B52, 8, 0x1c, createShapes1, 1);
		C_07D7_0008();
		(*D_2BBA)(&D_2B52, &D_2B1A);
		if(i == 0 || i == 5)
			C_0CED_0106(83405L);/*35 ms*/
		else if(i == 1 || i == 4)
			C_0CED_0106(59575L);/*25 ms*/
		else
			C_0CED_0106(35745L);/*15 ms*/
	}
}

/*write the new character into savegame\objlist; 0 on error*/
/*C_034C_25BC*/int SaveCharacter() {
	char far *buf;
	int i;
	int fd;

	buf = C_0BC5_00AD(0x1d73L, 0);
	InsertDisk(8);
	fd = C_0C02_0000("savegame\\objlist");
	C_0C06_000C(fd, 0L, 0x1d73L, buf);
	if(D_240C) {
		C_0C02_002D(fd);
		return 0;
	}
	buf[0x901] = D_2BCC;
	buf[0xa01] = D_2BCD;
	buf[0xb01] = D_2BCE;
	buf[0x14f2] = D_2BCD;
	buf[0x1d71] = 0;
	buf[0xe01] = D_2BCC * 2 + D_2BCE;
	buf[0x1d13] = 0;
	buf[0x1d14] = 8;
	buf[0x1d15] = 1;
	buf[0x1d16] = 1;
	buf[0x1d17] = buf[0x1d18] = 0;
	buf[0x1d72] = 1;
	for(i = 0; i < 13; i++) {
		buf[0xf00 + i] = D_2BCF[i];
		if(D_2BCF[i] == 0)
			break;
	}
	buf[0xf0d] = 0;
	buf[0x18f3] |= 1;
	buf[0x18f5] |= 1;
	buf[0x18f8] |= 1;
	buf[0x18fa] |= 1;
	buf[0x18f9] |= 1;
	buf[0x1908] |= 1;
	buf[0x18f6] |= 1;
	C_0C12_0000(fd, 0L, 0x1d73L, buf);
	C_0C02_002D(fd);
	return 1;
}

/*look at savegame\objlist: is a game in progress, has a character been
created*/
/*C_034C_2720*/void CheckSavedGame() {
	int r;

	r = GameInProgress();
	if(r < 0)
		D_2818 = 0;
	else
		D_2818 = -1;
	if(r > 0)
		D_34EC = -1;
	else
		D_34EC = 0;
	r = CharacterCreated();
	if(r < 0)
		D_2818 = 0;
	else
		D_2818 = -1;
	if(r > 0)
		D_3538 = -1;
	else
		D_3538 = 0;
}

/*a game is in progress (objlist's flag at 0x1912); -1 if it cannot be read*/
/*C_034C_277D*/int GameInProgress() {
	char far *buf;
	int fd;

	buf = C_0BC5_00AD(0x1d73L, 0);
	InsertDisk(8);
	fd = C_0C02_0000("savegame\\objlist");
	C_0C06_000C(fd, 0L, 0x1d73L, buf);
	if(D_240C) {
		C_0C02_002D(fd);
		return -1;
	}
	if(buf[0x1912] & 1) {
		C_0C02_002D(fd);
		return 1;
	}
	C_0C02_002D(fd);
	return 0;
}

/*a character has been created (objlist's byte at 0x1d72); -1 if it cannot
be read*/
/*C_034C_27FC*/int CharacterCreated() {
	char far *buf;
	int fd;

	buf = C_0BC5_00AD(0x1d73L, 0);
	InsertDisk(8);
	fd = C_0C02_0000("savegame\\objlist");
	C_0C06_000C(fd, 0L, 0x1d73L, buf);
	if(D_240C) {
		C_0C02_002D(fd);
		return -1;
	}
	if(buf[0x1d72] == 1) {
		C_0C02_002D(fd);
		return 1;
	}
	C_0C02_002D(fd);
	return 0;
}

/*the story so far*/
/*0623*/char *storyTexts[10] = {
	"You had disturbing dreams of a faraway jungle, an endangered princess, and the black moonstone you carried away from Britannia in your last adventure there.",
	"The spirit of Lord British commanded you to find out all you could about the stone.",
	"So you took it to your old friend Dr. Rafkin, curator of the local museum of natural history, hoping that he could unravel the mystery.",
	"At Dr. Rafkin's, you met ace reporter and \"Ultimate Adventures& correspondent Jimmy Malone. Malone's a little too nosy to suit you...",
	"...but he kept his wits when everything went haywire, when Dr. Rafkin's experiments on your moonstone sent the whole museum lab through a bizarre black moongate!",
	"You were shocked to lift your head and see jungle around you: Jungle, and this enormous creature descending... descending upon--",
	"Her! The woman you dreamt of! A fiery tribal she-warrior of some time-lost land... You and your friends rescued her from the giant pteranodon and befriended her.",
	"She said her name was Aiela, princess of the Kurak tribe, and seemed to be quite taken with you, her most unusual rescuer.",
	"But Aiela's hated suitor, Darden the Huge, prince of the Urali tribe, arrived with his men, scattering your friends and smashing you into the earth.",
	"Now you lie broken and defeated, struggling to regain consciousness..."
};

/*the story so far: ten pictures and texts*/
/*C_034C_287B*/void Story(int unused) {
	int i;

	InsertDisk(6);
	storyShapes = LoadLzc("create.lzc", 5);
	if(storyShapes == 0)
		Fatal();
	(*D_3516)(&D_2B1A, 0);
	SetPalette(1);
	(*D_3516)(&D_2B52, 0);
	(*D_2BC6)(&D_2B52, 0, 0, storyShapes, 0);
	FadeIn();
	D_201E = &D_2B72;
	D_2B3E.left = 0x19;
	D_2B3E.top = 0x88;
	D_2B3E.right = 0x125;
	D_2B3E.bottom = 0xb3;
	D_2B72.curX = 0x19;
	D_2B72.curY = 0x88;
	FlushKeys();
	for(i = 0; i < 10; i++) {
		ResetText(D_2B72.bg);
		(*D_2BC6)(&D_2B1A, 0x78, 0x2d, storyShapes, i + 1);
		C_078E_0252("%s%P", storyTexts[i]);
		if(WaitKeyPrompt() == 0x1b) {
			FadeOut();
			FreeFar(&storyShapes);
			FlushKeys();
			return;
		}
	}
	FadeOut();
	FreeFar(&storyShapes);
}

/*print "one unit" or "n units"*/
/*C_034C_2992*/void PrintCount(int n, char *unit) {
	if(n == 1)
		C_078E_0230("one %s", unit);
	else
		C_078E_0230("%d %ss", n, unit);
}

/*the endgame texts*/
/*0637*/char *endTexts[16] = {
	"Members of all the tribes come for the feast... the largest and most important feast ever held in the Valley of Eodon.",
	"Kurak sits beside Yolaru, Pindiro beside Barako, Haakur beside Sakkhra, and peace is sworn between the tribes.",
	"You know this peace isn't for you. Soon enough, you will hear the call to action again. But tonight, there is time for music and dance, friendship and love.",
	"At dawn, your past catches up with you... and your future beckons. A vision of Lord British appears before you.",
	"\"You have done well,& he says. \"But now I must take you where you are needed. For the sake of the friends you leave behind, I am sorry. Prepare yourself.&",
	"You say farewell to Aiela. \"Aiela does not know your world. Take her there,& she pleads. \"Teach her of your world.&",
	"You shake your head. \"My world would strangle you. You must stay. My heart remains with you. But my duty is elsewhere... with him.&",
	"Tears roll down her cheeks. \"Abandon duty, and you will not be the warrior Aiela loves. Choose duty, and you must leave. Either way, Aiela loses all... Farewell.&",
	"You say farewell to Tristia. \"I must go with Lord British,& you say. \"My duty lies with him, though it breaks my heart.&",
	"Tristia laughs sweetly. \"Tristia does not mind,& she says. \"Tristia has found another love: handsome Botorok. Botorok is so much better than you. Go with your chief. Go.&",
	"You are joined by Spector's former assistant, Fritz, who came out of hiding to fight the Myrmidex, and bears their scars.",
	"The moongate appears, summoned by the shade of Lord British. Jimmy, Spector and Fritz gather their belongings...",
	"But Rafkin does not. \"I'm staying, my friend,& he says. \"Someone must. I will stay here... and hope other scientists come. Farewell.&",
	"Saddened, you follow your friends and allies into the moongate, leaving the Valley of Eodon. Perhaps someday you will return to those you have left behind.",
	"CONGRATULATIONS. You have completed THE SAVAGE EMPIRE in ",
	"Communicate your success to Lord British at Origin!"
};

/*the endgame (D, B, Y on the command line: the time it took), then quit*/
/*C_034C_29C2*/void Endgame() {
	int state;

	InsertDisk(7);
	endShapes = LoadLzc("endgame.lzc", 0);
	if(endShapes == 0)
		Fatal();
	if(D_353E == -1)
		C_08E4_19FF(menuSong);
	(*D_3516)(&D_2B1A, 0);
	SetPalette(1);
	(*D_3516)(&D_2B52, 0);
	(*D_2BC6)(&D_2B52, 0, 0, endShapes, 0);
	FadeIn();
	D_201E = &D_2B72;
	D_2B3E.left = 0x19;
	D_2B3E.top = 0x88;
	D_2B3E.right = 0x125;
	D_2B3E.bottom = 0xb3;
	D_2B72.curX = 0x19;
	D_2B72.curY = 0x88;
	ResetText(D_2B72.bg);
	C_078E_0252("%s%P", endTexts[0]);
	WaitKeyPrompt();
	ResetText(D_2B72.bg);
	C_078E_0252("%s%P", endTexts[1]);
	WaitKeyPrompt();
	ResetText(D_2B72.bg);
	C_078E_0252("%s%P", endTexts[2]);
	WaitKeyPrompt();
	(*D_3516)(&D_2B52, 0);
	(*D_2BC6)(&D_2B52, 0, 0, endShapes, 1);
	C_07D7_0008();
	(*D_2BBA)(&D_2B52, &D_2B1A);
	ResetText(D_2B72.bg);
	C_078E_0252("%s%P", endTexts[3]);
	WaitKeyPrompt();
	ResetText(D_2B72.bg);
	C_078E_0252("%s%P", endTexts[4]);
	WaitKeyPrompt();
	if(endingChoice > 0) {
		(*D_3516)(&D_2B52, 0);
		(*D_2BC6)(&D_2B52, 0, 0, endShapes, 2);
		C_07D7_0008();
		(*D_2BBA)(&D_2B52, &D_2B1A);
		ResetText(D_2B72.bg);
		C_078E_0252("%s%P", endTexts[5]);
		WaitKeyPrompt();
		ResetText(D_2B72.bg);
		C_078E_0252("%s%P", endTexts[6]);
		WaitKeyPrompt();
		ResetText(D_2B72.bg);
		C_078E_0252("%s%P", endTexts[7]);
		WaitKeyPrompt();
	} else if(endingChoice < 0) {
		(*D_3516)(&D_2B52, 0);
		(*D_2BC6)(&D_2B52, 0, 0, endShapes, 3);
		C_07D7_0008();
		(*D_2BBA)(&D_2B52, &D_2B1A);
		ResetText(D_2B72.bg);
		C_078E_0252("%s%P", endTexts[8]);
		WaitKeyPrompt();
		ResetText(D_2B72.bg);
		C_078E_0252("%s%P", endTexts[9]);
		WaitKeyPrompt();
	}
	(*D_3516)(&D_2B52, 0);
	(*D_2BC6)(&D_2B52, 0, 0, endShapes, 4);
	C_07D7_0008();
	(*D_2BBA)(&D_2B52, &D_2B1A);
	ResetText(D_2B72.bg);
	C_078E_0252("%s%P", endTexts[10]);
	WaitKeyPrompt();
	ResetText(D_2B72.bg);
	C_078E_0252("%s%P", endTexts[11]);
	WaitKeyPrompt();
	ResetText(D_2B72.bg);
	C_078E_0252("%s%P", endTexts[12]);
	WaitKeyPrompt();
	ResetText(D_2B72.bg);
	C_078E_0252("%s%P", endTexts[13]);
	WaitKeyPrompt();
	FadeOut();
	(*D_3516)(&D_2B1A, 0);
	SetPalette(1);
	(*D_3516)(&D_2B52, 0);
	(*D_2BC6)(&D_2B52, 0, 0, endShapes, 1);
	FadeIn();
	ResetText(D_2B72.bg);
	C_078E_0252("%s%P", endTexts[14]);
	state = 0;
	if(D_2BB8 && D_2B2E && D_2BCA) {
		PrintCount(D_2B2E, "month");
		C_078E_0230(",\n");
		PrintCount(D_2BB8, "day");
		C_078E_0230(" and ");
		PrintCount(D_2BCA, "year");
	} else {
		if(D_2B2E) {
			PrintCount(D_2B2E, "month");
			state = 1;
		}
		if(D_2BB8) {
			if(state == 1) {
				C_078E_0230("\nand ");
				state = 2;
			}
			PrintCount(D_2BB8, "day");
			if(state < 2)
				state = 1;
		}
		if(D_2BCA) {
			if(state == 1) {
				C_078E_0230("\nand ");
				state = 2;
			}
			PrintCount(D_2BCA, "year");
		}
	}
	if(state != 2)
		C_078E_0230(".\n");
	else
		C_078E_0230(". ");
	C_078E_0252("%s%P", endTexts[15]);
	WaitKeyPrompt();
	Quit();
}
