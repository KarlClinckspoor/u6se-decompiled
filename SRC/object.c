/* flags: -d -mm -p -c -Z -O -y */
/*
 * The Savage Empire (Worlds of Ultima) - GAME.EXE
 * segment 0A5D: object list management (Ultima VI's object.c).
 *
 * All objects live in parallel arrays indexed by an object number:
 * ObjStatus[], ObjPos[] (packed x/y/z or container number), ObjShapeType[]
 * (type | frame << 10), Amount[] (quantity | quality << 8) and Link[].
 *   0x000-0x0FF  NPCs/monsters (0xC0-0xFF temporary monster slots, kept on
 *                their own free list FreeMonsterHead/FreeMonsterCount)
 *   0x100        head of the main object list (never a real object)
 *   0x101-0xBFF  ordinary objects (free list FreeObjHead/FreeObjCount)
 * Link[] chains every loaded object into one list starting at Link[0x100],
 * map objects sorted by (y, x); the contents of an NPC or a container follow
 * their owner. MapObjPtr[][] caches the first object of every tile of the
 * 40x40 area around the party.
 *
 * Only the 2x2 outdoor areas (128x128 tiles) near the party, or the current
 * underground level, are kept in the list; the rest is in the
 * "savegame\objblkXY" files (".tmp" copies for areas left behind, renamed
 * over the real files when the game is saved, OBJ_CommitBlocks).
 *
 * The module also has the location/inventory/area iterators, object
 * creation/deletion/moving (with weight and inventory size checks), the
 * creatures made of several objects, and the 11x11 per-tile display stacks
 * (ShowObject/ShowObjects).
 * Object positions (ObjStatus & 0x18): 0x00 on the map, 0x08 contained,
 * 0x10 in an inventory, 0x18 equipped.
 */
#include "se.h"

#define AREA_W 40
#define AREA_H 40

/*an objblk file in memory (ScratchBuf, a byte buffer): 0x800 entries of 8
  bytes (status, struct coord position, shape, amount), then the object
  number of each entry (0x800 words)*/
#define BLK_STATUS(i) ScratchBuf[(i) << 3]
#define BLK_POS(i) (*(struct coord far *)&ScratchBuf[((i) << 3) + 1])
#define BLK_SHAPE(i) (*(uint16 far *)&ScratchBuf[((i) << 3) + 4])
#define BLK_AMOUNT(i) (*(int16 far *)&ScratchBuf[((i) << 3) + 6])
#define BLK_OBJNUM(i) (*(int16 far *)&ScratchBuf[((i) << 1) + 0x4000])

/*OSI file routines (asm)*/
extern int16 OSI_open(char *);/*OSI_open*/
extern OSI_close(int16);/*OSI_close*/
extern int16 OSI_create(char *);/*OSI_create*/
extern OSI_delete(char *);/*OSI_delete*/
extern int32 OSI_read(int16, int32, int32, void far *);/*OSI_read*/
extern int32 OSI_write(int16, int32, int32, void far *);/*OSI_write*/
extern OSI_rename(char *, char *);/*OSI_rename*/
extern int16 C_2B57_001F(int16, int16);/*OSI_rand(lo, hi)*/
extern void far *LZC_load(char far *name, int16 index, void far *out, int16 memflags, char far *work);

/*other game modules*/
extern char DirIncrX[];/*DirIncrX*/
extern char DirIncrY[];/*DirIncrY*/
extern unsigned char StatusDirty;/*StatusDirty*/
extern int16 MapAnimEnabled;/*MapAnimEnabled*/
extern int16 Erupting;/*Erupting?*/
extern int16 StatusMember;/*StatusMember*/
extern int16 StatusDisplay;/*StatusDisplay: 0x92 when the equipment view is shown*/
extern int16 MapCoordMask;/*MapCoordMask*/
extern int16 ViewX, ViewY;/*ViewX, ViewY*/
extern int16 VisibilityDirty;/*VisibilityDirty*/
extern int16 MapX, MapY, MapZ;/*MapX, MapY, MapZ*/
extern unsigned char D_27FB;
extern int16 DiskFullChoice;/*DiskFullChoice*/
extern unsigned char D_329C[];/*ObjStatus*/
extern char far *D_3EB6;/*the .lzc loader's work area*/
extern unsigned char D_3EBA[AREA_H][AREA_W];/*AreaFlags*/
extern unsigned char far *D_4602;/*NPCFlag*/
extern unsigned char far *D_4706;/*NPCMode*/
extern unsigned char D_4EDA[];/*Party*/
extern unsigned char far *D_50F6;/*ScratchBuf*/
extern char D_5104[][14];/*Names*/
extern unsigned char far *D_53E4;/*NPCComMode*/
extern unsigned char D_53E8[];/*STREN*/
extern char far *D_54E8;/*ScratchBuf + 0x36b0*/
extern int16 D_54F0[];/*Amount*/
extern unsigned char D_6CF0[];/*NPCStatus*/
extern unsigned char far *D_6DF0;/*TileFlag*/
extern unsigned char D_6DF4[];/*HitPoints*/
extern int16 far *D_6EF4;/*BaseTile*/
extern unsigned char D_6EFD;
extern unsigned char D_6F1D;
extern unsigned char D_6FFC[AREA_H][AREA_W];/*AreaLight*/
extern unsigned char far *D_7640;/*TileFlag2*/
extern struct coord D_78E4[];/*ObjPos*/
extern unsigned char D_9CF6;/*PartySize*/
extern uint16 D_9CFA[];/*ObjShapeType*/
extern unsigned char far *D_B4FA;/*TerrainType*/
extern int16 far *D_B53E;/*OrigShapeType*/
extern int16 D_BAFE, D_BB00;/*AreaX, AreaY*/
extern int16 D_BD04[3];/*types of the parts of a creature (C_0A5D_0D5B)*/
extern int16 D_BD0A[3];/*their frame shifts*/
extern unsigned char D_BD10[11][11];/*Obj_11x11: first display node of each view cell*/
extern int16 D_BD8A[];/*CellObjTile: tile | frame of each display node*/
extern unsigned char D_BF8A[];/*CellObjNext: next display node ([0]: free list)*/
extern int16 D_C08A[11][11];/*Tile_11x11*/
extern int16 D_C17C[];/*Link*/
extern int16 D_D97C;/*FreeMonsterCount*/
extern int16 D_D97E, D_D980, D_D984, D_D986, D_D988;/*SearchArea's rectangle: x1, x2, y1, y2, z*/
extern int16 D_D982;/*FreeMonsterHead*/
extern int16 D_D98A;/*FreeObjHead*/
extern int16 D_D98C[AREA_H][AREA_W];/*MapObjPtr*/
extern char D_E60C[];/*TileStringBuf*/
extern int16 D_E626;/*FreeObjCount*/
extern int16 D_E668;/*WeightEquip*/
extern int16 D_E66A;/*WeightInven*/
extern int16 D_EADE;

extern ApplyTerrainEffects(int16);/*ApplyTerrainEffects*/
extern cdecl CON_printf(char far *, ...);/*CON_printf*/
extern int16 GetWeight(int16);/*GetWeight*/
extern int16 GetTypeWeight(int16, int16);/*GetTypeWeight*/
extern STAT_ComputeWeights(int16);/*STAT_ComputeWeights*/
extern STAT_RefreshInventory(int16);/*STAT_RefreshInventory*/
extern int16 C_130F_0070(int16);
extern int16 C_130F_00CA(int16);
extern int16 IsOutOfArea(int16);
extern SubMov(int16, int16);
extern int16 MaxHP(int16);/*MaxHP*/
extern int16 IsContainerType(int16);/*IsContainerType*/
extern DISK_confirm(int16);/*DISK_confirm*/

#define DirIncrX DirIncrX
#define DirIncrY DirIncrY
#define StatusDirty StatusDirty
#define MapAnimEnabled MapAnimEnabled
#define StatusMember StatusMember
#define StatusDisplay StatusDisplay
#define MapCoordMask MapCoordMask
#define ViewX ViewX
#define ViewY ViewY
#define VisibilityDirty VisibilityDirty
#define MapX MapX
#define MapY MapY
#define MapZ MapZ
#define DiskFullChoice DiskFullChoice
#define ObjStatus D_329C
#define AreaFlags D_3EBA
#define NPCFlag D_4602
#define NPCMode D_4706
#define Party D_4EDA
#define ScratchBuf D_50F6
#define Names D_5104
#define NPCComMode D_53E4
#define STREN D_53E8
#define Amount D_54F0
#define NPCStatus D_6CF0
#define TileFlag D_6DF0
#define HitPoints D_6DF4
#define BaseTile D_6EF4
#define AreaLight D_6FFC
#define TileFlag2 D_7640
#define ObjPos D_78E4
#define PartySize D_9CF6
#define ObjShapeType D_9CFA
#define TerrainType D_B4FA
#define OrigShapeType D_B53E
#define AreaX D_BAFE
#define AreaY D_BB00
#define Obj_11x11 D_BD10
#define CellObjTile D_BD8A
#define CellObjNext D_BF8A
#define Tile_11x11 D_C08A
#define Link D_C17C
#define FreeMonsterCount D_D97C
#define FreeMonsterHead D_D982
#define FreeObjHead D_D98A
#define MapObjPtr D_D98C
#define TileStringBuf D_E60C
#define FreeObjCount D_E626
#define WeightEquip D_E668
#define WeightInven D_E66A

#define OSI_open OSI_open
#define OSI_close OSI_close
#define OSI_create OSI_create
#define OSI_delete OSI_delete
#define OSI_read OSI_read
#define OSI_write OSI_write
#define OSI_rename OSI_rename
#define OSI_rand C_2B57_001F
#define ApplyTerrainEffects ApplyTerrainEffects
#define CON_printf CON_printf
#define GetWeight GetWeight
#define GetTypeWeight GetTypeWeight
#define STAT_ComputeWeights STAT_ComputeWeights
#define STAT_RefreshInventory STAT_RefreshInventory
#define MaxHP MaxHP
#define IsContainerType IsContainerType
#define DISK_confirm DISK_confirm

/*this module*/
int16 C_0A5D_0D5B(int16, int16 *, int16 *);
int16 C_0A5D_0DEF(int16);
int16 AddMapObj(int16, int16, int16, int16, int16, int16);
int16 AddInvObj(int16, int16, int16, int16);
DeleteObj(int16);
int16 OBJ_IsInside(int16, int16);
int16 MoveObj(int16, int16, int16, int16);
int16 InsertObj(int16, int16, int16);
int16 QuanType(int16);

#define LOCXYZ 0
#define CONTAINED 8
#define INVEN 0x10
#define EQUIP 0x18

#define GetQuan(o) (((unsigned char *)(Amount + (o)))[0])
#define GetQual(o) (((unsigned char *)(Amount + (o)))[1])
#define SetQuan(o, v) (GetQuan(o) = (v))
#define SetQual(o, v) (GetQual(o) = (v))
#define AddQuan(o, v) (GetQuan(o) += (v))
#define SubQuan(o, v) (GetQuan(o) -= (v))
#define QualQuan(quality, quantity) ((quality) << 8 | (quantity))

#define SetCoordUse(i, v) (ObjStatus[i] = (ObjStatus[i] & ~0x18) | (v))
#define GetCoordUse(i) (ObjStatus[i] & 0x18)
#define IsHeldByNPC(i) (ObjStatus[i] & 0x10)
#define IsInvisible(i) (ObjStatus[i] & 0x02)
#define IsLocal(i) (ObjStatus[i] & 0x20)
#define IsHatched(i) (ObjStatus[i] & 0x40)
#define SetLocal(i) ObjStatus[i] |= 0x20
#define ClrLocal(i) ObjStatus[i] &= ~0x20
#define ClrHatched(i) ObjStatus[i] &= ~0x40
#define IsDead(i) (NPCStatus[i] & 0x10)
#define IsPlrControl(i) (NPCStatus[i] & 0x80)
#define IsNPC(i) ((i) < 0x100 && (i) >= 0)

#define TypeFrame(t, s) ((t) + ((s) << 10))
#define GetType(i) (ObjShapeType[i] & 0x3ff)
#define GetFrame(i) (ObjShapeType[i] >> 10)
#define TILE_FRAME(i) (BaseTile[GetType(i)] + GetFrame(i))

#define GetCoordX(o) ((*(int16 *)&((o).raw[0]))&0x3ff)
#define GetCoordY(o) (((*(int16 *)&((o).raw[1]))>>2)&0x3ff)
#define GetCoordZ(o) ((o).raw[2] >> 4)
#define GetCoordAssoc(i) *(uint16 *)(&(i))
#define SetCoordX(o, v) { (*(int16 *)&((o).raw[0])) = (*(int16 *)&((o).raw[0])) & ~0x3ff; (*(int16 *)&((o).raw[0])) |= (v); }
#define SetCoordY(o, v) { (*(int16 *)&((o).raw[1])) = (*(int16 *)&((o).raw[1])) & ~(0x3ff << 2); (*(int16 *)&((o).raw[1])) |= (v) << 2; }
#define SetCoordXYZ(o, x, y, z) {      \
	(o).raw[2] = ((z) << 4);           \
	*(int16 *)&((o).raw[0]) = (x);       \
	*(int16 *)&((o).raw[1]) |= (y) << 2; \
}
#define SetCoordAssoc(i, j) *(uint16 *)(&(i)) = (j)
#define SetXYZ(i, x, y, z) SetCoordXYZ(ObjPos[i], x, y, z)
#define SetAssoc(i, j) SetCoordAssoc(ObjPos[i], j)
#define GetX(i) GetCoordX(ObjPos[i])
#define GetY(i) GetCoordY(ObjPos[i])
#define GetZ(i) GetCoordZ(ObjPos[i])
#define GetAssoc(i) GetCoordAssoc(ObjPos[i])

#define IsTileFor(tile) (TileFlag[tile] & 0x10)
#define IsTileDoubleV(tile) (TileFlag[tile] & 0x40)
#define IsTileDoubleH(tile) (TileFlag[tile] & 0x80)
#define IsTileBa(tile) (TileFlag2[tile] & 0x20)
#define IsTerrainWall(tile) (TerrainType[tile] & 0x04)

#define CHARPTR_POSTINC(p) (((char far *)p)++)

/*only cleared by ShowObjects (U6: the musician was near the party)*/
/*070C*/static int16 MusicianWasNear = 0;
/*when set, NextInv/NextInvType skip equipped items*/
/*070E*/int16 InvSkipEquipped = 0;
/*set by FindLoc/NextLoc: which part of a multi-tile object covers the
  searched tile (0 its own tile, 1 the tile left of/above it, 2/3 the
  upper/upper-left part of a 2x2 tile)*/
/*0710*/int16 LocPartOffset = 0;
/*set while a dead body is made; nothing reads it here (U6: let InsertObj move
  the Amulet of submission)*/
/*0712*/static int16 AllowSubmissionMove = 0;
/*[0..63] outdoor areas, [64..68] underground levels 1..5: a ".tmp" objblk
  file newer than the saved one exists*/
/*0714*/int16 ObjBlkTmpPending[8*8 + 5] = {0};
/*when set, MoveObj applies terrain effects to moved NPCs*/
/*079E*/int16 MoveAppliesTerrain = 0;
/*ids (yyyxxx) of the up to 4 outdoor areas loaded, -1 = free slot*/
/*07A0*/int16 LoadedAreas[4] = {-1, -1, -1, -1};
/*id of the top-left area of the loaded 2x2 window, -1 = none*/
/*07A8*/int16 LoadedWindowArea = -1;
/*z level loaded (0 = surface), -1 = none*/
/*07AA*/int16 LoadedZ = -1;
/*FindLoc/NextLoc state*/
/*07AC*/static int16 LocCursor = -1;
/*07AE*/static int16 LocOnNextRow = 0;
/*07B0*/static struct coord LocTarget = {0, 0, 0};
/*FindInv/NextInv/FindInvType/NextInvType state: position, owner, type,
  quality (-1 = any)*/
/*07B3*/int16 InvCursor = -1;
/*07B5*/int16 InvOwner = -1;
/*07B7*/int16 InvType = -1;
/*07B9*/int16 InvQuality = -1;
/*SearchArea/NextArea state: position and end position*/
/*07BB*/int16 AreaCursor = -1;
/*07BD*/int16 AreaEnd = -1;
/*last content object moved out by DeleteObj (-1 if none)*/
/*07BF*/int16 LastSpilledObj = -1;

/*objblk file names; characters [15] and [16] are patched with the area letters*/
/*07C1*/static char __Sav_file[] = "savegame\\objblkaa";
/*07D3*/static char __Tmp_file[] = "savegame\\objblkaa.tmp";
/*07E9*/static char __Bak_file[] = "savegame\\objblkaa.bak";

/*indexed by direction (0=N 2=E 4=S 6=W): TerrainType connection flag of that direction*/
/*07FF*/unsigned char DirConnFlag[] = {0x80,0,0x40,0,0x20,0,0x10};

/*object types counted by the quantity byte of Amount[]*/
/*0806*/int16 StackableTypes[] = {
	0x24,0x2D,0x29,0x36,0x37,0x38,0x48,0x46,0x49,0x4A,0x66,0x67,0x68,0x69,0x6C,0x6D,
	0x6E,0xD2,0x84,0x2C,0x85,0x73,0x78,0x7B,0x81,0x89,0xB4,0x82,0xC0,0x86,0x8B,0x8C,
	0xBF,0xCC,0xCE,0xD6,0x5D,0x30,0x0A,0x2F,0x1A,0x89,0xF0,0xD3,0xFC,0x1B,0xD0,0x45
};

/*is location (x, y, z) loaded in the object list?*/
/*C_0A5D_0001*/OBJ_IsLocLoaded(int16 x, int16 y, int16 z) {
	register int16 i, isLoaded;
	int16 mapId;

	isLoaded = 0;
	if(z) {
		isLoaded = (z == MapZ);
	} else if(MapZ == 0) {
		mapId = (x >> 7) + ((y >> 4) & 0x38);
		for(i = 0; i < 4; i ++)
			if(LoadedAreas[i] == mapId) {
				isLoaded = 1;
				break;
			}
	}
	return isLoaded;
}

/*frame number of a creature of type objTyp facing direction dir (0=N 2=E 4=S 6=W)*/
/*C_0A5D_0068*/OBJ_MakeDirFrame(register int16 objTyp, int16 direction) {
	int16 i;
	register int16 dir = direction;

	dir >>= 1;
	if(objTyp >= 0x136 && objTyp <= 0x146)
		return dir << 2;
	if(objTyp == 0x120)
		return (dir + (dir << 1) + 3 << 1) - 1;
	if(objTyp == 0x150 || objTyp == 0x151)
		return dir;
	if(objTyp == 0x14f)
		return ((dir << 1) + 1 << 1) - 1;
	if(C_0A5D_0DEF(objTyp)) {
		C_0A5D_0D5B(objTyp, D_BD0A, D_BD04);
		for(i = 0; i < 3; i ++) {
			if(D_BD04[i] == objTyp) {
				dir ++;
				return (dir + (dir << 1) << D_BD0A[i]) - 1;
			}
		}
		return 0;
	}
	return 0;
}

/*U6: the head of a multi-object creature; here objNum itself*/
/*C_0A5D_0118*/OBJ_getHead(register int16 objNum) {
	int16 objTyp;
	register int16 ret;

	if(objNum < 0)
		return -1;
	ret = objNum;
	objTyp = GetType(objNum);
	return ret;
}

/*returns the next free object slot if at least num slots are free, else -1*/
/*C_0A5D_0145*/static GetFreeObjSlot(int16 num) {
	if(FreeObjCount >= num)
		return FreeObjHead;
	return -1;
}

/*locate (x, y) in the sorted Link[] chain (z is ignored).
  wantPrev == 0: the first object whose key (y, x) is >= the searched one;
  wantPrev != 0: the node after which an object at (x, y) must be inserted
                 (0x100 = list head)*/
/*C_0A5D_015C*/static OBJ_FindListPos(int16 x, int16 y, int16 wantPrev) {
	register int16 i, j;
	struct coord xyz;
	int16 cur, prev, next, objY;
	int16 relX, relY;

	relX = (x - AreaX) & 0x3ff;
	relY = (y - AreaY) & 0x3ff;
	if(relX < AREA_W && relY < AREA_H) {
		j = relX + AREA_W * relY;
		if(wantPrev) {
			/*first non null object before j*/
			for(i = j - 1; i >= 0 && ((int16 *)MapObjPtr)[i] == -1; i --);
			/*first non null object after j*/
			for(; j < 0x63f && ((int16 *)MapObjPtr)[j] == -1; j ++);

			if(i >= 0)
				i = ((int16 *)MapObjPtr)[i];
			else
				i = 0x100;

			for(j = ((int16 *)MapObjPtr)[j], next = Link[i]; next >= 0 && next != j; i = next, next = Link[i]) {
				if(GetCoordUse(next) == LOCXYZ) {
					objY = GetY(next);
					if(
						(objY > y) ||
						((objY == y) && (GetX(next) >= x))
					) break;
				}
			}
		} else {
			i = ((int16 *)MapObjPtr)[j];
			if(i < 0) {
				for(; j < 0x63f && ((int16 *)MapObjPtr)[j] == -1; j ++);
				i = ((int16 *)MapObjPtr)[j];
			}
		}
	} else {
		SetCoordXYZ(xyz, x, y, 0);
		if(
			((AreaY + (AREA_H-1)) & 0x3ff) < y || (
			((AreaY + (AREA_W-1)) & 0x3ff) == y && ((AreaX + (AREA_W-1)) & 0x3ff) < x
		)) {
			for(i = 0x63f; i >= 0 && ((int16 *)MapObjPtr)[i] == -1; i --);
			if(i >= 0)
				prev = ((int16 *)MapObjPtr)[i];
			else
				prev = 0x100;
		} else {
			prev = 0x100;
		}
		for(cur = Link[prev]; cur >= 0; prev = cur, cur = Link[cur]) {
			if(GetCoordUse(cur) == LOCXYZ) {
				/*compares the 20 low bits of the coordinates, the key (y << 10 | x)*/
				if((ObjPos[cur].raw[2] & 0xf) > xyz.raw[2])
					break;
				if((ObjPos[cur].raw[2] & 0xf) == xyz.raw[2]) {
					if(ObjPos[cur].raw[1] > xyz.raw[1])
						break;
					if(ObjPos[cur].raw[1] == xyz.raw[1] && ObjPos[cur].raw[0] >= xyz.raw[0])
						break;
				}
			}
		}
		i = prev;
		if(!wantPrev)
			i = Link[i];
	}
	return i;
}

/*next object at the location set by FindLoc (same z), also returning the
  parts of double-width/double-height objects whose tile covers it
  (LocPartOffset tells which); -1 at the end*/
/*C_0A5D_039A*/NextLoc() {
	register struct coord *pos;
	register int16 found;
	struct coord locDown, locRight, locDownRight;
	int16 tileNum, locX, locY, locZ;

	found = -1;
	locX = GetCoordX(LocTarget);
	locY = GetCoordY(LocTarget);
	locZ = GetCoordZ(LocTarget);

	locDownRight =
	locDown =
	locRight = LocTarget;

	SetCoordX(locRight, locX + 1);
	SetCoordY(locDown, locY + 1);
	SetCoordX(locDownRight, locX + 1);
	SetCoordY(locDownRight, locY + 1);
	while(LocCursor >= 0 && found < 0) {
		if(GetCoordUse(LocCursor) == LOCXYZ && GetZ(LocCursor) == locZ) {
			pos = &(ObjPos[LocCursor]);
			tileNum = TILE_FRAME(LocCursor);
			if(
				pos->raw[0] == LocTarget.raw[0] &&
				pos->raw[1] == LocTarget.raw[1] &&
				pos->raw[2] == LocTarget.raw[2]
			) {
				found = LocCursor;
			} else if(
				pos->raw[0] == locRight.raw[0] &&
				pos->raw[1] == locRight.raw[1] &&
				pos->raw[2] == locRight.raw[2]
			) {
				if(IsTileDoubleH(tileNum)) {
					found = LocCursor;
					LocPartOffset = 1;
				}
			} else if(
				pos->raw[0] == locDown.raw[0] &&
				pos->raw[1] == locDown.raw[1] &&
				pos->raw[2] == locDown.raw[2]
			) {
				if(IsTileDoubleV(tileNum)) {
					found = LocCursor;
					LocPartOffset = IsTileDoubleH(tileNum)?2:1;
				}
			} else if(
				pos->raw[0] == locDownRight.raw[0] &&
				pos->raw[1] == locDownRight.raw[1] &&
				pos->raw[2] == locDownRight.raw[2]
			) {
				if((TileFlag[tileNum] & 0xc0) == 0xc0) {
					found = LocCursor;
					LocPartOffset = 3;
				}
			} else if(!LocOnNextRow) {
				/*past the location on row y: continue once on row y+1*/
				LocCursor = OBJ_FindListPos(locX, locY + 1, 0);
				LocOnNextRow = 1;
				continue;
			} else {
				LocCursor = -1;
				break;
			}
		}
		LocCursor = Link[LocCursor];
	}
	return found;
}

/*start iterating over the objects at (x, y, z); returns the first one or -1*/
/*C_0A5D_05C4*/FindLoc(register int16 x, register int16 y, int16 z) {
	if(z) {
		x &= 0xff;
		y &= 0xff;
	}
	LocPartOffset =
	LocOnNextRow = 0;
	LocCursor = OBJ_FindListPos(x, y, 0);
	SetCoordXYZ(LocTarget, x, y, z);
	return NextLoc();
}

/*is objNum within 5 tiles horizontally and 4 vertically of the party?*/
/*C_0A5D_0613*/C_0A5D_0613(int16 objNum) {
	register int16 x, y;

	x = GetX(objNum);
	y = GetY(objNum);
	return MapX - 5 <= x && MapX + 5 >= x && MapY - 4 <= y && MapY + 4 >= y;
}

/*find an object of type objTyp exactly at (x, y, MapZ); -1 if none*/
/*C_0A5D_0677*/OBJ_FindTypeAt(int16 objTyp, int16 x, int16 y) {
	register int16 obj, found;

	found = -1;
	for(
		obj = OBJ_FindListPos(x, y, 0);
		obj >= 0 && (
			GetCoordUse(obj) != LOCXYZ ||
			(GetX(obj) == x && GetY(obj) == y)
		);
		obj = Link[obj]
	) {
		if(GetCoordUse(obj) == LOCXYZ && GetZ(obj) == MapZ && GetType(obj) == objTyp) {
			found = obj;
			break;
		}
	}

	return found;
}

/*the NPC that holds objNum (directly or in a container), -1 if none*/
/*C_0A5D_070D*/C_0A5D_070D(int16 objNum) {
	register int16 i;

	for(i = GetAssoc(objNum); !IsNPC(i) && i != -1; i = GetAssoc(i));
	return i;
}

/*start iterating over the direct contents of objNum; returns the first one or -1*/
/*C_0A5D_0739*/FindInv(int16 objNum) {
	register int16 first, ret;

	ret = -1;
	first = Link[objNum];
	if(GetCoordUse(first) != LOCXYZ && GetAssoc(first) == objNum) {
		ret = first;
		InvCursor = Link[first];
		InvOwner = objNum;
		InvType = GetType(ret);
		InvQuality = -1;
	}
	return ret;
}

/*next direct content of the object given to FindInv (skips equipped
  items if InvSkipEquipped is set); -1 at the end*/
/*C_0A5D_0792*/NextInv() {
	register int16 ret;

	for(ret = -1; InvCursor >= 0 && ret < 0; InvCursor = Link[InvCursor]) {
		if(GetCoordUse(InvCursor) == LOCXYZ) {
			InvCursor = -1;
			break;
		}
		if(GetAssoc(InvCursor) == InvOwner) {
			if(InvSkipEquipped == 0 || GetCoordUse(InvCursor) != EQUIP || GetCoordUse(InvCursor) == CONTAINED)
				ret = InvCursor;
		}
	}
	return ret;
}

/*an object of type objTyp, not equipped, somewhere inside the map object
  objNum (at any depth); -1 if none*/
/*C_0A5D_080A*/C_0A5D_080A(int16 objNum, int16 objTyp) {
	int16 unused;
	register int16 obj, holder;

	unused = 0;/*set, never read*/
	for(obj = Link[objNum]; obj >= 0; obj = Link[obj]) {
		for(holder = GetAssoc(obj); GetCoordUse(holder) != LOCXYZ; holder = GetAssoc(holder));
		if(holder != objNum)
			return -1;
		if(GetCoordUse(obj) != EQUIP && GetType(obj) == objTyp)
			return obj;
	}
	return -1;
}

/*next map object inside the rectangle given to SearchArea; -1 at the end*/
/*C_0A5D_0873*/NextArea() {
	register int16 ret, y;
	int16 x, z;

	ret = -1;
	if(AreaCursor >= 0) {
		for(; AreaCursor != AreaEnd && AreaCursor >= 0 && ret < 0; AreaCursor = Link[AreaCursor]) {
			if(GetCoordUse(AreaCursor) == LOCXYZ) {
				x = GetX(AreaCursor) & MapCoordMask;
				y = GetY(AreaCursor) & MapCoordMask;
				z = GetZ(AreaCursor);
				if(
					x >= D_D97E && x <= D_D980 &&
					y >= D_D984 && y <= D_D986 &&
					z == D_D988
				) ret = AreaCursor;
			}
		}
		if(AreaCursor == AreaEnd)
			AreaCursor = AreaEnd = -1;
	}
	return ret;
}

/*start iterating over the map objects inside the rectangle
  (x1, y1)-(x2, y2) at level MapZ; returns the first one or -1*/
/*C_0A5D_0943*/SearchArea(register int16 x1, register int16 y1, int16 x2, int16 y2) {
	if(x1 < 0)
		x1 = 0;
	if(y1 < 0)
		y1 = 0;
	D_D97E = x1;
	D_D980 = x2;
	D_D984 = y1;
	D_D986 = y2;
	D_D988 = MapZ;
	AreaCursor = OBJ_FindListPos(x1, y1, 0);
	AreaEnd = OBJ_FindListPos(x2 + 1, y2, 0);
	return NextArea();
}

/*next direct content of the FindInvType owner with the wanted type
  (and quality, unless -1); -1 at the end*/
/*C_0A5D_099C*/NextInvType() {
	register int16 ret;

	for(ret = -1; InvCursor >= 0 && ret < 0; InvCursor = Link[InvCursor]) {
		if(GetCoordUse(InvCursor) == LOCXYZ) {
			InvCursor = -1;
			break;
		}
		if(GetAssoc(InvCursor) == InvOwner && GetType(InvCursor) == InvType) {
			if(
				(InvQuality < 0 || GetQual(InvCursor) == InvQuality) &&
				(InvSkipEquipped == 0 || GetCoordUse(InvCursor) != EQUIP || GetCoordUse(InvCursor) == CONTAINED)
			) ret = InvCursor;
		}
	}
	return ret;
}

/*first direct content of objNum of type objType and the given quality
  (-1 = any quality); -1 if none*/
/*C_0A5D_0A45*/FindInvType(int16 objNum, int16 objType, int16 quality) {
	register int16 first, ret;

	if(objNum < 0)
		return -1;
	ret = -1;
	first = Link[objNum];
	if(GetCoordUse(first) != LOCXYZ && GetAssoc(first) == objNum) {
		ret = first;
		InvCursor = first;
		InvOwner = objNum;
		InvType = objType;
		InvQuality = quality;
		ret = NextInvType();
	}
	return ret;
}

/*find a living NPC/monster (slot startNum..0xFF, at least 1) standing at
  (x, y, MapZ); -1 if none*/
/*C_0A5D_0AA0*/OBJ_FindNPCAt(int16 startNum, int16 x, int16 y) {
	register int16 i, ret;

	ret = -1;
	if(startNum < 1)
		startNum = 1;
	for(i = startNum; i < 0x100; i ++) {
		if(
			ObjShapeType[i] &&
			!IsDead(i) &&
			GetX(i) == x &&
			GetY(i) == y &&
			GetZ(i) == MapZ
		) {
			ret = i;
			break;
		}
	}
	return ret;
}

/*creatures made of several objects: objNum has moved from (oldX, oldY);
  move its other parts along (objects whose quality is objNum, of a type
  next to objNum's, joined by the TerrainType connection flags), or
  delete them if deleteParts is set*/
/*C_0A5D_0B28*/OBJ_MoveCreatureParts(int16 objNum, int16 oldX, int16 oldY, int16 deleteParts) {
	register int16 dir, part;
	int16 newX, newY, z, partType, connFlags, partConn;

	newX = GetX(objNum);
	newY = GetY(objNum);
	z = GetZ(objNum);
	connFlags = TerrainType[TILE_FRAME(objNum)];
	partType = GetType(objNum);
	for(dir = 0; dir < 8; dir ++) {
		if((dir & 1) == 0 && (connFlags & DirConnFlag[dir])) {
			for(
				part = FindLoc(oldX + DirIncrX[dir], oldY + DirIncrY[dir], z);
				part >= 0;
				part = NextLoc()
			) {
				partConn = TerrainType[TILE_FRAME(part)];
				if(
					!IsNPC(part) && GetQual(part) == objNum &&
					abs(GetType(part) - partType) <= 1 &&
					(partConn & DirConnFlag[dir ^ 4])
				) break;
			}
			if(part < 0)
				continue;
			if(deleteParts)
				DeleteObj(part);
			else
				MoveObj(part, newX + DirIncrX[dir], newY + DirIncrY[dir], z);
		}
	}
}

/*delete the other parts of the creature objNum (the objects of its part
  types whose quality is objNum)*/
/*C_0A5D_0CB1*/C_0A5D_0CB1(int16 objNum) {
	register int16 i, typ;
	int16 objTyp, type0, type1, type2, maxCount, count;
	int16 types[3], shifts[3];

	objTyp = GetType(objNum);
	if(C_0A5D_0D5B(objTyp, shifts, types)) {
		count = 0;
		type0 = types[0];
		type1 = types[1];
		type2 = types[2];
		if(type2 >= 0)
			maxCount = 1;
		else
			maxCount = 0;
		for(i = 0x100; i < 0xc00; i ++) {
			typ = GetType(i);
			if((typ == type0 || typ == type1 || typ == type2) && !IsNPC(i) && GetQual(i) == objNum) {
				DeleteObj(i);
				count ++;
				if(count > maxCount)
					break;
			}
		}
	}
}

/*the parts of a creature made of several objects whose head is of type
  type: types[] gets the part types (-1 = none), shifts[] the frame shift
  of each; returns type, or 0 if type is not such a creature*/
/*C_0A5D_0D5B*/C_0A5D_0D5B(register int16 type, register int16 *shifts, register int16 *types) {
	switch(type) {
		case 0x126:
			shifts[0] = 1;
			shifts[1] = 2;
			shifts[2] = 1;
			types[0] = type;
			types[1] = type - 1;
			types[2] = type + 1;
			return type;
		case 0x121:
			shifts[0] = 1;
			shifts[1] = 0;
			shifts[2] = -1;
			types[0] = type;
			types[1] = type + 1;
			types[2] = -1;
			return type;
		case 0x129:
			shifts[0] = 1;
			shifts[1] = 0;
			shifts[2] = -1;
			types[0] = type;
			types[1] = type + 1;
			types[2] = -1;
			return type;
		case 0x12f:
			shifts[0] = 1;
			shifts[1] = 0;
			shifts[2] = -1;
			types[0] = type;
			types[1] = type + 1;
			types[2] = -1;
			return type;
		case 0x11c:
			shifts[0] = 1;
			shifts[1] = 0;
			shifts[2] = -1;
			types[0] = type;
			types[1] = type + 1;
			types[2] = -1;
			return type;
		case 0x131:
			shifts[0] = 1;
			shifts[1] = 0;
			shifts[2] = -1;
			types[0] = type;
			types[1] = type + 1;
			types[2] = -1;
			return type;
	}
	return 0;
}

/*the head type of a creature made of several objects, from the type of
  any of its parts; 0 if objTyp is not such a creature*/
/*C_0A5D_0DEF*/C_0A5D_0DEF(int16 objTyp) {
	switch(objTyp) {
		case 0x125: case 0x126: case 0x127:
			return 0x126;
		case 0x121: case 0x122:
			return 0x121;
		case 0x129: case 0x12a:
			return 0x129;
		case 0x11c: case 0x11d:
			return 0x11c;
		case 0x12f: case 0x130:
			return 0x12f;
		case 0x131: case 0x132:
			return 0x131;
	}
	return 0;
}

/*create an object from a status and a struct coord (map location or
  container depending on the status); does not return the new object*/
/*C_0A5D_0E57*/AddObj(register int16 status, struct coord location, int16 shapeType, int16 shapeNum, int16 amount) {
	if((status & 0x18) == LOCXYZ)
		AddMapObj(shapeType | (shapeNum << 10), status, amount, GetCoordX(location), GetCoordY(location), GetCoordZ(location));
	else
		AddInvObj(shapeType | (shapeNum << 10), status, amount, GetCoordAssoc(location));
}

/*create an object on the map at (x, y, z) (in front of the objects
  already there); returns the new object or -1*/
/*C_0A5D_0EB6*/AddMapObj(int16 shapeType, int16 status, int16 amount, int16 x, register int16 y, int16 z) {
	register int16 objNum;
	int16 prev, posMask;

	posMask = z?0xff:0x3ff;
	x &= posMask;
	y &= posMask;
	if(!OBJ_IsLocLoaded(x, y, z))
		return -1;
	objNum = GetFreeObjSlot(1);
	if(objNum >= 0) {
		prev = OBJ_FindListPos(x, y, 1);
		FreeObjHead = Link[FreeObjHead];
		FreeObjCount --;
		ObjStatus[objNum] = status;
		SetLocal(objNum);
		SetXYZ(objNum, x, y, z);
		ObjShapeType[objNum] = shapeType;
		Amount[objNum] = amount;
		Link[objNum] = Link[prev];
		Link[prev] = objNum;
		x = (x - AreaX) & 0x3ff;
		y = (y - AreaY) & 0x3ff;
		if(x < AREA_W && y < AREA_H)
			MapObjPtr[y][x] = objNum;
	}
	return objNum;
}

/*number of objects directly inside objNum*/
/*C_0A5D_0FD6*/C_0A5D_0FD6(int16 objNum) {
	register int16 count, obj;

	count = 0;
	for(obj = FindInv(objNum); obj >= 0; obj = NextInv())
		count ++;
	return count;
}

/*number of objects equipped by objNum*/
/*C_0A5D_0FF9*/C_0A5D_0FF9(int16 objNum) {
	register int16 obj, count;

	count = 0;
	for(obj = FindInv(objNum); obj >= 0; obj = NextInv())
		if(GetCoordUse(obj) == EQUIP)
			count ++;
	return count;
}

/*create an object inside owner (an NPC or a container); refused (-1) when
  the owner is not loaded, its inventory is full (16 objects) or the
  weight is too much for its strength; stackable objects are added to an
  existing stack when possible. Returns the object.*/
/*C_0A5D_102A*/AddInvObj(int16 shapeType, int16 status, int16 amount, register int16 owner) {
	int16 root, obj, strength, weight, refused, coordUse, numInven;
	register int16 objNum;

	strength = -1;
	refused = 0;
	if(owner == -1)
		return -1;
	for(root = owner; GetCoordUse(root) != LOCXYZ; root = GetAssoc(root));
	if(!OBJ_IsLocLoaded(GetX(root), GetY(root), GetZ(root)))
		return -1;
	STAT_RefreshInventory(owner);
	numInven = C_0A5D_0FD6(owner) - C_0A5D_0FF9(owner);
	if(StatusDisplay == 0x92 && Party[StatusMember] == owner)
		StatusDirty = 1;
	coordUse = status & ~0x67;
	if(numInven >= 0x10 && (coordUse == INVEN || coordUse == CONTAINED) && !QuanType(shapeType) && GetType(owner) != 0xf9) {
		STAT_RefreshInventory(Party[StatusMember]);
		return -1;
	}
	STAT_ComputeWeights(owner);
	weight = GetTypeWeight(shapeType, amount);
	if(IsNPC(owner))
		strength = STREN[owner];
	if(strength != -1 && strength * 20 < weight + WeightInven + WeightEquip) {
		STAT_RefreshInventory(Party[StatusMember]);
		return -1;
	}
	SubMov(owner, 3);
	if(QuanType(shapeType) && coordUse != EQUIP) {
		for(obj = FindInvType(owner, shapeType, -1); obj != -1; obj = NextInvType()) {
			if(GetQuan(obj) + amount < 0x90 && GetCoordUse(obj) != EQUIP && IsHatched(obj) == 0)
				break;
		}
		if(obj != -1) {
			AddQuan(obj, amount);
			STAT_RefreshInventory(Party[StatusMember]);
			return obj;
		}
		if(numInven >= 0x10) {
			STAT_RefreshInventory(Party[StatusMember]);
			return -1;
		}
	}
	objNum = GetFreeObjSlot(1);
	if(owner != StatusMember)
		STAT_ComputeWeights(Party[StatusMember]);
	if(objNum >= 0 && !refused) {
		FreeObjHead = Link[FreeObjHead];
		FreeObjCount --;
		ObjStatus[objNum] = status;
		SetLocal(objNum);
		SetAssoc(objNum, owner);
		ObjShapeType[objNum] = shapeType;
		Amount[objNum] = amount;
		Link[objNum] = Link[owner];
		Link[owner] = objNum;
	}
	STAT_RefreshInventory(Party[StatusMember]);
	return objNum;
}

/*create a temporary monster of type objType at (x, y, z) in one of the
  slots 0xC0-0xFF; returns it or -1 if none is free*/
/*C_0A5D_12B4*/AddMonster(int16 objType, int16 x, register int16 y, int16 z) {
	register int16 objNum;
	int16 prev, posMask;

	posMask = z?0xff:0x3ff;
	x &= posMask;
	y &= posMask;
	objNum = -1;
	if(FreeMonsterCount > 0)
		objNum = FreeMonsterHead;
	if(objNum >= 0) {
		prev = OBJ_FindListPos(x, y, 1);
		FreeMonsterHead = Link[FreeMonsterHead];
		FreeMonsterCount --;
		ObjStatus[objNum] = 0x20;
		NPCStatus[objNum] = 0;
		NPCMode[objNum] = 8;
		SetXYZ(objNum, x, y, z);
		ObjShapeType[objNum] =
		OrigShapeType[objNum] = objType;
		Amount[objNum] = 1;
		Link[objNum] = Link[prev];
		Link[prev] = objNum;
		x = (x - AreaX) & 0x3ff;
		y = (y - AreaY) & 0x3ff;
		if(x < AREA_W && y < AREA_H)
			MapObjPtr[y][x] = objNum;
	}
	return objNum;
}

/*the node preceding objNum in the Link[] chain (-1 if not found)*/
/*C_0A5D_13D0*/static OBJ_FindPrevLink(register int16 objNum) {
	register int16 prev;

	if(GetCoordUse(objNum) == LOCXYZ)
		prev = OBJ_FindListPos(GetX(objNum), GetY(objNum), 1);
	else
		prev = GetAssoc(objNum);
	for(; prev >= 0 && Link[prev] != objNum; prev = Link[prev]);

	return prev;
}

/*objNum (at x, y, z) is being unlinked and nextObj now follows its
  predecessor: fix the MapObjPtr[] entry of the tile if it pointed to
  objNum (nextObj, or -1 if nextObj is on another tile)*/
/*C_0A5D_143E*/static OBJ_FixMapObjPtr(int16 x, int16 y, int16 z, int16 objNum, int16 nextObj) {
	register int16 relX, relY;
	int16 posMask;

	if(GetCoordUse(objNum) == LOCXYZ) {
		posMask = z?0xff:0x3ff;
		relX = (x - AreaX) & posMask;
		relY = (y - AreaY) & posMask;
		if(relX < AREA_W && relY < AREA_H && MapObjPtr[relY][relX] == objNum) {
			if(GetCoordUse(nextObj) == LOCXYZ && (GetX(nextObj) != x || GetY(nextObj) != y))
				MapObjPtr[relY][relX] = -1;
			else
				MapObjPtr[relY][relX] = nextObj;
		}
	}
}

/*delete an object: the other parts of a creature are deleted too; the
  contents of a container are dropped where it was (or put in its owner),
  those of anything else deleted; the slot goes back to the monster or
  object free list*/
/*C_0A5D_1506*/DeleteObj(register int16 objNum) {
	int16 objTyp, prev;
	register int16 content;

	if(objNum < 0)
		return;
	LastSpilledObj = -1;
	if(StatusDisplay == 0x92 && OBJ_IsInside(objNum, Party[StatusMember]))
		StatusDirty = 1;
	objTyp = GetType(objNum);
	if(IsNPC(objNum) && (C_130F_00CA(objNum) || C_130F_0070(objNum)))
		C_0A5D_0CB1(objNum);
	/*spill the contents*/
	for(content = FindInv(objNum); content >= 0; content = NextInv()) {
		LastSpilledObj = content;
		if(GetCoordUse(objNum) == LOCXYZ && IsContainerType(objTyp)) {
			MoveObj(content, GetX(objNum), GetY(objNum), GetZ(objNum));
		} else if(IsContainerType(objTyp)) {
			InsertObj(content, GetAssoc(objNum), GetCoordUse(objNum));
		} else {
			DeleteObj(content);
			LastSpilledObj = InvCursor;
		}
	}
	ObjShapeType[objNum] = 0;
	prev = OBJ_FindPrevLink(objNum);
	if(prev >= 0) {
		Link[prev] = Link[objNum];
		if(IsNPC(objNum)) {
			Link[objNum] = FreeMonsterHead;
			FreeMonsterHead = objNum;
			FreeMonsterCount ++;
		} else {
			Link[objNum] = FreeObjHead;
			FreeObjHead = objNum;
			FreeObjCount ++;
		}
	}
	OBJ_FixMapObjPtr(GetX(objNum), GetY(objNum), GetZ(objNum), objNum, Link[prev]);
}

/*make objNum (again) an object of an inventory: status, owner, shape and amount*/
/*C_0A5D_16BE*/C_0A5D_16BE(register int16 objNum, int16 shapeType, int16 status, int16 amount, int16 owner) {
	if(objNum < 0)
		return;
	if(StatusDisplay == 0x92 && OBJ_IsInside(objNum, Party[StatusMember]))
		StatusDirty = 1;
	ObjStatus[objNum] = status;
	SetLocal(objNum);
	SetAssoc(objNum, owner);
	ObjShapeType[objNum] = shapeType;
	Amount[objNum] = amount;
}

/*the loaded area is now centered on (x, y, z): delete the temporary
  (LOCAL) monsters and objects farther than 20 tiles away and let the eggs
  (type 0xe6) there hatch again. Far objects of type 0x4e are replaced by
  an egg holding a creature of type 0x124; type 0xf6 gets frame 0 again
  (everywhere), type 0xf7 quality 0.*/
/*C_0A5D_1727*/OBJ_PurgeFarLocals(int16 x, int16 y, int16 z) {
	register int16 obj, objY;
	int16 objTyp, next, objX, objZ, egg, item;

	x &= MapCoordMask;
	y &= MapCoordMask;
	/*delete local monsters*/
	for(obj = 0xc0; obj < 0x100; obj ++) {
		if(ObjShapeType[obj]) {
			objX = GetX(obj);
			objY = GetY(obj);
			objZ = GetZ(obj);
			if(
				x - 20 > objX || x + 20 <= objX ||
				y - 20 > objY || y + 20 <= objY ||
				objZ != z
			) {
				if(IsLocal(obj))
					DeleteObj(obj);
			}
		}
	}
	/*local objects and eggs*/
	for(obj = Link[0x100]; obj >= 0; obj = next) {
		next = Link[obj];
		objTyp = GetType(obj);
		if(objTyp == 0xf6)
			ObjShapeType[obj] = GetType(obj);
		if(GetCoordUse(obj) != LOCXYZ)
			continue;
		objX = GetX(obj);
		objY = GetY(obj);
		objZ = GetZ(obj);
		if(
			x - 20 > objX || x + 20 <= objX ||
			y - 20 > objY || y + 20 <= objY ||
			objZ != z
		) {
			objTyp = GetType(obj);
			if(IsLocal(obj) || ObjShapeType[obj] == 0) {
				if(obj == D_EADE)
					D_EADE = 0;
				if(objTyp == 0x4e && GetCoordUse(obj) != CONTAINED) {
					egg = AddMapObj(0xe6, 2, 0xc9, objX, objY, objZ);
					item = AddInvObj(0x124, 8, 1, egg);
					ObjShapeType[item] = GetType(item);
				}
				DeleteObj(obj);
				if(LastSpilledObj >= 0)
					next = LastSpilledObj;
			} else if(objTyp == 0x4e && GetCoordUse(obj) != CONTAINED) {
				egg = AddMapObj(0xe6, 2, 0xc9, objX, objY, objZ);
				item = AddInvObj(0x124, 8, 1, egg);
				ObjShapeType[item] = GetType(item);
				DeleteObj(obj);
			} else if(objTyp == 0xe6 && IsHatched(obj)) {
				ClrHatched(obj);
			} else if(objTyp == 0xf7) {
				SetQual(obj, 0);
			}
		}
	}
}

/*an NPC/monster dies: create its dead body (same shape, quality = the
  NPC number) at its location, put its inventory into the body, keep the
  object of type 0xf8 on top and send the NPC to level 6. Returns the body.*/
/*C_0A5D_19BF*/OBJ_MakeDeadBody(register int16 objNum) {
	int16 body, firstAtLoc, x, y, z;
	register int16 content;

	body = -1;
	if(objNum < 0)
		return -1;
	if(ObjShapeType[objNum]) {
		x = GetX(objNum);
		y = GetY(objNum);
		z = GetZ(objNum);
		firstAtLoc = FindLoc(x, y, z);

		AllowSubmissionMove = 1;
		body = AddMapObj(ObjShapeType[objNum], ObjStatus[objNum] | 0x20, QualQuan(objNum, 0), x, y, z);
		if(body >= 0) {
			if(objNum < 0xc0)
				ClrLocal(body);
			else
				SetLocal(body);
			for(content = FindInv(objNum); content >= 0; content = FindInv(objNum)) {
				if(GetCoordUse(content) == EQUIP) {
					InsertObj(content, body, CONTAINED);
					ObjStatus[content] |= 0x40;
				} else {
					InsertObj(content, body, CONTAINED);
				}
				ClrLocal(content);
			}
			if(GetType(firstAtLoc) == 0xf8)
				MoveObj(firstAtLoc, x, y, z);
			MoveObj(objNum, x, y, 6);
			NPCStatus[objNum] |= 0x10;
			if(objNum >= 0xc0)
				ObjShapeType[objNum] = 0;
		}
		AllowSubmissionMove = 0;
	}

	return body;
}

/*resurrect the NPC whose dead body is given (the NPC number is the body's
  quality): put the NPC where the body (or whoever carries it) is, give it
  back the body's contents (re-equipping what was equipped) and delete
  the body. Returns the NPC.*/
/*C_0A5D_1B36*/OBJ_ReviveFromBody(int16 body) {
	register int16 root, content;
	int16 npc, coordUse;

	if(body < 0)
		return -1;

	npc = GetQual(body);
	for(root = body; GetCoordUse(root) != LOCXYZ; root = GetAssoc(root));
	MoveObj(npc, GetX(root), GetY(root), GetZ(root));
	for(content = FindInv(body); content >= 0; content = FindInv(body)) {
		if(IsHatched(content))
			coordUse = EQUIP;
		else
			coordUse = INVEN;
		InsertObj(content, npc, coordUse);
	}
	DeleteObj(body);

	return npc;
}

/*is objNum (directly or not) inside container?*/
/*C_0A5D_1BF3*/OBJ_IsInside(register int16 objNum, int16 container) {
	register int16 ret;

	ret = 0;
	while(GetCoordUse(objNum) != LOCXYZ) {
		objNum = GetAssoc(objNum);
		if(objNum == container) {
			ret = 1;
			break;
		}
	}
	return ret;
}

/*last node of objNum's subtree (its contents, recursively), objNum if empty*/
/*C_0A5D_1C25*/static OBJ_LastOfSubtree(int16 objNum) {
	register int16 i, last;

	if(objNum < 0)
		return -1;
	for(i = last = objNum; Link[i] >= 0 && GetCoordUse(Link[i]) != LOCXYZ; ) {
		i = Link[i];
		if(OBJ_IsInside(i, objNum))
			last = i;
	}
	return last;
}

/*move an object (and its contents) to the map at (toX, toY, toZ); also
  moves the other parts of creatures made of several objects, flags a
  redraw when the view is concerned and applies terrain effects to NPCs
  (if MoveAppliesTerrain). Returns objNum, or -1 if refused.*/
/*C_0A5D_1C76*/MoveObj(register int16 objNum, int16 toX, register int16 toY, int16 toZ) {
	int16 insertAfter, prev, last, oldX, oldY, objTyp, posMask;

	if(objNum < 0)
		return -1;
	if(StatusDisplay == 0x92 && OBJ_IsInside(objNum, Party[StatusMember]))
		StatusDirty = 1;
	objTyp = GetType(objNum);
	if(IsNPC(objNum) && C_0A5D_0DEF(objTyp) && IsOutOfArea(objNum))
		return -1;
	posMask = toZ?0xff:0x3ff;
	toX &= posMask;
	toY &= posMask;
	oldX = GetX(objNum);
	oldY = GetY(objNum);
	if(
		((MapX - 5) <= oldX && (MapX + 5) >= oldX &&
		(MapY - 5) <= oldY && (MapY + 5) >= oldY) ||

		((MapX - 5) <= toX && (MapX + 5) >= toX &&
		(MapY - 5) <= toY && (MapY + 5) >= toY)
	) VisibilityDirty = 1;
	prev = OBJ_FindPrevLink(objNum);
	if(prev < 0)
		return -1;

	/*unlink the subtree prev->[objNum..last] and insert it at the new place*/
	last = OBJ_LastOfSubtree(objNum);
	Link[prev] = Link[last];
	OBJ_FixMapObjPtr(GetX(objNum), GetY(objNum), GetZ(objNum), objNum, Link[prev]);
	insertAfter = OBJ_FindListPos(toX, toY, 1);
	Link[last] = Link[insertAfter];
	Link[insertAfter] = objNum;
	SetCoordUse(objNum, LOCXYZ);
	SetXYZ(objNum, toX, toY, toZ);
	toX = (toX - AreaX) & 0x3ff;
	toY = (toY - AreaY) & 0x3ff;
	if(toX < AREA_W && toY < AREA_H)
		MapObjPtr[toY][toX] = objNum;
	if(IsNPC(objNum) && C_0A5D_0DEF(objTyp))
		OBJ_MoveCreatureParts(objNum, oldX, oldY, 0);

	if(IsNPC(objNum) && MoveAppliesTerrain) {
		NPCFlag[objNum] &= ~0x08;
		ApplyTerrainEffects(objNum);
	}

	return objNum;
}

/*put an object (and its contents) inside destNum with coord use objLoc
  (INVEN, EQUIP or CONTAINED), with the same checks as AddInvObj (a
  stackable object is added to an existing stack when possible); returns
  the object or -1 if refused*/
/*C_0A5D_1EDB*/InsertObj(register int16 objNum, register int16 destNum, int16 objLoc) {
	int16 prev, last, objTyp, coordUse, obj, quantity, strength, weight, numInven;

	strength = -1;
	if(objNum == -1 || destNum == -1)
		return -1;
	STAT_RefreshInventory(destNum);
	coordUse = objLoc & ~0x67;
	numInven = C_0A5D_0FD6(destNum) - C_0A5D_0FF9(destNum);
	if(numInven >= 0x10 && (coordUse == INVEN || coordUse == CONTAINED) && !QuanType(objTyp) && GetType(destNum) != 0xf9) {
		STAT_RefreshInventory(Party[StatusMember]);
		return -1;
	}
	if(objNum < 0 || destNum < 0) {
		STAT_RefreshInventory(Party[StatusMember]);
		return -1;
	}
	weight = GetWeight(objNum);
	quantity = GetQuan(objNum);
	if(IsNPC(destNum)) {
		STAT_ComputeWeights(destNum);
		strength = STREN[destNum];
		if(strength != -1 && strength * 20 < weight + WeightInven + WeightEquip) {
			STAT_RefreshInventory(Party[StatusMember]);
			return -1;
		}
		SubMov(destNum, 3);
	}
	objTyp = GetType(objNum);
	if(QuanType(objTyp) && coordUse != EQUIP) {
		for(obj = FindInvType(destNum, objTyp, -1); obj != -1; obj = NextInvType()) {
			if(GetQuan(obj) + quantity < 0x90 && GetCoordUse(obj) != EQUIP && IsHatched(obj) == 0)
				break;
		}
		if(obj != -1) {
			AddQuan(obj, quantity);
			STAT_RefreshInventory(Party[StatusMember]);
			return obj;
		}
		if(numInven >= 0x10) {
			STAT_RefreshInventory(Party[StatusMember]);
			return -1;
		}
	}
	if(objNum == destNum || OBJ_IsInside(destNum, objNum)) {
		STAT_RefreshInventory(Party[StatusMember]);
		return -1;
	}
	prev = OBJ_FindPrevLink(objNum);
	last = OBJ_LastOfSubtree(objNum);
	Link[prev] = Link[last];
	Link[last] = Link[destNum];
	Link[destNum] = objNum;
	OBJ_FixMapObjPtr(GetX(objNum), GetY(objNum), GetZ(objNum), objNum, Link[prev]);
	SetCoordUse(objNum, objLoc);
	SetAssoc(objNum, destNum);
	if(QuanType(objTyp))
		SetQuan(objNum, quantity);
	STAT_RefreshInventory(Party[StatusMember]);
	return objNum;
}

/*how an object type counts: 2 = quantity byte (stackable, see
  StackableTypes), 0 = one per object*/
/*C_0A5D_213E*/QuanType(int16 objTyp) {
	register int16 i, ret;

	ret = 0;
	for(i = 0; i < 48; i++)
		if(StackableTypes[i] == objTyp) {
			ret = 2;
			break;
		}
	return ret;
}

/*number of items an object stands for (stack quantity, at least 1)*/
/*C_0A5D_2169*/static OBJ_GetCount(int16 objNum) {
	register int16 quantity, quanType;

	quanType = QuanType(ObjShapeType[objNum]);
	quantity = GetQuan(objNum);
	if(quanType == 2) {
		if(quantity == 0)
			SetQuan(objNum, 1);
		else
			return quantity;
	}
	return 1;
}

/*give owner (NPC or container) amount (quantity | quality << 8) items of
  objType; what the owner can't take is dropped at its feet. Returns the
  last object created.*/
/*C_0A5D_21AC*/GiveObj(register int16 owner, int16 objType, int16 amount) {
	int16 obj, quality, quantity, quanType, coordUse, dropped, x, y;
	register int16 root;

	dropped = 0;
	for(root = owner; GetCoordUse(root) != LOCXYZ; root = GetAssoc(root));
	if(owner < 0)
		return -1;
	x = GetX(root);
	y = GetY(root);
	STAT_RefreshInventory(owner);
	quality = amount >> 8;
	quantity = (unsigned char)amount;
	quanType = QuanType(objType);
	if(IsNPC(owner))
		coordUse = INVEN;
	else
		coordUse = CONTAINED;
	if(quanType == 2) {
		obj = AddInvObj(objType, coordUse, QualQuan(quality, quantity), owner);
		if(obj == -1) {
			obj = AddMapObj(objType, 0, QualQuan(quality, quantity), x, y, MapZ);
			dropped = 1;
		}
	} else {
		for(; quantity > 0; quantity --) {
			obj = AddInvObj(objType, coordUse, QualQuan(quality, 0), owner);
			if(obj < 0) {
				dropped = 1;
				obj = AddMapObj(objType, 0, QualQuan(quality, 0), x, y, MapZ);
			}
		}
	}
	if(dropped)
		MoveObj(owner, x, y, MapZ);
	return obj;
}

/*remove amount (quantity | quality << 8, quality 0 = any) items of
  objType from owner's possessions (recursively)*/
/*C_0A5D_22F7*/TakeObj(int16 owner, int16 objType, int16 amount) {
	register int16 obj, changed;
	int16 quanType, count, next, quality, quantity;

	changed = 0;
	if(owner < 0)
		return -1;
	quality = amount >> 8;
	quantity = (unsigned char)amount;
	quanType = QuanType(objType);
	if(quanType != 4)
		amount = quantity;
	obj = FindInv(owner);
	while(obj >= 0 && GetCoordUse(obj) != LOCXYZ && OBJ_IsInside(obj, owner)) {
		if(GetType(obj) == objType) {
			if(quality == 0 || GetQual(obj) == quality)  {
				count = OBJ_GetCount(obj);
				if(amount >= count) {
					amount -= count;
					next = Link[obj];
					DeleteObj(obj);
					obj = next;
					changed = 1;
					continue;
				}
				if(quanType == 2)
					SubQuan(obj, amount);
				amount = 0;
				changed = 1;
				/*always true here*/
				if(amount <= 0)
					break;
			}
		}
		obj = Link[obj];
	}
	if(changed && StatusDisplay == 0x92 && Party[StatusMember] == owner)
		StatusDirty = 1;
	return obj;
}

/*count the items of objType that objNum possesses (recursively)*/
/*C_0A5D_2406*/TestObj(int16 objNum, int16 objType) {
	register int16 obj, total;

	total = 0;
	obj = FindInv(objNum);
	while(obj >= 0 && GetCoordUse(obj) != LOCXYZ && OBJ_IsInside(obj, objNum)) {
		if(GetType(obj) == objType)
			total += OBJ_GetCount(obj);
		obj = Link[obj];
	}
	return total;
}

/*move amount items of objType from oldOwner to newOwner*/
/*C_0A5D_2456*/TransferObj(int16 oldOwner, int16 newOwner, int16 objType, int16 amount) {
	if(TakeObj(oldOwner, objType, amount) >= 0)
		GiveObj(newOwner, objType, amount);
}

/*copy object objNum to entry index of the objblk buffer and free it
  (ObjShapeType = 0). BLK_OBJNUM() maps buffer entries back to
  object numbers so that a CONTAINED object can store its container as a
  buffer index; NPC-held items keep the NPC number.*/
/*C_0A5D_247B*/static __ObjSerialize(register int16 objNum, register int16 index) {
	struct coord containerPos;
	int16 containerIdx;

	if(GetCoordUse(objNum) != LOCXYZ) {
		if((int16)GetAssoc(objNum) < 0x100 && (int16)GetAssoc(objNum) >= 0) {
			if(GetCoordUse(objNum) == CONTAINED)
				SetCoordUse(objNum, INVEN);
		} else {
			SetCoordUse(objNum, CONTAINED);
		}
	}
	BLK_STATUS(index) = ObjStatus[objNum];
	BLK_POS(index) = ObjPos[objNum];
	BLK_SHAPE(index) = ObjShapeType[objNum];
	BLK_AMOUNT(index) = Amount[objNum];
	if(GetCoordUse(objNum) == CONTAINED) {
		for(
			containerIdx = index;
			BLK_OBJNUM(containerIdx) != GetAssoc(objNum);
			containerIdx --
		);
		SetCoordAssoc(containerPos, containerIdx);
		BLK_POS(index) = containerPos;
	}
	ObjShapeType[objNum] = 0;
	if(index + 1 >= 0x800)
		CON_printf(/*0866*/"Error-too many objects in region!\n");
}

/*delete every "objblkXY.tmp" file flagged in ObjBlkTmpPending*/
/*C_0A5D_25CC*/OBJ_DeleteTmpBlocks() {
	int16 areaId;

	DISK_confirm(8);
	for(areaId = 0; areaId < (8*8 + 5); areaId ++) {
		if(ObjBlkTmpPending[areaId]) {
			__Tmp_file[15] = (areaId & 7) + 'A';
			__Tmp_file[16] = (areaId >> 3) + 'A';
			OSI_delete(__Tmp_file);
		}
	}
}

/*write the numObj entries of the objblk buffer to __Tmp_file (whose
  letters were set by the caller): a word count then 8 bytes per object.
  If the disk gets full (DISK_full sets DiskFullChoice): on 'R' (restore)
  nothing more is written; on 'S' (save) every pending .tmp file is renamed
  over the real file, this block is written directly to __Sav_file and a
  save game is requested (DiskFullChoice = 'G').*/
/*C_0A5D_261B*/static OBJ_WriteBlockFile(int16 numObj, int16 areaId) {
	register int16 fd;
	int16 i;
	char savLetters[2], tmpLetters[2];

	ObjBlkTmpPending[areaId] = 1;
	DISK_confirm(8);
	fd = OSI_create(__Tmp_file);
	OSI_write(fd, 0, 2, &numObj);
	if(DiskFullChoice != 'S' && DiskFullChoice != 'R')
		OSI_write(fd, -1, (int32)numObj << 3, ScratchBuf);
	if(DiskFullChoice == 'S') {
		tmpLetters[0] = __Tmp_file[15];
		tmpLetters[1] = __Tmp_file[16];
		savLetters[0] = __Sav_file[15];
		savLetters[1] = __Sav_file[16];
		OSI_close(fd);
		for(i = 0; i < (8*8 + 5); i ++) {
			if(ObjBlkTmpPending[i]) {
				__Sav_file[15] = __Tmp_file[15] = (i & 7) + 'A';
				__Sav_file[16] = __Tmp_file[16] = (i >> 3) + 'A';
				OSI_delete(__Sav_file);
				if(i != areaId)
					OSI_rename(__Tmp_file, __Sav_file);
				ObjBlkTmpPending[i] = 0;
			}
		}
		__Tmp_file[15] = __Sav_file[15] = tmpLetters[0];
		__Tmp_file[16] = __Sav_file[16] = tmpLetters[1];
		fd = OSI_create(__Sav_file);
		OSI_write(fd, 0, 2, &numObj);
		OSI_write(fd, -1, (int32)numObj << 3, ScratchBuf);
		__Sav_file[15] = savLetters[0];
		__Sav_file[16] = savLetters[1];
		DiskFullChoice = 'G';
		MapAnimEnabled = 1;
	}
	OSI_close(fd);
}

/*unload the outdoor area areaId (yyyxxx, 128x128 tiles at z 0): every
  object there and its contents, including the inventories of the NPCs
  standing there (the NPCs themselves stay), is serialized, removed from
  the chain and freed, then written to the area's .tmp file*/
/*C_0A5D_278E*/static OBJ_UnloadArea(int16 areaId) {
	register int16 obj, numObj;
	int16 prev, last, baseX, baseY;

	if(areaId < 0 || DiskFullChoice == 'R')
		return;
	numObj = 0;
	baseX = (areaId & 7) << 7;
	baseY = (areaId & 0x38) << 4;
	prev = 0x100;
	obj = Link[0x100];
	while(numObj < 0x800 && obj >= 0) {
		if(GetCoordUse(obj) == LOCXYZ && GetZ(obj) == 0 &&
			(GetX(obj) & 0x380) == baseX && (GetY(obj) & 0x380) == baseY
		) {
			if(IsNPC(obj)) {
				prev = obj;
			} else {
				BLK_OBJNUM(numObj) = obj;
				__ObjSerialize(obj, numObj++);
				FreeObjCount ++;
			}
			last = obj;
			obj = Link[obj];
			while(obj >= 0 && GetCoordUse(obj) != LOCXYZ) {
				if(numObj < 0x800) {
					BLK_OBJNUM(numObj) = obj;
					__ObjSerialize(obj, numObj++);
				}
				FreeObjCount ++;
				last = obj;
				obj = Link[obj];
			}
			Link[last] = FreeObjHead;
			FreeObjHead = Link[prev];
			Link[prev] = obj;
		} else {
			prev = obj;
			obj = Link[obj];
		}
	}
	__Tmp_file[15] = (areaId & 7) + 'A';
	__Tmp_file[16] = (areaId >> 3) + 'A';
	OBJ_WriteBlockFile(numObj, areaId);
}

/*order of two objects in the chain for the merge: < 0 if obj0 must come
  first (-1 stands for the end of a list). An NPC-held object sorts
  before a map object (OBJ_RelinkHeldItems moves it behind its owner later);
  otherwise objects are compared by the location of their outermost
  container/owner: y, then x, then higher z first.*/
/*C_0A5D_28D6*/static OBJ_CompareOrder(int16 obj0, int16 obj1) {
	register int16 root0, root1;
	int16 dist;

	if(obj0 == -1)
		return 1;
	if(obj1 == -1)
		return -1;
	root0 = obj0; root1 = obj1;
	while(GetCoordUse(root0) == CONTAINED)
		root0 = GetAssoc(root0);
	while(GetCoordUse(root1) == CONTAINED)
		root1 = GetAssoc(root1);
	if(GetCoordUse(root0) != LOCXYZ && GetCoordUse(obj1) == LOCXYZ)
		return -1;
	if(GetCoordUse(root1) != LOCXYZ && GetCoordUse(obj0) == LOCXYZ)
		return 1;
	if(IsHeldByNPC(root0))
		root0 = GetAssoc(root0);
	if(IsHeldByNPC(root1))
		root1 = GetAssoc(root1);
	dist = GetY(root0) - GetY(root1);
	if(dist == 0)
		dist = GetX(root0) - GetX(root1);
	if(dist == 0)
		dist = GetZ(root1) - GetZ(root0);
	if(root0 == root1) {
		if(GetCoordUse(root0) == LOCXYZ)
			dist = -1;
		else if(GetCoordUse(root1) == LOCXYZ)
			dist = 1;
	}
	return dist;
}

/*after loading blocks: move every NPC-held (INVEN/EQUIP) object that is
  not inside its owner's subtree (with its own contents) behind its owner*/
/*C_0A5D_2A37*/static OBJ_RelinkHeldItems() {
	register int16 obj, walk;
	int16 prev, last, owner;

	prev = 0x100;
	while(Link[prev] >= 0) {
		obj = Link[prev];
		if(IsHeldByNPC(obj)) {
			owner = GetAssoc(obj);
			for(
				walk = Link[owner];
				walk >= 0 && GetCoordUse(walk) != LOCXYZ && walk != obj;
				walk = Link[walk]
			);
			if(walk != obj) {
				last = OBJ_LastOfSubtree(obj);
				owner = OBJ_LastOfSubtree(owner);
				Link[prev] = Link[last];
				Link[last] = Link[owner];
				Link[owner] = obj;
				continue;
			}
		}
		prev = obj;
	}
}

/*allocate numObj objects from the objblk buffer (as long as slots are
  free), translate the CONTAINED buffer indexes back to object numbers,
  then merge the new (sorted) chain into the main list*/
/*C_0A5D_2AE0*/static __ObjectsDeserialize(int16 numObj) {
	register int16 cur, incoming;
	int16 i, tmp, restart;

	for(i = 0; i < numObj; i ++) {
		if(FreeObjCount == 0) {
			BLK_OBJNUM(i) = -1;
			if(i)
				Link[BLK_OBJNUM(i - 1)] = -1;
			break;
		}
		cur = BLK_OBJNUM(i) = FreeObjHead;
		FreeObjHead = Link[FreeObjHead];
		FreeObjCount --;
		ObjStatus[cur] = BLK_STATUS(i);
		ObjPos[cur] =  BLK_POS(i);
		ObjShapeType[cur] = BLK_SHAPE(i);
		Amount[cur] = BLK_AMOUNT(i);
		if(GetCoordUse(cur) == CONTAINED)
			GetAssoc(cur) = BLK_OBJNUM((int16)GetAssoc(cur));
	}
	if(i == numObj)
		Link[BLK_OBJNUM(i - 1)] = -1;
	/*merge: cur walks the main list, incoming the new chain*/
	cur = 0x100;
	incoming = BLK_OBJNUM(0);
	restart = 0;
	while(cur >= 0 && incoming >= 0) {
		if(OBJ_CompareOrder(Link[cur], incoming) < 0) {
			cur = Link[cur];
		} else {
			tmp = Link[cur];
			Link[cur] = incoming;
			cur = tmp;
			while(Link[incoming] >= 0 && OBJ_CompareOrder(cur, Link[incoming]) > 0) {
				if(OBJ_CompareOrder(incoming, Link[incoming]) <= 0) {
					incoming = Link[incoming];
				} else {
					restart = 1;
					break;
				}
			}
			tmp = Link[incoming];
			Link[incoming] = cur;
			incoming = tmp;
			if(restart) {
				cur = 0x100;
				restart = 0;
				continue;
			}
		}
		if(Link[cur] < 0) {
			Link[cur] = incoming;
			cur = -1;
		}
	}
}

/*load the outdoor area areaId from its objblk file (the .tmp one if any)*/
/*C_0A5D_2CFD*/static OBJ_LoadArea(int16 areaId) {
	register int16 fd;
	int16 numObj;

	if(areaId < 0 || DiskFullChoice == 'R')
		return;
	areaId &= 0x3f;
	DISK_confirm(8);
	if(!ObjBlkTmpPending[areaId]) {
		__Sav_file[15] = 'A' + (areaId & 7);
		__Sav_file[16] = 'A' + (areaId >> 3);
		fd = OSI_open(__Sav_file);
	} else {
		__Tmp_file[15] = 'A' + (areaId & 7);
		__Tmp_file[16] = 'A' + (areaId >> 3);
		fd = OSI_open(__Tmp_file);
	}
	OSI_read(fd, 0, 2, (void far *)&numObj);
	if(numObj > 0x800)
		numObj = 0x800;
	if(numObj)
		OSI_read(fd, -1, (int32)numObj << 3, ScratchBuf);
	OSI_close(fd);
	if(numObj)
		__ObjectsDeserialize(numObj);
}

/*rebuild the MapObjPtr[][] cache: first chained object of every tile of
  the 40x40 area*/
/*C_0A5D_2DD5*/OBJ_RebuildMapObjPtr() {
	register int16 obj, relY;
	int16 relX;
	int16 unusedMask;

	for(relY = 0; relY < AREA_H; relY ++) {
		for(relX = 0; relX < AREA_W; relX ++)
			MapObjPtr[relY][relX] = -1;
	}
	for(obj = Link[0x100]; obj >= 0; obj = Link[obj]) {
		if(GetCoordUse(obj) == LOCXYZ) {
			unusedMask = GetZ(obj)?0xff:0x3ff;/*never used*/
			relX = (GetX(obj) - AreaX) & 0x3ff;
			relY = (GetY(obj) - AreaY) & 0x3ff;
			if(relX < AREA_W && relY < AREA_H && MapObjPtr[relY][relX] == -1)
				MapObjPtr[relY][relX] = obj;
		}
	}
}

/*unload underground level z (1..5) like OBJ_UnloadArea does for an outdoor
  area; its file is "objblk<'A'+z-1>i", flag ObjBlkTmpPending[64 + z - 1]*/
/*C_0A5D_2EC0*/static OBJ_UnloadDungeon(int16 z) {
	register int16 obj, numObj;
	int16 prev, last;

	if(z < 0 || z > 5 || DiskFullChoice == 'R')
		return;
	numObj = 0;
	prev = 0x100;
	obj = Link[0x100];
	while(numObj < 0x800 && obj >= 0) {
		if(GetCoordUse(obj) == LOCXYZ && GetZ(obj) == z) {
			if(IsNPC(obj)) {
				prev = obj;
			} else {
				BLK_OBJNUM(numObj) = obj;
				__ObjSerialize(obj, numObj++);
				FreeObjCount ++;
			}
			last = obj;
			obj = Link[obj];
			while(obj >= 0 && GetCoordUse(obj) != LOCXYZ) {
				if(numObj < 0x800) {
					BLK_OBJNUM(numObj) = obj;
					__ObjSerialize(obj, numObj++);
				}
				FreeObjCount ++;
				last = obj;
				obj = Link[obj];
			}
			Link[last] = FreeObjHead;
			FreeObjHead = Link[prev];
			Link[prev] = obj;
		} else {
			prev = obj;
			obj = Link[obj];
		}
	}
	__Tmp_file[15] = z - 1 + 'A';
	__Tmp_file[16] = 'I';
	OBJ_WriteBlockFile(numObj, z + 0x3f);
}

/*load underground level z (1..5) from its objblk file (the .tmp one if any)*/
/*C_0A5D_2FBD*/static OBJ_LoadDungeon(int16 z) {
	register int16 fd;
	int16 numObj;

	if(z < 0 || z > 5 || DiskFullChoice == 'R')
		return;
	DISK_confirm(8);
	if(!ObjBlkTmpPending[8*8 + z - 1]) {
		__Sav_file[15] = 'A' + z - 1;
		__Sav_file[16] = 'I';
		fd = OSI_open(__Sav_file);
	} else {
		__Tmp_file[15] = 'A' + z - 1;
		__Tmp_file[16] = 'I';
		fd = OSI_open(__Tmp_file);
	}
	OSI_read(fd, 0, 2, (void far *)&numObj);
	if(numObj > 0x800)
		numObj = 0x800;
	if(numObj)
		OSI_read(fd, -1, (int32)numObj << 3, ScratchBuf);
	OSI_close(fd);
	if(numObj)
		__ObjectsDeserialize(numObj);
}

/*make sure the objects around (x, y, z) are loaded. On the surface a
  2x2 window of areas is kept; it is only shifted when (x, y) comes
  within 16 tiles of its border. Areas leaving the window (or the level
  left) are unloaded, new ones loaded, then NPC inventories relinked.*/
/*C_0A5D_3083*/LoadNewRegions(int16 x, int16 y, int16 z) {
	register int16 i, j;
	int16 shiftX, shiftY, relX, relY, numUnloaded, numLoaded;
	int16 mapIds[4];

	numUnloaded =
	numLoaded = 0;
	if(LoadedZ > 0 && LoadedZ != z) {
		OBJ_PurgeFarLocals(x, y, z);
		OBJ_UnloadDungeon(LoadedZ);
		numUnloaded ++;
	}
	if(z == 0) {
		/*mapIds[0] = top-left area of the window*/
		if(LoadedWindowArea == -1) {
			mapIds[0] = (((x - 0x40) >> 7) & 7) + (((y - 0x40) >> 4) & 0x38);
		} else {
			shiftX = 0;
			relX = x - ((LoadedWindowArea & 7) << 7);
			for(; relX < 0x10; relX += 0x80, shiftX --);
			for(; relX >= 0xf0; relX -= 0x80, shiftX ++);
			shiftY = 0;
			relY = y - ((LoadedWindowArea & 0x38) << 4);
			for(; relY < 0x10; relY += 0x80, shiftY -= 8);
			for(; relY >= 0xf0; relY -= 0x80, shiftY += 8);
			mapIds[0] = (((LoadedWindowArea & 7) + shiftX) & 7) + (((LoadedWindowArea & 0x38) + shiftY) & 0x38);
		}
		mapIds[1] = ((mapIds[0] + 1) & 7) + (mapIds[0] & 0x38);
		mapIds[2] = (mapIds[0] + 8) & 0x3f;
		mapIds[3] = (mapIds[1] + 8) & 0x3f;
		LoadedWindowArea = mapIds[0];
	} else {
		for(i = 0; i < 4; i ++)
			mapIds[i] = -1;
		LoadedWindowArea = -1;
	}
	/*unload the areas no longer needed*/
	if(LoadedZ == 0) {
		for(i = 0; i < 4; i ++) {
			for(j = 0; j < 4; j ++) {
				if(mapIds[j] == LoadedAreas[i])
					break;
			}
			if(j != 4 || LoadedAreas[i] == -1)
				continue;
			if(numUnloaded == 0)
				OBJ_PurgeFarLocals(x, y, z);
			OBJ_UnloadArea(LoadedAreas[i]);
			numUnloaded ++;
			LoadedAreas[i] = -1;
		}
	}
	/*load the new ones*/
	if(z == 0) {
		for(j = 0; j < 4; j ++) {
			for(i = 0; i < 4; i++) {
				if(mapIds[j] == LoadedAreas[i])
					break;
			}
			if(i != 4)
				continue;
			for(i = 0; i < 4; i ++) {
				if(LoadedAreas[i] == -1)
					break;
			}
			OBJ_LoadArea(mapIds[j]);
			numLoaded ++;
			LoadedAreas[i] = mapIds[j];
		}
	} else if(LoadedZ != z) {
		OBJ_LoadDungeon(z);
		numLoaded ++;
	}
	LoadedZ = z;
	if(numLoaded)
		OBJ_RelinkHeldItems();
}

/*"commit" obj files (save game): write the loaded blocks, rename every
  .tmp file over its objblk file, then reload the blocks*/
/*C_0A5D_32CC*/OBJ_CommitBlocks() {
	int16 i;

	if(MapZ) {
		OBJ_UnloadDungeon(MapZ);
	} else {
		for(i = 0; i < 4; i ++)
			OBJ_UnloadArea(LoadedAreas[i]);
	}
	for(i = 0; i < (8*8 + 5); i ++) {
		if(ObjBlkTmpPending[i]) {
			__Tmp_file[15] = __Sav_file[15] = __Bak_file[15] = (i & 7) + 'A';
			__Tmp_file[16] = __Sav_file[16] = __Bak_file[16] = (i >> 3) + 'A';/*.bak is never used*/
			OSI_delete(__Sav_file);
			OSI_rename(__Tmp_file, __Sav_file);
			ObjBlkTmpPending[i] = 0;
		}
	}
	for(i = 0; i < 4; i ++)
		LoadedAreas[i] = -1;
	LoadedWindowArea = LoadedZ = -1;
	LoadNewRegions(MapX, MapY, MapZ);
	OBJ_RebuildMapObjPtr();
}

/*add a tile to the display stack of the 11x11 view cell (x, y).
  Each cell's stack is a list of nodes: Obj_11x11[y][x] = first node,
  CellObjNext[node] = next node (CellObjNext[0] = free node list),
  CellObjTile[node] = tile | frame; it is drawn from the first node on.
  A "background" tile replaces the cell's map tile instead.
  Non-foreground tiles go first (below everything already there);
  foreground tiles (or mode 2) go, for an odd mode, last (on top), else
  just before the first foreground tile already there.
  Nothing is shown on hidden or dark cells; eggs are never shown.*/
/*C_0A5D_339C*/ShowObject(int16 tileFrame, int16 x, int16 y, int16 mode) {
	register unsigned char *slot;
	register int16 tile;
	int16 mapTile, onTop/*set, never read*/, node;

	tile = tileFrame & 0x7ff;
	if(BaseTile[0xe6/*egg*/] == tile)
		return;
	mapTile = Tile_11x11[y][x];
	if(mapTile == 0xff/*hidden*/ || mapTile == 0x1bc/*darkness*/)
		return;
	if(IsTileBa(tile)) {
		Tile_11x11[y][x] = tileFrame;
		return;
	}
	node = CellObjNext[0];
	if(node == 0)
		return;
	CellObjNext[0] = CellObjNext[node];
	CellObjTile[node] = tileFrame;
	slot = &(Obj_11x11[y][x]);
	if(IsTileFor(tile) || mode == 2) {
		onTop = 1;
		if(mode & 1) {
			while(*slot)
				slot = CellObjNext + *slot;
		} else {
			while(*slot && !IsTileFor(CellObjTile[*slot] & 0x7ff))
				slot = CellObjNext + *slot;
		}
	}
	CellObjNext[node] = *slot;
	*slot = node;
}

/*show tile + frame at view cell (x, y) together with the left/upper parts
  of double-width/double-height tiles (x, y may be 11 for those)*/
/*C_0A5D_34A6*/static OBJ_ShowMultiTile(int16 tile, int16 frame, register int16 x, register int16 y) {
	int16 flags;

	if(x < 11 && y < 11)
		ShowObject(tile + frame, x, y, 0);

	flags = TileFlag[tile];
	if(flags & 0x80) {
		if(x != 0 && y < 11)
			ShowObject(tile + frame - 1, x - 1, y, 1);
		if(flags & 0x40) {
			if(x < 11 && y != 0)
				ShowObject(tile + frame - 2, x, y - 1, 1);
			if(x != 0 && y != 0)
				ShowObject(tile + frame - 3, x - 1, y - 1, 1);
		}
	} else if(x < 11 && y != 0 && (flags & 0x40)) {
		ShowObject(tile + frame - 1, x, y - 1, 1);
	}
}

/*build the display stacks of the 11x11 view from the objects around
  (MapX, MapY) (with an outline for invisible, protected, ... NPCs), then
  hide what is on walls not seen from the party side and draw the
  darkness of the badly lit cells*/
/*C_0A5D_3577*/ShowObjects() {
	register int16 objNum, j;
	int16 i, tile, objX, objY, light, newFrame, objTyp, stack, node;

	for(i = 0; i < 0x100; i ++)
		CellObjNext[i] = i + 1;

	for(i = 0; i < 11; i ++)
		for(j = 0; j < 11; j ++)
			Obj_11x11[i][j] = 0;

	for(
		objNum = SearchArea(MapX - 5, MapY - 5, MapX + 6, MapY + 6);
		objNum >= 0;
		objNum = NextArea()
	) {
		newFrame = 0;
		objTyp = GetType(objNum);
		if(!IsInvisible(objNum) || (IsNPC(objNum) && IsPlrControl(objNum))) {
			if(!IsNPC(objNum) || !(NPCFlag[objNum] & 0x10)) {
				tile = TILE_FRAME(objNum);
				/*outline drawn with the tile (bits 11-12 of the tile number)*/
				if(IsInvisible(objNum))
					newFrame = (2 << 10);
				if(IsNPC(objNum)) {
					if(NPCStatus[objNum] & 0x01)
						newFrame = (2 << 10);
					if(ObjStatus[objNum] & 0x40)
						newFrame = (6 << 10);
					if(ObjStatus[objNum] & 0x04)
						newFrame = (2 << 10);
					if(D_27FB && (NPCStatus[objNum] & 0x20))
						newFrame = (4 << 10);
					if(ObjStatus[objNum] & 0x80)
						newFrame = (6 << 10);
					if((objNum == 0x2f || objNum == 0x37 || objNum == 0x1f) && !(D_6EFD & 4))
						newFrame = OSI_rand(0, 1)?(6 << 10):(2 << 10);
				}
				objX = GetX(objNum) - MapX + 5;
				objY = GetY(objNum) - MapY + 5;
				if(newFrame)
					OBJ_ShowMultiTile(tile, newFrame, objX, objY);
				if(!IsInvisible(objNum))
					OBJ_ShowMultiTile(tile, 0, objX, objY);
			}
		}
	}
	for(i = 0; i < 11; i ++) {
		for(j = 0; j < 11; j ++) {
			if(IsTerrainWall(Tile_11x11[j][i])) {
				if(
					!(AreaFlags[j     + ViewY][i + 1 + ViewX] & 0x80) ||
					!(AreaFlags[j + 1 + ViewY][i     + ViewX] & 0x80) ||
					!(AreaFlags[j + 1 + ViewY][i + 1 + ViewX] & 0x80)
				) {
					stack = Obj_11x11[j][i];
					/*wall not seen from the party side: hide what is on it*/
					if(stack) {
						Obj_11x11[j][i] = 0;
						for(node = stack; CellObjNext[node]; node = CellObjNext[node]);
						CellObjNext[node] = CellObjNext[0];
						CellObjNext[0] = stack;
					}
				}
			}
			/*draw obscurity*/
			light = AreaLight[j + ViewY][i + ViewX];
			if(light < 4 && light > 0) {
				tile = 0x1bc + light;
				ShowObject(tile, i, j, 3);
			}
		}
	}
	if(Erupting == 1)
		Erupting = 0;
	MusicianWasNear = 0;
}

/*Initialization: empty object list, all slots free (monsters 0xC0-0xFF,
  objects 0x101-0xBFF), no area loaded*/
/*C_0A5D_38A9*/OBJ_InitLists() {
	register int16 i;

	for(i = 0; i < 0xbff; i ++)
		Link[i] = i + 1;
	Link[0xbff] = -1;
	Link[0xff] = -1;
	FreeMonsterHead = 0xc0;
	FreeMonsterCount = 0x3f;
	FreeObjHead = 0x101;
	FreeObjCount = 0xaff;
	Link[0x100] = -1;
	for(i = 0; i < 4; i ++)
		LoadedAreas[i] = -1;
	LoadedWindowArea = LoadedZ = -1;
}

/*called after a restore: link the existing NPCs (0x00-0xFF) into the
  sorted object list, remove the used temporary monster slots from their
  free list and (unless D_6F1D & 1) give the party NPCs 1-15 full hit
  points and their combat mode*/
/*C_0A5D_390D*/OBJ_RelinkNPCs() {
	register int16 npc, prev;
	int16 lastFree;

	lastFree = 0x100;
	for(npc = 0; npc < 0x100; npc ++) {
		if(ObjShapeType[npc]) {
			if(npc >= 0xc0) {
				if(lastFree == 0x100)
					FreeMonsterHead = npc + 1;
				else
					Link[lastFree] = npc + 1;
				FreeMonsterCount --;
			} else if(npc > 0 && npc < 0x10 && !(D_6F1D & 1)) {
				HitPoints[npc] = MaxHP(npc);
				if(npc == 9 || npc == 7)
					NPCComMode[npc] = 4;
				else if(npc == 1)
					NPCComMode[1] = 2;
				else
					NPCComMode[npc] = 8;
			}
			for(prev = 0x100; Link[prev] >= 0 && OBJ_CompareOrder(Link[prev], npc) < 0; prev = Link[prev]);
			Link[npc] = Link[prev];
			Link[prev] = npc;
		} else {
			if(npc >= 0xc0)
				lastFree = npc;
		}
	}
}

/*load tile description from "look.lzc": entries are (word first tile,
  zero-terminated string) sorted by tile; the entry for numTile is the
  first one whose first tile is >= numTile. Returns "nothing" if empty.*/
/*C_0A5D_39F8*/char *GetTileString(int16 numTile) {
	register char *dst;
	register int16 tileNum;
	int16 far *src;

	DISK_confirm(3);
	src = LZC_load(/*0889*/"look.lzc", 0, D_54E8, 0, D_3EB6);
	tileNum = *(src++);
	while(tileNum < numTile) {
		while(*CHARPTR_POSTINC(src));
		tileNum = *(src++);
	}
	dst = TileStringBuf;
	while(*dst++ = *CHARPTR_POSTINC(src));
	return TileStringBuf[0]?TileStringBuf:/*0892*/"nothing";
}

/*name of an object: the party member's name, the original shape's name for
  a dead or sleeping NPC, otherwise its tile description*/
/*C_0A5D_3A76*/char *GetObjectString(register int16 objNum) {
	char *str;
	register int16 i;

	if(objNum < 0) {
		str = /*089A*/"Nothing";
	} else if(objNum < 0x10) {
		str = GetTileString(TILE_FRAME(objNum));
		for(i = 0; i < PartySize; i ++) {
			if(Party[i] == objNum) {
				strcpy(TileStringBuf, Names[i]);
				str = TileStringBuf;
				break;
			}
		}
	} else if(IsNPC(objNum) && (NPCStatus[objNum] & 0x14)) {
		str = GetTileString(BaseTile[OrigShapeType[objNum] & 0x3ff] + (OrigShapeType[objNum] >> 10));
	} else {
		str = GetTileString(TILE_FRAME(objNum));
	}
	return str;
}
