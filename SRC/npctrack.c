/* flags: -d -mm -p -c -Z -O -y */
/*
 * The Savage Empire (Worlds of Ultima) - GAME.EXE
 * segment 130F: NPC tracker, movement and A.I. of every actor (objects
 * 0..0xff), from Ultima VI's npctrack.c.
 *
 * - passability test (CanMoveTo) shared by the party, NPCs and creatures,
 *   with the big creatures of Savage Empire: 2x2 bodies, creatures made of
 *   several objects (heads and tails), tile edge flags that let actors
 *   cross the open sides of impassable tiles; facing/walk-animation update
 *   (UpdateMoveFrame), which also turns the other parts of a creature;
 *   movement-point accounting (SubMov, SubTerrainMov);
 * - party movement: the leader's step (PartyAdvance), followers keeping a
 *   formation behind him (MoveFollowers), boarding/leaving a vehicle
 *   (Board/Unboard), entrances (CheckGatesAndEntrances);
 * - NPC schedules: hourly schedule switch (UpdateSchedules), start of the
 *   scheduled action (StartScheduleAction), off-screen teleport
 *   (TeleportToSchedule);
 * - path finding (PTH_*): a bidirectional uniform-cost search on the 40x40
 *   area around the player, using a bucket queue in ScratchBuf; up to 8
 *   paths are stored run-length encoded in PathData (slot = Leader[npc]);
 * - the per-NPC A.I. dispatcher (NPC_TakeTurn) and the turn scheduler that
 *   lets every actor act in proportion to its dexterity until it is the
 *   player's turn again (RunNPCTurns).
 *
 * NPCMode values are Savage Empire's (U6's names in comments where the
 * mode is the same: 0 motionless, 1 follow, 2 command, 0x80 schedule,
 * 0x81 find path, 0x82 seek object, 0x83 on path, 0x84/0x85 path blocked,
 * 0x86 path failed, 0x91 sleep).
 */
#include "se.h"



#define AREA_W 40
#define AREA_H 40

/*a schedule entry (Ultima VI's)*/
struct tSchedule/*size:5*/ {
	unsigned char time;/*bits 0-4 hour, bits 5-7 day of the week*/
	unsigned char action;
	struct coord xyz;
};

/*path slots (Ultima VI's)*/
struct tPathData {
	unsigned char content[8][0x20];
};

/*the path search work area in ScratchBuf (Ultima VI's)*/
typedef struct {
	unsigned char /*_0000*/PTH_map[AREA_H][AREA_W];
	int16 /*_0640*/PTH_rank[0x100];
	int16 /*_0840*/PTH_link[0x100];
	unsigned char /*_0a40*/PTH_x[0x100],/*_0b40*/PTH_y[0x100];
	unsigned char /*_0c40*/PTH_resist[AREA_H][AREA_W];
} t40x40;

/*console*/
extern cdecl CON_printf(char far *, ...);/*CON_printf*/
/*map*/
extern MAP_LoadAround();/*MAP_LoadAround*/
extern PartyEnter(int16);/*PartyEnter*/
extern PartyExit(int16, int16, int16, int16);/*PartyExit*/
extern unsigned char GetTileAtXYZ(int16, int16, int16);/*GetTileAtXYZ*/
/*object*/
extern int16 OBJ_IsLocLoaded(int16, int16, int16);/*OBJ_IsLocLoaded(x, y, z)*/
extern int16 NextLoc();/*NextLoc*/
extern int16 FindLoc(int16, int16, int16);/*FindLoc(x, y, z)*/
extern int16 C_0A5D_0D5B(int16, int16 *, int16 *);/*the parts of a creature made of several objects (frame shifts, types); 0 if none*/
extern int16 C_0A5D_0DEF(int16);/*OBJ_PartHeadType: type of the head of a multi-tile creature part, 0 if none*/
extern int16 AddMapObj(int16, int16, int16, int16, int16, int16);/*AddMapObj(type and frame, status, amount, x, y, z)*/
extern DeleteObj(int16);/*DeleteObj*/
extern int16 MoveObj(int16, int16, int16, int16);/*MoveObj(objNum, x, y, z)*/
extern InsertObj(int16, int16, int16);/*InsertObj(objNum, destNum, objLoc)*/
extern ShowObjects();/*ShowObjects*/
/*combat*/
extern COMBAT_breakOff();/*COMBAT_breakOff*/
extern int16 COMBAT_TileDistance(int16, int16, int16);/*COMBAT_TileDistance(objNum, x, y)*/
extern COMBAT_stopCharm(int16);/*COMBAT_stopCharm*/
extern SetTypeUnconscious(int16);/*(an NPC going to sleep, U6 nearest: SearchObj)*/
extern int16 COMBAT_canSee(int16, int16);/*COMBAT_canSee(objNum, target)*/
extern CloneObject(int16, int16);/*copies NPC src's data into NPC dst (U6: CloneObject)*/
extern COMBAT_showHit(int16);/*COMBAT_showHit*/
extern int16 COMBAT_pickOpponent(int16);/*COMBAT_pickOpponent*/
extern COMBAT_AI_Assault(int16);/*A.I. of modes 0x08 and 0x9b (U6: COMBAT_AI_Assault)*/
extern COMBAT_AI_Range(int16);/*A.I. of mode 0x04 (U6 nearest: COMBAT_AI_Assault)*/
extern COMBAT_AI_Berserk(int16);/*A.I. of mode 0x06 and of the creatures with ObjStatus 0x80 (U6: COMBAT_AI_Berserk)*/
extern COMBAT_AI_Retreat(int16);/*A.I. of modes 0x07, 0x09, 0x13 and 0x9a (U6: COMBAT_AI_Retreat)*/
extern COMBAT_AI_Like(int16);/*A.I. of mode 0x0a (U6: COMBAT_AI_Like)*/
extern COMBAT_AI_Unfriendly(int16);/*A.I. of mode 0x0b (U6 nearest: COMBAT_AI_Like; COMBAT_AI_Unfriendly?)*/
extern COMBAT_AI_Immobile(int16);/*A.I. of mode 0x0e (U6 nearest: COMBAT_AI_Immobile)*/
extern COMBAT_AI_Tangle(int16);/*A.I. of mode 0x0d (U6: COMBAT_AI_Tangle)*/
/*other modules*/
extern FlushViewIfDirty();/*FlushViewIfDirty*/
extern SetActive(int16);/*SetActive(partyIdx)*/
extern SetPartyMode();/*SetPartyMode*/
extern NextArea();/*NextArea*/
extern int16 SearchArea(int16, int16, int16, int16);/*SearchArea(x1, y1, x2, y2)*/
extern C_2701_0556();/*the Avatar died (U6: PartyDeath)*/
extern TALK_talkTo(int16, int16, int16);/*TALK_talkTo(partyId, objNum, showName)*/
extern PassTime(int16);/*PassTime(minutes)*/
extern C_08CB_0855(int16);/*goes through the entrance objNum (cave, stairs...)*/
extern C_08CB_0B8B(int16);/*goes through objNum (object type 0x44)*/
extern STAT_ShowParty(int16);/*STAT_ShowParty*/
extern MUS_PlayAreaSong();/*MUS_PlayAreaSong*/
extern DrawSkyBar();/*DrawSkyBar*/
extern char far MustBePartyModeMsg[];/*MustBePartyModeMsg: "You must be in party mode to enter.\n"*/
extern int16 C_1DBD_0A29(int16);/*tests a tile (U6 nearest: IsBridgeEndTile)*/
extern int16 GetMonsterClass(int16);/*GetMonsterClass*/
extern int16 OSI_rand(int16, int16);/*OSI_rand(lo, hi)*/
extern int16 far MonsterFlags[];

extern char DirIncrX[], DirIncrY[];/*DirIncrX, DirIncrY*/
extern unsigned char PromptFlag;/*PromptFlag*/
extern int16 StatusDisplay;/*StatusDisplay*/
extern int16 MousePress;/*MousePress*/
extern int16 MouseMapX, MouseMapY;/*MouseMapX, MouseMapY*/
extern int16 MapCoordMask;/*MapCoordMask*/
extern int16 LocPartOffset;/*LocPartOffset*/
extern int16 MoveAppliesTerrain;/*MoveAppliesTerrain*/
extern unsigned char Time_H;/*Time_H*/
extern unsigned char Date_D;/*Date_D*/
extern unsigned char Active;/*Active*/
extern int16 MapX, MapY, MapZ;/*MapX, MapY, MapZ*/
extern unsigned char DrunkCounter;/*DrunkCounter*/
extern unsigned char PathObject[8];/*PathObject: NPC following each path slot (0 = free)*/
extern unsigned char PathCounter[8];/*PathCounter: current entry of each path*/
extern unsigned char PathTries[8];/*PathTries: steps done in the current entry*/
extern int16 SpeakGargish;/*Avatar "solid" (debug)*/
extern char InCombat;/*InCombat*/
extern char SoloMember;/*SoloMember*/
extern unsigned char far *D_3298;/*0x100 bytes (0x80: NPC's "WAIT" flag)*/
extern unsigned char D_329C[];/*ObjStatus*/
extern char D_44FA[];/*MovePts*/
extern unsigned char far *D_45FA;/*SchedIndex: current schedule entry of each NPC, relative to SchedPointer*/
extern struct tPathData far *D_45FE;/*PathData*/
extern unsigned char D_4606[];/*DEXTE*/
extern unsigned char far *D_4602;/*NPCFlag*/
extern unsigned char far *D_4706;/*NPCMode*/
extern unsigned char D_470A[AREA_H][AREA_W];/*AreaTiles: the ground tiles of the area*/
extern unsigned char D_4EDA[];/*Party*/
extern struct tSchedule far *D_4EF0;/*Schedule*/
extern int16 D_4EF4[0x101];/*SchedPointer: first schedule entry of each NPC*/
extern t40x40 far *D_50F6;/*ScratchBuf (the path search work area)*/
extern unsigned char far *D_5100;/*Leader: path slot of an NPC following a path (else its leader/target)*/
extern char D_5104[][14];/*Names*/
extern unsigned char far *D_53E4;/*NPCComMode*/
extern int16 D_54F0[];/*Amount*/
extern unsigned char D_6CF0[];/*NPCStatus*/
extern unsigned char far *D_6DF0;/*TileFlag*/
extern unsigned char D_6DF4[];/*HitPoints*/
extern int16 far *D_6EF4;/*BaseTile*/
extern unsigned char far *D_7640;/*TileFlag2*/
extern struct coord D_78E4[];/*ObjPos*/
extern unsigned char D_9CF6;/*PartySize*/
extern uint16 D_9CFA[];/*ObjShapeType*/
extern unsigned char far *D_B4FA;/*TerrainType*/
extern int16 far *D_B53E;/*OrigShapeType*/
extern int16 D_BAFE, D_BB00;/*AreaX, AreaY*/
extern unsigned char D_EBAA[];/*FormationObjs: the leader then the followers placed so far*/
extern unsigned char D_EBBA;/*PTH_Cur: cost bucket being expanded*/
extern int16 D_EBBC, D_EBBE;/*PTH_AnX, PTH_AnY: edge square reached (goal outside the area)*/
extern unsigned char D_EBC0;/*PTH_MaxCost: highest cost queued so far*/
extern unsigned char D_EBC1;/*PTH_Index: path slot being searched*/
extern int16 D_EBC4;/*PTH_Top: head of the free node list*/
extern int16 D_EBC6;/*PTH_object: object type seeked, -1 if none*/
extern int16 D_EBC8;/*PTH_direct: area edge to reach (0 N, 1 E, 2 S, 3 W), -1 if none*/
extern int16 D_EBCA;/*PTH_Found*/

#define CON_printf CON_printf
#define MAP_LoadAround MAP_LoadAround
#define PartyEnter PartyEnter
#define PartyExit PartyExit
#define GetTileAtXYZ GetTileAtXYZ
#define AddMapObj AddMapObj
#define DeleteObj DeleteObj
#define InsertObj InsertObj
#define ShowObjects ShowObjects
#define COMBAT_breakOff COMBAT_breakOff
#define COMBAT_TileDistance COMBAT_TileDistance
#define COMBAT_stopCharm COMBAT_stopCharm
#define COMBAT_canSee COMBAT_canSee
#define COMBAT_showHit COMBAT_showHit
#define COMBAT_pickOpponent COMBAT_pickOpponent
#define TALK_talkTo TALK_talkTo
#define PassTime PassTime
#define STAT_ShowParty STAT_ShowParty
#define MUS_PlayAreaSong MUS_PlayAreaSong
#define DrawSkyBar DrawSkyBar
#define MustBePartyModeMsg MustBePartyModeMsg
#define OBJ_IsLocLoaded OBJ_IsLocLoaded
#define NextArea NextArea
#define SearchArea SearchArea
#define FlushViewIfDirty FlushViewIfDirty
#define SetActive SetActive
#define SetPartyMode SetPartyMode
#define NextLoc NextLoc
#define FindLoc FindLoc
#define MoveObj MoveObj
#define GetMonsterClass GetMonsterClass
#define OSI_rand OSI_rand

#define DirIncrX DirIncrX
#define DirIncrY DirIncrY
#define PromptFlag PromptFlag
#define StatusDisplay StatusDisplay
#define MousePress MousePress
#define MouseMapX MouseMapX
#define MouseMapY MouseMapY
#define MapCoordMask MapCoordMask
#define LocPartOffset LocPartOffset
#define MoveAppliesTerrain MoveAppliesTerrain
#define Time_H Time_H
#define Date_D Date_D
#define Active Active
#define MapX MapX
#define MapY MapY
#define MapZ MapZ
#define DrunkCounter DrunkCounter
#define PathObject PathObject
#define PathCounter PathCounter
#define PathTries PathTries
#define InCombat InCombat
#define SoloMember SoloMember
#define ObjStatus D_329C
#define MovePts D_44FA
#define SchedIndex D_45FA
#define PathData D_45FE
#define DEXTE D_4606
#define NPCFlag D_4602
#define NPCMode D_4706
#define AreaTiles D_470A
#define Party D_4EDA
#define Schedule D_4EF0
#define SchedPointer D_4EF4
#define _ScratchBuf D_50F6
#define Leader D_5100
#define Names D_5104
#define NPCComMode D_53E4
#define Amount D_54F0
#define NPCStatus D_6CF0
#define TileFlag D_6DF0
#define HitPoints D_6DF4
#define BaseTile D_6EF4
#define TileFlag2 D_7640
#define ObjPos D_78E4
#define PartySize D_9CF6
#define ObjShapeType D_9CFA
#define TerrainType D_B4FA
#define OrigShapeType D_B53E
#define AreaX D_BAFE
#define AreaY D_BB00
#define FormationObjs D_EBAA
#define PTH_Cur D_EBBA
#define PTH_AnX D_EBBC
#define PTH_AnY D_EBBE
#define PTH_MaxCost D_EBC0
#define PTH_Index D_EBC1
#define PTH_Top D_EBC4
#define PTH_object D_EBC6
#define PTH_direct D_EBC8
#define PTH_Found D_EBCA

#define MonsterFlag(cls) (MonsterFlags + 1)[cls]
#define TypeFrame(t, s) ((t) + ((s) << 10))
#define GetType(i) (ObjShapeType[i] & 0x3ff)
#define GetFrame(i) (ObjShapeType[i] >> 10)
#define SetType(i, v) (ObjShapeType[i] = (ObjShapeType[i] & ~0x3ff) + (v))
#define SetFrame(i, v) (ObjShapeType[i] = TypeFrame(GetType(i),(v)))
#define TILE_FRAME(i) (BaseTile[GetType(i)] + GetFrame(i))
#define GetQuan(i) (((unsigned char *)(Amount + (i)))[0])
#define GetQual(i) (((unsigned char *)(Amount + (i)))[1])
#define GetCoordX(o) ((*(int16 *)&((o).raw[0]))&0x3ff)
#define GetCoordY(o) (((*(int16 *)&((o).raw[1]))>>2)&0x3ff)
#define GetCoordZ(o) ((o).raw[2] >> 4)
#define GetX(i) GetCoordX(ObjPos[i])
#define GetY(i) GetCoordY(ObjPos[i])
#define GetZ(i) GetCoordZ(ObjPos[i])
#define GetCoordUse(i) (ObjStatus[i] & 0x18)
#define IsNPC(i) ((i) < 0x100 && (i) >= 0)
#define IsPlrControl(i) (NPCStatus[i] & 0x80)
#define SetPlrControl(i) (NPCStatus[i] |= 0x80)
#define SetAlignment(i, v) (NPCStatus[i] = (NPCStatus[i] & ~0x60) | (v))
#define ClrInvisible(i) (ObjStatus[i] &= ~0x02)
#define ClrLocal(i) (ObjStatus[i] &= ~0x20)
#define IN_VEHICLE (Active == PartySize)
#define IsParalyzed(i) (NPCStatus[i] & 0x02)
#define IsAsleep(i) (NPCStatus[i] & 0x04)
#define ClrAilments(i) (NPCStatus[i] &= ~(0x10|0x08|0x04|0x02))
#define ClrCursedCharmed(i) (ObjStatus[i] &= ~(0x40|0x04))
#define ClrDraggedUnder(i) (NPCFlag[i] &= ~0x10)
#define IsCharmed(i) (ObjStatus[i] & 0x04)
#define IsDead(i) (NPCStatus[i] & 0x10)
#define SetAsleep(i) (NPCStatus[i] |= 0x04)
#define ClrAsleep(i) (NPCStatus[i] &= ~0x04)
#define IsIncapacitated(i) (NPCStatus[i] & 0x16)
#define IsDraggedUnder(i) (NPCFlag[i] & 0x10)
#define IsTerrainWet(tile) (TerrainType[tile] & 1)
#define IsTerrainWall(tile) (TerrainType[tile] & 4)
#define IsTerrainImpass(tile) (TerrainType[tile] & 2)
/*"ignore" tile (the !! gives the original's code)*/
#define IsTileIg(tile) (!!(TileFlag2[tile] & 0x10))
#define IsWalking(i) (NPCFlag[i] & 0x80)
#define SetWalking(i) (NPCFlag[i] |= 0x80)
#define ClrWalking(i) (NPCFlag[i] &= ~0x80)
#define GetDirection(i) (NPCFlag[i] & 7)
#define SetDirection(i, v) (NPCFlag[i] = (NPCFlag[i] & ~7) | (v))

/*furniture an NPC looks for (see AI_SEEKOBJ): only one kind left (type 0xa5)*/
/*0D34*/int16 FurnitureTypes[] = {0xa5};
/*0D36*/int16 AllowNPCTeleport = 0;
/*set before UpdateMoveFrame when the object really stepped (multi-tile parts follow)*/
/*0D38*/int16 StepMovesParts = 0;
/*number of schedule teleports done this turn (max 3)*/
/*0D3A*/int16 TeleportCount = 0;
/*number of path searches done this turn (max 3)*/
/*0D3C*/int16 PathSearchCount = 0;
/*output of CanMoveTo: the tile holds damaging terrain*/
/*0D3E*/int16 HazardFound = 0;
/*never set: if non-zero TryMoveTo would not avoid damaging tiles*/
/*0D40*/int16 IgnoreHazards = 0;
/*print the barrier message (only once per party step; see PartyAdvance)*/
/*0D42*/unsigned char ShowQuestBarrierMsg = 0;
/*the leader's last step was blocked (read by the mouse code to avoid repeats)*/
/*0D43*/int16 AdvanceBlocked = 0;
/*visible area must be recomputed (see FlushViewIfDirty)*/
/*0D45*/int16 VisibilityDirty = 0;
/*CanMoveTo: party members may walk through each other*/
/*0D47*/int16 PassThroughParty = 1;
/*CanMoveTo: refuse damaging terrain*/
/*0D49*/int16 AvoidHazards = 0;
/*0D4B*/int16 EnemyWarningGiven = 0;

/*follower formation: position k is FormationSide[k] tiles sideways and
 FormationBack[k] tiles behind the leader (relative to his facing)*/
/*0D4D*/char FormationSide[] = { 0,-1, 1, 0,-2, 2,-1, 1,-3, 3, 0};
/*0D58*/char FormationBack[] = { 0, 1, 1, 2, 2, 2, 3, 3, 3, 3, 1};
/*CanMoveTo: the square the creature moves from (moved to its front part
 for the big creatures); -1 when it is not on the map. CanMoveTo takes the
 creature's position unless MoveFromSet says the caller set them
 (IsNextToFormation).*/
/*0D63*/int16 MoveFromX = -1;
/*0D65*/int16 MoveFromY = -1;
/*0D67*/int16 MoveFromSet = 0;
/*TerrainType edge flag of each direction (N, -, E, -, S, -, W)*/
/*0D69*/unsigned char DirEdgeFlag[] = {0x80,0,0x40,0,0x20,0,0x10};
/*ship movement cost, index = (move dir - WindDir) & 7 (unused)*/
/*0D70*/unsigned char ShipWindCost[] = {10,5,3,4, 5,4,3,5};
/*skiff movement cost, index = (move dir - current dir) & 7 (unused)*/
/*0D78*/unsigned char SkiffCurrentCost[] = { 3,4,5,7,10,7,5,4};
/*raft drift: RaftDriftDir[current dir][dir the wind blows to] (unused)*/
/*0D80*/unsigned char RaftDriftDir[][8] = {
	{0,0,1,1,0,7,7,0},
	{1,1,1,2,2,1,0,0},
	{1,2,2,2,3,3,2,1},
	{2,2,3,3,3,4,4,3},
	{4,3,3,4,4,4,5,5},
	{6,5,4,4,5,5,5,6},
	{7,7,6,5,5,6,6,6},
	{7,0,0,7,6,6,7,7}
};
/*x/y increments for the 4 path directions N, E, S, W*/
/*0DC0*/char PTH_DirX[] = { 0,1,0,-1};
/*0DC4*/char PTH_DirY[] = {-1,0,1, 0};

/*creature types that take a square more to the west/north (2x2 body
 anchored at its south-east square)*/
/*C_130F_0002*/C_130F_0002(int16 objNum) {
	int16 objTyp;

	objTyp = GetType(objNum);
	return
		objTyp == 0x11b || objTyp == 0x12c || objTyp == 0x147 ||
		objTyp == 0x14a || objTyp == 0x14b || objTyp == 0x14c ||
		objTyp == 0x14d || objTyp == 0x14e || objTyp == 0x14f
	;
}

/*C_130F_004D*/C_130F_004D(int16 objNum) {
	int16 objTyp;

	objTyp = GetType(objNum);
	return objTyp == 0x120;
}

/*C_130F_0070*/C_130F_0070(int16 objNum) {
	int16 objTyp;

	objTyp = GetType(objNum);
	return
		objTyp == 0x11c || objTyp == 0x121 || objTyp == 0x129 ||
		objTyp == 0x12f || objTyp == 0x131
	;
}

/*C_130F_00A7*/C_130F_00A7(int16 objNum) {
	int16 objTyp;

	objTyp = GetType(objNum);
	return objTyp == 0x128;
}

/*C_130F_00CA*/C_130F_00CA(int16 objNum) {
	int16 objTyp;

	objTyp = GetType(objNum);
	return objTyp == 0x126;
}

/*is objTyp a single-square creature?*/
/*C_130F_00ED*/C_130F_00ED(int16 objTyp) {
	return
		(objTyp >= 0x133 && objTyp <= 0x146) || objTyp >= 0x150 ||
		objTyp == 0x12d || objTyp == 0x12e || objTyp == 0x12b ||
		objTyp == 0x123 || objTyp == 0x11a || objTyp == 0x124
	;
}

/*C_130F_0136*/C_130F_0136(int16 objNum) {
	return C_130F_00ED(GetType(objNum));
}

/*is there a tile of the C_1DBD_0A29 kind under the 2x2 body of objNum
 (its square and the 3 squares to the west/north)?*/
/*C_130F_014E*/C_130F_014E(int16 objNum) {
	int16 x, y;
	int16 i, j;

	x = GetX(objNum);
	y = GetY(objNum);
	for(i = 0; i < 2; i ++) {
		for(j = 0; j < 2; j ++) {
			if(C_1DBD_0A29(GetTileAtXYZ(x - i, y - j, MapZ)))
				return 1;
		}
	}
	return 0;
}

/*can the 2x2 body of objNum move by (delta_x, delta_y)? 0 if one of its
 squares is on a C_1DBD_0A29 tile and so is the square it would move to*/
/*C_130F_01C6*/C_130F_01C6(int16 objNum, int16 delta_x, int16 delta_y) {
	int16 x, y;
	int16 i, j;

	x = GetX(objNum);
	y = GetY(objNum);
	for(i = 0; i < 2; i ++) {
		for(j = 0; j < 2; j ++) {
			if(
				C_1DBD_0A29(GetTileAtXYZ(x - i, y - j, MapZ)) &&
				C_1DBD_0A29(GetTileAtXYZ(x + delta_x - i, y + delta_y - j, MapZ))
			) return 0;
		}
	}
	return 1;
}

/*can actor objNum step onto (x, y, MapZ)?
 Checks the base tile terrain and every object on the destination, taking
 into account how the creature moves (walks, flies, swims, ethereal, boats)
 and, for the big (2x2) creatures, the squares their body covers and the
 edge flags of the tiles (TerrainType 0x10 W, 0x20 S, 0x40 E, 0x80 N).
 Returns 1 if passable. Side effect: HazardFound = 1 if the tile is
 damaging; with AvoidHazards set such tiles are refused.*/
/*C_130F_0265*/CanMoveTo(int16 objNum, int16 x, int16 y) {
	int16 otherType;
	int16 terrain;
	int16 tileTerrain;
	int16 objTyp;
	int16 canLeave;
	int16 flies;
	int16 swims;
	int16 walks;
	int16 ethereal;
	int16 objCls;
	int16 otherTile;
	int16 keepFind;
	int16 isBoat;
	int16 leaderObj;
	int16 is2x2;
	int16 ownTerrain;
	int16 ownSideTerrain;
	int16 east, west, north, south;
	int16 destSideTerrain;
	int16 ownSide_x, ownSide_y, destSide_x, destSide_y;
	int16 dir_x, dir_y;
	int16 isBig;
	int16 isSmall;
	int16 otherObj;
	int16 canEnter;

	canLeave = 0;
	ownSideTerrain = -1;
	destSideTerrain = -1;
	canEnter = 0;
	isBig = 0;
	HazardFound = 0;
	if(Party[Active] == objNum && SpeakGargish == 1)
		return 1;
	objTyp = GetType(objNum);
	is2x2 = ((TileFlag[TILE_FRAME(objNum)] & 0xc0) == 0xc0);
	leaderObj = Party[Active];
	objCls = GetMonsterClass(objTyp);
	flies = MonsterFlag(objCls) & 0x100;
	isBoat = (objTyp == 0xe9 || objTyp == 0xed);
	swims = ((MonsterFlag(objCls) & 0x200) && !isBoat);
	ethereal = MonsterFlag(objCls) & 0x400;
	walks = (!swims && !flies && !isBoat && !ethereal);
	if(!IsNPC(objNum) && !isBoat) {
		flies = 0;
		isBoat = 0;
		swims = 0;
		ethereal = 0;
		walks = 1;
	}
	isSmall = C_130F_0136(objNum);
	if(GetType(objNum) == 0x148 || objNum == 0x19 || objNum == 0x23)
		return 0;
	if(GetZ(objNum) == MapZ && GetCoordUse(objNum) == 0) {
		if(!MoveFromSet) {
			MoveFromX = GetX(objNum);
			MoveFromY = GetY(objNum);
		}
		east = (MoveFromX < x);
		west = (MoveFromX > x);
		north = (MoveFromY > y);
		south = (MoveFromY < y);
		/*only small creatures move diagonally*/
		if(!isSmall && objTyp != 0x149 && ((north & east) || (north & west) || (south & east) || (south & west)))
			return 0;
		if(C_130F_00CA(objNum)) {
			isBig = 1;
			if(east) {
				MoveFromX ++;
				x ++;
				ownSide_x = MoveFromX;
				ownSide_y = MoveFromY - 1;
				destSide_x = x;
				destSide_y = y - 1;
			}
			if(west) {
				MoveFromX -= 2;
				x -= 2;
				ownSide_x = MoveFromX;
				ownSide_y = MoveFromY - 1;
				destSide_x = x;
				destSide_y = y - 1;
			}
			if(north) {
				MoveFromY -= 2;
				y -= 2;
				ownSide_x = MoveFromX - 1;
				ownSide_y = MoveFromY;
				destSide_x = x - 1;
				destSide_y = y;
			}
			if(south) {
				MoveFromY ++;
				y ++;
				ownSide_x = MoveFromX - 1;
				ownSide_y = MoveFromY;
				destSide_x = x - 1;
				destSide_y = y;
			}
		} else if(C_130F_00A7(objNum) || isBoat) {
			isBig = 1;
			if(east) {
				ownSide_x = MoveFromX;
				ownSide_y = MoveFromY - 1;
				destSide_x = x;
				destSide_y = y - 1;
			}
			if(west) {
				MoveFromX --;
				x --;
				ownSide_x = MoveFromX;
				ownSide_y = MoveFromY - 1;
				destSide_x = x;
				destSide_y = y - 1;
			}
			if(north) {
				MoveFromY --;
				y --;
				ownSide_x = MoveFromX - 1;
				ownSide_y = MoveFromY;
				destSide_x = x - 1;
				destSide_y = y;
			}
			if(south) {
				ownSide_x = MoveFromX - 1;
				ownSide_y = MoveFromY;
				destSide_x = x - 1;
				destSide_y = y;
			}
		} else if(C_130F_0070(objNum)) {
			if(east) {
				MoveFromX ++;
				x ++;
			}
			if(west) {
				MoveFromX --;
				x --;
			}
			if(north) {
				MoveFromY --;
				y --;
			}
			if(south) {
				MoveFromY ++;
				y ++;
			}
		} else if(C_130F_0002(objNum)) {
			if(west) {
				MoveFromX --;
				x --;
			}
			if(north) {
				MoveFromY --;
				y --;
			}
		} else if(C_130F_004D(objNum)) {
			isBig = 1;
			if(east || west) {
				ownSide_x = MoveFromX;
				ownSide_y = MoveFromY - 1;
				destSide_x = x;
				destSide_y = y - 1;
			}
			if(north || south) {
				ownSide_x = MoveFromX - 1;
				ownSide_y = MoveFromY;
				destSide_x = x - 1;
				destSide_y = y;
			}
		}
		if(isBig) {
			destSideTerrain = TerrainType[GetTileAtXYZ(destSide_x, destSide_y, MapZ)];
			ownSideTerrain = TerrainType[GetTileAtXYZ(ownSide_x, ownSide_y, MapZ)];
		}
		terrain = TerrainType[GetTileAtXYZ(x, y, MapZ)];
		ownTerrain = TerrainType[GetTileAtXYZ(MoveFromX, MoveFromY, MapZ)];
	} else {
		MoveFromX = MoveFromY = -1;
	}
	/*-- walls --*/
	if(!ethereal && !flies && isSmall) {
		if((terrain & 4) && (terrain & 2))
			return 0;
	} else if(flies || !isSmall) {
		if((terrain & 4) || (destSideTerrain != -1 && (destSideTerrain & 4)))
			return 0;
	}
	/*-- base tiles --*/
	if(walks && MoveFromX != -1 && !(ownTerrain & 1)) {
		if(!(terrain & 2) && !(ownTerrain & 2) && !isBig) {
			canLeave = canEnter = 1;
		} else if(isBig) {
			if(!(terrain & 2) && !(ownTerrain & 2) && !(destSideTerrain & 2) && !(ownSideTerrain & 2))
				canLeave = canEnter = 1;
		} else {
			/*an impassable tile may be crossed on its open edges*/
			canLeave = canEnter = 1;
			if(terrain & 2) {
				if((west && !(terrain & 0x40)) || (east && !(terrain & 0x10)))
					canEnter = 0;
				if((north && !(terrain & 0x20)) || (south && !(terrain & 0x80)))
					canEnter = 0;
			}
			if(ownTerrain & 2) {
				if((west && !(ownTerrain & 0x10)) || (east && !(ownTerrain & 0x40)))
					canLeave = 0;
				if((north && !(ownTerrain & 0x80)) || (south && !(ownTerrain & 0x20)))
					canLeave = 0;
			}
			if(destSideTerrain != -1 && (destSideTerrain & 2)) {
				if((west && !(destSideTerrain & 0x40)) || (east && !(destSideTerrain & 0x10)))
					canEnter = 0;
				if((north && !(destSideTerrain & 0x20)) || (south && !(destSideTerrain & 0x80)))
					canEnter = 0;
			}
			if(ownSideTerrain != -1 && (ownSideTerrain & 2)) {
				if((west && !(ownSideTerrain & 0x10)) || (east && !(ownSideTerrain & 0x40)))
					canLeave = 0;
				if((north && !(ownSideTerrain & 0x80)) || (south && !(ownSideTerrain & 0x20)))
					canLeave = 0;
			}
		}
		if((north & east) || (north & west) || (south & east) || (south & west)) {
			if((ownTerrain & 2) || (terrain & 2))
				canLeave = canEnter = 0;
		}
	} else if(walks) {
		canLeave = 1;
		if(
			isSmall && objTyp != 0x149 &&
			((north & east) || (north & west) || (south & east) || (south & west)) &&
			!(ownTerrain & 2) && !(terrain & 2)
		) canEnter = 1;
		if(destSideTerrain == -1 && !(terrain & 2))
			canEnter = 1;
		else if(!(terrain & 2) && !(destSideTerrain & 2))
			canEnter = 1;
	} else if(swims) {
		canLeave = 1;
		if(destSideTerrain == -1 && (terrain & 2) && (terrain & 1))
			canEnter = 1;
		else if((terrain & 2) && (terrain & 1) && (destSideTerrain & 2) && (destSideTerrain & 1))
			canEnter = 1;
	} else if(ethereal || flies) {
		return 1;
	} else if(isBoat) {
		dir_x = east?1:(west?-1:0);
		dir_y = south?1:(north?-1:0);
		canLeave = 1;
		if(C_130F_01C6(objNum, dir_x, dir_y) && (terrain & 1) && (destSideTerrain & 1))
			canEnter = 1;
	}
	if((terrain & 8) || (destSideTerrain != -1 && (destSideTerrain & 8)))
		HazardFound = 1;
	/*-- objects on the square left --*/
	keepFind = 1;
	for(otherObj = FindLoc(MoveFromX, MoveFromY, MapZ); otherObj >= 0; otherObj = NextLoc()) {
		if(otherObj == objNum)
			continue;
		otherTile = TILE_FRAME(otherObj) - LocPartOffset;
		if(TileFlag2[otherTile] & 0x10)
			continue;
		tileTerrain = TerrainType[otherTile];
		if(walks && isSmall && (TileFlag2[otherTile] & 4)) {
			canLeave = 1;
			if(tileTerrain & 2) {
				if((west && !(tileTerrain & 0x10)) || (east && !(tileTerrain & 0x40)))
					keepFind = canLeave = 0;
				if((north && !(tileTerrain & 0x80)) || (south && !(tileTerrain & 0x20)))
					keepFind = canLeave = 0;
			}
			if(!(TileFlag2[otherTile] & 0x10))
				break;
		}
		if(!keepFind)
			break;
	}
	/*-- objects on the destination --*/
	if(keepFind) {
		for(otherObj = FindLoc(x, y, MapZ); otherObj >= 0; otherObj = NextLoc()) {
			if(otherObj == objNum)
				continue;
			otherType = GetType(otherObj);
			otherTile = TILE_FRAME(otherObj) - LocPartOffset;
			if(TileFlag2[otherTile] & 0x10)
				continue;
			tileTerrain = TerrainType[otherTile];
			if(!ethereal && !flies && isSmall) {
				if((tileTerrain & 4) && (tileTerrain & 2))
					keepFind = canEnter = 0;
			} else if(flies || !isSmall) {
				if(tileTerrain & 4)
					keepFind = canEnter = 0;
			}
			if(walks && isSmall && (TileFlag2[otherTile] & 4)) {
				/*bridge-like object*/
				canEnter = 1;
				if(tileTerrain & 2) {
					if((west && !(tileTerrain & 0x40)) || (east && !(tileTerrain & 0x10)))
						keepFind = canEnter = 0;
					if((north && !(tileTerrain & 0x20)) || (south && !(tileTerrain & 0x80)))
						keepFind = canEnter = 0;
				}
				if(canEnter == 1)
					HazardFound = 0;
				if(!(TileFlag2[otherTile] & 0x10) && otherType != 0xa3)
					break;
			} else if((tileTerrain & 8) || (objTyp != 0x11a && otherType == 0x124)) {
				HazardFound = 1;
				if(AvoidHazards)
					return 0;
			}
			/*c_0e1d = "this object does not block by its terrain"*/
			if(walks) {
				if(!(tileTerrain & 2))
					goto c_0e1d;
			}
			if(swims) {
				if(otherTile == 0xc)
					goto c_0e1d;
				if(IsTileIg(otherTile))
					goto c_0e1d;
			}
			if(isBoat) {
				if(tileTerrain & 1)
					goto c_0e1d;
				if(IsTileIg(otherTile))
					goto c_0e1d;
			}
			if(flies) {
				if(!(tileTerrain & 4)) {
					if(!IsNPC(otherObj))
						goto c_0e1d;
					if(!(MonsterFlag(GetMonsterClass(otherType)) & 0x100))
						goto c_0e1d;
				}
			}
			if(ethereal) {
				if(!IsNPC(otherObj))
					goto c_0e1d;
			}
			if(IsPlrControl(objNum)) {
				/*party members may swap with an able party member (not the leader)*/
				if(
					otherObj != leaderObj &&
					PassThroughParty &&
					IsNPC(otherObj) &&
					!!IsPlrControl(otherObj) &&
					!(IsIncapacitated(otherObj) || IsDraggedUnder(otherObj))
				)
					continue;
				keepFind = canEnter = 0;
			} else {
				keepFind = canEnter = 0;
			}
c_0e1d:
			/*an actor (or an object of the C_0A5D_0DEF kind) blocks*/
			if((IsNPC(otherObj) || C_0A5D_0DEF(GetType(otherObj))) && otherObj) {
				keepFind = canEnter = 0;
				break;
			}
			if(!keepFind)
				break;
		}
	}
	/*-- objects on the side square of a big creature --*/
	if(keepFind && isBig) {
		for(otherObj = FindLoc(destSide_x, destSide_y, MapZ); otherObj >= 0; otherObj = NextLoc()) {
			if(otherObj == objNum)
				continue;
			otherType = GetType(otherObj);
			otherTile = TILE_FRAME(otherObj) - LocPartOffset;
			tileTerrain = TerrainType[otherTile];
			if(TileFlag2[otherTile] & 0x10)
				continue;
			if(tileTerrain & 4) {
				keepFind = canEnter = 0;
			} else if((tileTerrain & 8) || (objTyp != 0x11a && otherType == 0x124)) {
				HazardFound = 1;
				if(AvoidHazards)
					return 0;
			}
			if(walks) {
				if(!(tileTerrain & 2))
					goto c_0faa;
			}
			if(swims) {
				if(otherTile == 0xc)
					goto c_0faa;
				if(IsTileIg(otherTile))
					goto c_0faa;
			}
			if(isBoat) {
				if(tileTerrain & 1)
					goto c_0faa;
				if(IsTileIg(otherTile))
					goto c_0faa;
			}
			if(flies) {
				if(!(tileTerrain & 4)) {
					if(!IsNPC(otherObj))
						goto c_0faa;
					if(!(MonsterFlag(GetMonsterClass(otherType)) & 0x100))
						goto c_0faa;
				}
			}
			if(ethereal) {
				if(!IsNPC(otherObj))
					goto c_0faa;
			}
			keepFind = canEnter = 0;
c_0faa:
			if((IsNPC(otherObj) || C_0A5D_0DEF(GetType(otherObj))) && otherObj) {
				keepFind = canEnter = 0;
				break;
			}
			if(!keepFind)
				break;
		}
	}

	return canLeave && canEnter;
}

/*is the object outside of the 40x40 loaded area (or on another level)?*/
/*C_130F_1003*/IsOutOfArea(int16 objNum) {
	return !(
		GetZ(objNum) == MapZ &&
		((GetX(objNum) - AreaX) & MapCoordMask) <= (AREA_W-1) &&
		((GetY(objNum) - AreaY) & MapCoordMask) <= (AREA_H-1)
	);
}

/*is (x, y, MapZ) blocked for creature objNum (a big creature turning)?
 1 if the base tile or an object there blocks it, or another actor is there*/
/*C_130F_106D*/IsBlockedAt(int16 x, int16 y, int16 objNum) {
	int16 obj, tile;

	tile = GetTileAtXYZ(x, y, MapZ);
	if(GetType(objNum) == 0x120) {
		if(TerrainType[tile] & 4)
			return 1;
	} else if(GetType(objNum) == 0x12c) {
		if(!(TerrainType[tile] & 1))
			return 1;
	} else if((TerrainType[tile] & 2) || (TerrainType[tile] & 4)) {
		return 1;
	}
	for(obj = FindLoc(x, y, MapZ); obj >= 0; obj = NextLoc()) {
		tile = TILE_FRAME(obj) - LocPartOffset;
		if(IsNPC(obj) && obj != objNum)
			return 1;
		if(GetType(objNum) == 0x120) {
			if(TerrainType[tile] & 4)
				return 1;
		} else if(GetType(objNum) == 0x12c) {
			if(!(TerrainType[tile] & 1))
				return 1;
		} else if((TerrainType[tile] & 2) && GetQual(obj) != objNum) {
			return 1;
		}
	}
	return 0;
}

/*C_130F_1199*/C_130F_1199(int16 objNum) {
	int16 objTyp;

	objTyp = GetType(objNum);
	return objTyp == 0x120 || objTyp == 0x121 || objTyp == 0x126;
}

/*C_130F_11C6*/C_130F_11C6(int16 objNum) {
	int16 objTyp;

	objTyp = GetType(objNum);
	return (objTyp >= 0x136 && objTyp <= 0x146) || objTyp == 0x12d;
}

/*turn the 4-way facing "frame" (0 N,1 E,2 S,3 W) toward the 8-way "dir":
 straight dirs set it directly; for a diagonal the facing is kept if it is
 one of the two components, otherwise it is reversed*/
#define FACE_TOWARD(dir,frame) {                        \
	if(!((dir) & 1))                                \
		(frame) = (dir) >> 1;                       \
	else if(((((dir) >> 1) - (frame) + 1) & 3) > 1) \
		(frame) = ((frame) + 2) & 3;                \
}

/*make objNum face the 8-way direction dir and update its shape frame
 (step animation, alternating feet with the NPCFlag "walking" bit).
 Creatures made of several objects (C_0A5D_0D5B) turn their other parts
 too; the 2x2 creatures may refuse to turn when the squares they would
 sweep are blocked (returns 0, else 1). Always clears StepMovesParts
 (except when refusing).*/
/*C_130F_11F4*/UpdateMoveFrame(int16 objNum, int16 dir) {
	int16 objTyp, objFrm, dirFram;
	int16 tmpObj, isOnChair, partTerrain;
	int16 cur_x, cur_y, frm, terrain, partDir, turn;
	int16 shifts[3], types[3];

	isOnChair = 0;
	objTyp = GetType(objNum);
	terrain = TerrainType[TILE_FRAME(objNum)];
	objFrm = GetFrame(objNum);
	if((objTyp >= 0x136 && objTyp <= 0x146) || objTyp == 0x12d) {
		/*humanoids: 4 frames per facing (1 standing, 0/2 steps, 3 sitting)*/
		if(isOnChair) {
			objFrm = 3;
			dirFram = 2;
		} else {
			dirFram = objFrm >> 2;
			FACE_TOWARD(dir, dirFram);
			switch(objFrm & 3) {
				case 0: ClrWalking(objNum); objFrm = 1; break;
				case 1: objFrm = IsWalking(objNum)?0:2; break;
				case 2: SetWalking(objNum); objFrm = 1; break;
				default: objFrm = 1;
			}
		}
		objFrm = objFrm + (dirFram << 2);
		SetFrame(objNum, objFrm);
	} else if(objTyp == 0x148) {
		/*from time to time, a random frame 4..7 for the first object of
		this type (>= 0xff) on its square*/
		if(objFrm >= 4 && !OSI_rand(0, 2) && !IsOutOfArea(objNum)) {
			for(tmpObj = FindLoc(GetX(objNum), GetY(objNum), GetZ(objNum)); tmpObj >= 0xff; tmpObj = NextLoc()) {
				if(GetType(tmpObj) == 0x148)
					break;
			}
			if(tmpObj >= 0)
				SetFrame(tmpObj, OSI_rand(4, 7));
		}
	} else if(
		objTyp == 0x150 || objTyp == 0x151 || objTyp == 0x135 || objTyp == 0x134 ||
		objTyp == 0x133 || objTyp == 0x12e || objTyp == 0x12b
	) {
		/*2 frames per facing*/
		dirFram = objFrm >> 1;
		if(dir & 1) {
			cur_x = GetX(objNum);
			cur_y = GetY(objNum);
			if(IsBlockedAt(cur_x, cur_y - DirIncrY[dir], objNum))
				dir = (dir & 4) + 2;
			else if(IsBlockedAt(cur_x - DirIncrX[dir], cur_y, objNum))
				dir = (dir + 1) & 4;
		}
		FACE_TOWARD(dir, dirFram);
		if(!(objFrm & 1))
			objFrm = 1;
		else
			objFrm = 0;
		objFrm = (objFrm & 1) + (dirFram << 1);
		SetFrame(objNum, objFrm);
	} else if(
		objTyp == 0x14f || objTyp == 0x11b || objTyp == 0x12c || objTyp == 0x147 ||
		objTyp == 0x14a || objTyp == 0x14b || objTyp == 0x14c || objTyp == 0x14d ||
		objTyp == 0x14e || objTyp == 0x14f
	) {
		/*2x2 creatures, 4 frames per facing*/
		dirFram = objFrm >> 2;
		cur_x = GetX(objNum);
		cur_y = GetY(objNum);
		switch(dirFram << 1) {
			case 0: case 4:
				if((dir == 2 || dir == 6) && IsBlockedAt(cur_x - 1, cur_y, objNum))
					return 0;
			break;
			case 2: case 6:
				if((dir == 0 || dir == 4) && IsBlockedAt(cur_x, cur_y - 1, objNum))
					return 0;
			break;
		}
		if(dir & 1) {
			if(IsBlockedAt(cur_x, cur_y - DirIncrY[dir], objNum))
				dir = (dir & 4) + 2;
			else if(IsBlockedAt(cur_x - DirIncrX[dir], cur_y, objNum))
				dir = (dir + 1) & 4;
		}
		FACE_TOWARD(dir, dirFram);
		if(!(objFrm & 2))
			objFrm = 1;
		else
			objFrm = 0;
		objFrm = ((objFrm + (dirFram << 1) + 1) << 1) - 1;
		SetFrame(objNum, objFrm);
	} else if(objTyp == 0x128 || objTyp == 0xed) {
		/*16 frames per facing*/
		dirFram = objFrm >> 4;
		if(dir & 1) {
			cur_x = GetX(objNum);
			cur_y = GetY(objNum);
			if(IsBlockedAt(cur_x, cur_y - DirIncrY[dir], objNum))
				dir = (dir & 4) + 2;
			else if(IsBlockedAt(cur_x - DirIncrX[dir], cur_y, objNum))
				dir = (dir + 1) & 4;
		}
		FACE_TOWARD(dir, dirFram);
		if(!(objFrm & 4))
			objFrm = 1;
		else
			objFrm = 0;
		objFrm = ((((objFrm + (dirFram << 1) + 1) << 1) - 1) << 1) + 1;
		SetFrame(objNum, objFrm);
	} else if(objTyp == 0x120) {
		/*6 frames per facing (3 steps x 2)*/
		dirFram = (objFrm >> 1) / 3;
		cur_x = GetX(objNum);
		cur_y = GetY(objNum);
		switch(dirFram << 1) {
			case 0: case 4:
				if((dir == 2 || dir == 6) && IsBlockedAt(cur_x - 1, cur_y, objNum))
					return 0;
			break;
			case 2: case 6:
				if((dir == 0 || dir == 4) && IsBlockedAt(cur_x, cur_y - 1, objNum))
					return 0;
			break;
		}
		if(dir & 1) {
			cur_x = GetX(objNum);
			cur_y = GetY(objNum);
			if(IsBlockedAt(cur_x, cur_y - DirIncrY[dir], objNum))
				dir = (dir & 4) + 2;
			else if(IsBlockedAt(cur_x - DirIncrX[dir], cur_y, objNum))
				dir = (dir + 1) & 4;
		}
		FACE_TOWARD(dir, dirFram);
		frm = (objFrm >> 1) % 3;
		switch(frm) {
			case 0: ClrWalking(objNum); frm = 1; break;
			case 1: frm = IsWalking(objNum)?0:2; break;
			case 2: SetWalking(objNum); frm = 1; break;
		}
		frm = frm + (dirFram << 1) + dirFram + 1;
		SetFrame(objNum, (frm << 1) - 1);
	} else if(C_0A5D_0D5B(objTyp, shifts, types)) {
		/*creature made of several objects*/
		dir >>= 1;
		cur_x = GetX(objNum);
		cur_y = GetY(objNum);
		if(C_130F_1199(objNum)) {
			frm = (objFrm >> shifts[0]) % 3;
			dirFram = (objFrm >> shifts[0]) / 3;
			switch(frm) {
				case 0: ClrWalking(objNum); frm = 1; break;
				case 1: frm = IsWalking(objNum)?0:2; break;
				case 2: SetWalking(objNum); frm = 1; break;
			}
			frm = frm + (dir << 1) + dir + 1;
		} else {
			frm = (objFrm >> shifts[0]) % 2;
			dirFram = (objFrm >> shifts[0]) / 2;
			if(frm == 0)
				frm = 1;
			else
				frm = 0;
			frm = frm + (dir << 1) + 1;
		}
		/*turning: the squares swept must be free*/
		switch(dirFram << 1) {
			case 0: case 4:
				if((dir << 1) == 2 || (dir << 1) == 6) {
					if(objTyp == 0x126 && (
						IsBlockedAt(cur_x - 2, cur_y - 1, objNum) ||
						IsBlockedAt(cur_x + 1, cur_y - 1, objNum) ||
						IsBlockedAt(cur_x - 2, cur_y, objNum) ||
						IsBlockedAt(cur_x + 1, cur_y, objNum)
					)) return 0;
					if(objTyp != 0x126 && (
						IsBlockedAt(cur_x - 1, cur_y, objNum) ||
						IsBlockedAt(cur_x + 1, cur_y, objNum)
					)) return 0;
				}
			break;
			case 2: case 6:
				if((dir << 1) == 0 || (dir << 1) == 4) {
					if(objTyp == 0x126 && (
						IsBlockedAt(cur_x - 1, cur_y - 2, objNum) ||
						IsBlockedAt(cur_x - 1, cur_y + 1, objNum) ||
						IsBlockedAt(cur_x, cur_y - 2, objNum) ||
						IsBlockedAt(cur_x, cur_y + 1, objNum)
					)) return 0;
					if(objTyp != 0x126 && (
						IsBlockedAt(cur_x, cur_y - 1, objNum) ||
						IsBlockedAt(cur_x, cur_y + 1, objNum)
					)) return 0;
				}
			break;
		}
		SetFrame(objNum, (frm << shifts[0]) - 1);
		/*the other parts: next to the head on the side its tile's edge
		flags point to, with the matching edge flag back*/
		turn = abs(dirFram - dir);
		if(types[1] != -1) {
			for(partDir = 0; partDir < 8; partDir ++) {
				if(partDir & 1)
					continue;
				if(!(terrain & DirEdgeFlag[partDir]))
					continue;
				for(tmpObj = FindLoc(cur_x + DirIncrX[partDir], cur_y + DirIncrY[partDir], MapZ); tmpObj >= 0; tmpObj = NextLoc()) {
					partTerrain = TerrainType[TILE_FRAME(tmpObj)];
					if(!IsNPC(tmpObj) && GetType(tmpObj) == types[1] && GetQual(tmpObj) == objNum && (partTerrain & DirEdgeFlag[partDir ^ 4]))
						break;
				}
				if(tmpObj >= 0) {
					if(turn & 1)
						MoveObj(tmpObj, cur_x + DirIncrY[partDir], cur_y + DirIncrX[partDir], MapZ);
					SetFrame(tmpObj, (frm << shifts[1]) - 1);
					break;
				}
			}
		}
		if(types[2] != -1) {
			for(partDir = 0; partDir < 8; partDir ++) {
				if(partDir & 1)
					continue;
				if(!(terrain & DirEdgeFlag[partDir]))
					continue;
				for(tmpObj = FindLoc(cur_x + DirIncrX[partDir], cur_y + DirIncrY[partDir], MapZ); tmpObj >= 0; tmpObj = NextLoc()) {
					partTerrain = TerrainType[TILE_FRAME(tmpObj)];
					if(!IsNPC(tmpObj) && GetType(tmpObj) == types[2] && GetQual(tmpObj) == objNum && (partTerrain & DirEdgeFlag[partDir ^ 4]))
						break;
				}
				if(tmpObj >= 0) {
					if(turn & 1)
						MoveObj(tmpObj, cur_x + DirIncrY[partDir], cur_y + DirIncrX[partDir], MapZ);
					SetFrame(tmpObj, (frm << shifts[2]) - 1);
				}
			}
		}
	}
	StepMovesParts = 0;
	return 1;
}
#undef FACE_TOWARD

/*spend pts movement points (MovePts) for objNum: 2 less for a party
 member out of combat; at least 1*/
/*C_130F_1DCC*/SubMov(int16 objNum, int16 pts) {
	if(objNum < 0x10 && !InCombat)
		pts -= 2;
	if(pts < 1)
		pts = 1;
	MovePts[objNum] -= pts;
}

/*formation helper: is (x, y) next to one of the first "count" members
 already placed in FormationObjs[] (leader first), i.e. could that member
 step onto it? 0 if (x, y) is occupied by one of them*/
/*C_130F_1DFC*/IsNextToFormation(int16 count, int16 x, int16 y) {
	int16 placedObj, placedType, delta_x, delta_y;
	int16 k, isAdjacent;

	isAdjacent = 0;
	MoveFromSet = 1;
	for(k = 0; k < count; k ++) {
		placedObj = FormationObjs[k];
		placedType = GetType(placedObj);
		delta_x = GetX(placedObj) - x;
		delta_y = GetY(placedObj) - y;
		if(delta_x == 0 && delta_y == 0) {
			isAdjacent = 0;
			break;
		}
		if(delta_x > -2 && delta_x < 2 && delta_y > -2 && delta_y < 2) {
			MoveFromX = x;
			MoveFromY = y;
			if(CanMoveTo(placedObj, x + delta_x, y + delta_y))
				isAdjacent = 1;
		}
	}
	MoveFromSet = 0;
	return isAdjacent;
}

/*move the AI_FOLLOW party members one step after the leader moved.
 Member k aims for formation slot k (FormationSide/FormationBack, relative to the leader's
 facing). Pass 0 only moves members that lost contact with the chain;
 pass 1 also moves those behind their slot (or, with regroup set, every
 member not exactly on its slot). Each move picks among the 8 neighbours
 the one that keeps contact and is closest to the slot, avoiding damaging
 tiles. Followers pay 5 points.*/
/*C_130F_1EC7*/MoveFollowers(int16 leader, int16 regroup) {
	int16 follower, follow_pos;
	int16 partyIdx, try_dir, pass;
	int16 follower_x, follower_y;
	int16 try_x, try_y, eager, new_dir;
	int16 max_eager, savedMovePts, facing_dir, leaderDir, x, y, target_x, target_y;
	int16 leaderType, wasNextTo;

	if(IN_VEHICLE)
		return;
	FormationObjs[0] = leader;
	x = GetX(leader);
	y = GetY(leader);
	follow_pos = 1;
	leaderDir = GetDirection(leader);
	leaderType = GetType(leader);
	facing_dir = leaderDir & 6;
	for(pass = 0; pass < 2; pass ++) {
		follow_pos = 1;
		for(partyIdx = 0; partyIdx < PartySize; partyIdx ++) {
			FormationObjs[follow_pos] = follower = Party[partyIdx];
			/*formation slot = leader - facing * back + perpendicular * side*/
			target_x = x
				- DirIncrX[facing_dir] * FormationBack[follow_pos]
				- DirIncrY[facing_dir] * FormationSide[follow_pos]
			;
			target_y = y
				+ DirIncrX[facing_dir] * FormationSide[follow_pos]
				- DirIncrY[facing_dir] * FormationBack[follow_pos]
			;
			if(follower == leader)
				continue;
			if(GetZ(follower) != MapZ)
				continue;
			if(NPCMode[follower] != 1/*AI_FOLLOW*/)
				continue;
			if(IsIncapacitated(follower))
				continue;
			if(IsDraggedUnder(follower))
				continue;
			savedMovePts = MovePts[follower];
			follower_x = GetX(follower);
			follower_y = GetY(follower);
			if(
				!IsNextToFormation(follow_pos, follower_x, follower_y)
				/*on second pass*/
				|| (pass == 1 && (
					(regroup && !(target_x == follower_x && target_y == follower_y)) ||
					(!regroup && (target_x - follower_x) * DirIncrX[facing_dir] + (target_y - follower_y) * DirIncrY[facing_dir] > 0)
				))
			) {
				wasNextTo = IsNextToFormation(follow_pos, follower_x, follower_y);
				new_dir = -1;
				max_eager = 0;
				for(try_dir = 0; try_dir < 8; try_dir ++) {
					try_x = follower_x + DirIncrX[try_dir];
					try_y = follower_y + DirIncrY[try_dir];
					/*score: 256 (128 on a damaging tile, refused if already in
					contact), +256 if the square keeps contact with the chain, 0 if
					it would break an existing contact, minus the distance to the slot*/
					if(CanMoveTo(follower, try_x, try_y)) {
						if(HazardFound) {
							if(wasNextTo)
								continue;
							eager = 128;
						} else {
							eager = 256;
						}
						if(IsNextToFormation(follow_pos, try_x, try_y))
							eager += 256;
						else if(wasNextTo)
							eager = 0;
						eager -= abs(try_x - target_x);
						eager -= abs(try_y - target_y);
						if(eager > max_eager) {
							max_eager = eager;
							new_dir = try_dir;
						}
					}
				}
				if(new_dir >= 0) {
					StepMovesParts = 1;
					if(UpdateMoveFrame(follower, new_dir) == 1)
						MoveObj(follower, follower_x + DirIncrX[new_dir], follower_y + DirIncrY[new_dir], MapZ);
				}
			}
			/*following always costs 5 points whatever the terrain*/
			MovePts[follower] = savedMovePts;
			SubMov(Party[partyIdx], 5);
			follow_pos ++;
		}
	}
}

/*board the vehicle map object objNum (object type 0xed when boarded, 0xe9
 when left: the raft?). Actor 0 becomes the vehicle: it gets the vehicle's
 shape and its hit points (= object quantity); the party members are put
 into its inventory and Active = PartySize (see IN_VEHICLE).*/
/*C_130F_21CB*/Board(int16 objNum) {
	int16 i;
	int16 new_objNum, carriedBoat;

	carriedBoat = -1;
	if(objNum < 0)
		return;
	COMBAT_breakOff();
	MapX = GetX(objNum);
	MapY = GetY(objNum);
	PartyEnter(0);
	new_objNum = 0;
	CloneObject(Party[0], new_objNum);
	ClrLocal(new_objNum);
	ClrInvisible(new_objNum);
	MovePts[new_objNum] = DEXTE[new_objNum];
	MoveObj(new_objNum, MapX, MapY, MapZ);
	SetType(new_objNum, 0xed);
	SetFrame(new_objNum, 3);
	HitPoints[new_objNum] = GetQuan(objNum);
	SetAlignment(new_objNum, 0x40/*GOOD*/);
	SetPlrControl(new_objNum);
	NPCMode[new_objNum] = 2/*AI_COMMAND*/;
	StatusDisplay = 0x91;
	STAT_ShowParty(0);
	ObjShapeType[objNum] = TypeFrame(0x45, 0);
	DeleteObj(objNum);
	for(i = 0; i < PartySize; i ++)
		InsertObj(Party[i], new_objNum, 0x10/*INVEN*/);
	Active = PartySize;
	Party[Active] = new_objNum;
	for(i = 0; i < 14; i ++)
		Names[Active][i] = Names[0][i];
	ShowObjects();
	VisibilityDirty = 1;
	MUS_PlayAreaSong();
}

/*leave the vehicle (actor 0 = Party[PartySize]): put back a map object
 (type 0xe9, frame 3) with the vehicle's hit points as quantity, park
 actor 0 in limbo (z = 6) and let the party out. Returns the new map object.*/
/*C_130F_2343*/Unboard(int16 onLand) {
	int16 vehicle;
	int16 mapObj;

	vehicle = Party[PartySize];
	mapObj = AddMapObj(GetType(vehicle), 0, HitPoints[vehicle], MapX, MapY, MapZ);
	ClrLocal(mapObj);/*note: done before the mapObj >= 0 test*/
	if(mapObj >= 0) {
		SetType(mapObj, 0xe9);
		SetFrame(mapObj, 3);
	}
	ObjShapeType[vehicle] = TypeFrame(0x45, 0);
	NPCMode[vehicle] = 0/*AI_MOTIONLESS*/;
	MoveObj(vehicle, MapX, MapY, 6);
	Active = 0;
	PartyExit(MapX, MapY, MapZ, 0);
	MUS_PlayAreaSong();
	return mapObj;
}

/*after the party leader moved: take the entrance he stands on (object
 types 0xa3, 0xe3, 0x103, 0xa0: C_08CB_0855; type 0x44: C_08CB_0B8B), only
 in party mode. Then the sky bar and one minute pass.*/
/*C_130F_2404*/CheckGatesAndEntrances() {
	int16 objNum, objType;
	int16 needPartyMode;

	needPartyMode = 0;
	if(!IN_VEHICLE) {
		for(objNum = FindLoc(MapX, MapY, MapZ); objNum >= 0; objNum = NextLoc()) {
			if(Party[Active] == objNum)
				continue;
			objType = GetType(objNum);
			if(objType == 0xa3 || objType == 0xe3 || objType == 0x103 || objType == 0xa0) {
				if(SoloMember != -1)
					needPartyMode = 1;
				else
					C_08CB_0855(objNum);
				break;
			}
			if(objType == 0x44) {
				if(SoloMember != -1)
					needPartyMode = 1;
				else
					C_08CB_0B8B(objNum);
				break;
			}
		}
	}
	DrawSkyBar();
	PassTime(1);
	if(needPartyMode) {
		CON_printf(MustBePartyModeMsg);
		PromptFlag = 1;
	}
}

/*[advance] the party leader (or vehicle) steps in direction dir.
 Handles drunkenness, sliding along obstacles when moving diagonally with
 the mouse, swapping places with a party member standing in the way,
 movement costs (terrain; diagonal steps cost 5 more), then moves the
 followers and checks entrances. Sets AdvanceBlocked when the step was
 blocked.*/
/*C_130F_24C3*/PartyAdvance(int16 dir) {
	int16 obj, partyIdx, new_x, tmp_dir;
	int16 new_y, tileOrType;

	ShowQuestBarrierMsg = 1;
	obj = Party[Active];
	MapX = GetX(obj);
	MapY = GetY(obj);
	AdvanceBlocked = 0;
	if(DrunkCounter > 3 && OSI_rand(0, 1)) {
		CON_printf("Hic!\n");
		dir = OSI_rand(0, 7);
	}
	tmp_dir = dir;
	new_x = (MapX + DirIncrX[tmp_dir]) & 0x3ff;
	new_y = (MapY + DirIncrY[tmp_dir]) & 0x3ff;
	if(!CanMoveTo(obj, new_x, new_y)) {
		AdvanceBlocked = 1;
		/*diagonal mouse move blocked: slide along the main axis of the mouse*/
		if(MousePress && DirIncrX[tmp_dir] && DirIncrY[tmp_dir]) {
			if(abs(MouseMapX - MapX) > abs(MouseMapY - MapY)) {
				if(CanMoveTo(obj, new_x, MapY)) {
					AdvanceBlocked = 0;
					new_y = MapY;
				} else if(CanMoveTo(obj, MapX, new_y)) {
					AdvanceBlocked = 0;
					new_x = MapX;
				}
			} else {
				if(CanMoveTo(obj, MapX, new_y)) {
					AdvanceBlocked = 0;
					new_x = MapX;
				} else if(CanMoveTo(obj, new_x, MapY)) {
					AdvanceBlocked = 0;
					new_y = MapY;
				}
			}
		}
	}
	if(AdvanceBlocked) {
		MapX = GetX(obj);
		MapY = GetY(obj);
		ShowQuestBarrierMsg = 0;
		return;
	}
	/*a party member stands on the destination: it swaps places with the leader*/
	for(partyIdx = 0; partyIdx < PartySize; partyIdx ++) {
		if(Active != partyIdx) {
			obj = Party[partyIdx];
			if(
				!(NPCStatus[obj] & 0x10) &&
				GetX(obj) == new_x && GetY(obj) == new_y && GetZ(obj) == MapZ
			) {
				MoveObj(obj, MapX, MapY, MapZ);
				StepMovesParts = 1;
				UpdateMoveFrame(obj, (tmp_dir + 4) & 7);
				break;
			}
		}
	}
	/*movement cost; diagonal steps cost 5 more*/
	obj = Party[Active];
	tileOrType = GetType(obj);
	if(DirIncrX[tmp_dir] && DirIncrY[tmp_dir]) {
		SubTerrainMov(obj);
		SubMov(obj, 5);
	} else {
		SubTerrainMov(obj);
	}
	MapX = new_x;
	MapY = new_y;
	MAP_LoadAround();
	if(MoveObj(obj, new_x, new_y, MapZ) >= 0) {
		SetDirection(obj, tmp_dir);
		StepMovesParts = 1;
		if(UpdateMoveFrame(obj, tmp_dir) == 1)
			SetDirection(obj, tmp_dir);
		VisibilityDirty = 1;
	} else {
		MapX = GetX(obj);
		MapY = GetY(obj);
		MAP_LoadAround();
		CON_printf("Object mismatch!\n");
	}
	/*the followers don't trigger the barrier message*/
	ShowQuestBarrierMsg = 0;
	MoveFollowers(obj, 0);
	CheckGatesAndEntrances();
	ShowQuestBarrierMsg = 0;
}

/*look for an object on the 4 squares next to {x, y, z} (N, E, S, W).
 Returns 1 + direction (1, 3, 5, 7) of the first square holding one, 0 if
 none (Ultima VI looked for a plate)*/
/*C_130F_27FA*/FindAdjacentPlate(int16 x, int16 y, int16 z) {
	int16 dir, obj;

	for(dir = 0; dir < 8; dir += 2) {
		for(obj = FindLoc(x + DirIncrX[dir], y + DirIncrY[dir], z); obj >= 0; obj = NextLoc());
		if(obj >= 0)
			break;
	}
	if(dir < 8)
		return dir + 1;
	return 0;
}

/*what is left of Ultima VI's FindFurnitureHere: the search loop has lost
 its body (it never ends if there is an object on objNum's square)*/
/*C_130F_284B*/FindFurnitureHere(int16 objNum, int16 kind) {
	int16 obj, wantedType;
	int16 x, y, z;

	x = GetX(objNum);
	y = GetY(objNum);
	z = GetZ(objNum);
	wantedType = FurnitureTypes[kind];
	for(obj = FindLoc(x, y, z); obj >= 0; );
	return obj;
}

/*start the current schedule entry's action: NPCMode = action. NPC 0x23
 stays motionless; an NPC waiting (D_3298 0x80) out of the party gets
 mode 0x92. Sleeping (0x91): the NPC is set asleep (and the object of
 type 0xf6 on its square gets frame 1); while it is out of the loaded
 area NPCMode is left at AI_SCHEDULE (0x80). Otherwise it wakes up (back
 to its own shape, the 0xf6 object to frame 0). If the NPC is not on
 its scheduled square NPCMode becomes AI_FINDPATH (0x81).*/
/*C_130F_28BD*/StartScheduleAction(int16 objNum) {
	int16 obj;
	struct coord xyz;
	int16 action, isAtDest;

	action = Schedule[SchedIndex[objNum] + SchedPointer[objNum]].action;
	xyz = Schedule[SchedIndex[objNum] + SchedPointer[objNum]].xyz;
	isAtDest = (
		ObjPos[objNum].raw[0] == xyz.raw[0] &&
		ObjPos[objNum].raw[1] == xyz.raw[1] &&
		ObjPos[objNum].raw[2] == xyz.raw[2]
	);
	NPCMode[objNum] = action;
	if(objNum == 0x23) {
		isAtDest = 1;
		NPCMode[objNum] = 0/*AI_MOTIONLESS*/;
	} else if((D_3298[objNum] & 0x80) && !(NPCStatus[objNum] & 0x80)) {
		isAtDest = 1;
		NPCMode[objNum] = 0x92;
	} else if(action == 0x91/*AI_SLEEP*/) {
		isAtDest = 1;
		SetAsleep(objNum);
		if(!IsOutOfArea(objNum)) {
			SetTypeUnconscious(objNum);
			if(OBJ_IsLocLoaded(GetX(objNum), GetY(objNum), GetZ(objNum))) {
				for(obj = FindLoc(GetX(objNum), GetY(objNum), GetZ(objNum)); obj >= 0; obj = NextLoc()) {
					if(GetType(obj) == 0xf6) {
						SetFrame(obj, 1);
						break;
					}
				}
			}
		} else {
			NPCMode[objNum] = 0x80/*AI_SCHEDULE*/;
		}
	} else {
		ClrAsleep(objNum);
		ObjShapeType[objNum] = OrigShapeType[objNum];
		if(OBJ_IsLocLoaded(GetX(objNum), GetY(objNum), GetZ(objNum))) {
			for(obj = FindLoc(GetX(objNum), GetY(objNum), GetZ(objNum)); obj >= 0; obj = NextLoc()) {
				if(GetType(obj) == 0xf6) {
					SetFrame(obj, 0);
					MoveObj(objNum, GetX(objNum), GetY(objNum), GetZ(objNum));
					break;
				}
			}
		}
	}
	if(!isAtDest)
		NPCMode[objNum] = 0x81/*AI_FINDPATH*/;
}

/*====---- path finding ----====
 The search works on the 40x40 area whose corner is
 ((MapX - 0x10) & 0x3f8, (MapY - 0x10) & 0x3f8) and uses ScratchBuf as:
  PTH_map[y][x]   : 0xff = not reached, else cost (0..0x7f) | front flag
                    (0x00 = front grown from the NPC, 0x80 = from the goal);
  PTH_rank[cost]  : bucket queue, head node of the list of cells at that cost;
  PTH_link[node]  : next node in a bucket, or in the free list (PTH_Top);
  PTH_x/PTH_y     : cell of each node;
  PTH_resist[y][x]: cost of entering a cell (0xff = impassable).
 It is a uniform-cost (Dijkstra) search with buckets, run from both ends at
 once when the goal is a square; it stops when the fronts meet, or when the
 area edge in direction PTH_direct is reached (goal outside the area).
 A found path is stored in path slot PTH_Index of PathData (8 slots of 32
 bytes): each byte = direction (bits 0-1: 0 N, 1 E, 2 S, 3 W) + 4 * number
 of steps in that direction; 0 ends the path.*/

/*build the path once the search succeeded at cell (area_x, area_y):
 first walk down the costs back to the cost-0 start cell, prepending the
 reversed steps; then (two-ended search only) walk from the meeting cell
 down the goal front to the goal, appending the steps. Path longer than 31
 entries are truncated. The result goes to PathData->content[PTH_Index].*/
/*C_130F_2BBD*/PTH_BuildPath(int16 area_x, int16 area_y) {
	int16 i, pathLen;
	int16 some_min, dir, dir_min, back_x, back_y, new_x, new_y;

	back_x = area_x;
	back_y = area_y;

	pathLen =
	dir = 0;
	PathData->content[PTH_Index][0] = 0;

	while(_ScratchBuf->PTH_map[area_y][area_x]) {
		some_min = 0xff;
		for(i = 0; i < 4; i ++) {
			new_x = area_x + PTH_DirX[i];
			new_y = area_y + PTH_DirY[i];
			if(new_x >= 0 && new_x <= (AREA_W-1) && new_y >= 0 && new_y <= (AREA_H-1)) {
				if(
					_ScratchBuf->PTH_map[new_y][new_x] < some_min ||
					(_ScratchBuf->PTH_map[new_y][new_x] == some_min && dir == i)
				) {
					some_min = _ScratchBuf->PTH_map[new_y][new_x];
					dir_min = i;
				}
			}
		}
		dir = dir_min;
		area_x += PTH_DirX[dir];
		area_y += PTH_DirY[dir];

		/*new direction: shift the path and insert at the front; same
		direction: one more step in the first entry*/
		if(pathLen == 0 || (PathData->content[PTH_Index][0] & 3) != (dir ^ 2)) {
			if(pathLen < 0x1f)
				pathLen ++;
			for(i = pathLen; i > 0; i --)
				PathData->content[PTH_Index][i] = PathData->content[PTH_Index][i - 1];
			PathData->content[PTH_Index][0] = (dir ^ 2) + 4;
		} else {
			PathData->content[PTH_Index][0] += 4;
		}
	}

	if(PTH_object < 0 && PTH_direct < 0) {
		area_x = back_x;
		area_y = back_y;

		_ScratchBuf->PTH_map[area_y][area_x] = 0xff;

		/*follow the goal front (values >= 0x80) down to the goal (0x80)*/
		while(_ScratchBuf->PTH_map[area_y][area_x] > 0x80) {
			some_min = 0xff;
			for(i = 0; i < 4; i ++) {
				new_x = area_x + PTH_DirX[i];
				new_y = area_y + PTH_DirY[i];
				if(new_x >= 0 && new_x <= (AREA_W-1) && new_y >= 0 && new_y <= (AREA_H-1)) {
					/*only through the goal front*/
					if(_ScratchBuf->PTH_map[area_y + PTH_DirY[i]][area_x + PTH_DirX[i]] < 0x80)
						continue;
					if(
						_ScratchBuf->PTH_map[new_y][new_x] < some_min ||
						(_ScratchBuf->PTH_map[new_y][new_x] == some_min && dir == i)
					) {
						some_min = _ScratchBuf->PTH_map[new_y][new_x];
						dir_min = i;
					}
				}
			}
			dir = dir_min;
			area_x += PTH_DirX[dir];
			area_y += PTH_DirY[dir];
			if(pathLen >= 0x1f)
				break;
			if(pathLen == 0 || (PathData->content[PTH_Index][pathLen - 1] & 3) != dir) {
				PathData->content[PTH_Index][pathLen] = dir + 4;
				pathLen ++;
			} else {
				PathData->content[PTH_Index][pathLen - 1] += 4;
			}
		}
	}
	PathData->content[PTH_Index][pathLen] = 0;
}

/*teleport an NPC directly to its scheduled location and start the action,
 unless it or the destination is within 5 squares of the player (on his
 level) - always done when AllowNPCTeleport is set. Frees its path slot.*/
/*C_130F_2F03*/TeleportToSchedule(int16 objNum) {
	struct coord sched_xyz;
	int16 x, y, z;

	x = GetX(objNum);
	y = GetY(objNum);
	z = GetZ(objNum);
	if(
		!AllowNPCTeleport &&
		z == MapZ &&
		x >= MapX - 5 && x <= MapX + 5 &&
		y >= MapY - 5 && y <= MapY + 5
	) return;
	sched_xyz = Schedule[SchedIndex[objNum] + SchedPointer[objNum]].xyz;
	x = GetCoordX(sched_xyz);
	y = GetCoordY(sched_xyz);
	/*note: tests the NPC's level, not the destination's*/
	if(
		!AllowNPCTeleport &&
		z == MapZ &&
		x >= MapX - 5 && x <= MapX + 5 &&
		y >= MapY - 5 && y <= MapY + 5
	) return;
	if(NPCMode[objNum] >= 0x83/*AI_ONPATH*/ && NPCMode[objNum] < 0x86/*AI_PATH_FAILED*/)
		PathObject[Leader[objNum]] = 0;
	/*MoveAppliesTerrain = 0: no terrain-damage check in MoveObj*/
	MoveAppliesTerrain = 0;
	MoveObj(objNum, x, y, GetCoordZ(sched_xyz));
	MoveAppliesTerrain = 1;
	StartScheduleAction(objNum);
	SubMov(objNum, 10);
	TeleportCount ++;
}

/*path search: reach cell (x, y) with total cost "resist" from the front
 flag_80 (0 or 0x80). Detects success (other front met, area edge reached
 in direction PTH_direct); otherwise queues the cell in bucket
 PTH_rank[resist] if this is cheaper than what it had. (The search for an
 object type of Ultima VI is gone: only its FindLoc call is left.)*/
/*C_130F_3060*/PTH_Visit(int16 resist, int16 x, int16 y, int16 flag_80) {
	int16 obj, newTop;
	int16 origin_x, origin_y;

	if(
		PTH_Found ||
		resist > 0x7f ||
		!(x >= 0 && x <= (AREA_W-1) && y >= 0 && y <= (AREA_H-1))
	) return;

	/*reached a cell of the other front: the two searches met*/
	if(_ScratchBuf->PTH_map[y][x] != 0xff && (_ScratchBuf->PTH_map[y][x] & 0x80) != flag_80) {
		PTH_Found = 1;
		PTH_BuildPath(x, y);
		return;
	}

	if(PTH_object >= 0) {
		origin_x = (MapX - 0x10) & 0x3f8;
		origin_y = (MapY - 0x10) & 0x3f8;
		obj = FindLoc(x + origin_x, y + origin_y, MapZ);
	}

	/*goal outside the area: reached the edge on its side?*/
	if(PTH_direct >= 0) {
		if(
			(x == 0 && PTH_direct == 3) || (x == (AREA_W-1) && PTH_direct == 1) ||
			(y == 0 && PTH_direct == 0) || (y == (AREA_H-1) && PTH_direct == 2)
		) {
			PTH_Found = 1;
			PTH_AnX = x;
			PTH_AnY = y;
			PTH_BuildPath(x, y);
			return;
		}
	}
	/*already reached at a lower or equal cost (0xff & 0x7f = 0x7f = never)*/
	if((_ScratchBuf->PTH_map[y][x] & 0x7f) <= resist)
		return;
	/*take a node from the free list (none left: give up on this cell)*/
	newTop = PTH_Top;
	if(newTop < 0)
		return;
	/*PTH_MaxCost is in fact the highest cost queued so far*/
	if(PTH_MaxCost < resist)
		PTH_MaxCost = resist;
	PTH_Top = _ScratchBuf->PTH_link[newTop];
	_ScratchBuf->PTH_link[newTop] = _ScratchBuf->PTH_rank[resist];
	_ScratchBuf->PTH_x[newTop] = x;
	_ScratchBuf->PTH_y[newTop] = y;
	_ScratchBuf->PTH_rank[resist] = newTop;
	_ScratchBuf->PTH_map[y][x] = resist | flag_80;
}

/*reverse path slot "index" in place: entries in reverse order and each
 direction flipped (^2: N<->S, E<->W). Used when the search was run from
 the goal toward the NPC.*/
/*C_130F_321A*/PTH_ReversePath(int16 index) {
	int16 k, len;
	int16 tmp;

	for(len = 0; len < 0x20; len ++) {
		if(PathData->content[index][len] == 0)
			break;
	}
	for(k = 0; k < (len + 1) / 2; k ++) {
		tmp = PathData->content[index][k] ^ 2;
		PathData->content[index][k] = PathData->content[index][len - 1 - k] ^ 2;
		PathData->content[index][len - 1 - k] = tmp;
	}
}

/*find a path for objNum (stored in slot PTH_Index):
 - x >= 0: to the square (x, y). If the NPC or the goal is outside the
   area the NPC is teleported if possible (TeleportToSchedule), else the search
   goes toward the area edge on the far side (PTH_direct); when the NPC is
   the one outside, the search is run from the goal and the NPC is put on
   the edge square found ("swapped").
 - x < 0: PTH_object = y (no longer searched for, see PTH_Visit).
 Success: NPCMode = AI_ONPATH, Leader[npc] = path slot. Failure:
 AI_FINDPATH -> AI_PATH_FAILED (retry later), AI_SEEKOBJ -> AI_SCHEDULE.*/
/*C_130F_32AD*/PTH_FindPath(int16 objNum, int16 x, int16 y) {
	int16 start_x, start_y, i, j, node;
	int16 area_x, area_y, cost, flag_80, tmp, swapped;

	PTH_Found = swapped = 0;
	PTH_direct = -1;
	area_x = (MapX - 0x10) & 0x3f8;
	area_y = (MapY - 0x10) & 0x3f8;
	/*coord "in area"*/
	start_x = GetX(objNum) - area_x;
	start_y = GetY(objNum) - area_y;
	if(x >= 0) {
		/*coord "in area"*/
		x -= area_x;
		y -= area_y;
		PTH_object = -1;
		if(start_x == x && start_y == y) {
			StartScheduleAction(objNum);
			return;
		}
		if(!(
			GetZ(objNum) == MapZ &&
			start_x >= 0 && start_x <= (AREA_W-1) &&
			start_y >= 0 && start_y <= (AREA_H-1) &&
			x >= 0 && x <= (AREA_W-1) &&
			y >= 0 && y <= (AREA_H-1)
		)) {
			/*at most 3 teleports per turn*/
			if(TeleportCount >= 3)
				return;
			TeleportToSchedule(objNum);
			if(NPCMode[objNum] != 0x81/*AI_FINDPATH*/)
				return;
			if(!(start_x >= 0 && start_x <= (AREA_W-1) && start_y >= 0 && start_y <= (AREA_H-1))) {
				tmp = x; x = start_x; start_x = tmp;
				tmp = y; y = start_y; start_y = tmp;
				swapped = 1;
			}
			/*edge to head for: the side of the area (seen from its center
			20,20) where the outside end lies; 0 N, 1 E, 2 S, 3 W*/
			if(abs(x - 20) > abs(y - 20))
				PTH_direct = (x < 20)?3:1;
			else
				PTH_direct = (y < 20)?0:2;
		}
	} else {
		/*seeked object*/
		PTH_object = y;
	}

	/*at most 3 searches per turn (see PTH_SearchPaths)*/
	PathSearchCount ++;
	__ComputeResistance(objNum);

	/*reset the map, the node free list and the buckets*/
	for(i = 0; i < AREA_H; i ++) {
		for(j = 0; j < AREA_W; j ++)
			_ScratchBuf->PTH_map[i][j] = 0xff;
	}
	for(i = 0; i < 0x100; i ++)
		_ScratchBuf->PTH_link[i] = i + 1;
	for(i = 0; i < 0x100; i ++)
		_ScratchBuf->PTH_rank[i] = -1;
	_ScratchBuf->PTH_link[0xff] = -1;
	PTH_Top = PTH_Found = 0;
	PTH_Cur = PTH_MaxCost = 0;

	/*seed the NPC's square (front 0) and, for a square goal, the goal
	(front 0x80)*/
	PTH_Visit(0, start_x, start_y, 0);
	if(PTH_object == -1 && PTH_direct == -1)
		PTH_Visit(0, x, y, 0x80);

	/*pop the cheapest cell, give its node back to the free list and
	expand its 4 neighbours*/
	while(_ScratchBuf->PTH_rank[PTH_Cur] >= 0 && !PTH_Found) {
		node = _ScratchBuf->PTH_rank[PTH_Cur];
		_ScratchBuf->PTH_rank[PTH_Cur] = _ScratchBuf->PTH_link[node];
		x = _ScratchBuf->PTH_x[node];
		y = _ScratchBuf->PTH_y[node];
		_ScratchBuf->PTH_link[node] = PTH_Top;
		PTH_Top = node;
		cost = _ScratchBuf->PTH_map[y][x];
		flag_80 = cost & 0x80;
		cost &= 0x7f;
		PTH_Visit(cost + _ScratchBuf->PTH_resist[y][x - 1], x - 1, y, flag_80);
		PTH_Visit(cost + _ScratchBuf->PTH_resist[y][x + 1], x + 1, y, flag_80);
		PTH_Visit(cost + _ScratchBuf->PTH_resist[y - 1][x], x, y - 1, flag_80);
		PTH_Visit(cost + _ScratchBuf->PTH_resist[y + 1][x], x, y + 1, flag_80);
		/*bucket empty: advance to the next non-empty one*/
		if(_ScratchBuf->PTH_rank[PTH_Cur] < 0) {
			while(PTH_Cur < PTH_MaxCost && _ScratchBuf->PTH_rank[PTH_Cur] < 0)
				PTH_Cur ++;
		}
	}
	if(PTH_Found) {
		/*searched from the goal: put the NPC on the edge square reached
		and turn the path around*/
		if(swapped) {
			MoveAppliesTerrain = 0;
			MoveObj(objNum, PTH_AnX + area_x, PTH_AnY + area_y, GetZ(objNum));
			MoveAppliesTerrain = 1;
			PTH_ReversePath(PTH_Index);
		}
		NPCMode[objNum] = 0x83/*AI_ONPATH*/;
		Leader[objNum] = PTH_Index;
		PathObject[PTH_Index] = objNum;
		PathCounter[PTH_Index] = 0;
		PathTries[PTH_Index] = 0;
	} else if(NPCMode[objNum] == 0x81/*AI_FINDPATH*/) {
		NPCMode[objNum] = 0x86/*AI_PATH_FAILED*/;
	} else if(NPCMode[objNum] == 0x82/*AI_SEEKOBJ*/) {
		NPCMode[objNum] = 0x80/*AI_SCHEDULE*/;
	}
}

/*random signed offset with a geometric distribution: 0 (50%), +-1 (25%)...*/
/*C_130F_3732*/RandomSpread() {
	int16 n;

	for(n = 0; OSI_rand(0, 1); n ++);
	if(OSI_rand(0, 1))
		n = -n;
	return n;
}

/*consume movement points according to terrain: 5 + the upper nibble of
 TerrainType (movement cost) of the base tile and of every object on the
 object's square (impassable tiles excluded)*/
/*C_130F_3766*/SubTerrainMov(int16 numObject) {
	int16 numTile;
	int16 x, y, z, obj, pts;

	pts = 5;
	x = GetX(numObject);
	y = GetY(numObject);
	z = GetZ(numObject);
	numTile = GetTileAtXYZ(x, y, z);
	if(!IsTerrainImpass(numTile))
		pts += TerrainType[numTile] >> 4;
	for(obj = FindLoc(x, y, z); obj >= 0; obj = NextLoc()) {
		numTile = TILE_FRAME(obj);
		if(!IsTerrainImpass(numTile))
			pts += TerrainType[numTile] >> 4;
	}
	SubMov(numObject, pts);
}

/*try to step objNum one square in direction dir (refusing damaging tiles if
 avoidHazard). Pays the terrain cost even when blocked. A party member in
 combat won't leave the 17x17 square around the leader. Returns 1 if moved.*/
/*C_130F_384C*/TryStraightMove(int16 objNum, int16 dir, int16 avoidHazard) {
	int16 new_x, new_y;

	dir &= 7;
	SubTerrainMov(objNum);
	new_x = GetX(objNum) + DirIncrX[dir];
	new_y = GetY(objNum) + DirIncrY[dir];
	if(IsPlrControl(objNum) && InCombat && !CLOSE_ENOUGH0(8, MapX, MapY, new_x, new_y))
		return 0;
	AvoidHazards = avoidHazard;
	if(CanMoveTo(objNum, new_x, new_y)) {
		AvoidHazards = 0;
		StepMovesParts = 1;
		if(UpdateMoveFrame(objNum, dir) == 1) {
			MoveObj(objNum, new_x, new_y, MapZ);
			SetDirection(objNum, dir);
			return 1;
		}
		return 0;
	}
	AvoidHazards = 0;
	return 0;
}

/*loiter around (x, y): 1 chance in 8 to take a random straight step,
 biased toward (x, y) the farther the NPC is; otherwise just wait*/
/*C_130F_3930*/LoiterNear(int16 objNum, int16 x, int16 y) {
	int16 delta_x, delta_y, dir;

	if(OSI_rand(0, 7) == 0) {
		delta_x = GetX(objNum) - x;
		delta_y = GetY(objNum) - y;
		if(abs(delta_y) - abs(delta_x) < RandomSpread())
			dir = (RandomSpread() > delta_x)?2:6;
		else
			dir = (RandomSpread() > delta_y)?4:0;
		TryStraightMove(objNum, dir, 1);
	} else {
		SubMov(objNum, 5);
	}
}

/*try a diagonal step combining h_dir (2/6) and v_dir (0/4).
 Returns 1 if moved.*/
/*C_130F_39DD*/__TryDiagMove(int16 objNum, int16 h_dir, int16 v_dir) {
	int16 dir;
	int16 new_x, new_y;

	if(v_dir == 0 && h_dir == 6)
		dir = 7;
	else
		dir = (v_dir + h_dir) >> 1;
	SubTerrainMov(objNum);
	new_x = GetX(objNum) + DirIncrX[dir];
	new_y = GetY(objNum) + DirIncrY[dir];
	PassThroughParty = 1;
	AvoidHazards = 1;
	PassThroughParty = 0;
	if(CanMoveTo(objNum, new_x, new_y)) {
		StepMovesParts = 1;
		if(UpdateMoveFrame(objNum, dir) == 1) {
			AvoidHazards = 0;
			MoveObj(objNum, new_x, new_y, MapZ);
			SetDirection(objNum, dir);
			return 1;
		}
		return 0;
	}
	AvoidHazards = 0;

	return 0;
}

/*take one step from objNum's position toward (new_x, new_y) (not further).
 Chooses the main axis (the larger delta; randomly weighted when far), then
 tries in order: straight on the main axis, diagonal, straight on the other
 axis, and (1 chance in 2) straight back on the other axis. Party members
 don't walk through each other meanwhile (PassThroughParty = 0).
 Returns 1 if a step was made.*/
/*C_130F_3AB8*/TryMoveTo(int16 objNum, int16 new_x, int16 new_y) {
	int16 delta_x, delta_y;
	int16 h_dir, v_dir, moved, isVMove, avoidHazard;

	avoidHazard = !IgnoreHazards;
	moved = 1;
	delta_x = new_x - GetX(objNum);
	delta_y = new_y - GetY(objNum);
	if((delta_x == 0 && delta_y == 0) || NPCMode[objNum] == 0xe/*AI_IMMOBILE*/) {
		SubMov(objNum, 5);
		return 0;
	}
	if(delta_x == 0)
		h_dir = OSI_rand(0, 1)?2:6;
	else if(delta_x < 0)
		h_dir = 6;
	else
		h_dir = 2;
	if(delta_y == 0)
		v_dir = OSI_rand(0, 1)?0:4;
	else if(delta_y < 0)
		v_dir = 0;
	else
		v_dir = 4;
	PassThroughParty = 0;
	if(abs(delta_x) < 4 && abs(delta_y) < 4) {
		isVMove =
			(abs(delta_x) < abs(delta_y)) ||
			(abs(delta_x) == abs(delta_y) && OSI_rand(0, 1))
		;
	} else {
		isVMove = OSI_rand(1, abs(delta_x) + abs(delta_y)) > abs(delta_x);
	}
	if(isVMove) {
		if(
			!TryStraightMove(objNum, v_dir, avoidHazard) &&
			!__TryDiagMove(objNum, h_dir, v_dir) &&
			!TryStraightMove(objNum, h_dir, avoidHazard) &&
			(OSI_rand(0, 1) || !TryStraightMove(objNum, h_dir ^ 4, 1))
		) {
			SubTerrainMov(objNum);
			moved = 0;
		}
	} else {
		if(
			!TryStraightMove(objNum, h_dir, avoidHazard) &&
			!__TryDiagMove(objNum, h_dir, v_dir) &&
			!TryStraightMove(objNum, v_dir, avoidHazard) &&
			(OSI_rand(0, 1) || !TryStraightMove(objNum, v_dir ^ 4, 1))
		) {
			SubTerrainMov(objNum);
			moved = 0;
		}
	}
	PassThroughParty = 1;

	return moved;
}

/*wander at random: a party member sometimes walks back to the leader,
 others take a random straight step 1 time in 8; otherwise wait*/
/*C_130F_3CF6*/WanderRandomly(int16 objNum) {
	if(NPCMode[objNum] != 0xe/*AI_IMMOBILE*/) {
		if(
			IsPlrControl(objNum) &&
			(GetX(objNum) != MapX || GetY(objNum) != MapY) &&
			OSI_rand(0, 3) == 0
		) {
			TryMoveTo(objNum, MapX, MapY);
			return;
		}
		if(OSI_rand(0, 7) == 0) {
			TryStraightMove(objNum, OSI_rand(0, 3) << 1, 1);
			return;
		}
	}
	SubMov(objNum, 5);
}

/*follow the stored path one step (AI_ONPATH): slot = Leader[npc],
 PathCounter[slot] = current entry, PathTries[slot] = steps already done
 in it. At the end of the path the schedule action starts if the NPC is
 within 1 square of (x, y) (its scheduled spot), else it searches again.
 When blocked it waits up to 3 turns (AI_ONPATH -> AI_PATH_BLOCKED1 -> AI_PATH_BLOCKED2 -> AI_PATH_FAILED),
 then gives up the path (AI_PATH_FAILED = to be searched again later).*/
/*C_130F_3D98*/__DoOnPath(int16 objNum, int16 x, int16 y) {
	int16 index;
	int16 info;

	index = Leader[objNum];
	info = PathData->content[index][PathCounter[index]];
	if(info == 0) {
		PathObject[index] = 0;
		if(COMBAT_TileDistance(objNum, x, y) < 2)
			StartScheduleAction(objNum);
		else
			NPCMode[objNum] = 0x81/*AI_FINDPATH*/;
		SubMov(objNum, 5);
	} else if(TryStraightMove(objNum, (info & 3) << 1, 1)) {
		if(++PathTries[index] >= (info >> 2)) {
			PathCounter[index] ++;
			PathTries[index] = 0;
		}
		NPCMode[objNum] = 0x83/*AI_ONPATH*/;
	} else if(NPCMode[objNum] == 0x83/*AI_ONPATH*/) {
		NPCMode[objNum] = 0x84/*AI_PATH_BLOCKED1*/;
		MovePts[objNum] = 0;
	} else if(NPCMode[objNum] == 0x84/*AI_PATH_BLOCKED1*/) {
		NPCMode[objNum] = 0x85/*AI_PATH_BLOCKED2*/;
		MovePts[objNum] = 0;
	} else if(NPCMode[objNum] == 0x85/*AI_PATH_BLOCKED2*/) {
		NPCMode[objNum] = 0x86/*AI_PATH_FAILED*/;
		MovePts[objNum] = 0;
		PathObject[index] = 0;
		/*blocked right next to the goal: good enough
		(note: DirIncrX[info] uses the raw path byte, not (info & 3) << 1)*/
		if(GetX(objNum) + DirIncrX[info] == x && GetY(objNum) + DirIncrY[info] == y)
			StartScheduleAction(objNum);
	}
}

/*walk toward the nearest visible party member (or the leader's square when
 none is visible). Returns that member if now adjacent, else -1.*/
/*C_130F_3ECD*/ApproachNearestMember(int16 objNum) {
	int16 nearest, member;
	int16 partyIdx, x, y, x_dest, y_dest, dist, dist_min;

	nearest = -1;
	x = GetX(objNum);
	y = GetY(objNum);
	dist_min = 50;
	for(partyIdx = 0; partyIdx < PartySize; partyIdx ++) {
		member = Party[partyIdx];
		if(ObjShapeType[member] && IsDead(member) == 0 && COMBAT_canSee(objNum, member)) {
			x_dest = GetX(member);
			y_dest = GetY(member);
			dist = (x_dest - x) * (x_dest - x) + (y_dest - y) * (y_dest - y);
			if(
				(dist < dist_min) ||
				(dist == dist_min && OSI_rand(0, 1))
			) {
				dist_min = dist;
				nearest = member;
			}
		}
	}
	if(nearest < 0) {
		TryMoveTo(objNum, MapX, MapY);
	} else {
		TryMoveTo(objNum, GetX(nearest), GetY(nearest));
		if(!CLOSE_ENOUGH0(1, GetX(nearest), GetY(nearest), GetX(objNum), GetY(objNum)))
			nearest = -1;
	}
	return nearest;
}

/*mode 0x96: the NPC starts a conversation with the party.
 note: TALK_talkTo() expects a party index as first parameter but gets
 the member's object number here*/
/*C_130F_4079*/TalkToParty(int16 npc, int16 member) {
	TALK_talkTo(member, npc, 0);
	PromptFlag = 1;
}

/*find a free path slot (0..7), freeing the slots whose NPC is no longer
 following a path. Returns 8 if none.*/
/*C_130F_4093*/__NewPathIndex() {
	int16 slot, mode;

	for(slot = 0; slot < 8; slot ++) {
		if(PathObject[slot]) {
			mode = NPCMode[PathObject[slot]];
			if(!(mode >= 0x83/*AI_ONPATH*/  && mode <= 0x86/*AI_PATH_FAILED*/))
				PathObject[slot] = 0;
		}
		if(!PathObject[slot])
			break;
	}
	return slot;
}

/*C_130F_40DB*/C_130F_40DB(int16 objNum) {
	int16 objTyp;

	objTyp = GetType(objNum);
	return
		(objTyp >= 0x136 && objTyp <= 0x146) || (objTyp >= 0x13a && objTyp <= 0x13c) ||
		objTyp == 0x145 || objTyp == 0x142 || objTyp == 0x140
	;
}

/*A.I. engine: let actor objNum act once according to NPCMode (combat
 strategies, schedule behaviours, path following...). The creatures with
 ObjStatus 0x80 use COMBAT_AI_Berserk whatever their mode. In modes 0x0f, 0x10
 and 0x8b-0x8e the NPC turns into type 0x149 (its shape is saved in
 OrigShapeType and given back in the other modes). An actor that did not
 spend any movement point loses 10.*/
/*C_130F_4121*/NPC_TakeTurn(int16 objNum) {
	int16 other;
	int16 sched_x, sched_y, startMovePts;
	struct coord sched_xyz;

	if(IsPlrControl(objNum) && !IsCharmed(objNum))
		COMBAT_stopCharm(objNum);
	startMovePts = MovePts[objNum];
	sched_xyz = Schedule[SchedIndex[objNum] + SchedPointer[objNum]].xyz;
	sched_x = GetCoordX(sched_xyz);
	sched_y = GetCoordY(sched_xyz);
	if(
		GetType(objNum) == 0x149 &&
		NPCMode[objNum] != 0x8b && NPCMode[objNum] != 0x8c &&
		NPCMode[objNum] != 0x8d && NPCMode[objNum] != 0x8e
	) SetType(objNum, OrigShapeType[objNum]);
	if(ObjStatus[objNum] & 0x80) {
		COMBAT_AI_Berserk(objNum);
	} else switch(NPCMode[objNum]) {
		case 0x04: COMBAT_AI_Range(objNum); break;
		case 0x06: COMBAT_AI_Berserk(objNum); break;
		case 0x08:
		case 0x9b: COMBAT_AI_Assault(objNum); break;
		case 0x0e: COMBAT_AI_Immobile(objNum); break;
		case 0x0d: COMBAT_AI_Tangle(objNum); break;
		case 0x07:
		case 0x09:
		case 0x13:
		case 0x9a: COMBAT_AI_Retreat(objNum); break;
		case 0x0a: COMBAT_AI_Like(objNum); break;
		case 0x0b: COMBAT_AI_Unfriendly(objNum); break;
		case 0x99:
			/*brawl: from time to time go and (harmlessly) hit an
			opponent near the brawl spot, then come back*/
			if(OSI_rand(0, 3) == 0) {
				other = COMBAT_pickOpponent(objNum);
				if(other >= 0) {
					if(COMBAT_TileDistance(other, sched_x, sched_y) < 5) {
						if(COMBAT_TileDistance(objNum, GetX(other), GetY(other)) < 2) {
							COMBAT_showHit(other);
							TryMoveTo(objNum, sched_x, sched_y);
						} else {
							TryMoveTo(objNum, GetX(other), GetY(other));
						}
						break;
					}
				}
			}
			LoiterNear(objNum, sched_x, sched_y);
		break;
		case 0x96:
			/*converse*/
			if(CLOSE_ENOUGH0_S(5, sched_x, sched_y, MapX, MapY)) {
				if((other = ApproachNearestMember(objNum)) >= 0) {
					NPCMode[objNum] = 0x90;
					TalkToParty(objNum, other);
				}
			} else {
				LoiterNear(objNum, sched_x, sched_y);
			}
		break;
		case 0x83:
		case 0x84:
		case 0x85: __DoOnPath(objNum, sched_x, sched_y); break;
		case 0x82:
			/*Leader[] holds the furniture kind (index in FurnitureTypes)*/
			PTH_Index = __NewPathIndex();
			if(PTH_Index < 8) {
				PTH_FindPath(objNum, -1, FurnitureTypes[Leader[objNum]]);
				SubMov(objNum, 5);
			} else {
				MovePts[objNum] = 0;
			}
		break;
		case 0x0c:
		case 0x8f: WanderRandomly(objNum); break;
		case 0x90:
		case 0x94: LoiterNear(objNum, sched_x, sched_y); break;
		case 0x0f:
		case 0x10:
		case 0x8b:
		case 0x8c:
		case 0x8d:
		case 0x8e:
			OrigShapeType[objNum] = ObjShapeType[objNum];
			if(C_130F_40DB(objNum))
				SetType(objNum, 0x149 | (OSI_rand(0, 3) << 10));
			else
				SetType(objNum, 0x149 | (OSI_rand(4, 7) << 10));
		break;
		default:
			SubMov(objNum, 5);
	}
	if(MovePts[objNum] == startMovePts)
		SubMov(objNum, 10);
}

/*add the terrain of an object tile to the path resistance of a cell:
 a non-impassable object on impassable water makes it passable (bridge),
 an impassable one blocks it, others add their movement cost (max 0xff)*/
/*C_130F_44A3*/PTH_AddObjResist(int16 tile, int16 area_x, int16 area_y) {
	int16 sum;

	if(_ScratchBuf->PTH_resist[area_y][area_x] == 0xff) {
		if(IsTerrainWet(AreaTiles[area_y][area_x]) && !IsTerrainImpass(tile))
			_ScratchBuf->PTH_resist[area_y][area_x] = (TerrainType[tile] >> 4) + 1;
	} else if(IsTerrainImpass(tile)) {
		_ScratchBuf->PTH_resist[area_y][area_x] = 0xff;
	} else if(!IsTerrainWall(tile)) {
		sum = _ScratchBuf->PTH_resist[area_y][area_x] + (TerrainType[tile] >> 4);
		if(sum > 0xff)
			sum = 0xff;
		_ScratchBuf->PTH_resist[area_y][area_x] = sum;
	}
}

/*fill PTH_resist for the 40x40 area: 1 + movement cost of the base tile
 (0xff if impassable), then objects: unlocked doors (type 0xea, frame < 8)
 cost 1 more, locked doors block, doors also block the square beside them
 (the frame), other objects (all their tiles for multi-tile ones) go
 through PTH_AddObjResist; actors are ignored*/
/*C_130F_45A2*/__ComputeResistance(int16 unusedObj) {
	int16 area_y, area_x;
	int16 row, col, objNum, tile;
	int16 origin_x, origin_y, objTyp, tileFlags;

	origin_x = (MapX - 0x10) & 0x3f8;
	origin_y = (MapY - 0x10) & 0x3f8;

	for(row = 0; row < AREA_H; row ++) {
		for(col = 0; col < AREA_W; col ++) {
			tile = AreaTiles[row][col];
			if(IsTerrainImpass(tile))
				_ScratchBuf->PTH_resist[row][col] = 0xff;
			else
				_ScratchBuf->PTH_resist[row][col] = (TerrainType[tile] >> 4) + 1;
		}
	}

	for(objNum = SearchArea(origin_x, origin_y, origin_x + (AREA_W-1), origin_y + (AREA_H-1)); objNum >= 0; objNum = NextArea()) {
		area_x = GetX(objNum) - origin_x;
		area_y = GetY(objNum) - origin_y;
		objTyp = GetType(objNum);
		tileFlags = TileFlag[TILE_FRAME(objNum)];
		if(objTyp == 0xea) {/*door*/
			if(GetFrame(objNum) < 8 && _ScratchBuf->PTH_resist[area_y][area_x] < 0xff)
				_ScratchBuf->PTH_resist[area_y][area_x] = _ScratchBuf->PTH_resist[area_y][area_x] + 1;
			else
				_ScratchBuf->PTH_resist[area_y][area_x] = 0xff;
			if(tileFlags & 0x40/*TILE_DOUBLE_HEIGHT*/) {
				if(area_y)
					_ScratchBuf->PTH_resist[area_y - 1][area_x] = 0xff;
			} else {
				if(area_x)
					_ScratchBuf->PTH_resist[area_y][area_x - 1] = 0xff;
			}
		} else if(!IsNPC(objNum)) {
			tile = TILE_FRAME(objNum);
			PTH_AddObjResist(tile, area_x, area_y);
			if(tileFlags & 0x80/*TILE_DOUBLE_WIDTH*/) {
				if(area_x)
					PTH_AddObjResist(tile - 1, area_x - 1, area_y);
				if(tileFlags & 0x40/*TILE_DOUBLE_HEIGHT*/) {
					if(area_y)
						PTH_AddObjResist(tile - 2, area_x, area_y - 1);
					if(area_x && area_y)
						PTH_AddObjResist(tile - 3, area_x - 1, area_y - 1);
				}
			} else if(tileFlags & 0x40/*TILE_DOUBLE_HEIGHT*/) {
				if(area_y)
					PTH_AddObjResist(tile - 1, area_x, area_y - 1);
			}
		}
	}
}

/*once per turn: path searches for the NPCs (2..0xbf) waiting in AI_FINDPATH,
 at most 3 teleports and 3 searches per turn while free path slots
 remain. If no search was needed, the NPCs whose path failed (AI_PATH_FAILED) are
 put back to AI_FINDPATH for another try.*/
/*C_130F_4847*/PTH_SearchPaths() {
	int16 objNum, noneSearched;
	int16 schedEntry;
	struct coord xyz;

	noneSearched = 1;
	PTH_Index = __NewPathIndex();
	if(PTH_Index == 8)
		return;
	TeleportCount =
	PathSearchCount = 0;
	for(objNum = 2; objNum < 0xc0; objNum ++) {
		if(NPCMode[objNum] == 0x81/*AI_FINDPATH*/) {
			if(TeleportCount < 3)
				TeleportToSchedule(objNum);
			if(NPCMode[objNum] == 0x81/*AI_FINDPATH*/ && PathSearchCount < 3) {
				noneSearched = 0;
				schedEntry = SchedIndex[objNum] + SchedPointer[objNum];
				xyz = Schedule[schedEntry].xyz;
				PTH_FindPath(objNum, GetCoordX(xyz), GetCoordY(xyz));
				while(PathObject[PTH_Index] && PTH_Index < 8)
					PTH_Index ++;
				if(PTH_Index == 8)
					break;
			}
		}
	}
	if(noneSearched) {
		for(objNum = 2; objNum < 0xc0; objNum ++) {
			if(NPCMode[objNum] == 0x86/*AI_PATH_FAILED*/) {
				NPCMode[objNum] = 0x81/*AI_FINDPATH*/;
				if(TeleportCount < 3)
					TeleportToSchedule(objNum);
			}
		}
	}
}

/*check who the player controls:
 - in a vehicle, actor 0 (the vehicle) is kept healthy;
 - the Avatar (object 1) dead -> C_2701_0556;
 - if the active member is asleep/paralyzed/dead/dragged under/charmed,
   control goes to the next able member (in solo mode: back to party mode);
 - out of combat, control goes back to the Avatar as soon as he is able
   (in combat the former active member resumes its combat mode).*/
/*C_130F_4943*/CheckActiveMember() {
	int16 i;

	if(Active > PartySize)
		return;
	if(IN_VEHICLE) {
		ClrCursedCharmed(0);
		ClrAilments(0);
		ClrDraggedUnder(0);
		SetAlignment(0, 0x40/*GOOD*/);
	}
	if(IsDead(1))
		C_2701_0556();
	if(SoloMember >= 0) {
		if(IsIncapacitated(Party[Active]) || IsDraggedUnder(Party[Active]) || IsCharmed(Party[Active]))
			SetPartyMode();
	}
	if(SoloMember >= 0)
		return;
	if(
		IsIncapacitated(Party[Active]) ||
		IsDraggedUnder(Party[Active]) ||
		IsCharmed(Party[Active])
	) {
		for(i = 0; i < PartySize; i ++) {
			if(
				!(IsIncapacitated(Party[i]) || IsDraggedUnder(Party[i])) &&
				!IsCharmed(Party[i]) &&
				NPCMode[Party[i]] != 0/*AI_MOTIONLESS*/
			) {
				NPCMode[Party[Active]] = 1/*AI_FOLLOW*/;
				NPCMode[Party[i]] = 2/*AI_COMMAND*/;
				Active = i;
				PromptFlag = 1;
				MapX = GetX(Party[i]);
				MapY = GetY(Party[i]);
				MAP_LoadAround();
				break;
			}
		}
	}
	if(Active < PartySize && Active && SoloMember < 0) {
		if(
			!(IsIncapacitated(Party[0]) || IsDraggedUnder(Party[0])) &&
			!IsCharmed(Party[0])
		) {
			if(InCombat) {
				NPCMode[Party[Active]] = NPCComMode[Party[Active]];
			} else {
				NPCMode[Party[Active]] = 1/*AI_FOLLOW*/;
				Active = 0;
				PromptFlag = 1;
				MapX = GetX(Party[0]);
				MapY = GetY(Party[0]);
				MAP_LoadAround();
			}
			NPCMode[Party[0]] = 2/*AI_COMMAND*/;
		}
	}
}

/*the turn scheduler, called after each player command: run the A.I. of all
 the actors of the area until a player-controlled one (AI_COMMAND) has
 to act. The next actor is the one with the highest MovePts/DEXTE ratio
 (one with MovePts >= DEXTE is taken at once); when nobody has points left
 everybody gets DEXTE more points and the clock advances one minute.
 Actors walking off the area are frozen (path NPCs teleported/re-pathed).
 AI_FOLLOW members move only with the leader (MoveFollowers); the leader
 can't act while a follower has no points left.*/
/*C_130F_4C0F*/RunNPCTurns() {
	int16 i, bestPts;
	int16 area_x, area_y, actor, bestDex, cmp, coordMask;

	CheckActiveMember();
	PTH_SearchPaths();
	actor = Party[Active];
	/*coordinates wrap at 0x400 on the surface, at 0x100 underground*/
	if(MapZ)
		coordMask = 0xff;
	else
		coordMask = 0x3ff;

	do {
		actor = -1;
		bestPts = 0;
		bestDex = 1;

		do {
			area_x = (MapX - 0x10) & 0x3f8;
			area_y = (MapY - 0x10) & 0x3f8;

			for(i = 1; i < PartySize; i ++) {
				if(NPCMode[Party[i]] == 1/*AI_FOLLOW*/ && MovePts[Party[i]] <= 0)
					MovePts[Party[0]] = 0;
			}

			for(i = 0; i < 0x100; i ++) {
				if(
					!ObjShapeType[i] ||
					NPCMode[i] == 0/*AI_MOTIONLESS*/ ||
					IsParalyzed(i) ||
					(IsAsleep(i) && NPCMode[i] != 0x80/*AI_SCHEDULE*/) ||
					IsDead(i) ||
					MovePts[i] <= 0 ||
					GetCoordUse(i) || GetZ(i) != MapZ
				) continue;
				if(
					((GetX(i) - area_x) & coordMask) > (AREA_W-1) ||
					((GetY(i) - area_y) & coordMask) > (AREA_H-1)
				) {
					if(NPCMode[i] >= 0x83/*AI_ONPATH*/ && NPCMode[i] <= 0x86/*AI_PATH_FAILED*/)
						TeleportToSchedule(i);
					if(NPCMode[i] >= 0x83/*AI_ONPATH*/ && NPCMode[i] < 0x86/*AI_PATH_FAILED*/) {
						PathObject[Leader[i]] = 0;
						NPCMode[i] = 0x81/*AI_FINDPATH*/;
					}
					MovePts[i] = 0;
				} else {
					if(NPCMode[i] == 1/*AI_FOLLOW*/)
						continue;
					if(NPCMode[i] == 0x80/*AI_SCHEDULE*/)
						StartScheduleAction(i);
					/*cmp > 0 <=> MovePts[i]/DEXTE[i] > bestPts/bestDex*/
					cmp = MovePts[i] * bestDex - DEXTE[i] * bestPts;
					if(
						MovePts[i] >= DEXTE[i] || cmp > 0 ||
						(cmp == 0 && DEXTE[i] > bestDex)
					) {
						actor = i;
						bestPts = MovePts[i];
						bestDex = DEXTE[i];
					}
					if(MovePts[i] >= DEXTE[i])
						break;
				}
			}

			/*new round: give everybody its dexterity in points (debts are
			carried over) and let a minute pass*/
			if(bestPts <= 0) {
				for(i = 0; i < 0x100; i ++) {
					MovePts[i] = (MovePts[i] < 0)?(MovePts[i] + DEXTE[i]):DEXTE[i];
				}
				PassTime(1);
			}

		} while(bestPts <= 0);

		if(NPCMode[actor] != 2/*AI_COMMAND*/ && NPCMode[actor] != 1/*AI_FOLLOW*/) {
			NPC_TakeTurn(actor);
			CheckActiveMember();
			/*in combat-type modes Leader[] is the actor's current target/leader;
			reset it to itself*/
			if(NPCMode[actor] > 1 && NPCMode[actor] < 0x10)
				Leader[actor] = actor;
		}
		FlushViewIfDirty();

	} while(NPCMode[actor] != 2/*AI_COMMAND*/);

	/*give control to the member whose turn it is*/
	if(IN_VEHICLE)
		actor = 0;
	if(Party[Active] != actor) {
		for(i = 0; i < PartySize; i ++) {
			if(Party[i] == actor) {
				SetActive(i);
				PromptFlag = 1;
				break;
			}
		}
	}
}

/*initialization after a game is loaded/started: no actor with 0 dexterity,
 actor 0 (vehicle) not in a schedule mode*/
/*C_130F_4EEF*/InitNPCTracker() {
	int16 i;

	for(i = 0; i < 0x100; i ++) {
		if(DEXTE[i] == 0)
			DEXTE[i] = 1;
	}
	if(NPCMode[0] >= 0x80/*AI_SCHEDULE*/)
		NPCMode[0] = 0/*AI_MOTIONLESS*/;
}

/*NPC update, every hour: for each scheduled NPC (2..0x97) not in party or
 combat (except mode 7), find the schedule entry for this hour and day and
 make it walk there (AI_FINDPATH). Schedule entries of NPC n are
 Schedule[SchedPointer[n] .. SchedPointer[n+1]-1]; SchedIndex[n] = current
 entry relative to SchedPointer[n]. Entry time byte: bits 0-4 hour,
 bits 5-7 day of the week (0 = every day). Mode 0x9b calms down; an NPC
 waiting (D_3298 0x80) out of the party gets mode 0x92.*/
/*C_130F_4F1B*/UpdateSchedules() {
	int16 npc, entry;
	int16 first, last, when;

	for(npc = 2; npc < 0x98; npc ++) {
		if(NPCMode[npc] == 0x9b)
			NPCMode[npc] = 0x81/*AI_FINDPATH*/;
		if((D_3298[npc] & 0x80) && !IsPlrControl(npc)) {
			NPCMode[npc] = 0x92;
			continue;
		}
		if(ObjShapeType[npc] == 0)
			continue;
		if(NPCMode[npc] < 0x80/*AI_SCHEDULE*/ && NPCMode[npc] != 7)
			continue;
		if(!IsPlrControl(npc)) {
			first = SchedPointer[npc];
			last = SchedPointer[npc + 1] - 1;
			if(last >= first) {
				for(entry = last; entry >= first; entry --) {
					when = Schedule[entry].time;
					/*searched backward, so the last matching entry wins*/
					if((when & 0x1f) == Time_H)
					if((when >> 5) == ((Date_D - 1) % 7 + 1) || (when & 0xe0) == 0) {
						SchedIndex[npc] = entry - first;
						NPCMode[npc] = 0x81/*AI_FINDPATH*/;
						break;
					}
				}
			}
		}
	}
}
