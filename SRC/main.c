/*
 * The Savage Empire (Worlds of Ultima) - GAME.EXE
 * Main/startup module, from Ultima VI's main.c.
 *
 * main() parses the configuration (CONFIG.SE + command line), sets up the
 * memory manager of Savage Empire's library (MEM_init, conventional or EMS
 * memory) and a work area for the .lzc loader, the mouse and the screen
 * surfaces (scanline offset tables for VGA/EGA/Tandy/CGA/Hercules), loads
 * the graphic driver, the palette, the font (system.lzc), the main screen,
 * the tile tables and animation data, loads the saved game and runs the game
 * loop (GameLoop). Without a character it goes back to SAVAGE.EXE.
 * Also here: the allocator __MemAlloc() (zeroed blocks from MEM_alloc), the
 * loader callback given to the graphic driver (LoadFileGr), the OSI file
 * error handler (FileErrorHandler) and the VGA palette cycling for the
 * fire/water colors (PaletteAnimation).
 */
#include "se.h"

/*text window (Ultima VI's)*/
struct tClipInfo/*size:9*/ {
	unsigned char originX, originY, cols, rows;
	unsigned char curX, curY;
	unsigned char fgColor, bgColor;
	unsigned char attrFlags;/*1 wrap, 2 center, 4 capitalize, 8 runes, 0x10 paginate, 0x20 scroll*/
};

/*a drawing surface of the mouse code (Ultima VI's)*/
struct tSurface {
	uint16 segment;/*_00*/
	uint16 far *pScanlines;/*_02*/
	uint16 right, left, top, bottom;/*_06,_08,_0a,_0c*/
};

/*the mouse state (Ultima VI's)*/
struct tMouse {
	unsigned char buttonL, buttonR;
	int16 posX, posY;
	uint16 unused06;
	void far *shapes;/*_08 pointer shapes*/
	uint16 shapeIndex;
	struct tSurface *surface;/*_0e*/
	void far *saveBuf;/*_10 background save area*/
	void (far *onMove)();/*_14*/
	unsigned char installed;
};

/*the graphic driver's state (Ultima VI's tGr)*/
struct tGr {
	unsigned char header[6];/*_00*/
	void far *font;/*_06*/
	unsigned char textFg, textBg;
	unsigned char xorMode;
	unsigned char unknown0d;
	unsigned char animOn;
	unsigned char toBackBuffer;
	void (*pfnDiskConfirm)();/*_10*/
	unsigned char reserved14[8];
	void far *pWorkBuf;/*_1c*/
	void far *pDriverMem;/*_20*/
};

/*Turbo C's in-line port output (TC++ 1.00's dos.h makes outportb a call)*/
void cdecl __outportb__(int portid, unsigned char value);
#undef outportb
#define outportb(portid, value) __outportb__(portid, value)

/*a palette entry*/
struct tRGB {
	unsigned char content[3];
};

/*the graphic driver: D_ED10's offset selects the entry*/
#define GR_SHUTDOWN()             (D_ED10.iii.ofs = 0x03, (*D_ED10.f)())
#define GR_INIT_PALETTE()         (D_ED10.iii.ofs = 0x24, (*D_ED10.f)())
#define GR_CYCLE_PALETTE(a)       (D_ED10.iii.ofs = 0x27, (*D_ED10.f)(a))
#define GR_INIT_MEMORY(seg,size)  (D_ED10.iii.ofs = 0x2a, (*D_ED10.f)(seg,size))
/*0x30: draw a bitmap on a surface (surface, 0, 0, bitmap, 0)*/
#define GR_DRAW_BITMAP(s,a,b,p,c) (D_ED10.iii.ofs = 0x30, (*D_ED10.f)(s,a,b,p,c))

/*OSI file routines (asm)*/
extern union tHandler D_01F6;/*OSI_ErrorHandler*/
extern int16 D_01FA;/*OSI_ErrorCode*/
extern int16 D_01FC;/*OSI_ErrorFunc*/
extern char *D_01FE;/*OSI_ErrorFile*/
extern int16 C_0000_0245(char *);/*OSI_open*/
extern C_0000_0284(int16);/*OSI_close*/
extern int32 C_0000_0336(int16, int32, int32, void far *);/*OSI_read*/
extern int32 C_0000_0617(int16);/*OSI_fileSize*/
extern int16 D_2B7C;/*OSI_Started*/
extern C_2AFC_000A();/*OSI_lowInit?*/
extern int16 C_2B57_001F(int16, int16);/*OSI_rand(lo, hi)*/
extern C_2B57_0044(int16);/*OSI_randomize*/

/*other game modules*/
extern char D_03AE;/*AnimateAtPrompt*/
extern struct tClipInfo *D_03EC;/*CurClip*/
extern int16 D_2362;
extern int16 D_2810;/*music on*/
extern char D_2869;/*AvatarPortrait: 0 = no character*/
extern C_0359_1A65();/*GameLoop*/
extern far C_05D0_01F5();/*CON_UpdateMousePointer*/
extern C_05D0_047A();/*GAME_Load*/
extern int16 C_05D0_22C8();/*CON_getch*/
extern C_05D0_2562(struct tClipInfo *, int16, int16, int16, int16);/*CON_createClip*/
extern C_05D0_25AD(struct tClipInfo *);/*CON_setClip*/
extern cdecl C_05D0_2DB9(char far *, ...);/*CON_printf*/
extern C_08CB_0517();/*MAP_InitChunkCache*/
extern C_0A5D_25CC();/*OBJ_DeleteTmpBlocks*/
extern C_0A5D_38A9();/*OBJ_InitLists*/
extern C_2701_000A();
extern C_2701_0190(int16);
extern C_2701_01F1();
extern C_276B_0005(int16);/*InitGraphicsDriver*/
extern int16 C_29D1_000E(int16);/*KBD_GetKey*/
extern C_2A81_0098(int16);/*DISK_confirm*/
extern C_2A81_0255();/*DISK_full*/
extern C_2A81_02A6();/*DISK_init*/
extern cdecl C_2ABF_0008(struct tMouse *);/*MOUSE_INIT*/
extern cdecl C_2ABF_005E(void far *, int16);/*MOUSE_POINTER*/
extern cdecl C_2ABF_013D();/*MOUSE_SHOW*/

/*Savage Empire's library*/
extern int16 soundCard;
extern int16 osiError;
extern char far *lzcWorkArea;
extern cdecl MEM_init(int16);
extern cdecl MEM_shutdown(int16);
extern void far * cdecl MEM_alloc(int32 size, int16 flags);
extern cdecl MEM_free(void far *);
extern int32 cdecl MEM_largestFree(int16);
extern cdecl ShutdownSound(void);
extern int32 LIB_entrySize(char far *name, int16 index);
extern void far *LZC_load(char far *name, int16 index, void far *out, int16 memflags, char far *work);
extern int32 PtrToLinear(void far *);
extern void far *LinearToPtr(int32);

/*uninitialised data*/
extern struct tSurface D_3268;/*Screen*/
extern unsigned char D_3276[0x20];/*AnimFrameMask*/
extern int16 D_3296;/*free memory for the driver, in KB*/
extern char far *D_3298;/*0x100 bytes*/
extern struct tMouse D_3E9C;/*Mouse*/
extern char far *D_3EB6;/*work area of the .lzc loader (0x3500 bytes)*/
extern char far *D_45FA;/*0x100 bytes*/
extern char far *D_45FE;/*0x100 bytes (PathData?)*/
extern char far *D_4602;/*0x100 bytes*/
extern char far *D_4706;/*0x100 bytes (NPCMode?)*/
extern uint16 D_4D4A[200];/*ScreenScanlines*/
extern char far *D_4EEC;/*0x100 bytes (Level?)*/
extern char far *D_4EF0;/*3000 bytes (Schedule?)*/
extern char far *D_50F6;/*ScratchBuf: 0x7900 bytes, also the map view back buffer*/
extern uint16 D_50FA;/*GrMemorySeg*/
extern char far *D_50FC;/*PointerShapes*/
extern char far *D_5100;/*0x100 bytes*/
extern char far *D_53E4;/*0x100 bytes*/
extern char far *D_54E8;/*ScratchBuf + 0x36b0*/
extern uint32 D_54EC;/*Mem_Remain: memory given to the graphic driver*/
extern char far *D_6DF0;/*TileFlag*/
extern char far *D_6EF4;/*BaseTile*/
extern char far *D_6EF8;/*ScratchBuf, again*/
extern char far *D_763C;/*TypeWeight*/
extern char far *D_7640;/*TileFlag2*/
extern int16 D_7744[0x20];/*AnimFirstFrame*/
extern uint16 D_7784[176];/*MapScanlines*/
extern struct tSurface D_9CE4;/*MapSurface*/
extern void huge *D_9CF2;/*GrMemory*/
extern int16 D_9CF8;/*NumAnimData*/
extern char far *D_B4FA;/*TerrainType*/
extern int16 D_B4FE[0x20];/*AnimTile*/
extern int16 far *D_B53E;/*0x100 words (OrigShapeType?)*/
extern unsigned char D_B542[0x20];/*AnimFrameShift*/
extern struct tRGB far *D_B562;/*VGAPalette*/
extern struct tClipInfo D_B644[4];/*TextWindows*/
extern char far *D_BAFA;/*0x600 bytes (ChunkIndexBuf?)*/
extern struct tGr far *D_ED0C;/*GrInfo*/
extern union tHandler D_ED10;/*GrDriver*/

/*====---- ----====*/
/*00A8*/int16 IsZFile = 0;/*Ultima VI's LoadFile() flag (LZW files); not used in this module*/
/*00AA*/char *CompressedFileList[] = {
	""
};
/*00AC*/char DirIncrX[] = { 0, 1, 1, 1, 0,-1,-1,-1};
/*00B4*/char DirIncrY[] = {-1,-1, 0, 1, 1, 1, 0,-1};
/*00BC*/char *DirNames[] = {
	"North",
	"Northeast",
	"East",
	"Southeast",
	"South",
	"Southwest",
	"West",
	"Northwest"
};
/*00CC*/int16 Unused00CC = 0;
/*00CE*/int16 Unused00CE = 0;
/*00D0*/int16 IgnoreShortRead = 1;/*1: ignore "short read" errors (end of file)*/
/*00D2*/int16 GrAdapter = 0;/*graphic adapter: 0 VGA/MCGA, 1 EGA, 2 Tandy (3 CGA, 4 Hercules: Ultima VI's)*/
/*00D4*/int16 MusicAdapter = 0;/*1: AdLib or Roland, 0: none*/
/*00D6*/int16 GrDriverOption = 0;/*'$' config flag*/
/*00D8*/unsigned char MouseOn = 1;
/*00D9*/char ConsoleReady = 0;/*1 once the console can display messages (error handler)*/
/*00DA*/unsigned char PaletteCycle = 0;/*palette cycling counter*/
/*00DB*/char GrAdapterLetters[] = "vetch";/*config letters for GrAdapter*/
/*00E1*/char MusicAdapterLetters[] = "narp";/*config letters (None/Adlib/Roland/Pc speaker); indexed by MusicAdapter, so only n/a come out*/
/*====---- ----====*/

/*allocate "size" zeroed bytes with MEM_alloc; exits to DOS if there is
  not enough memory*/
/*C_0268_000B*/static __MemAlloc(char huge **pptr, uint32 size) {
	uint32 offset;

	if(MEM_largestFree(0) < size) {
		/*not enough memory*/
		GR_SHUTDOWN();
		ShutdownSound();
		MEM_shutdown(8);
		exit(1);
		return;
	}

	*pptr = MEM_alloc(size, 0);
	/*zero memory*/
	for(offset = 0; offset < size; offset ++)
		*(*pptr + offset) = 0;
}

/*file loader callback given to the graphic driver: the file name is a far
  string (copied locally); isZ: an .lzc file, loaded with LZC_load.
  Returns nothing (pRetBuff is set but unused)*/
/*C_0268_00AA*/LoadFileGr(char far *fn, void far *pBuffer, int16 isZ) {
	void far *pRetBuff;
	char fname[14];
	int16 i, fileHandle;

	for(i = 0; i < 13; i++)
		fname[i] = fn[i];

	pRetBuff = pBuffer;
	if(isZ == 0) {
		fileHandle = C_0000_0245(fname);
		C_0000_0336(fileHandle, 0, C_0000_0617(fileHandle), pBuffer);
		C_0000_0284(fileHandle);
	} else {
		LZC_load(fname, 0, pBuffer, 0, D_3EB6);
	}
}

/*OSI file error handler (installed in OSI_ErrorHandler by main).
  Called by the OSI file functions with OSI_ErrorCode = DOS error code (-1 = fewer
  bytes read/written than asked) and OSI_ErrorFunc = function (1 open, 2 close,
  3 create, 4 delete, 5 read, 6 write, 7 rename, 8 copy).
  Leaving OSI_ErrorCode != 0 makes the operation fail, clearing it means "retry".
  A short write is reported as "disk full", other errors are displayed and
  a key is awaited: <CTRL+C>/<ESC> give up, any other key retries.*/
/*C_0268_0128*/void FileErrorHandler() {
	int16 key;
	struct tClipInfo *savedClip;

	/*end of file reached: not an error*/
	if(D_01FA == -1 && D_01FC == 5/*OSI_read*/)
		if(IgnoreShortRead)
			return;
	/*deleting a file that does not exist: not an error*/
	if(D_01FA == 2 && D_01FC == 4/*OSI_delete*/)
		return;
	if(ConsoleReady) {
		savedClip = D_03EC;
		C_05D0_25AD(&(D_B644[3]));
		if(D_01FA == -1 && D_01FC == 6/*OSI_write*/) {
			C_2A81_0255();
		} else {
			C_05D0_2DB9("DISK ERROR!!!! Function: %d, error: %d, file: ", D_01FC, D_01FA);
			C_05D0_2DB9(D_01FE);
			C_05D0_2DB9("\n");
			/*wait for a key (flushing the keyboard buffer)*/
			while(!(key = C_29D1_000E(1)));
		}
		C_05D0_25AD(savedClip);
	}
	/*not <CTRL+C>/<ESC>: retry*/
	if(key != 3 && key != 27)
		D_01FA = 0;
}

/*give all the remaining memory (but 16 bytes) to the graphic driver (which
  loads its tile graphics there), then load the tile animation table
  "animdata" and the tile/type property tables "tileflag"*/
/*C_0268_01D2*/static InitGraphicsData() {
	uint16 aSegment;
	union tHandler memBP;/*the block, paragraph aligned (unused)*/
	int32 linearAddr;
	void far *pBlock;
	int16 animFile, flagFile;

	D_54EC = MEM_largestFree(0) - 0x10;
	pBlock = MEM_alloc(D_54EC, 2);
	linearAddr = ((uint32)FP_SEG(pBlock) << 4) + FP_OFF(pBlock);
	if(linearAddr & 0xf) {
		D_54EC -= 0x10 - (linearAddr & 0xf);
		linearAddr = (linearAddr & ~0xf) + 0x10;
		memBP.iii.seg = linearAddr >> 4;
		memBP.iii.ofs = linearAddr & 0xf;
	}
	aSegment = linearAddr >> 4;
	C_2A81_0098(4);
	GR_INIT_MEMORY(aSegment, (uint16)(D_54EC >> 4));
	D_3296 = D_54EC >> 10;
	D_9CF2 = D_ED0C->pDriverMem;
	if(D_9CF2 == 0) {
		C_05D0_2DB9("Insufficient memory. See your Ref Card.\n");
		C_05D0_22C8();
		GR_SHUTDOWN();
		ShutdownSound();
		MEM_shutdown(8);
		exit(1);
	}
	D_50FA = ((int32)D_9CF2 >> 16) + (((int32)D_9CF2 & 0xffff) >> 4);

	/*animated tiles: count, tile number, first frame, frame mask, shift*/
	C_2A81_0098(1);
	animFile = C_0000_0245("animdata");
	C_0000_0336(animFile, 0, 2, &D_9CF8);
	C_0000_0336(animFile, -1, 0x20 * sizeof(int16), D_B4FE);
	C_0000_0336(animFile, -1, 0x20 * sizeof(int16), D_7744);
	C_0000_0336(animFile, -1, 0x20, D_3276);
	C_0000_0336(animFile, -1, 0x20, D_B542);
	C_0000_0284(animFile);

	/*0x800 tiles terrain flags, 0x800 tiles flags, 0x400 types weight,
	  0x800 tiles flags #2*/
	C_2A81_0098(1);
	flagFile = C_0000_0245("tileflag");
	C_0000_0336(flagFile, 0, 0x800, D_B4FA);
	C_0000_0336(flagFile, -1, 0x800, D_6DF0);
	C_0000_0336(flagFile, -1, 0x400, D_763C);
	C_0000_0336(flagFile, -1, 0x800, D_7640);
	C_0000_0284(flagFile);
}

/*display the main screen background (screen.lzc, in a temporary block)*/
/*C_0268_0456*/ShowMainScreen() {
	void far *pixmap;

	C_2A81_0098(1);
	pixmap = MEM_alloc(LIB_entrySize("screen.lzc", 0), 0);
	LZC_load("screen.lzc", 0, pixmap, 0, D_3EB6);
	GR_DRAW_BITMAP(&D_3268, 0, 0, pixmap, 0);
	MEM_free(pixmap);
}

/*VGA only: send the whole 256 colors palette VGAPalette to the DAC
  (BIOS int 10h/1012h)*/
/*C_0268_04C9*/SetVGAPalette() {
	union REGS regs;
	struct SREGS sregs;

	if(GrAdapter != 0)
		return;
	regs.x.ax = 0x1012;
	regs.x.bx = 0;
	regs.x.cx = 0x100;
	regs.x.dx = FP_OFF(D_B562);
	segread(&sregs);
	sregs.es = FP_SEG(D_B562);
	int86x(0x10, &regs, &regs, &sregs);
}

/*load and set the palette ("sepal" for VGA, by the driver otherwise)*/
/*C_0268_0514*/LoadPalette() {
	int16 fileHandle;

	if(GrAdapter == 0) {
		__MemAlloc(&D_B562, 0x100 * 3);
		C_2A81_0098(1);
		fileHandle = C_0000_0245("sepal");
		C_0000_0336(fileHandle, 0, 0x100 * 3, D_B562);
		C_0000_0284(fileHandle);
		SetVGAPalette();
	} else {
		GR_INIT_PALETTE();
	}
}

/*palette cycling for fire/water/lava colors.
  VGA: color 0xff pulses in grey levels 12..19, colors 0xe0-0xe7 and
  0xe8-0xef rotate by one step per call, colors 0xf0-0xf3, 0xf4-0xf7 and
  0xf8-0xfb every second call (values taken from the VGAPalette palette).
  Other adapters: done by the graphic driver*/
/*C_0268_0574*/PaletteAnimation() {
	int16 step;
	union REGS regs;
	int16 rgb;

	if(GrAdapter == 0) {
		/*triangle wave 0..7..0 -> grey level 12..19*/
		step = (PaletteCycle ++) & 0xf;
		if(step > 7)
			step = 0xf - step;
		step += 0xc;
		/*BIOS: set DAC register bx to (dh,ch,cl)*/
		regs.x.ax = 0x1010;
		regs.x.bx = 0xff;
		regs.h.dh =
		regs.h.ch =
		regs.h.cl = step;
		int86(0x10, &regs, &regs);
		/*0x3c8: DAC write index, 0x3c9: r,g,b*/
		for(step = 0; step < 8; step ++) {
			outportb(0x3c8, ((PaletteCycle - step) & 7) + 0xe0);
			for(rgb = 0; rgb < 3; rgb ++)
				outportb(0x3c9, D_B562[0xe7 - step].content[rgb]);

			outportb(0x3c8, ((PaletteCycle - step) & 7) + 0xe8);
			for(rgb = 0; rgb < 3; rgb ++)
				outportb(0x3c9, D_B562[0xef - step].content[rgb]);
		}
		for(step = 0; step < 4; step ++) {
			outportb(0x3c8, (((PaletteCycle >> 1) - step) & 3) + 0xf0);
			for(rgb = 0; rgb < 3; rgb ++)
				outportb(0x3c9, D_B562[0xf3 - step].content[rgb]);

			outportb(0x3c8, (((PaletteCycle >> 1) - step) & 3) + 0xf4);
			for(rgb = 0; rgb < 3; rgb ++)
				outportb(0x3c9, D_B562[0xf7 - step].content[rgb]);

			outportb(0x3c8, (((PaletteCycle >> 1) - step) & 3) + 0xf8);
			for(rgb = 0; rgb < 3; rgb ++)
				outportb(0x3c9, D_B562[0xfb - step].content[rgb]);
		}
	} else {
		GR_CYCLE_PALETTE((unsigned char)(PaletteCycle ++));
	}
}

/*build the configuration string given back to SAVAGE.EXE (same format as
  CONFIG.SE): adapter letter, 'M'ouse/'K'eyboard, music letter*/
/*C_0268_0708*/BuildConfigString(char *dst) {
	*(dst++) = GrAdapterLetters[GrAdapter];
	*(dst++) = MouseOn?'M':'K';
	*(dst++) = MusicAdapterLetters[MusicAdapter];
	*dst = 0;
}

/*parse a configuration string (CONFIG.SE or a command line argument):
  V/E/T graphic adapter, M/K mouse or keyboard, N/A/R/P music (none, AdLib,
  Roland, none), '$' see GrDriverOption*/
/*C_0268_073D*/cfg_parse(unsigned char *str) {
	while(*str) {
		switch(toupper(*str++)) {
			case '$': GrDriverOption = 1; break;
			case 'V': GrAdapter = 0; break;
			case 'E': GrAdapter = 1; break;
			case 'T': GrAdapter = 2; break;
			case 'M': MouseOn = 1; break;
			case 'K': MouseOn = 0; break;
			case 'N': MusicAdapter = 0; soundCard = 0; D_2810 = 0; break;
			case 'A': MusicAdapter = 1; soundCard = 2; D_2810 = 1; break;
			case 'R': MusicAdapter = 1; soundCard = 1; D_2810 = 1; break;
			case 'P': MusicAdapter = 0; soundCard = 3; D_2810 = 1; break;
		}
	}
}

/*program entry: initialization, game loop, exit*/
/*C_0268_0813*/cdecl main(int16 argc, char **argv) {
	char cfgString[7];
	int16 hercRow;
	int32 linearAddr;
	char far *pFont;
	unsigned now[2];
	int16 i, fileHandle;

	/*seed the random generator from the time of day*/
	gettime((struct time *)now);
	MEM_init(8);
	C_2B57_0044(now[1]);
	for(i = 0; i < now[1]; i++)
		C_2B57_001F(0, 2);
	/*config: CONFIG.SE (6 characters) then the command line arguments*/
	fileHandle = C_0000_0245("config.se");
	if(fileHandle >= 0) {
		C_0000_0336(fileHandle, 0, 6, cfgString);
		cfgString[6] = 0;
		cfg_parse(cfgString);
		C_0000_0284(fileHandle);
	}
	for(i = 1; i < argc; i ++)
		cfg_parse(argv[i]);
	/*the .lzc loader's work area*/
	D_3EB6 = MEM_alloc(0x3500, 2);
	if(D_3EB6 == 0) {
		GR_SHUTDOWN();
		ShutdownSound();
		MEM_shutdown(8);
		exit(1);
	}
	lzcWorkArea = D_3EB6;

	C_2A81_02A6();
	C_2AFC_000A();
	/*install the file error handler*/
	D_01F6.f = FileErrorHandler;
	/*the surfaces (scanline offset tables) are only used by the mouse code:
	  Screen = the 320x200 screen, MapSurface = the map view*/
	D_3268.pScanlines = D_4D4A;
	D_3268.left = 0;
	D_3268.right = 319;
	D_3268.top = 0;
	D_3268.bottom = 199;

	D_9CE4.pScanlines = D_7784;
	D_9CE4.left = 0;
	D_9CE4.right = 159;
	D_9CE4.top = 8;
	D_9CE4.bottom = 151;

	if(GrAdapter == 0) {/*VGA*/
		D_3268.segment = 0xa000;
		D_4D4A[0] = 0;
		for(i = 1; i < 200; i++)
			D_4D4A[i] = D_4D4A[i - 1] + 0x140;
	} else
	if(GrAdapter == 1) {/*EGA*/
		D_3268.segment = 0xa000;
		D_4D4A[0] = 0;
		for(i = 1; i < 200; i++)
			D_4D4A[i] = D_4D4A[i - 1] + 40;
		D_3268.pScanlines = D_4D4A;
		D_7784[0] = 0x141;
		for(i = 1; i < 176; i++)
			D_7784[i] = D_7784[i - 1] + 40;
		D_9CE4.segment = 0xa200;
	} else
	if(GrAdapter == 2) {/*TANDY*/
		D_3268.segment = 0xb800;
		D_4D4A[0] = 0;
		D_4D4A[1] = 0x2000;
		D_4D4A[2] = 0x4000;
		D_4D4A[3] = 0x6000;
		for(i = 4; i < 200; i++)
			D_4D4A[i] = D_4D4A[i - 4] + 0xa0;
		D_3268.pScanlines = D_4D4A;
	} else
	if(GrAdapter == 3) {/*CGA*/
		D_3268.segment = 0xb800;
		D_4D4A[0] = 0;
		D_4D4A[1] = 0x2000;
		for(i = 2; i < 200; i++)
			D_4D4A[i] = D_4D4A[i - 2] + 80;
		D_3268.pScanlines = D_4D4A;
	} else
	if(GrAdapter == 4) {/*HERCULES*/
		D_3268.segment = 0xb000;
		/*200 lines stretched to 300 (x1.5), 4 interleaved banks
		  of 90 bytes per line*/
		for(i = 0; i < 200; i++) {
			hercRow = (i * 3 >> 1) + 0xf;
			D_4D4A[i] = ((hercRow & 3) << 13) + (hercRow >> 2) * 90 + 5;
		}
		D_3268.pScanlines = D_4D4A;
	}
	if(MouseOn) {
		/*background save area and pointer shapes*/
		__MemAlloc(&D_3E9C.saveBuf, 0x400);
		__MemAlloc(&D_50FC, 0x800);
		LZC_load("secursor.ptr", 0, D_50FC, 0, D_3EB6);
		if(osiError) {
			MEM_shutdown(8);
			exit(1);
		}
		D_3E9C.shapes = D_50FC;
		D_3E9C.surface = &D_3268;
		C_2ABF_0008(&D_3E9C);
		D_3E9C.onMove = C_05D0_01F5;
		C_2ABF_005E(D_50FC, 0);
	}
	C_276B_0005(GrAdapter);/*init gr*/
	LoadPalette();
	/*font*/
	__MemAlloc(&pFont, LIB_entrySize("system.lzc", 3));
	D_ED0C->font = LZC_load("system.lzc", 3, pFont, 0, D_3EB6);
	ConsoleReady = 1;
	/*text windows: [0] whole screen, [1] status area, [3] message area*/
	C_05D0_2562(&(D_B644[0]), 0, 0, 39, 24);
	C_05D0_25AD(&(D_B644[0]));
	D_03EC->attrFlags |= 1;
	D_03EC->attrFlags |= 0x20;
	D_03EC->attrFlags |= 4;
	ShowMainScreen();
	C_05D0_2562(&(D_B644[1]), 23, 1, 38, 13);
	C_05D0_2562(&(D_B644[3]), 23, 16, 38, 23);
	C_05D0_2562(&(D_B644[2]), 2, 3, 19, 18);
	C_05D0_25AD(&(D_B644[3]));
	/*message area: word wrap, scroll, capitalize*/
	D_03EC->attrFlags |= 1;
	D_03EC->attrFlags |= 0x20;
	D_03EC->attrFlags |= 4;
	C_2701_000A();

	__MemAlloc(&D_B4FA, 0x800);
	__MemAlloc(&D_6DF0, 0x800);
	__MemAlloc(&D_6EF4, 0x800);
	__MemAlloc(&D_763C, 0x400);
	__MemAlloc(&D_7640, 0x800);
	__MemAlloc(&D_4EF0, 3000);
	__MemAlloc(&D_4706, 0x100);
	__MemAlloc(&D_3298, 0x100);
	__MemAlloc(&D_53E4, 0x100);
	__MemAlloc(&D_45FA, 0x100);
	__MemAlloc(&D_5100, 0x100);
	__MemAlloc(&D_4602, 0x100);
	__MemAlloc(&D_B53E, 0x100 * sizeof(int16));
	__MemAlloc(&D_4EEC, 0x100);
	__MemAlloc(&D_45FE, 0x100);
	__MemAlloc(&D_BAFA, 0x600);

	/*ScratchBuf is also the 176x176 (0x7900 bytes) map view back buffer:
	  MapScanlines gives the offsets of its inner part (8 pixels border)*/
	__MemAlloc(&D_50F6, 0x7900);
	D_6EF8 = D_ED0C->pWorkBuf = D_50F6;
	linearAddr = PtrToLinear(D_50F6);
	D_54E8 = LinearToPtr(linearAddr + 0x36b0);
	if(GrAdapter == 0) {/*VGA*/
		D_9CE4.segment = FP_SEG(D_50F6);
		D_7784[0] = FP_OFF(D_50F6) + 0x588;
		for(i = 1; i < 0xa0; i++)
			D_7784[i] = D_7784[i - 1] + 0xb0;
	} else
	if(GrAdapter == 2) {/*TANDY*/
		D_9CE4.segment = FP_SEG(D_50F6);
		D_7784[0] = FP_OFF(D_50F6) + 0x404;
		for(i = 1; i < 0xa0; i++)
			D_7784[i] = D_7784[i - 1] + 0x80;
	} else
	if(GrAdapter == 3 || GrAdapter == 4) {/*CGA/HERCULES*/
		D_9CE4.segment = FP_SEG(D_50F6);
		D_7784[0] = FP_OFF(D_50F6) + 0x282;
		for(i = 1; i < 0xa0; i++)
			D_7784[i] = D_7784[i - 1] + 0x50;
	}

	C_0A5D_38A9();

	InitGraphicsData();

	D_2B7C = 1;
	C_08CB_0517();/*empty the map chunk cache*/
	D_2362 = 1;
	C_05D0_047A();/*load the game*/

	/*no character in the saved game: go back to SAVAGE.EXE*/
	if(D_2869 == 0) {
		C_05D0_2DB9("You must first create a character. ");
		D_03AE = 0;
		C_05D0_22C8();
		C_2A81_0098(1);
		GR_SHUTDOWN();
		ShutdownSound();
		BuildConfigString(cfgString);
		while(1)
			execl("savage.exe", "savage.exe", cfgString, "X", 0L);
	}

	if(MouseOn)
		C_2ABF_013D();
	C_2701_01F1();

	/*-- game loop --*/
	C_0359_1A65();
	/*-- --*/

	/*exit: stop the music, delete the temporary files*/
	C_2701_0190(0);
	C_0A5D_25CC();
	GR_SHUTDOWN();
	ShutdownSound();
	MEM_shutdown(8);
	exit(0);
}
