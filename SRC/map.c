/* flags: -d -mm -p -c -Z -O -y */
/*
 * The Savage Empire (Worlds of Ultima) - GAME.EXE
 * segment 08CB: the map (Ultima VI's map.c).
 *
 * Ground tiles are stored in two files:
 * - "chunks": 8x8-tile blocks (64 bytes each), referenced by a 12-bit index;
 * - "map": the chunk indexes, packed 2 per 3 bytes (12 bits each).
 *   The surface (z=0, 1024x1024 tiles) is split in 8x8 "superchunks"
 *   of 16x16 chunks (0x180 bytes each); only a 2x2 window of superchunks
 *   is kept in the chunk index buffer (D_BAFA). The other levels are 32x32
 *   chunks (0x600 bytes) each and are loaded entirely.
 * The 40x40 tiles around the party (D_470A, origin D_BAFE/D_BB00) are
 * rebuilt from a small LRU cache of 16 chunks (D_B6FA).
 * This module also moves the whole party between places: gathering
 * (PartyEnter), spreading out (PartyExit), teleport, going in and out
 * of caves and dungeons by their entrances, and coming back to life
 * after the party died.
 */
#include "se.h"

#define AREA_W 40
#define AREA_H 40

/*a text window of the console*/
struct tClipInfo/*size:9*/ {
	unsigned char originX, originY, cols, rows;
	unsigned char curX, curY;
	unsigned char fgColor, bgColor;
	unsigned char attrFlags;/*1 wrap, 2 center, 4 capitalize, 8 runes, 0x10 paginate, 0x20 scroll*/
};

/*the graphic driver: D_ED10's offset selects the entry*/
#define GR_PUT_TILE_SCREEN(tile,x,y) (D_ED10.iii.ofs = 0x2d, (*D_ED10.f)(tile,x,y))
#define GR_SET_ANIM_FRAME(a,b)       (D_ED10.iii.ofs = 0x4b, (*D_ED10.f)(a,b))

/*OSI file routines (asm)*/
extern int16 C_0000_0245(char *);/*OSI_open*/
extern C_0000_0284(int16);/*OSI_close*/
extern int32 C_0000_0336(int16, int32, int32, void far *);/*OSI_read*/
extern int16 C_2B57_001F(int16, int16);/*OSI_rand(lo, hi)*/

/*other game modules*/
extern char D_0202;
extern int16 D_03B1;
extern int16 D_03C5;/*index in the party of the member who acts?*/
extern struct tClipInfo *D_03EC;/*CurClip*/
extern int16 D_079E;/*MoveAppliesTerrain*/
extern int16 D_0D45;/*VisibilityDirty*/
extern int16 D_0D4B;
extern int16 D_0DE4;
extern int16 D_2360;/*ForceHatching*/
extern int16 D_24D6;/*ScreenFade*/
extern unsigned char D_27EC;/*day of the month (1-28)*/
extern unsigned char D_27ED;/*month (1-12)*/
extern int16 D_27EE;/*year*/
extern unsigned char D_27F2;/*Active*/
extern int16 D_27F4;/*MapX*/
extern int16 D_27F6;/*MapY*/
extern int16 D_27F8;/*MapZ*/
extern char D_27FA[2];
extern char D_2860;/*InCombat*/
extern char D_2861;
extern char D_2862;
extern unsigned char D_329C[];/*ObjStatus*/
extern char D_44FA[];/*MovePts*/
extern unsigned char far *D_4602;/*NPCFlag*/
extern unsigned char D_4606[];/*DEXTE*/
extern unsigned char far *D_4706;/*NPCMode*/
extern unsigned char D_470A[AREA_H][AREA_W];/*AreaTiles*/
extern unsigned char D_4EDA[];/*Party*/
extern unsigned char far *D_5100;
extern uint16 D_51E4[];/*ExpPoints?*/
extern int16 D_54F0[];/*Amount: quantity (low byte), quality (high byte)*/
extern unsigned char D_6CF0[];/*NPCStatus*/
extern unsigned char D_6DF4[];/*HitPoints*/
extern int16 far *D_6EF4;/*BaseTile*/
extern unsigned char D_6EFC[];/*TalkFlags (per NPC); D_6EFC[1] & 4: a story flag (the trap door to the underground city has fallen?)*/
extern struct coord D_78E4[];/*ObjPos*/
extern unsigned char D_9CF6;/*PartySize*/
extern uint16 D_9CFA[];/*ObjShapeType*/
extern int16 far *D_B53E;/*OrigShapeType*/
extern int16 D_B642;/*Selection.obj*/
extern int16 D_B6BA[16];/*ChunkCacheAge*/
extern int16 D_B6DA[16];/*ChunkCacheIdx*/
extern unsigned char D_B6FA[16][8][8];/*ChunkCache*/
extern unsigned char far *D_BAFA;/*ChunkIndexBuf*/
extern int16 D_BAFE;/*AreaX*/
extern int16 D_BB00;/*AreaY*/
extern int16 D_C17C[];/*Link*/
extern int16 D_D98C[AREA_H][AREA_W];/*MapObjPtr*/
extern int16 D_EBD2[0x40];/*MissileHitList*/
extern union tHandler D_ED10;/*GrDriver*/
extern int16 ViewX, ViewY;

extern C_0359_0CD4();/*DisplayTick*/
extern C_0359_11B6(int16);/*PassTime*/
extern C_0359_1905();/*SetPartyMode*/
extern int16 C_05D0_0004(int16, int16, int16, int16);
extern C_05D0_004D(int16);
extern cdecl C_05D0_2DB9(char far *, ...);/*CON_printf*/
extern VIS_UpdateView();
extern int16 C_0A5D_0677(int16, int16, int16);/*OBJ_FindTypeAt(type, x, y)*/
extern int16 C_0A5D_0A45(int16, int16, int16);
extern int16 C_0A5D_0EB6(int16, int16, int16, int16, int16, int16);/*AddObj?*/
extern C_0A5D_1506(int16);/*DeleteObj*/
extern C_0A5D_1727(int16, int16, int16);/*OBJ_PurgeFarLocals*/
extern C_0A5D_1B36(int16);/*OBJ_ReviveFromBody*/
extern C_0A5D_1C76(int16, int16, int16, int16);/*MoveObj*/
extern C_0A5D_2DD5();/*OBJ_RebuildMapObjPtr*/
extern C_0A5D_3083(int16, int16, int16);/*LoadNewRegions*/
extern C_0A5D_3577();/*ShowObjects*/
extern C_0E13_0BA1();
extern C_11FF_00B0(int16);/*SPELL_ReviveNPC*/
extern C_130F_1EC7(int16, int16);/*MoveFollowers*/
extern C_130F_384C(int16, int16, int16);/*TryStraightMove*/
extern int16 C_130F_3AB8(int16, int16, int16);/*TryMoveTo*/
extern int16 C_1810_0068(int16, int16, int16);/*COMBAT_TileDistance*/
extern C_1810_023B();/*COMBAT_breakOff*/
extern int16 C_1810_062A(int16);/*MaxHP*/
extern C_2667_08E0();/*EGG_hatchArea*/
extern C_2701_01F1();/*MUS_PlayAreaSong?*/
extern C_2701_0648(int16);/*MUS_PlaySfx?*/
extern C_2701_0684();
extern C_276B_1756();/*DrawSkyBar*/
extern C_2A81_0098(int16);/*DISK_confirm*/

#define GetType(i) (D_9CFA[i] & 0x3ff)
#define GetFrame(i) (D_9CFA[i] >> 10)
#define GetQual(i) (((unsigned char *)(D_54F0 + (i)))[1])
#define GetX(i) ((*(int16 *)&(D_78E4[i].raw[0]))&0x3ff)
#define GetY(i) (((*(int16 *)&(D_78E4[i].raw[1]))>>2)&0x3ff)
#define GetZ(i) (D_78E4[i].raw[2] >> 4)
#define TILE_FRAME(i) (D_6EF4[GetType(i)] + GetFrame(i))

/*04C2*/int16 MapCoordMask = 0x3FF;/*coordinate wrap mask: 0x3ff on surface, 0xff underground*/
/*04C4*/int16 ChunksFile = -1;/*file "chunks" handler*/
/*04C6*/int16 MapFile = -1;/*file "map" handler*/
/*04C8*/int16 LoadedMapZ = -1;/*level (z) currently loaded in the chunk index buffer*/
/*04CA*/int16 LoadedSuperChunks[4] = {-1, -1, -1, -1};/*surface: ids of the 2x2 superchunks loaded (top-left, top-right, bottom-left, bottom-right)*/
/*04D2*/int16 BuiltAreaX = -1;/*AreaX of the area tiles currently built*/
/*04D4*/int16 BuiltAreaY = -1;/*AreaY of the area tiles currently built*/
/*04D6*/int16 BuiltAreaZ = -1;/*MapZ of the area tiles currently built*/

/*where the entrances lead (x, y), by the entrance object's quality (see C_08CB_0855)*/
/*04D8*/int16 D_04D8[8][2] = {/*object 0x16, quality 1-8*/
	{0x18C,0x044},{0x258,0x0DF},{0x08A,0x1C9},{0x08B,0x1FD},
	{0x170,0x150},{0x279,0x1D0},{0x29A,0x2B1},{0x0EA,0x03D}
};
/*04F8*/int16 D_04F8[13][2] = {/*object 0x14, quality 0-12 (level 1)*/
	{0x161,0x279},{0x08B,0x00B},{0x0DA,0x01B},{0x013,0x053},
	{0x00B,0x0CB},{0x062,0x053},{0x0EB,0x054},{0x0E3,0x0B3},
	{0x0F3,0x0CC},{0x033,0x0F3},{0x123,0x045},{0x110,0x045},
	{0x1F1,0x04D}
};
/*052C*/int16 D_052C[17][2] = {/*object 0xe3, quality 104-120 (surface)*/
	{0x389,0x2ED},{0x335,0x15D},{0x3DA,0x19D},{0x08C,0x24E},
	{0x07C,0x256},{0x06C,0x25E},{0x34D,0x28E},{0x36D,0x28E},
	{0x35D,0x296},{0x37D,0x296},{0x2AD,0x176},{0x2C5,0x18D},
	{0x2D5,0x19E},{0x2DD,0x19E},{0x2BD,0x156},{0x3BC,0x0EC},
	{0x3A1,0x34D}
};
/*0570*/int16 D_0570[20][2] = {/*object 0xa3, quality 101-120 (level 2)*/
	{0x02D,0x082},{0x019,0x083},{0x051,0x093},{0x0D1,0x05B},
	{0x095,0x0EC},{0x04A,0x0C3},{0x0F4,0x0AC},{0x0E4,0x0B4},
	{0x0D4,0x0BC},{0x08D,0x03C},{0x0AD,0x03C},{0x09D,0x044},
	{0x0BD,0x044},{0x00D,0x0F4},{0x0B5,0x0E4},{0x0C5,0x0F4},
	{0x0CD,0x0F4},{0x0ED,0x0F4},{0x014,0x0DA},{0x0B1,0x083}
};
/*05C0*/int16 D_05C0[15][2] = {/*C_08CB_0B8B on the surface, by quality*/
	{0x19C,0x123},{0x0CE,0x26B},{0x171,0x25C},{0x2B1,0x307},
	{0x26B,0x25A},{0x067,0x267},{0x216,0x11E},{0x178,0x0AD},
	{0x0B6,0x1C8},{0x0B8,0x1C9},{0x0BA,0x1C9},{0x0BB,0x1C7},
	{0x0B6,0x1C6},{0x0BB,0x1C5},{0x0B7,0x1C4}
};

/*superchunk id = row * 8 + column (each superchunk is 128x128 tiles)*/
#define MK_MAP_ID(x,y) (((x) >> 7) + (((y) >> 4) & 0x38))

/*make sure chunk #chunkIdx is in the chunk cache (load it over the
  least recently used entry if needed); returns its cache slot*/
static /*C_08CB_0003*/cacheChunk(int16 chunkIdx) {
	int16 i, cacheIdx;
	int16 oldestIdx;

	cacheIdx = -1;
	oldestIdx = 0;
	/*lookup chunk in cache*/
	for(i = 0; i < 16; i ++) {
		if(D_B6DA[i] == chunkIdx) {
			cacheIdx = i;
			break;
		}
		if(D_B6BA[i] > D_B6BA[oldestIdx])
			oldestIdx = i;
	}
	/*load if not cached*/
	if(cacheIdx < 0) {
		cacheIdx = oldestIdx;
		D_B6DA[cacheIdx] = chunkIdx;
		C_2A81_0098(3);
		ChunksFile = C_0000_0245("chunks");
		C_0000_0336(ChunksFile, (int32)chunkIdx << 6, 0x40, D_B6FA[cacheIdx]);
		C_0000_0284(ChunksFile);
	}
	/*update chunks "age"*/
	for(i = 0; i < 16; i ++) {
		if(D_B6BA[i] < D_B6BA[cacheIdx])
			D_B6BA[i] ++;
	}
	D_B6BA[cacheIdx] = 0;

	return cacheIdx;
}

/*load into the chunk index buffer the chunk indexes needed for (x, y, z):
  the whole level if z != 0, else a 2x2 window of superchunks
  containing (x, y); the window is only moved when (x, y) leaves it*/
/*C_08CB_00D0*/MAP_LoadChunkIndexes(int16 x, int16 y, int16 z) {
	int16 mapId, i;
	int16 mustLoad, colDiff, rowDiff, bufOfs;
	int32 fileOfs;
	unsigned char far *pSlot;

	mustLoad = (LoadedMapZ != z);
	if(!mustLoad && z == 0) {
		mapId = MK_MAP_ID(x, y);
		mustLoad = 1;
		for(i = 0; i < 4; i ++) {
			if(LoadedSuperChunks[i] == mapId) {
				mustLoad = 0;
				break;
			}
		}
	}
	if(!mustLoad)
		return;
	if(z != 0) {
		C_2A81_0098(8);
		MapFile = C_0000_0245("map");
		/*z * 0x600 + 0x5a00 = 0x6000 + (z - 1) * 0x600*/
		C_0000_0336(MapFile, (int32)((z + z + z) << 9) + 0x5a00, 4 * 0x180, D_BAFA);
		C_0000_0284(MapFile);
		LoadedMapZ = z;
		for(i = 0; i < 4; i ++)
			LoadedSuperChunks[i] = -1;
	} else {
		mapId = MK_MAP_ID(x, y);
		if(LoadedSuperChunks[0] >= 0) {
			/*move the current window just enough to contain mapId
			  (column/row differences are modulo 8, the world wraps)*/
			colDiff = (mapId - LoadedSuperChunks[0]) & 7;
			rowDiff = (mapId - LoadedSuperChunks[0]) & 0x38;
			mapId = LoadedSuperChunks[0];
			if(colDiff > 1 && colDiff < 5)
				mapId = (mapId & 0x38) + ((mapId + colDiff - 1) & 7);
			else if(colDiff > 4)
				mapId = (mapId & 0x38) + ((mapId + colDiff) & 7);
			/*note: "- 1" here only lowers the row when the column is 0
			  ("- 8" was probably meant); the window still contains the target*/
			if(rowDiff > 0x08 && rowDiff < 0x28)
				mapId = (mapId & 7) + ((mapId + rowDiff - 1) & 0x38);
			else if(rowDiff > 0x20)
				mapId = (mapId & 7) + ((mapId + rowDiff) & 0x38);
		}
		if(mapId != LoadedSuperChunks[0]) {
			LoadedSuperChunks[0] = mapId;
			LoadedSuperChunks[1] = (mapId + 1) & 0x3f;
			LoadedSuperChunks[2] = (mapId + 8) & 0x3f;
			LoadedSuperChunks[3] = (mapId + 9) & 0x3f;
			C_2A81_0098(8);
			MapFile = C_0000_0245("map");
			for(i = 0; i < 4; i ++) {
				/*superchunk id * 0x180*/
				fileOfs = (int32)LoadedSuperChunks[i] << 7; fileOfs += fileOfs << 1;
				/*window slot * 0x180*/
				bufOfs = i << 7; bufOfs += bufOfs << 1;
				pSlot = D_BAFA + bufOfs;
				C_0000_0336(MapFile, fileOfs, 0x180, pSlot);
			}
			C_0000_0284(MapFile);
		}
		LoadedMapZ = 0;
	}
}

/*recompute AreaX/AreaY (40x40 area aligned on chunks, party around the
  middle) and ViewX/ViewY (top-left of the 11x11 view inside the area);
  if the area moved, re-place the NPCs, rebuild the area tiles from the
  chunks and hatch the eggs; then ask for a view redraw*/
/*C_08CB_02FA*/MAP_BuildArea() {
	unsigned char *pMap, *pChunk;
	int16 i, j;
	int16 anX, anY, cacheIdx;

	D_BAFE = anX = (D_27F4 - 0x10) & 0x3f8;
	ViewX = (D_27F4 - anX - 5) & 0x1f;
	D_BB00 = anY = (D_27F6 - 0x10) & 0x3f8;
	ViewY = (D_27F6 - anY - 5) & 0x1f;
	if(D_27F8)
		MapCoordMask = 0xff;
	else
		MapCoordMask = 0x3ff;
	/*redundant?*/
	anX = D_BAFE;
	anY = D_BB00;
	/* */
	if(anX != BuiltAreaX || anY != BuiltAreaY || D_27F8 != BuiltAreaZ) {
		C_0A5D_2DD5();
		/*NPCs inside the new area but missing from MapObjPtr: put them back*/
		D_079E = 0;
		for(i = 0; i < 0x100; i ++) {
			anX = (GetX(i) - D_BAFE) & MapCoordMask;
			anY = (GetY(i) - D_BB00) & MapCoordMask;
			if(D_9CFA[i])
			if((D_329C[i] & 0x18) == 0)
			if(GetZ(i) == D_27F8)
			if(anX < 40 && anY < 40)
			if(D_D98C[anY][anX] != i)
				C_0A5D_1C76(i, GetX(i), GetY(i), D_27F8);
		}
		D_079E = 1;

		anX = D_BAFE;
		anY = D_BB00;
		C_0A5D_1727(D_BAFE + 20, D_BB00 + 20, D_27F8);

		BuiltAreaX = anX;
		BuiltAreaY = anY;
		BuiltAreaZ = D_27F8;
		/*copy the 5x5 chunks of the area (i = x, j = y)*/
		for(i = 0; i < 40; i += 8) {
			for(j = 0; j < 40; j += 8) {
				cacheIdx = cacheChunk(getChunkIdxAt((BuiltAreaX + i) & 0x3ff, (BuiltAreaY + j) & 0x3ff, D_27F8));
				pChunk = D_B6FA[cacheIdx][0];
				pMap = &(D_470A[j][i]);
				for(anY = 0; anY < 8; anY ++) {
					for(anX = 0; anX < 8; anX ++)
						*pMap++ = *pChunk++;
					pMap += 32;/*next line: AREA_W - 8*/
				}
			}
		}
		C_2667_08E0();
	}
	D_0D45 = 1;
}

/*empty the chunk cache*/
/*C_08CB_0517*/MAP_InitChunkCache() {
	int16 i;

	for(i = 0; i < 0x10; i++) {
		D_B6DA[i] = -1;
		D_B6BA[i] = i;
	}
}

/*load everything (objects regions, chunk indexes, area tiles)
  around MapX, MapY, MapZ*/
/*C_08CB_053A*/MAP_LoadAround() {
	C_0A5D_3083(D_27F4, D_27F6, D_27F8);
	MAP_LoadChunkIndexes(D_27F4, D_27F6, D_27F8);
	MAP_BuildArea();
}

/*gather the party on the leader's square (MapX, MapY) and take
  everybody off the map (z = 6), e.g. before a teleport or a level change.
  withAnimation: 0 = instant; else members walk to the leader for up to 30
  steps, with 1 the screen fades when a member disappears*/
/*C_08CB_0564*/PartyEnter(int16 withAnimation) {
	int16 i;

	/*(U6 dismounted the horses here)*/
	for(i = 0; i < D_9CF6; i ++);

	MAP_LoadAround();

	if(withAnimation) {
		int16 j, allArrived;
		for(j = 0; j < 30; j ++) {
			allArrived = 1;
			for(i = 0; i < D_9CF6; i ++) {
				int16 objNum = D_4EDA[i];
				/*wake up*/
				if(D_6CF0[objNum] & 0x04)
					D_9CFA[objNum] = D_B53E[objNum];
				D_6CF0[objNum] &= ~(0x04|0x02);
				if(GetZ(objNum) == D_27F8 && !((D_6CF0[objNum] & 0x16) || (D_4602[objNum] & 0x10))) {
					if(GetX(objNum) == D_27F4 && GetY(objNum) == D_27F6) {
						/*arrived: remove from map*/
						if(withAnimation) {
							C_0A5D_3577();
							C_0359_0CD4();
						}
						D_079E = 0;
						C_0A5D_1C76(objNum, D_27F4, D_27F6, 6);
						D_079E = 1;
						if(withAnimation == 1) {
							D_24D6 = 1;
							C_0A5D_3577();
							C_0359_0CD4();
						}
					} else if(!C_130F_3AB8(objNum, D_27F4, D_27F6)) {
						/*blocked: jump if adjacent, else random step*/
						if(C_1810_0068(objNum, D_27F4, D_27F6) < 2)
							C_0A5D_1C76(objNum, D_27F4, D_27F6, D_27F8);
						else
							C_130F_384C(objNum, C_2B57_001F(0, 3) << 1, 1);
					} else {
						allArrived = 0;
					}
				}
				D_44FA[objNum] = D_4606[objNum];
			}
			if(withAnimation) {
				C_0A5D_3577();
				C_0359_0CD4();
			}
			if(allArrived)
				break;
		}
	}

	/*whoever is left (asleep, paralyzed, other level...) is taken anyway*/
	D_079E = 0;
	for(i = 0; i < D_9CF6; i ++) {
		if(GetZ(D_4EDA[i]) != 6)
			C_0A5D_1C76(D_4EDA[i], D_27F4, D_27F6, 6);
	}
	D_079E = 1;
}

/*put the whole party back on the map at (x, y, z) and let the
  followers step out around the leader (5 steps)*/
/*C_08CB_0754*/PartyExit(int16 x, int16 y, int16 z, int16 withAnimation) {
	int16 i;

	D_079E = 0;
	for(i = 0; i < D_9CF6; i ++)
		C_0A5D_1C76(D_4EDA[i], x, y, z);
	D_079E = 1;

	C_0359_1905();

	if(withAnimation == 1) {
		D_24D6 = 1;
		C_0A5D_3577();
		C_0359_0CD4();
	}

	for(i = 0; i < 5; i ++) {
		C_130F_1EC7(D_4EDA[D_27F2], 1);
		if(withAnimation) {
			C_0A5D_3577();
			C_0359_0CD4();
		}
	}
	for(i = 0; i < D_9CF6; i ++)
		D_44FA[D_4EDA[i]] = D_4606[D_4EDA[i]];
}

/*move the whole party to (x, y, z)*/
/*C_08CB_07FE*/PartyTeleport(int16 x, int16 y, int16 z, int16 withAnimation) {
	PartyEnter(withAnimation);

	D_27F4 = x;
	D_27F6 = y;
	D_27F8 = z;
	MAP_LoadAround();

	D_2360 = 1;
	C_2667_08E0();
	D_2360 = 0;

	VIS_UpdateView();
	C_276B_1756();
	C_0359_11B6(0);
	PartyExit(x, y, z, withAnimation);
}

/*go through the entrance objNum (cave, stairs...): the party moves to the
  place its type and quality give (tables above)*/
/*C_08CB_0855*/C_08CB_0855(int16 objNum) {
	int16 toLevel1, found, other;
	int16 qual, type;

	toLevel1 = 0;
	qual = GetQual(objNum);
	type = GetType(objNum);
	if(D_2860)
		C_1810_023B();
	D_27F4 = GetX(objNum);
	D_27F6 = GetY(objNum);
	PartyEnter((type == 0xe3)?0:2);
	C_2701_0684();
	if(type == 0x14) {
		found = C_0A5D_0A45(D_4EDA[D_03C5], 0x15, -1);
		if(found >= 0) {
			GR_SET_ANIM_FRAME(TILE_FRAME(found), 0x10c);
			C_0E13_0BA1();
		}
		D_27F4 = D_04F8[qual][0];
		D_27F6 = D_04F8[qual][1];
		D_27F8 = 1;
		toLevel1 = 1;
	} else if(type == 0xa3) {
		D_27F4 = D_0570[qual - 101][0];
		D_27F6 = D_0570[qual - 101][1];
		D_27F8 = 2;
	} else if(type == 0x16) {
		found = C_0A5D_0A45(D_4EDA[D_03C5], 0x15, -1);
		if(found >= 0) {
			GR_SET_ANIM_FRAME(TILE_FRAME(found), 0x10b);
			C_0E13_0BA1();
		}
		D_27F4 = D_04D8[qual - 1][0];
		D_27F6 = D_04D8[qual - 1][1];
		if(qual == 8)
			D_27F8 = 2;
		else
			D_27F8 = 0;
	} else if(type == 0xe3) {
		D_27F4 = D_052C[qual - 104][0];
		D_27F6 = D_052C[qual - 104][1];
		D_27F8 = 0;
	} else if(type == 0x103) {
		D_27F4 = 0xc0;
		D_27F6 = 0x1cc;
		D_27F8 = 0;
	} else if(type == 0xa0) {
		D_27F4 = 0x9d;
		D_27F6 = 0x89;
		D_27F8 = 3;
	}
	MAP_LoadAround();
	if(toLevel1) {
		/*the way back: an object 0x16 of the same quality*/
		other = C_0A5D_0677(0x16, D_27F4, D_27F6);
		if(other >= 0 && GetQual(other) == qual) {
			D_9CFA[other] = GetType(other) + 0x400;
		} else {
			if(other >= 0)
				C_0A5D_1506(other);
			D_329C[C_0A5D_0EB6(0x416, 0, qual << 8, D_27F4, D_27F6, D_27F8)] &= ~0x20;
		}
	}
	VIS_UpdateView();
	C_276B_1756();
	C_0359_11B6(0);
	PartyExit(D_27F4, D_27F6, D_27F8, (type == 0xe3 || type == 0xa3)?0:2);
	if(type == 0x103 && (D_6EFC[1] & 4)) {
		D_03EC->attrFlags |= 0x10;
		C_05D0_2DB9("\nThe mechanism supporting the trap door fails just as you");
		C_05D0_2DB9(" and your companions leave the stairwell, sealing the entrance");
		C_05D0_2DB9(" to the underground city.\n");
		C_0A5D_1506(C_0A5D_0677(0xa0, 0xc0, 0x1cc));
		D_03B1 = 1;
	}
	C_2701_01F1();
}

/*go through objNum (object type 0x44): underground, to the other object
  0x44 of the same quality; on the surface, to the place of table D_05C0*/
/*C_08CB_0B8B*/C_08CB_0B8B(int16 objNum) {
	int16 type;
	int16 i, qual;

	qual = GetQual(objNum);
	type = GetType(objNum);
	if(D_2860)
		C_1810_023B();
	PartyEnter(1);
	if(D_27F8) {
		for(i = 0; i < 0xc00; i ++) {
			if(GetType(i) == 0x44 && i != objNum && GetQual(i) == qual) {
				D_27F4 = GetX(i);
				D_27F6 = GetY(i);
				break;
			}
		}
	} else {
		D_27F4 = D_05C0[qual][0];
		D_27F6 = D_05C0[qual][1];
	}
	MAP_LoadAround();
	VIS_UpdateView();
	C_276B_1756();
	C_0359_11B6(0);
	PartyExit(D_27F4, D_27F6, D_27F8, 1);
	C_2701_01F1();
}

/*after the party's death: bring the dead party members back to life,
  heal everybody (they lose a quarter of their experience?) and bring the
  party to (0x19c, 0x123); a week passes*/
/*C_08CB_0C73*/C_08CB_0C73() {
	int16 i, flag, clip;
	int16 memberObj, bodyObj;

	D_27F2 = -1;
	for(i = 0; i < 0xc0; i ++) {
		if((D_6CF0[i] & 0x80) && (D_6CF0[i] & 0x10)) {
			/*look for the NPC's dead body*/
			for(bodyObj = D_C17C[0x100]; bodyObj >= 0; bodyObj = D_C17C[bodyObj]) {
				if(GetType(bodyObj) == 0xf9 && GetQual(bodyObj) == i) {
					D_B642 = bodyObj;
					if(i == 1) {
						C_0A5D_1B36(bodyObj);
						D_6CF0[1] &= ~0x10;
						D_9CFA[1] = D_B53E[1];
						D_6CF0[1] &= ~0x1f;
					} else {
						D_079E = 0;
						C_11FF_00B0(bodyObj);
					}
					break;
				}
			}
		}
		if(D_4706[i] == 0x9b)
			D_4706[i] = 0x81;
	}
	for(i = 0; i < D_9CF6; i ++) {
		memberObj = D_4EDA[i];
		if(D_6CF0[memberObj] & 0x10)
			continue;
		if(D_6CF0[memberObj] & 0x04) {
			D_6CF0[memberObj] &= ~0x04;
			D_9CFA[memberObj] = D_B53E[memberObj];
		} else if(GetType(memberObj) == 0x133) {
			D_9CFA[memberObj] = D_B53E[memberObj];
		}
		D_6CF0[memberObj] = (D_6CF0[memberObj] & ~0x60) | 0x40;
		D_329C[memberObj] &= ~0x46;
		D_6CF0[memberObj] &= ~0x1e;
		D_6DF4[memberObj] = C_1810_062A(memberObj);
		D_51E4[memberObj] = D_51E4[memberObj] * 3 / 4;
		D_44FA[memberObj] = D_4606[memberObj];
	}
	for(i = 0; i < 2; i ++)
		D_27FA[i] = 0;
	for(i = 0; i < 0x40; i ++)
		D_EBD2[i] = -1;
	C_0359_1905();
	D_2860 = D_0D4B = 0;
	if(D_2861 < 0)
		flag = 1;
	else
		flag = 0;
	for(i = 0; i < D_9CF6; i ++) {
		if(D_6CF0[D_4EDA[i]] & 0x10) {
			D_4706[D_4EDA[i]] = 0;
		} else if(!(D_329C[D_4EDA[i]] & 0x04)) {
			D_4706[D_4EDA[i]] = flag;
			if(flag == 1)
				D_5100[D_4EDA[i]] = i;
		}
	}
	if(D_2861 < 0)
		i = 0;
	else
		i = D_2861;
	D_2862 = D_4706[D_4EDA[i]];
	D_4706[D_4EDA[i]] = 2;
	clip = C_05D0_0004(0x40, 0, 0x5f, 0xf);
	GR_PUT_TILE_SCREEN(0x173, 0x40, 0);
	GR_PUT_TILE_SCREEN(0x174, 0x50, 0);
	C_05D0_004D(clip);
	D_0DE4 = 0;
	PartyTeleport(0x19c, 0x123, 0, 0);
	/*a week passes*/
	if((D_27EC += 7) > 28) {
		D_27EC -= 28;
		if(++D_27ED > 12) {
			D_27ED = 1;
			D_27EE ++;
		}
	}
	D_24D6 = 1;
	C_0A5D_3577();
	C_2701_0648(0xd);
	C_0359_0CD4();
	D_0202 = 1;
	D_079E = 1;
}

/*12-bit index of the chunk containing tile (x, y, z)
  (loads the needed part of the map first)*/
/*C_08CB_0F9A*/getChunkIdxAt(register int16 x, int16 y, int16 z) {
	int16 chunkIdx, mapId, quadOfs;
	register int16 bufPos;

	x &= 0x3ff;
	y &= 0x3ff;
	MAP_LoadChunkIndexes(x, y, z);
	if(z != 0) {
		/*32x32 chunks: (chunkY * 32 + chunkX) * 1.5*/
		bufPos = ((x >> 3) & 0x1f) + ((y << 2) & 0x3e0);
		bufPos += bufPos >> 1;
		/*odd chunk: upper 12 bits, even chunk: lower 12 bits*/
		if(x & 8)
			chunkIdx = *((int16 far *)(D_BAFA + bufPos)) >> 4;
		else
			chunkIdx = *((int16 far *)(D_BAFA + bufPos)) & 0xfff;
	} else {
		/*which superchunk of the 2x2 window (x 0x100 chunk entries)*/
		mapId = MK_MAP_ID(x, y);
		quadOfs = 0;
		if((mapId - LoadedSuperChunks[0]) & 1)
			quadOfs = 0x100;
		if((mapId - LoadedSuperChunks[0]) & 8)
			quadOfs += 0x200;
		/*16x16 chunks: (quadOfs + chunkY * 16 + chunkX) * 1.5*/
		bufPos = quadOfs + ((x >> 3) & 0xf) + ((y << 1) & 0xf0);
		bufPos += bufPos >> 1;
		if(x & 8)
			chunkIdx = *((int16 far *)(D_BAFA + bufPos)) >> 4;
		else
			chunkIdx = *((int16 far *)(D_BAFA + bufPos)) & 0xfff;
	}

	return chunkIdx;
}

/*ground tile at (x, y, z): from the area tiles if inside the current area,
  else from the chunks*/
unsigned char /*C_08CB_107B*/GetTileAtXYZ(int16 x, int16 y, int16 z) {
	int16 cacheIdx;
	int16 relX, relY;

	x &= MapCoordMask;
	y &= MapCoordMask;
	relX = (x - D_BAFE) & MapCoordMask;
	relY = (y - D_BB00) & MapCoordMask;
	if(relX < 40 && relY < 40)
		return D_470A[relY][relX];

	cacheIdx = cacheChunk(getChunkIdxAt(x, y, z));
	return D_B6FA[cacheIdx][y & 7][x & 7];
}
