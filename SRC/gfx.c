/*
 * The Savage Empire (Worlds of Ultima) - GAME.EXE
 * segment 276B: graphic effects, missiles, the sky bar, portraits,
 * conversations, the paper doll, the peer map (Ultima VI's gfx.c).
 *
 * - InitGraphicsDriver loads the graphic driver (an entry of system.lzc) and
 *   gives it its callbacks. The driver is called through the GR_xx macros:
 *   D_ED10 holds its far address, the offset part being set to the entry
 *   number before each call; D_ED0C is the driver's state block (tGr).
 * - Missiles: up to 16 sprites moving with a Bresenham line from one tile to
 *   several destination tiles, with rotation, collision with walls/objects
 *   and a list of the NPCs crossed (D_EBD2). Explosion/MagicWind/AnimWebs/
 *   AnimHailStorm build on it. The animation is drawn in the offscreen map
 *   buffer; a "dirty rectangle" (D_ED14 left, D_ED16 top, D_ED1A right,
 *   D_ED1C bottom) is accumulated and copied to screen. The map window is
 *   x 8..167, y 16..159 of the buffer.
 * - misc. effects: magical halo, screen shake, map window clearing.
 * - the sky bar (sun or moon) above the map window, the portraits (faces.lzc
 *   over a background of bkgrnd.lzc chosen by place and time), the
 *   conversation scripts (talk.lzc), the paper doll, the "peer" overview of
 *   the loaded area, the midgame picture sequences (MIDGAMEx.LZC).
 */
#include "se.h"

#undef abs
extern int cdecl abs(int);

#define AREA_W 40
#define AREA_H 40

/*text window (Ultima VI's)*/
struct tClipInfo/*size:9*/ {
	unsigned char originX, originY, cols, rows;
	unsigned char curX, curY;
	unsigned char fgColor, bgColor;
	unsigned char attrFlags;
};

/*a drawing surface (Ultima VI's)*/
struct tSurface {
	uint16 segment;/*_00*/
	uint16 far *pScanlines;/*_02*/
	uint16 right, left, top, bottom;/*_06,_08,_0a,_0c*/
};

/*the graphic driver's state (Ultima VI's tGr)*/
struct tGr {
	unsigned char header[6];/*_00*/
	void far *font;/*_06*/
	unsigned char textFg, textBg;
	unsigned char xorMode;
	unsigned char unknown0d;
	unsigned char animOn;
	unsigned char toBackBuffer;/*_0f: 1: color/fill/pixel primitives draw into the back buffer*/
	void (*pfnDiskConfirm)();/*_10*/
	unsigned char reserved14[8];
	void far *pWorkBuf;/*_1c*/
	void far *pDriverMem;/*_20*/
	void (*pfnLoadFile)();/*_24*/
	void (*pfnOpen)();/*_28*/
	void (*pfnRead)();/*_2c*/
	void (*pfnClose)();/*_30*/
};

/*an area of the surface whose portraits get a given background*/
struct tBgArea {
	int16 x0, y0, x1, y1;/*the rectangle, bounds excluded*/
	int16 bg;/*entry of bkgrnd.lzc*/
};

/*the graphic driver: D_ED10's offset selects the entry*/
#define GR_INIT(a)                         (D_ED10.iii.ofs = 0x00, (struct tGr far *)(*D_ED10.f)(a))
#define GR_SHUTDOWN()                      (D_ED10.iii.ofs = 0x03, (*D_ED10.f)())
#define GR_SET_COLOR(c)                    (D_ED10.iii.ofs = 0x06, (*D_ED10.f)(c))
#define GR_PUT_PIXEL(x,y)                  (D_ED10.iii.ofs = 0x09, (*D_ED10.f)(x,y))
#define GR_FILL_RECT(x0,y0,x1,y1)          (D_ED10.iii.ofs = 0x12, (*D_ED10.f)(x0,y0,x1,y1))
#define GR_PUT_TILE_SCREEN(tile,x,y)       (D_ED10.iii.ofs = 0x2d, (*D_ED10.f)(tile,x,y))
/*draw a bitmap on a surface (surface, x, y, bitmap, 0)*/
#define GR_DRAW_BITMAP(s,x,y,p,c)          (D_ED10.iii.ofs = 0x30, (*D_ED10.f)(s,x,y,p,c))
/*(slot, tile, rotation, pivotX, pivotY) build a rotated sprite*/
#define GR_DEFINE_SPRITE(i,a,b,c,d)        (D_ED10.iii.ofs = 0x33, (*D_ED10.f)(i,a,b,c,d))
/*draw sprite slot at x,y in the back buffer (also puts a saved background back)*/
#define GR_DRAW_SPRITE(i,x,y)              (D_ED10.iii.ofs = 0x36, (*D_ED10.f)(i,x,y))
/*save the back buffer under x,y into a sprite slot*/
#define GR_SAVE_SPRITE_BG(i,x,y)           (D_ED10.iii.ofs = 0x39, (*D_ED10.f)(i,x,y))
#define GR_PUT_TILE(tile,x,y)              (D_ED10.iii.ofs = 0x42, (*D_ED10.f)(tile,x,y))
/*copy back buffer rect x0,y0,x1,y1 to screen at x,y*/
#define GR_COPY_TO_SCREEN(x0,y0,x1,y1,x,y) (D_ED10.iii.ofs = 0x45, (*D_ED10.f)(x0,y0,x1,y1,x,y))
/*same with a fade-in effect*/
#define GR_FADE_TO_SCREEN(x0,y0,x1,y1,x,y) (D_ED10.iii.ofs = 0x48, (*D_ED10.f)(x0,y0,x1,y1,x,y))

/*OSI*/
extern int16 C_0000_0245(char *);/*OSI_open*/
extern C_0000_0284(int16);/*OSI_close*/
extern int32 C_0000_0336(int16, int32, int32, void far *);/*OSI_read*/
extern cdecl C_2ABF_013D();/*MOUSE_SHOW*/
extern cdecl C_2ABF_014E();/*MOUSE_HIDE*/
extern int16 C_2B57_001F(int16, int16);/*OSI_rand(lo, hi)*/

/*other game modules*/
extern C_0268_00AA();/*LoadFileGr*/
extern C_0268_0574();/*PaletteAnimation*/
extern C_0359_0B23(int16);/*DrawMapWindow*/
extern C_0359_0CD4();/*DisplayTick*/
extern int16 C_05D0_0004(int16, int16, int16, int16);/*CON_mouseOffAt*/
extern C_05D0_004D(int16);/*CON_mouseOn*/
extern int16 C_05D0_22C8();/*CON_getch*/
extern C_05D0_25E4(int16, int16);/*(text window: cursor to x,y?)*/
extern cdecl C_05D0_2DB9(char far *, ...);/*CON_printf*/
extern C_08CB_02FA();/*MAP_BuildArea*/
extern unsigned char C_08CB_107B(int16, int16, int16);/*GetTileAtXYZ*/
extern C_09DA_0144(int16, int16, int16, int16);/*VIS_FloodFill*/
extern C_09DA_0327();/*VIS_UpdateView*/
extern int16 C_0A5D_039A();/*NextLoc*/
extern int16 C_0A5D_05C4(int16, int16, int16);/*FindLoc(x, y, z)*/
extern int16 C_0A5D_0873();/*NextArea*/
extern int16 C_0A5D_0943(int16, int16, int16, int16);/*SearchArea(x1, y1, x2, y2)*/
extern C_0A5D_339C(int16, int16, int16, int16);/*ShowObject(tile, x, y, ?)*/
extern C_0A5D_3577();/*ShowObjects*/
extern int16 C_1810_0D5B(int16, int16, int16);/*COMBAT_GetTargetAt(shooter, x, y)*/
extern int16 C_1810_0F71(int16);/*(tile class test, peer map)*/
extern int16 C_1810_289B(int16, int16);/*LoseHP(objNum, hp): experience earned*/
extern C_1DBD_0D8F(int16);/*(ShowMidgame: per-picture event)*/
extern C_2701_0648(int16);/*(ShakeScreen: sound?)*/
extern C_2A81_0098(int16);/*DISK_confirm*/

/*Savage Empire's library*/
extern int16 osiError;
extern void far * cdecl MEM_alloc(int32 size, int16 flags);
extern cdecl MEM_shutdown(int16);
extern int32 LIB_entrySize(char far *name, int16 index);
extern void far *LZC_load(char far *name, int16 index, void far *out, int16 memflags, char far *work);
extern void cdecl StopAllSounds(void);

extern int16 D_00A8;/*IsZFile*/
extern char D_00AC[8];/*DirIncrX*/
extern char D_00B4[8];/*DirIncrY*/
extern int16 D_00D6;/*GrDriverOption*/
extern unsigned char D_00D8;/*MouseOn*/
extern char D_03AE;/*AnimateAtPrompt*/
extern int16 D_03CD;
extern struct tClipInfo *D_03EC;/*CurClip*/
extern int16 D_069C;/*ViewX*/
extern int16 D_069E;/*ViewY*/
extern int16 D_0710;/*LocPartOffset*/
extern char D_0A39;/*set by ShowMidgame*/
extern int16 D_0D45;/*VisibilityDirty*/
extern int16 D_0DE0;/*ShowHitFlash*/
extern int16 D_0DE2;/*MissileShooter*/
extern unsigned char D_0DE6[8][8];/*RangeTable*/
extern unsigned char D_0E26[11][11];/*SpreadDirTable*/
extern unsigned char D_27EA;/*Time_M*/
extern unsigned char D_27EB;/*Time_H*/
extern int16 D_27F4;/*MapX*/
extern int16 D_27F6;/*MapY*/
extern int16 D_27F8;/*MapZ*/
extern char D_2860;

/*uninitialised data*/
extern struct tSurface D_3268;/*Screen*/
extern unsigned char D_329C[];/*ObjStatus*/
extern char far *D_3EB6;/*work area of the .lzc loader*/
extern unsigned char D_3EBA[AREA_H][AREA_W];/*AreaFlags*/
extern unsigned char D_470A[AREA_H][AREA_W];/*AreaTiles*/
extern char far *D_50F6;/*ScratchBuf*/
extern char far *D_54E8;/*ScratchBuf + 0x36b0*/
extern unsigned char D_6CF0[];/*NPCStatus*/
extern unsigned char far *D_6DF0;/*TileFlag*/
extern int16 far *D_6EF4;/*BaseTile*/
extern char far *D_6EF8;/*ScratchBuf, again*/
extern unsigned char D_6FFC[AREA_H][AREA_W];/*AreaLight*/
extern struct coord D_78E4[];/*ObjPos*/
extern uint16 D_9CFA[];/*ObjShapeType*/
extern unsigned char far *D_B4FA;/*TerrainType*/
extern int16 D_B6B6;
extern int16 D_BAFE;/*AreaX*/
extern int16 D_BB00;/*AreaY*/
extern unsigned char D_BD10[11][11];/*Obj_11x11: first entry of each cell's object tiles*/
extern int16 D_BD8A[];/*CellObjTile*/
extern unsigned char D_BF8A[];/*CellObjNext*/
extern int16 D_C08A[11][11];/*Tile_11x11*/
extern int16 D_EBCC;/*MissileHitObj*/
extern int16 D_EBCE[2];/*MissileEnd*/
extern int16 D_EBD2[];/*MissileHitList*/
extern struct tGr far *D_ED0C;/*GrInfo*/
extern union tHandler D_ED10;/*GrDriver*/
extern int16 D_ED14, D_ED16, D_ED1A, D_ED1C;/*DirtyRect: left, top, right, bottom*/
extern int16 D_ED18;/*width - 1 of the shaken rectangle*/

#define MapX D_27F4
#define MapY D_27F6
#define MapZ D_27F8
#define TileFlag D_6DF0
#define BaseTile D_6EF4
#define TerrainType D_B4FA
#define MissileHitList D_EBD2
#define GetType(i) (D_9CFA[i] & 0x3ff)
#define GetFrame(i) (D_9CFA[i] >> 10)
#define GetX(i) ((*(int16 *)&(D_78E4[i].raw[0]))&0x3ff)
#define GetY(i) (((*(int16 *)&(D_78E4[i].raw[1]))>>2)&0x3ff)
#define TILE_FRAME(i) (BaseTile[GetType(i)] + GetFrame(i))
#define IsTileWin(tile) (TileFlag[tile] & 0x08)
#define IsTileNos(tile) (TileFlag[tile] & 0x20)
#define HideMouse if(D_00D8) C_2ABF_014E();
#define ShowMouse if(D_00D8) C_2ABF_013D();

/*====---- ----====*/
/*driver init parameter, indexed by adapter type:
  BIOS video mode (MCGA/EGA/Tandy) or video segment (CGA/Hercules)*/
/*24CC*/int16 GrDriverParam[] = {
	0x13,
	0xD,
	0x9,
	0xB800,
	0xB000
};
/*24D6*/int16 ScreenFade = 1;
/*colors*/
/*24D8*/int16 ColorRed = 4;/*disk prompt text; peer: damaging terrain*/
/*24DA*/int16 ColorGreen = 2;/*peer: passable terrain*/
/*24DC*/int16 ColorBlue = 9;/*peer: impassable wet terrain*/
/*24DE*/int16 ColorMagenta = 0xD;
/*24E0*/int16 ColorYellow = 0xE;
/*24E2*/int16 ColorBlack = 0;
/*24E4*/int16 ColorPaper = 0x48;/*background of the map window and the midgame text*/
/*24E6*/int16 D_24E6 = 0;/*normal text color*/
/*24E8*/int16 D_24E8 = 0;
/*24EA*/int16 ColorGray = 7;
/*24EC*/int16 ColorBrown = 6;
/*24EE*/int16 D_24EE = 0x4F;
/*24F0*/int16 D_24F0 = 1;
/*24F2*/int16 ColorLightGreen = 0xA;/*peer: impassable terrain*/

/*init gr:
  load the graphic driver for adapter type "adapter" (entry of system.lzc,
  0:MCGA 1:EGA 2:Tandy 3:CGA, 4 uses 3's), initialise it and give it the
  callbacks it needs (disk request, file loading/open/read/close)*/
/*C_276B_0005*/InitGraphicsDriver(int16 adapter) {
	D_00A8 = 1;
	D_ED10.p = MEM_alloc(LIB_entrySize("system.lzc", adapter == 4?3:adapter) + 0x10, 2);
	if(D_ED10.p == 0) {
		GR_SHUTDOWN();
		MEM_shutdown(8);
		exit(1);
	}
	D_ED10.p = LZC_load("system.lzc", adapter == 4?3:adapter, D_ED10.p, 2, D_3EB6);
	D_00A8 = 0;
	D_ED0C = GR_INIT(GrDriverParam[adapter] + D_00D6);
	D_ED0C->pfnDiskConfirm = C_2A81_0098;
	D_ED0C->pfnLoadFile = C_0268_00AA;
	D_ED0C->pfnOpen = C_0000_0245;
	D_ED0C->pfnRead = C_0000_0336;
	D_ED0C->pfnClose = C_0000_0284;
	if(adapter) {
		if(adapter == 1)
			D_ED18 = 0x8f;
		else
			D_ED18 = 0x95;
		ColorPaper = ColorGray;
		D_24EE = ColorBlack;
	} else {
		D_ED18 = 0x96;
	}
}

/*briefly flash object objNum (if on the map and in view) with a
  highlight effect: the high bits of the tile number (above 0x7ff) are
  drawing attributes for the driver, chosen by kind (0, 1, 2)*/
/*C_276B_012F*/MagicalHalo(int16 objNum, int16 kind) {
	int16 delta_x,delta_y,tileFrame;
	int16 attr;

	if(D_329C[objNum] & 0x18)
		return;
	delta_x = (GetX(objNum) - MapX + 5) & 0x3ff;
	delta_y = (GetY(objNum) - MapY + 5) & 0x3ff;
	switch(kind) {
		case 0: attr = 4 << 10; break;
		case 1: attr = 6 << 10; break;
		case 2: attr = 2 << 10; break;
	}
	tileFrame = TILE_FRAME(objNum) + attr;
	if(delta_x < 11 && delta_y < 11) {
		C_0A5D_339C(tileFrame, delta_x, delta_y, 2);
		ScreenFade = 1;
		C_0359_0B23(1);
		C_0A5D_3577();
		ScreenFade = 1;
		C_0359_0B23(1);
	}
}

/*redefineClipping:
  extend the dirty rectangle with the 16x16 square centered on x,y
  (map buffer coordinates), clamped to the map window (8..167, 16..159)*/
/*C_276B_0212*/static ExtendDirtyRect(int16 x, int16 y) {
	if(x < 16)
		D_ED14 = 8;
	else if(x - 8 < D_ED14)
		D_ED14 = x - 8;

	if(y < 24)
		D_ED16 = 16;
	else if(y - 8 < D_ED16)
		D_ED16 = y - 8;

	if(x > 159)
		D_ED1A = 167;
	else if(x + 8 > D_ED1A)
		D_ED1A = x + 8;

	if(y > 151)
		D_ED1C = 159;
	else if(y + 8 > D_ED1C)
		D_ED1C = y + 8;
}

/*redraw, over the sprites, what must stay in front of them in the
  rectangle x0,y0-x1,y1 of the offscreen buffer: foreground tiles
  of the objects and hidden/dark tiles*/
/*C_276B_02A5*/static RedrawForeground(int16 x0, int16 y0, int16 x1, int16 y1) {
	int16 tx, ty;
	int16 entry, tile;

	for(tx = SCR2TIL(x0); tx <= SCR2TIL(x1); tx ++) {
		for(ty = SCR2TIL(y0); ty <= SCR2TIL(y1); ty ++) {
			/*objects' tiles of this cell: linked list CellObjNext, tiles in CellObjTile*/
			for(entry = D_BD10[ty][tx]; entry; entry = D_BF8A[entry]) {
				tile = D_BD8A[entry];
				if(tile && (TileFlag[tile & 0x7ff] & 0x10))
					GR_PUT_TILE(tile & 0x7ff, TIL2SCR(tx), TIL2SCR(ty));
			}
			if(D_C08A[ty][tx] == 0xff || D_C08A[ty][tx] == 0x1bc)
				GR_PUT_TILE(0xff, TIL2SCR(tx), TIL2SCR(ty));
		}
	}
}

/*distance.rotation angles map:
  angle of the vector (dx,dy) indexed [dy + 7][dx + 7];
  0x80 is a full turn, 0x00=east, 0x20=north, 0x40=west, 0x60=south*/
/*24F4*/unsigned char MissileAngles[][16] = {
	{0x30,0x2E,0x2D,0x2B,0x28,0x26,0x23,0x20,0x1D,0x1A,0x18,0x15,0x13,0x12,0x10,0x0F},
	{0x32,0x30,0x2E,0x2C,0x29,0x26,0x23,0x20,0x1D,0x1A,0x17,0x14,0x12,0x10,0x0E,0x0D},
	{0x34,0x32,0x30,0x2E,0x2B,0x27,0x24,0x20,0x1C,0x19,0x15,0x12,0x10,0x0E,0x0D,0x0B},
	{0x35,0x34,0x32,0x30,0x2D,0x29,0x25,0x20,0x1B,0x17,0x13,0x10,0x0E,0x0C,0x0B,0x09},
	{0x38,0x37,0x35,0x33,0x30,0x2C,0x26,0x20,0x1A,0x14,0x10,0x0D,0x0B,0x09,0x08,0x07},
	{0x3A,0x3A,0x39,0x37,0x34,0x30,0x29,0x20,0x17,0x10,0x0C,0x09,0x07,0x06,0x06,0x05},
	{0x3D,0x3D,0x3C,0x3B,0x3A,0x37,0x30,0x20,0x10,0x09,0x06,0x05,0x04,0x03,0x03,0x03},
	{0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	{0x43,0x43,0x44,0x45,0x46,0x49,0x50,0x60,0x70,0x77,0x7A,0x7B,0x7C,0x7D,0x7D,0x7D},
	{0x46,0x46,0x47,0x49,0x4C,0x50,0x57,0x60,0x69,0x70,0x74,0x77,0x79,0x7A,0x7A,0x7B},
	{0x48,0x49,0x4B,0x4D,0x50,0x54,0x5A,0x60,0x66,0x6C,0x70,0x73,0x75,0x77,0x78,0x79},
	{0x4B,0x4C,0x4E,0x50,0x53,0x57,0x5B,0x60,0x65,0x69,0x6D,0x70,0x72,0x74,0x75,0x77},
	{0x4D,0x4E,0x50,0x52,0x55,0x59,0x5C,0x60,0x64,0x67,0x6B,0x6E,0x70,0x72,0x73,0x75},
	{0x4E,0x50,0x52,0x54,0x57,0x5A,0x5D,0x60,0x63,0x66,0x69,0x6C,0x6E,0x70,0x72,0x73},
	{0x50,0x52,0x53,0x55,0x58,0x5A,0x5D,0x60,0x63,0x66,0x68,0x6B,0x6D,0x6E,0x70,0x71},
	{0x51,0x53,0x55,0x57,0x59,0x5B,0x5D,0x60,0x63,0x65,0x67,0x69,0x6B,0x6D,0x6F,0x70}
};
/*missile tiles; the following tables are indexed by the position of the
  projectile tile in this list ("misType", 0 = unknown tile)*/
/*25F4*/uint16 MissileTiles[] = {
	0,
	0x223,0x224,0x225,0x230,0x236,
	0x237,0x188,0x189,0x235,0x18A,
	0x18B,0x18C,0x18D,0x17C,0x17D,
	0x18D,0x17F
};
/*angle at which the tile is drawn (subtracted from the flight angle)*/
/*2618*/unsigned char MissileTileAngle[] = {0x40,0x10,0x00,0x10,0x00,0x00,0x00,0x20,0x20,0x00,0x20,0x20,0x20,0x20,0x00,0x00,0x00,0x00};
/*spin added at each redraw*/
/*262A*/unsigned char MissileSpin[] = {0x00,0x00,0x0A,0x0A,0x0A,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0A,0x0A,0x0A,0x0A};
/*rotation pivot x in the tile (0x7f = center)*/
/*263C*/unsigned char MissilePivotX[] = {0x7F,0x75,0x7F,0x86,0x7A,0x71,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F};
/*rotation pivot y in the tile (0x7f = center)*/
/*264E*/unsigned char MissilePivotY[] = {0x7F,0x7F,0x7F,0x8B,0x7F,0x46,0x3D,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F};
/*the sprite is redrawn every N pixels of movement*/
/*2660*/unsigned char MissileRefresh[] = {0x06,0x03,0x02,0x02,0x04,0x04,0x04,0x02,0x02,0x03,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02};

#define MAX_MISSILES 16

/*animate num_dst projectiles (tile projTile, 0 = invisible) flying from
  tile src_x,src_y to the tiles pX_dst[],pY_dst[], one pixel per step
  (Bresenham). A projectile stops when it goes out of max_range (RangeTable
  distance), hits a tile that blocks missiles (windows are crossed when
  adjacent to the source) or an object whose tile blocks missiles.
  attr: 1 = boomerang return (flies from the destination back to the source)
        2 = "trail" mode: the sprites are never erased; slow fixed refresh rate
        4 = (U6: explosion-like sound)
        8 = one non rotated sprite (slot 15) drawn at every position
  Out:
   - pX_dst/pY_dst = tile where each projectile stopped,
   - D_EBCE = last tile reached by projectile #0,
   - D_EBCC = object hit at the stop tile of #0 (if D_0DE2, the shooter, is set),
   - MissileHitList = list of the NPCs (< 0x100) crossed/hit, -1 terminated.
  returns 1 if every projectile reached its destination*/
/*C_276B_0390*/Missiles(
	int16 src_x, int16 src_y,
	int16 num_dst, int16 *pX_dst, int16 *pY_dst,
	int16 max_range, int16 projTile,
	int16 attr
) {
	int16 i;
	int16 dst;
	int16 objNum,refreshRate,temp,misType,delta_max;
	int16 tile_delta_x,tile_delta_y;
	int16 ret,whichDir;
	int16 spr_new_x,spr_new_y;
	int16 mustDisplay,numTile;
	int16 unused;
	/*current position in world pixels (tile * 16 + 8)*/
	int16 cur_x[MAX_MISSILES],cur_y[MAX_MISSILES];
	/*Bresenham: major/minor deltas, increments [0]=straight [1]=diagonal step*/
	int16 delta_x[MAX_MISSILES],delta_y[MAX_MISSILES];
	int16 incr_x[MAX_MISSILES][2],incr_y[MAX_MISSILES][2];
	int16 b_error[MAX_MISSILES],berr_incr[MAX_MISSILES][2];
	/*last tile entered*/
	int16 tile_x[MAX_MISSILES],tile_y[MAX_MISSILES];
	int16 spr_rot[MAX_MISSILES];
	/*1 bit per NPC (0x100 NPCs)*/
	int16 collision_mask[0x100/8];
	/*sprite position in the map buffer*/
	int16 spr_x[MAX_MISSILES],spr_y[MAX_MISSILES];

	unused = 0;
	C_0359_0B23(0);

	for(i = 0; i < 0x100/8; i ++)
		collision_mask[i] = 0;

	HideMouse
	ret = 1;

	D_EBCE[0] =
	tile_x[0] = src_x;
	D_EBCE[1] =
	tile_y[0] = src_y;

	if(attr & 1) {
		temp = tile_x[0]; tile_x[0] = pX_dst[0]; pX_dst[0] = temp;
		temp = tile_y[0]; tile_y[0] = pY_dst[0]; pY_dst[0] = temp;
	}

	cur_x[0] = TIL2SCR(tile_x[0]) + 8;
	cur_y[0] = TIL2SCR(tile_y[0]) + 8;

	misType = 0;
	for(i = 0; i < 18; i ++) {
		if(MissileTiles[i] == projTile) {
			misType = i;
			break;
		}
	}

	if(attr & 2)
		refreshRate = 6;
	else
		refreshRate = MissileRefresh[misType];

	/*define the sprites: oriented toward each destination*/
	if(projTile) {
		if(attr & 8) {
			GR_DEFINE_SPRITE(15, projTile, 0, 0x7f, 0x7f);
		} else {
			for(dst = 0; dst < num_dst; dst ++) {
				spr_rot[dst] =
					MissileAngles[pY_dst[dst] - tile_y[0] + 7][pX_dst[dst] - tile_x[0] + 7] - MissileTileAngle[misType];
				if(attr & 2)
					GR_DEFINE_SPRITE(dst, projTile, spr_rot[dst], MissilePivotX[misType], MissilePivotY[misType]);
				else
					GR_DEFINE_SPRITE(dst + 8, projTile, spr_rot[dst], MissilePivotX[misType], MissilePivotY[misType]);
			}
		}
	}

	/*Bresenham setup*/
	delta_max = 0;
	for(dst = 0; dst < num_dst; dst ++) {
		cur_x[dst] = cur_x[0];
		cur_y[dst] = cur_y[0];
		tile_x[dst] = tile_x[0];
		tile_y[dst] = tile_y[0];
		delta_x[dst] = TIL2SCR(pX_dst[dst] - tile_x[0]);
		delta_y[dst] = TIL2SCR(pY_dst[dst] - tile_y[0]);
		incr_y[dst][0] = 0;
		if(delta_x[dst] < 0) {
			delta_x[dst] = -delta_x[dst];
			incr_x[dst][0] =
			incr_x[dst][1] = -1;
		} else {
			incr_x[dst][0] =
			incr_x[dst][1] = 1;
		}
		if(delta_y[dst] < 0) {
			delta_y[dst] = -delta_y[dst];
			incr_y[dst][1] = -1;
		} else {
			incr_y[dst][1] = 1;
		}
		/*y major: swap the roles*/
		if(delta_x[dst] < delta_y[dst]) {
			temp = delta_x[dst];
			delta_x[dst] = delta_y[dst];
			delta_y[dst] = temp;
			incr_y[dst][0] = incr_y[dst][1];
			incr_x[dst][1] = incr_x[dst][0];
			incr_x[dst][0] = 0;
		}
		if(delta_x[dst] > delta_max)
			delta_max = delta_x[dst];
		b_error[dst] = delta_y[dst] + delta_y[dst] - delta_x[dst];
		berr_incr[dst][0] = delta_y[dst] + delta_y[dst];
		berr_incr[dst][1] = (delta_y[dst] - delta_x[dst]) << 1;
	}

	/*one pixel per step; delta_x[dst] is the number of steps of
	  projectile dst (set to the current step when it stops)*/
	for(i = 0; i < delta_max; i ++) {
		mustDisplay = 0;
		/*empty dirty rectangle*/
		D_ED14 = 168;
		D_ED16 = 160;
		D_ED1A = 7;
		D_ED1C = 15;
		for(dst = 0; dst < num_dst; dst ++) {
			if(delta_x[dst] <= i) {
				/*arrived/stopped: its last place must still be refreshed*/
				if(i)
					ExtendDirtyRect(spr_x[dst], spr_y[dst]);
				continue;
			}
			whichDir = (b_error[dst] >= 0);
			b_error[dst] += berr_incr[dst][whichDir];
			cur_x[dst] += incr_x[dst][whichDir];
			cur_y[dst] += incr_y[dst][whichDir];

			if(projTile && refreshRate && (i % refreshRate) == 0) {
				spr_new_x = cur_x[dst] - TIL2SCR(MapX - 5);
				spr_new_y = cur_y[dst] - TIL2SCR(MapY - 5);
				if(spr_new_x >= 8 && spr_new_x < 168 && spr_new_y >= 8 && spr_new_y < 168) {
					if(!mustDisplay) {
						if(dst == 0) {
							/*(U6 played the flight sound here; the test is left)*/
						}
						mustDisplay = 1;
					}
					ExtendDirtyRect(spr_new_x, spr_new_y);
					if(attr & 2) {
						/*trail: draw directly, never erased*/
						GR_DRAW_SPRITE(dst, spr_new_x, spr_new_y);
					} else {
						if(i)
							ExtendDirtyRect(spr_x[dst], spr_y[dst]);
						spr_x[dst] = spr_new_x;
						spr_y[dst] = spr_new_y;
					}
				}
				/*spinning projectile*/
				if(MissileSpin[misType]) {
					 spr_rot[dst] += MissileSpin[misType];
					 if(!(attr & 2) && !(attr & 8))
						 GR_DEFINE_SPRITE(dst + 8, projTile, spr_rot[dst], MissilePivotX[misType], MissilePivotY[misType]);
				}
			}

			/*still in the same tile?*/
			if(SCR2TIL(cur_x[dst]) == tile_x[dst] && SCR2TIL(cur_y[dst]) == tile_y[dst])
				continue;

			/*entering a new tile: check range and obstacles*/
			tile_delta_x = abs(SCR2TIL(cur_x[dst]) - src_x);
			tile_delta_y = abs(SCR2TIL(cur_y[dst]) - src_y);
			if(tile_delta_x > 7 || tile_delta_y > 7 || D_0DE6[tile_delta_x][tile_delta_y] > max_range) {
stop:
				/*blocked or out of range: this projectile stops here.
				  note: the break also leaves the loop for the remaining
				  projectiles of this step*/
				delta_x[dst] = i;
				break;
			}
			tile_x[dst] = SCR2TIL(cur_x[dst]);
			tile_y[dst] = SCR2TIL(cur_y[dst]);
			numTile = C_08CB_107B(tile_x[dst], tile_y[dst], MapZ);
			if((tile_delta_x > 1 || tile_delta_y > 1 || !IsTileWin(numTile)) && IsTileNos(numTile))
				goto stop;
			/*mark the NPCs of the tile; stop at the first object blocking missiles*/
			for(objNum = C_0A5D_05C4(tile_x[dst], tile_y[dst], MapZ); objNum >= 0; objNum = C_0A5D_039A()) {
				if(objNum < 0x100 && objNum >= 0)
					collision_mask[objNum >> 3] |= (1 << (objNum & 7));
				if(IsTileNos(TILE_FRAME(objNum) - D_0710))
					break;
			}
			if(objNum >= 0)
				goto stop;
			if(dst == 0) {
				D_EBCE[0] = tile_x[0];
				D_EBCE[1] = tile_y[0];
			}
		}

		if(!mustDisplay)
			continue;

		if(!(i & 3))
			C_0268_0574();

		/*save the background under the sprites (slots 0-7)*/
		if(!(attr & 2)) {
			for(dst = 0; dst < num_dst; dst ++) {
				if(delta_x[dst] > i || (attr & 8))
					GR_SAVE_SPRITE_BG(dst, spr_x[dst], spr_y[dst]);
			}
		}
		/*draw the projectiles*/
		if(!(attr & 2)) {
			for(dst = 0; dst < num_dst; dst ++) {
				if(attr & 8)
					GR_DRAW_SPRITE(15, spr_x[dst], spr_y[dst]);
				else if(delta_x[dst] > i)
					GR_DRAW_SPRITE(dst + 8, spr_x[dst], spr_y[dst]);
			}
		}
		/*foreground + copy the dirty rectangle to screen*/
		if(D_ED14 <= D_ED1A && D_ED16 <= D_ED1C) {
			RedrawForeground(D_ED14, D_ED16, D_ED1A, D_ED1C);
			GR_COPY_TO_SCREEN(D_ED14, D_ED16, D_ED1A, D_ED1C, D_ED14, D_ED16);
		}
		/*restore the background in the offscreen buffer*/
		if(!(attr & 2)) {
			for(dst = 0; dst < num_dst; dst ++) {
				if(delta_x[dst] > i || (attr & 8))
					GR_DRAW_SPRITE(dst, spr_x[dst], spr_y[dst]);
			}
		}
	}

	if(projTile && !(attr & 2) && !(attr & 8))
		C_0359_0CD4();
	/*report where the projectiles stopped*/
	for(dst = 0; dst < num_dst; dst ++) {
		if(
			tile_x[dst] == pX_dst[dst] &&
			(tile_y[dst] == pY_dst[dst] || (attr & 1))
		) continue;
		ret = 0;
		pX_dst[dst] = tile_x[dst];
		pY_dst[dst] = tile_y[dst];
		if(dst == 0 && D_0DE2 >= 0)
			D_EBCC = C_1810_0D5B(D_0DE2, tile_x[0], tile_y[0]);
	}
	ShowMouse

	/*collision bitmap -> list of NPCs (max 0x3f)*/
	num_dst = 0;
	for(i = 0; i < 0x100/8; i ++) {
		for(dst = 0; dst < 8; dst ++) {
			if(collision_mask[i] & (1 << dst)) {
				MissileHitList[num_dst] = (i << 3) + dst;
				num_dst ++;
			}
			if(num_dst > 0x3e)
				break;
		}
		if(num_dst > 0x3e)
			break;
	}
	MissileHitList[num_dst] = -1;
	return ret;
}

/*explosion pattern: the 3x3 square around the center*/
/*2672*/char ExplosionDX[] = {-1, 0, 1,-1, 0, 1,-1, 0, 1};
/*267B*/char ExplosionDY[] = {-1,-1,-1, 0, 0, 0, 1, 1, 1};

/*explosion centered on x,y: 9 trails of missileTile toward the pattern
  above (range 3); returns the -1 terminated list (MissileHitList) of the NPCs
  hit, including those on the center tile*/
int16 far /*C_276B_0E85*/*Explosion(int16 x, int16 y, int16 missileTile) {
	int16 count, objNum;
	int16 x_dst[9], y_dst[9];

	for(count = 0; count < 9; count ++) {
		x_dst[count] = x + ExplosionDX[count];
		y_dst[count] = y + ExplosionDY[count];
	}
	Missiles(x, y, 9, x_dst, y_dst, 3, missileTile, 4|2);
	/*append the NPCs of the center tile*/
	for(count = 0; count < 0x3e; count ++)
		if(MissileHitList[count] < 0)
			break;
	for(objNum = C_0A5D_05C4(x, y, MapZ); count < 0x3f && objNum >= 0; objNum = C_0A5D_039A()) {
		if(objNum < 0x100 && objNum >= 0)
			MissileHitList[count ++] = objNum;
		if(IsTileNos(TILE_FRAME(objNum) - D_0710))
			break;
	}
	MissileHitList[count] = -1;
	return MissileHitList;
}

/*wind from objNum toward x,y:
  5 trails of missileTile (range 7), one aimed at x,y and two pairs going
  1 and 2 tiles to each side (the two side directions come from SpreadDirTable,
  high/low nibble). returns the list of NPCs hit (MissileHitList)*/
int16 far /*C_276B_0F76*/*MagicWind(int16 objNum, int16 x, int16 y, int16 missileTile) {
	int16 sideDirs;
	int16 x_dst[5], y_dst[5];

	sideDirs = D_0E26[y - GetY(objNum) + 5][x - GetX(objNum) + 5];
	x_dst[0] = x;
	x_dst[1] = x + D_00AC[sideDirs >> 4];
	x_dst[2] = x + D_00AC[sideDirs & 0xf];
	x_dst[3] = x_dst[1] + D_00AC[sideDirs >> 4];
	x_dst[4] = x_dst[2] + D_00AC[sideDirs & 0xf];
	y_dst[0] = y;
	y_dst[1] = y + D_00B4[sideDirs >> 4];
	y_dst[2] = y + D_00B4[sideDirs & 0xf];
	y_dst[3] = y_dst[1] + D_00B4[sideDirs >> 4];
	y_dst[4] = y_dst[2] + D_00B4[sideDirs & 0xf];
	Missiles(GetX(objNum), GetY(objNum), 5, x_dst, y_dst, 7, missileTile, 4|2);
	return MissileHitList;
}

/*web animation: 9 web tiles thrown by objNum to the 3x3 square
  centered on x,y; returns the Missiles() result*/
/*C_276B_10AC*/AnimWebs(int16 objNum, int16 x, int16 y) {
	int16 i;
	int16 dst_x[9], dst_y[9];

	for(i = 0; i < 8; i ++) {
		dst_x[i] = x + D_00AC[i];
		dst_y[i] = y + D_00B4[i];
	}
	dst_x[8] = x;
	dst_y[8] = y;
	return Missiles(GetX(objNum), GetY(objNum), 9, dst_x, dst_y, 7, BaseTile[0x11], 8|4);
}

/*hail storm around tile x,y:
  10 to 30 hailstones (up to 6 at a time) fall diagonally (from 50 pixels
  up-left) to random points around x,y; each NPC on an impact tile loses
  1 HP (with a hit tile flash). returns the total experience earned*/
/*C_276B_1142*/AnimHailStorm(int16 x, int16 y) {
	int16 i;
	int16 stonesLeft;
	int16 stone_x[6], stone_y[6];/*current position (world pixels), <0: free slot*/
	int16 impact_x[6], impact_y[6];/*impact point (world pixels)*/
	int16 scr_x, scr_y;
	int16 objNum, anyFalling, frameCnt, soundArg, totalExp;

	anyFalling = 0;
	frameCnt = 0;
	totalExp = 0;
	for(i = 0; i < 6; i ++) {
		stone_x[i] =
		stone_y[i] =
		impact_x[i] =
		impact_y[i] = -1;
	}
	stonesLeft = C_2B57_001F(10, 30);
	GR_DEFINE_SPRITE(7, 0x18e, 0, 0x7f, 0x7f);
	D_ED14 = D_ED16 = 168;
	D_ED1A = D_ED1C = 7;
	while(stonesLeft || anyFalling) {
		soundArg = 0;
		/*maybe launch a new hailstone in a free slot*/
		for(i = 0; i < 6; i ++)
			if(stone_x[i] < 0)
				break;
		if(stonesLeft && i < 6 && C_2B57_001F(0, 7) == 0) {
			impact_x[i] = TIL2SCR(x) + C_2B57_001F(1, 30) + C_2B57_001F(1, 30) - 23;
			impact_y[i] = TIL2SCR(y) + C_2B57_001F(1, 30) + C_2B57_001F(1, 30) - 23;
			stone_x[i] = impact_x[i] - 50;
			stone_y[i] = impact_y[i] - 50;
			stonesLeft --;
		}
		anyFalling = 0;
		/*redraw every 4 pixels: save bg (slot i), draw stone (slot 7),
		  copy to screen, restore bg*/
		if(++frameCnt >= 4) {
			for(i = 0; i < 6; i ++) {
				if(stone_x[i] >= 0) {
					scr_x = stone_x[i] - TIL2SCR(MapX - 5);
					scr_y = stone_y[i] - TIL2SCR(MapY - 5);
					if(scr_x > 15 && scr_x < 152 && scr_y > 15 && scr_y < 152) {
						GR_SAVE_SPRITE_BG(i, scr_x, scr_y);
						ExtendDirtyRect(scr_x, scr_y);
					}
				}
			}
			for(i = 0; i < 6; i ++) {
				if(stone_x[i] >= 0) {
					scr_x = stone_x[i] - TIL2SCR(MapX - 5);
					scr_y = stone_y[i] - TIL2SCR(MapY - 5);
					if(scr_x > 15 && scr_x < 152 && scr_y > 15 && scr_y < 152) {
						GR_DRAW_SPRITE(7, scr_x, scr_y);
						ExtendDirtyRect(scr_x, scr_y);
					}
				}
			}
			if(D_ED14 <= D_ED1A && D_ED16 <= D_ED1C)
				GR_COPY_TO_SCREEN(D_ED14, D_ED16, D_ED1A, D_ED1C, D_ED14, D_ED16);
			D_ED14 = D_ED16 = 168;
			D_ED1A = D_ED1C = 7;
			for(i = 0; i < 6; i ++) {
				if(stone_x[i] >= 0) {
					scr_x = stone_x[i] - TIL2SCR(MapX - 5);
					scr_y = stone_y[i] - TIL2SCR(MapY - 5);
					if(scr_x > 15 && scr_x < 152 && scr_y > 15 && scr_y < 152) {
						GR_DRAW_SPRITE(i, scr_x, scr_y);
						ExtendDirtyRect(scr_x, scr_y);
					}
				}
			}
			frameCnt = 0;
		}
		/*move the stones one pixel down-right; handle impacts*/
		for(i = 0; i < 6; i ++) {
			if(stone_x[i] >= 0) {
				stone_x[i] ++;
				stone_y[i] ++;
				if(impact_x[i] == stone_x[i] && impact_y[i] == stone_y[i]) {
					stone_x[i] = -1;
					soundArg = C_2B57_001F(400, 500);
					scr_x = SCR2TIL(impact_x[i]);
					scr_y = SCR2TIL(impact_y[i]);
					for(objNum = C_0A5D_05C4(scr_x, scr_y, MapZ); objNum >= 0; objNum = C_0A5D_039A()) {
						if(objNum >= 0x100 || objNum < 0)
							continue;
						D_0DE0 = 0;/*no standard hit display*/
						totalExp += C_1810_289B(objNum, 1);
						soundArg = -1;
						/*hit flash (scr_x/scr_y: tile -> map buffer center).
						  note: converted in place, so a 2nd NPC on the same
						  tile gets a wrong flash position*/
						scr_x = TIL2SCR(scr_x - (MapX - 5)) + 8;
						scr_y = TIL2SCR(scr_y - (MapY - 5)) + 8;
						if(scr_x >= 8 && scr_x < 168 && scr_y >= 16 && scr_y < 160) {
							GR_SAVE_SPRITE_BG(6, scr_x, scr_y);
							GR_PUT_TILE(0x101, scr_x - 8, scr_y - 8);
							GR_COPY_TO_SCREEN(scr_x - 8, scr_y - 8, scr_x + 7, scr_y + 7, scr_x - 8, scr_y - 8);
							GR_DRAW_SPRITE(6, scr_x, scr_y);
							ExtendDirtyRect(scr_x, scr_y);
						}
					}
				} else {
					anyFalling = 1;
				}
			}
		}
		/*(U6 played the sound soundArg here)*/
	}
	C_0359_0CD4();
	return totalExp;
}

/*shake the map window "duration" times: the inner part of the
  offscreen map is copied to screen at a random offset of +/-ampl pixels*/
/*C_276B_1647*/ShakeScreen(int16 duration, int16 ampl) {
	int16 i;
	int16 ofs_x, ofs_y;

	C_0359_0B23(0);
	for(i = 0; i < duration; i ++) {
		C_2701_0648(1);
		ofs_x = C_2B57_001F(8 - ampl, ampl + 8);
		ofs_y = C_2B57_001F(16 - ampl, ampl + 16);
		GR_COPY_TO_SCREEN(ofs_x, ofs_y, ofs_x + D_ED18, ofs_y + 0x8f, 16, 16);
		StopAllSounds();
	}
	C_0359_0CD4();
}

/*clear the 11x11 view: all tiles hidden, no object, and rebuild the
  free list of the object tile entries (CellObjNext[i] = i + 1)*/
/*C_276B_16C7*/static ClearMapView() {
	int16 i, j;

	for(i = 0; i < 11; i ++) {
		for(j = 0; j < 11; j ++)
			D_C08A[i][j] = 0xff;
	}
	for(i = 0; i < 0x100; i++)
		D_BF8A[i] = i + 1;
	for(i = 0; i < 11; i ++) {
		for(j = 0; j < 11; j ++)
			D_BD10[i][j] = 0;
	}
}

/*blank the map window (displayed), then rebuild the view
  (tile map and visible tiles/objects) without displaying it*/
/*C_276B_172E*/ResetMapView() {
	ClearMapView();
	D_0D45 = 0;
	ScreenFade = 1;
	C_0359_0B23(1);
	C_08CB_02FA();
	C_09DA_0327();
}

/*the strip above the map window: the sky (tile 0x189 row), then on the
  surface the day sky with the sun moving right to left from 6:00 to 18:00,
  or the night sky with the moon, underground a cave ceiling; then the frame
  tiles 0x160.. and the middle ornament (0x177/0x178 when D_2860 is set,
  0x164/0x165 when NPC 1 is of type 0xf9, else 0x173/0x174)*/
/*C_276B_1756*/DrawSkyBar() {
	int16 i, t;
	int16 moon, mouseState;

	t = D_27EB * 60 + D_27EA;
	for(i = 0; i < 11; i ++)
		GR_PUT_TILE(0x189, TIL2SCR(i), 0);
	if(MapZ) {
		for(i = 0; i < 11; i ++)
			GR_PUT_TILE(0x17a, TIL2SCR(i), 0);
	} else if(t > 359 && t < 1081) {
		for(i = 0; i < 11; i ++)
			GR_PUT_TILE(0x17b, TIL2SCR(i), 0);
		GR_PUT_TILE(0x171, (1080 - t) / 34 * 8, 0);
	} else {
		for(i = 0; i < 11; i ++)
			GR_PUT_TILE(0x179, TIL2SCR(i), 0);
		moon = (t + 721) % 1440;
		GR_PUT_TILE(0x172, (1080 - moon) / 34 * 8, 0);
	}
	for(i = 0; i < 4; i ++)
		GR_PUT_TILE(i + 0x160, TIL2SCR(i), 0);
	for(i = 6; i < 11; i ++)
		GR_PUT_TILE(i + 0x160, TIL2SCR(i), 0);
	if(D_2860) {
		GR_PUT_TILE(0x177, 0x40, 0);
		GR_PUT_TILE(0x178, 0x50, 0);
	} else if(GetType(1) == 0xf9) {
		GR_PUT_TILE(0x164, 0x40, 0);
		GR_PUT_TILE(0x165, 0x50, 0);
	} else {
		GR_PUT_TILE(0x173, 0x40, 0);
		GR_PUT_TILE(0x174, 0x50, 0);
	}
	mouseState = C_05D0_0004(0, 0, 0xaf, 0xf);
	GR_COPY_TO_SCREEN(0, 0, 0xaf, 0xf, 0, 0);
	C_05D0_004D(mouseState);
}

/*2684*/char FacesFile[] = "faces.lzc";
/*268E*/char BkgrndFile[] = "bkgrnd.lzc";

PortraitBg(int16 x, int16 y);

/*draw the portrait of NPC objNum at screen x,y: a face of faces.lzc over
  the background of bkgrnd.lzc for the NPC's place and the time (PortraitBg).
  1000 and, after a midgame (D_0A39), NPCs 0xfe and up are faces drawn alone;
  0xfd then is drawn over the background only. Temporary NPCs (0xc0 and up)
  of the types 0x140, 0x13c, 0x13d, 0x123 have generic faces, the others none;
  NPCs 0x40-0x97 share faces 4 by 4. returns 0 if there is no portrait*/
/*C_276B_1953*/DrawPortrait(int16 objNum, int16 x, int16 y) {
	int16 bg;

	C_2A81_0098(2);
	if(objNum == 1000) {
		objNum = 0x59;
		LZC_load(FacesFile, objNum, D_54E8, 0, D_3EB6);
		GR_DRAW_BITMAP(&D_3268, x, y, D_54E8, 0);
		return;
	}
	if(D_0A39 && objNum >= 0xfe) {
		objNum = objNum - 0xa4;
		LZC_load(FacesFile, objNum, D_54E8, 0, D_3EB6);
		GR_DRAW_BITMAP(&D_3268, x, y, D_54E8, 0);
		return;
	}
	bg = PortraitBg(GetX(objNum), GetY(objNum));
	if(D_0A39 && objNum == 0xfd) {
		LZC_load(BkgrndFile, bg, D_54E8, 0, D_3EB6);
		GR_DRAW_BITMAP(&D_3268, x + 1, y + 1, D_54E8, 0);
		return;
	}
	if(objNum >= 0xc0) {
		switch(GetType(objNum)) {
			case 0x140: objNum = 0x56; break;
			case 0x13c: objNum = 0x57; break;
			case 0x13d: objNum = 0x58; break;
			case 0x123: objNum = 0x59; break;
			default: return 0;
		}
	} else if(objNum >= 0x98) {
		return 0;
	} else if(objNum >= 0x40) {
		objNum = ((objNum - 0x40) >> 2) + 0x40;
	} else {
		objNum = objNum;/*(a statement the compiler drops: it leaves the jmp before it)*/
	}
	LZC_load(BkgrndFile, bg, D_54E8, 0, D_3EB6);
	GR_DRAW_BITMAP(&D_3268, x + 1, y + 1, D_54E8, 0);
	LZC_load(FacesFile, objNum - 1, D_54E8, 0, D_3EB6);
	GR_DRAW_BITMAP(&D_3268, x, y, D_54E8, 0);
}

/*portrait backgrounds by place on the surface (first match)*/
/*2699*/struct tBgArea BgAreas[] = {
	{0x1E8, 0x208, 0x287, 0x25F, 4},
	{0x0E0, 0x260, 0x107, 0x287, 12},
	{0x140, 0x0C0, 0x187, 0x0EF, 12},
	{0x168, 0x218, 0x19F, 0x23F, 12},
	{0x290, 0x318, 0x2C7, 0x34F, 12},
	{0x188, 0x108, 0x1B7, 0x13F, 12},
	{0x3D0, 0x130, 0x3F7, 0x167, 12},
	{0x220, 0x120, 0x25F, 0x15F, 12},
	{0x228, 0x060, 0x25F, 0x08F, 12},
	{0x0F0, 0x0A8, 0x307, 0x12F, 1},
	{0x1F8, 0x040, 0x257, 0x08F, 1},
	{0x1D8, 0x140, 0x207, 0x167, 1},
	{0x340, 0x0E8, 0x3FF, 0x1DF, 1},
	{0x130, 0x130, 0x167, 0x1A7, 1},
	{0x168, 0x1D0, 0x209, 0x227, 1},
	{0x058, 0x268, 0x090, 0x2A8, 1},
	{0x118, 0x040, 0x13F, 0x077, 1},
	{0x280, 0x310, 0x29F, 0x327, 1},
	{0x160, 0x0F8, 0x21F, 0x1A7, 11},
	{0x230, 0x160, 0x2C7, 0x1E7, 11},
	{0x100, 0x048, 0x15F, 0x0A7, 11},
	{0x178, 0x100, 0x1C8, 0x12F, 11},
	{0x120, 0x0B0, 0x13F, 0x0DF, 11},
	{0x1A8, 0x078, 0x1EF, 0x0B7, 11},
	{0x0A8, 0x238, 0x100, 0x27F, 11},
	{0x148, 0x228, 0x1BF, 0x287, 11},
	{0x000, 0x000, 0x3FF, 0x3FF, 2}
};

/*the portrait background (entry of bkgrnd.lzc) at x,y: 10 on level 3,
  9 underground, else from BgAreas, +4 (night variant) for the first ones
  at night (18:01-5:59)*/
/*C_276B_1AFD*/PortraitBg(int16 x, int16 y) {
	int16 i, bg;
	int16 t;

	if(MapZ == 3) {
		bg = 10;
	} else if(MapZ > 0) {
		bg = 9;
	} else {
		for(i = 0; i < 27; i ++) {
			if(BgAreas[i].x0 < x && BgAreas[i].y0 < y && BgAreas[i].x1 > x && BgAreas[i].y1 > y) {
				bg = BgAreas[i].bg;
				break;
			}
		}
		t = D_27EB * 60 + D_27EA;
		if((t >= 1081 || (t >= 0 && t <= 359)) && bg < 5)
			bg += 4;
	}
	return bg;
}

/*draw the status area background (bkgrnd.lzc entry 0) at 183,6*/
/*C_276B_1BAF*/C_276B_1BAF() {
	C_2A81_0098(2);
	LZC_load("bkgrnd.lzc", 0, D_54E8, 0, D_3EB6);
	GR_DRAW_BITMAP(&D_3268, 0xb7, 6, D_54E8, 0);
}

/*load the conversation script of NPC objNum (an entry of talk.lzc) into
  dest. Temporary NPCs (0xc0 and up) of the types 0x140, 0x13c, 0x13d, 0x123
  have generic scripts (0x49..0x4c), the others none; NPCs 0x40-0x97 share
  scripts 8 by 8. returns objNum, or 0 if the NPC has no conversation*/
/*C_276B_1BFC*/LoadConversation(int16 objNum, void far *dest) {
	/*27A7*/static char loadName[] = "talk.lzc";
	int16 objType;
	int16 convIndex;

	objType = GetType(objNum);
	if(objNum >= 0xc0) {
		switch(GetType(objNum)) {
			case 0x140: convIndex = 0x49; break;
			case 0x13c: convIndex = 0x4a; break;
			case 0x13d: convIndex = 0x4b; break;
			case 0x123: convIndex = 0x4c; break;
			default: return 0;
		}
	} else if(objNum >= 0x98) {
		return 0;
	} else if(objNum >= 0x40) {
		convIndex = ((objNum - 0x40) >> 3) + 0x3e;
	} else {
		convIndex = objNum - 2;
	}
	C_2A81_0098(4);
	LZC_load(loadName, convIndex, dest, 0, D_3EB6);
	if(osiError)
		return 0;
	return objNum;
}

/*same choice as LoadConversation: returns objNum if NPC objNum has a
  conversation script (a non empty entry of talk.lzc), else 0*/
/*C_276B_1CC5*/HasConversation(int16 objNum) {
	/*27B0*/static char checkName[] = "talk.lzc";
	int16 objType;
	int16 convIndex;

	objType = GetType(objNum);
	if(objNum >= 0xc0) {
		switch(GetType(objNum)) {
			case 0x140: convIndex = 0x49; break;
			case 0x13c: convIndex = 0x4a; break;
			case 0x13d: convIndex = 0x4b; break;
			case 0x123: convIndex = 0x4c; break;
			default: return 0;
		}
	} else if(objNum >= 0x98) {
		return 0;
	} else if(objNum >= 0x40) {
		convIndex = ((objNum - 0x40) >> 3) + 0x3e;
	} else {
		convIndex = objNum - 2;
	}
	C_2A81_0098(4);
	if(LIB_entrySize(checkName, convIndex) <= 0 || osiError)
		return 0;
	return objNum;
}

/*display equipment bg chara: the 2x2 tiles paper doll at screen x,y,
  one of three bodies by kind*/
/*C_276B_1D87*/DrawPaperDoll(int16 x, int16 y, int16 kind) {
	switch(kind) {
		case 1: case 4: case 7:
			GR_PUT_TILE_SCREEN(0x194, x,      y);
			GR_PUT_TILE_SCREEN(0x195, x + 16, y);
			GR_PUT_TILE_SCREEN(0x196, x,      y + 16);
			GR_PUT_TILE_SCREEN(0x197, x + 16, y + 16);
			break;
		case 6: case 0x25: case 0x30:
			GR_PUT_TILE_SCREEN(0x198, x,      y);
			GR_PUT_TILE_SCREEN(0x199, x + 16, y);
			GR_PUT_TILE_SCREEN(0x19a, x,      y + 16);
			GR_PUT_TILE_SCREEN(0x19b, x + 16, y + 16);
			break;
		default:
			GR_PUT_TILE_SCREEN(0x190, x,      y);
			GR_PUT_TILE_SCREEN(0x191, x + 16, y);
			GR_PUT_TILE_SCREEN(0x192, x,      y + 16);
			GR_PUT_TILE_SCREEN(0x193, x + 16, y + 16);
	}
}

/*tile in 0x4c-0x5c or 0x6c-0x6f (drawn brown by the peer map)*/
/*C_276B_1EB2*/C_276B_1EB2(int16 tile) {
	return (tile >= 0x4c && tile <= 0x5c) || (tile >= 0x6c && tile <= 0x6f);
}

/*peer: draw one map tile as a 4x4 dot at x,y:
  tileNum < 0 = an NPC (white with green corners), 0xff = black,
  damaging = red, C_276B_1EB2 = brown, C_1810_0F71 = gray, impassable =
  blue (wet) or light green, other terrain (< 0x100) = green;
  passable object tiles are not drawn*/
/*C_276B_1ED9*/static peer_tile(int16 tileNum, int16 x, int16 y) {
	int16 color;
	int16 i, j;

	if(tileNum < 0) {
		color = 15;
	} else if(tileNum == 0xff) {
		color = 0;
	} else if(TerrainType[tileNum] & 8) {
		color = ColorRed;
	} else if(C_276B_1EB2(tileNum)) {
		color = ColorBrown;
	} else if(C_1810_0F71(tileNum)) {
		color = ColorGray;
	} else if(TerrainType[tileNum] & 2) {
		if(TerrainType[tileNum] & 1)
			color = ColorBlue;
		else
			color = ColorLightGreen;
	} else {
		color = -1;
	}
	if(color < 0 && tileNum >= 0x100)
		return;
	if(color < 0)
		color = ColorGreen;
	GR_SET_COLOR(color);
	for(i = 0; i < 4; i ++) {
		for(j = 0; j < 4; j ++)
			GR_PUT_PIXEL(i + x, j + y);
	}
	if(color == 15) {
		GR_SET_COLOR(ColorGreen);
		GR_PUT_PIXEL(x, y);
		GR_PUT_PIXEL(x + 3, y);
		GR_PUT_PIXEL(x, y + 3);
		GR_PUT_PIXEL(x + 3, y + 3);
	}
}

/*peer:
  overview of the whole loaded 40x40 area, 4x4 pixels per tile, drawn
  over the map window. Underground only the tiles reachable from the party
  (AreaFlags bit 0x80, computed by VIS_FloodFill) are shown. The party is a
  white square. Waits for a key (without animations), then recomputes the view*/
/*C_276B_2008*/PeerMap() {
	int16 row, col;
	int16 rowStart, rowEnd, colStart, colEnd;
	int16 tile, objNum, mouseState, isUnderground;

	isUnderground = (MapZ > 0 && MapZ < 5);
	for(row = 0; row < AREA_H; row ++) {
		for(col = 0; col < AREA_W; col ++)
			D_3EBA[row][col] =
			D_6FFC[row][col] = 0;
	}
	rowStart = colStart = 0;
	rowEnd = colEnd = 40;
	if(isUnderground) {
		C_09DA_0144(D_069C + 5, D_069E + 5, 40, 2);
	} else if(MapZ == 0) {
		/*surface: special cases near the world edges (0x400 wrap)*/
		if(D_BAFE == 0x3f0) {
			colStart = 8;
		} else if(D_BAFE == 0x3e0) {
			colEnd = 0x20;
		} else if(D_BB00 >= 0x3f0) {
			rowStart = 0x400 - D_BB00;
		} else if(D_BB00 == 0x3e0 || D_BB00 == 0x3e8) {
			rowEnd = 0x400 - D_BB00;
		}
	}
	D_ED0C->toBackBuffer = 1;
	GR_SET_COLOR(0);
	GR_FILL_RECT(8, 16, 167, 159);
	/*terrain*/
	for(row = rowStart; row < rowEnd; row ++) {
		for(col = colStart; col < colEnd; col ++) {
			tile = D_470A[row][col];
			if(isUnderground && !(D_3EBA[row][col] & 0x80))
				tile = 0xff;
			peer_tile(tile, (col << 2) + 8, (row << 2) + 8);
		}
	}
	/*objects (NPC #0 only if under player control)*/
	for(objNum = C_0A5D_0943(D_BAFE, D_BB00, D_BAFE + (AREA_W-1), D_BB00 + (AREA_H-1)); objNum >= 0; objNum = C_0A5D_0873()) {
		if(objNum == 0 && (D_6CF0[objNum] & 0x80) == 0)
			continue;
		col = GetX(objNum) - D_BAFE;
		row = GetY(objNum) - D_BB00;
		if(isUnderground && (D_3EBA[row][col] & 0x80) == 0)
			continue;
		if(objNum < 0x100 && objNum >= 0) {
			peer_tile(-1, (col << 2) + 8, (row << 2) + 8);
		} else {
			tile = TILE_FRAME(objNum);
			peer_tile(tile, (col << 2) + 8, (row << 2) + 8);
			/*multi-tile objects extend to the left/up*/
			if((TileFlag[tile] & 0x80) && col) {
				if((TileFlag[tile] & 0x40) && row) {
					peer_tile(tile - 2, (col << 2) + 8, (row << 2) + 4);
					peer_tile(tile - 3, (col << 2) + 4, (row << 2) + 4);
				}
				peer_tile(tile - 1, (col << 2) + 4, (row << 2) + 8);
			} else if((TileFlag[tile] & 0x40) && row) {
				peer_tile(tile - 1, (col << 2) + 8, (row << 2) + 4);
			}
		}
	}
	/*the party (center of the view)*/
	GR_SET_COLOR(15);
	GR_FILL_RECT((D_069C << 2) + 28, (D_069E << 2) + 28, (D_069C << 2) + 31, (D_069E << 2) + 31);
	GR_SET_COLOR(0);
	if(D_BAFE == 0x3f0)
		;/*(empty: the comparison is in the original)*/
	D_ED0C->toBackBuffer = 0;
	mouseState = C_05D0_0004(8, 16, 167, 159);
	GR_FADE_TO_SCREEN(8, 16, 167, 159, 8, 16);
	C_05D0_004D(mouseState);
	D_03AE = 0;
	C_05D0_22C8();
	D_03AE = 1;
	ScreenFade = 1;
	C_09DA_0327();
}

/*fill the map window with ColorPaper and show it (fading in if fade)*/
/*C_276B_23CA*/ClearMapWindow(int16 fade) {
	int16 mouseState;

	mouseState = C_05D0_0004(8, 16, 167, 159);
	D_ED0C->toBackBuffer = 1;
	GR_SET_COLOR(ColorPaper);
	GR_FILL_RECT(8, 16, 167, 159);
	if(fade)
		GR_FADE_TO_SCREEN(8, 16, 167, 159, 8, 16);
	else
		GR_COPY_TO_SCREEN(8, 16, 167, 159, 8, 16);
	D_ED0C->toBackBuffer = 0;
	C_05D0_004D(mouseState);
}

/*show the pictures of MIDGAMEn.LZC (entries 4*i + 3, i < count) in the map
  window, each with up to 3 pages of text (entries 4*i + 0..2, "*E" ends
  them) printed under it, waiting for a key after each page; text[i], when
  not 0, is given to C_1DBD_0D8F before each picture*/
/*C_276B_245A*/ShowMidgame(int16 n, int16 count, unsigned char *text) {
	int16 saveBack;
	int16 j;
	int16 saveD03CD;
	char fileName[13] = "MIDGAMEX.LZC";
	int16 disks[6] = {0, 2, 2, 7, 7, 7};
	int16 i, k;

	fileName[7] = n + '0';
	D_0A39 = 1;
	k = 0;
	saveBack = D_ED0C->toBackBuffer;
	D_ED0C->toBackBuffer = 0;
	saveD03CD = D_03CD;
	D_03CD = 0x9e;
	HideMouse
	for(i = 0; i < count; i ++, k ++) {
		if(text[k])
			C_1DBD_0D8F(text[k]);
		C_2A81_0098(disks[n]);
		if(LZC_load(fileName, i * 4 + 3, D_50F6, 0, D_3EB6) == 0)
			break;
		GR_DRAW_BITMAP(&D_3268, 8, 16, D_50F6, 0);
		GR_SET_COLOR(ColorPaper);
		GR_FILL_RECT(0xb8, 0x80, 0x137, 0xbf);
		C_05D0_25E4(0, 0);
		D_B6B6 = D_03EC->rows;
		for(j = 0; j < 3; j ++) {
			C_2A81_0098(disks[n]);
			LZC_load(fileName, i * 4 + j, D_6EF8, 0, D_3EB6);
			if(D_6EF8[0] == '*' && D_6EF8[1] == 'E')
				break;
			D_03EC->fgColor = D_24E6;
			C_05D0_2DB9(D_6EF8);
			D_03EC->fgColor = D_24F0;
			C_05D0_22C8();
			GR_SET_COLOR(ColorPaper);
			GR_FILL_RECT(0xb8, 0x80, 0x137, 0xbf);
			C_05D0_25E4(0, 0);
			D_B6B6 = D_03EC->rows;
		}
		D_03EC->fgColor = D_24E6;
	}
	ShowMouse
	D_03CD = saveD03CD;
	D_ED0C->toBackBuffer = saveBack;
}
