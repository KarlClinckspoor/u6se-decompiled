/* flags: -d -mm -p -c -Z -O -y */
/*
 * The Savage Empire (Worlds of Ultima) - GAME.EXE
 * segment 0359: game loop and world clock (Ultima VI's gameloop.c).
 *
 * GameLoop is the main command loop (called once from main.c): it reads a
 * key, turns letter keys into commands, runs the command, then the end of
 * turn (NPC turns, disk full recovery, prompt). PassTime is the world
 * clock: it advances time by N minutes and handles what depends on it
 * (spell effects, the explosive's fuse, torches burning out, status
 * effects wearing off, poison, terrain damage, hour/day/month/year,
 * NPC schedules, the sky tile, ambient light).
 * AnimateTilesAndActors / DrawMapWindow / DisplayTick are the per frame
 * display update: animated tiles, the idle animations of some creatures,
 * the direction shown by an inventory object (type 0x10b), and the map
 * window (with the target cursor and the mouse pointer).
 * TerrainDamage / ApplyTerrainEffects apply the effects of standing on a
 * harmful tile. SetActive / SetPartyMode switch solo mode / party mode.
 */
#include "se.h"

#undef abs/*abs() is the library function here, not the inline __abs__*/

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

/*the graphic driver: D_ED10's offset selects the entry*/
#define GR_SET_COLOR(c)               (D_ED10.iii.ofs = 0x06, (*D_ED10.f)(c))
#define GR_PUT_PIXEL(x,y)             (D_ED10.iii.ofs = 0x09, (*D_ED10.f)(x,y))
#define GR_PUT_TILE(tile,x,y)         (D_ED10.iii.ofs = 0x42, (*D_ED10.f)(tile,x,y))
#define GR_COPY_TO_SCREEN(a,b,c,d,e,g) (D_ED10.iii.ofs = 0x45, (*D_ED10.f)(a,b,c,d,e,g))
#define GR_FADE_TO_SCREEN(a,b,c,d,e,g) (D_ED10.iii.ofs = 0x48, (*D_ED10.f)(a,b,c,d,e,g))
#define GR_SET_ANIM_FRAME(a,b)        (D_ED10.iii.ofs = 0x4b, (*D_ED10.f)(a,b))

/*OSI library (asm)*/
extern int16 C_2B57_001F(int16, int16);/*OSI_rand(lo, hi)*/
extern cdecl MOUSE_SERVE();/*MOUSE_SERVE*/
extern cdecl C_2ABF_0164();/*MOUSE_DRAW*/
extern cdecl C_2ABF_018B();/*MOUSE_SAVE_BG*/
extern int16 cdecl MOUSE_IN_AREA(struct tSurface *);/*MOUSE_IN_AREA*/

/*far data*/
extern int16 far MonsterFlags[];
extern char far StuckInWebMsg[];
extern char far BurnOutMsg[];
extern char far NotOnShipMsg[];
extern char far NotAllHereMsg[];
extern char far PartyModeMsg[];
extern char far FailsToRespondMsg[];
extern char far SoloModeMsg[];
extern char far ExitToDOSMsg[];
extern char far NotInSoloMsg[];
extern char far BreakOffMsg[];
extern char far CombatMsg[];
extern char far WhatMsg[];
extern char far NotInCombatMsg[];
extern char far SaveMsg[];
extern char far PassMsg[];
extern char far SoundOffMsg[];
extern char far SoundOnMsg[];
extern char far VersionMsg[];

/*console*/
extern CON_UpdateMousePointer();/*CON_UpdateMousePointer*/
extern GAME_Restore();/*GAME_Restore*/
extern GAME_Save();/*GAME_Save*/
extern DrawSlotCursor(int16);/*DrawSlotCursor*/
extern int16 CON_getch();/*CON_getch*/
extern CON_gets(char *, int16);/*CON_gets*/
extern cdecl CON_printf(char far *, ...);/*CON_printf*/
/*map, visible*/
extern MAP_LoadAround();
extern PartyTeleport(int16, int16, int16, int16);
extern unsigned char GetTileAtXYZ(int16, int16, int16);
extern VIS_UpdateView();
/*object*/
extern int16 OBJ_IsLocLoaded(int16, int16, int16);/*OBJ_ISLOCLOADED(x, y, z)*/
extern int16 NextLoc();/*NextLoc*/
extern int16 FindLoc(int16, int16, int16);/*FindLoc(x, y, z)*/
extern int16 OBJ_FindTypeAt(int16, int16, int16);/*OBJ_FindTypeAt(type, x, y)*/
extern int16 FindInv(int16);/*FindInv*/
extern int16 NextInv();/*NextInv*/
extern int16 FindInvType(int16, int16, int16);/*FindInvType(owner, type, quality)*/
extern int16 AddMapObj(int16, int16, int16, int16, int16, int16);/*AddMapObj(type and frame, quantity, owner, x, y, z)*/
extern DeleteObj(int16);/*DeleteObj*/
extern MoveObj(int16, int16, int16, int16);/*MoveObj(objNum, x, y, z)*/
extern ShowObjects();/*ShowObjects*/
extern char *GetObjectString(int16);/*GetObjectString*/
/*status, talk*/
extern STAT_ShowParty(int16);
extern STAT_ShowPortrait(int16);
extern STAT_DrawInventory();
extern STAT_ShowInventory(int16);
extern TALK_talkTo(int16, int16, int16);
extern char *TALK_GetCharName(int16);/*TALK_GetCharName*/
/*npctrack*/
extern int16 CanMoveTo(int16, int16, int16);/*can objNum move to (x, y)?*/
extern MoveFollowers(int16, int16);/*MoveFollowers*/
extern PartyAdvance(int16);/*PartyAdvance*/
extern PTH_SearchPaths();/*PTH_SearchPaths*/
extern RunNPCTurns();/*RunNPCTurns*/
extern InitNPCTracker();/*InitNPCTracker*/
extern UpdateSchedules();/*UpdateSchedules*/
/*combat*/
extern COMBAT_stopCharm(int16);/*COMBAT_stopCharm*/
extern COMBAT_begin();/*COMBAT_begin*/
extern COMBAT_breakOff();/*COMBAT_breakOff*/
extern int16 GetStr(int16);/*GetDex*/
extern LoseHP(int16, int16);/*LoseHP(objNum, points)*/
extern COMBAT_attack();/*COMBAT_attack*/
/*objcmd*/
extern int16 IsLavaTile(int16);/*is the tile a fire (burns)?*/
extern ShowPortrait(int16);/*shows an NPC's portrait and inventory*/
extern CmdLook();/*CmdLook*/
extern CmdDrop();/*CmdDrop*/
extern CmdGet();/*CmdGet*/
extern CmdMove();/*CmdMove*/
extern Detonate(int16);/*Detonate*/
extern CmdUse();/*CmdUse*/
/*egg, music, gfx, rest*/
extern int16 GetMonsterClass(int16);/*GetMonsterClass*/
extern MUS_PlaySong(int16);
extern MUS_PlayAreaSong();/*MUS_PlayAreaSong?*/
extern C_2701_045E();
extern MUS_PlaySfx(int16);/*MUS_PlaySfx?*/
extern C_2701_0684();
extern ShakeScreen(int16, int16);/*ShakeScreen*/
extern DrawSkyBar();/*DrawSkyBar*/
extern PeerMap();/*PeerMap*/
extern REST_Command();/*REST_Command*/

/*variables of the other modules*/
extern char DirIncrX[];/*DirIncrX*/
extern char DirIncrY[];/*DirIncrY*/
extern unsigned char MouseOn;/*MouseOn*/
extern unsigned char SelectMode;/*SelectMode*/
extern char AimX, AimY;/*AimX, AimY*/
extern char SelectRange;/*SelectRange*/
extern char FreeAimMode;/*FreeAimMode*/
extern char SlotCursorCol, SlotCursorRow;/*SlotCursorCol, SlotCursorRow*/
extern int16 HelpOn;/*HelpOn*/
extern int16 LastAttackTarget;/*LastAttackTarget*/
extern int16 StatusMember;/*StatusMember*/
extern int16 MouseMode;/*MouseMode*/
extern int16 StatusDisplay;/*StatusDisplay*/
extern int16 PendingCommand;/*PendingCommand*/
extern int16 AllowMouseMov;/*AllowMouseMov*/
extern int16 AllowNPCTeleport;/*AllowNPCTeleport*/
extern int16 VisibilityDirty;/*VisibilityDirty*/
extern int16 CombatOutcome;
extern int16 HatchingDisabled;
extern int16 ScreenFade;/*ScreenFade*/
extern int16 D_24E8;/*ColorBlack*/
extern unsigned char NextSleep;/*NextSleep*/
extern unsigned char Time_M;/*Time_M*/
extern unsigned char Time_H;/*Time_H*/
extern unsigned char Date_D;/*Date_D*/
extern unsigned char Date_M;/*Date_M*/
extern int16 Date_Y;/*Date_Y*/
extern unsigned char Active;/*Active*/
extern char AmbientLight;/*AmbientLight*/
extern int16 MapX, MapY, MapZ;/*MapX, MapY, MapZ*/
extern unsigned char SpellFx[2];/*SpellFx*/
extern unsigned char HourlyUnused;/*HourlyUnused*/
extern unsigned char CaughtFish;/*CaughtFish*/
extern unsigned char DrunkCounter;/*DrunkCounter*/
extern unsigned char MustRingBell;/*MustRingBell*/
extern int16 SoundFlag;/*SoundFlag*/
extern int16 KegFuse;/*KegFuse*/
extern int16 KegObject;/*KegObject*/
extern int16 SpeakGargish;/*Avatar "solid" (debug)*/
extern char InCombat;/*InCombat*/
extern char SoloMember;/*SoloMember*/
extern unsigned char SavedNPCMode;/*SavedNPCMode*/
extern int16 DiskFullChoice;/*DiskFullChoice*/
extern struct tSurface D_3268;/*Screen*/
extern unsigned char D_3276[0x20];/*AnimFrameMask*/
extern int16 D_3296;/*free memory for the tiles, in KB*/
extern unsigned char far *D_3298;/*0x100 bytes (0x80: NPC's "WAIT" flag)*/
extern unsigned char D_329C[];/*ObjStatus*/
extern struct tMouse D_3E9C;/*Mouse*/
extern unsigned char D_44FA[];/*MovePts*/
extern unsigned char far *D_4602;/*NPCFlag*/
extern unsigned char far *D_4706;/*NPCMode*/
extern unsigned char D_4EDA[];/*Party*/
extern char D_5104[][14];/*Names*/
extern int16 D_54F0[];/*Amount*/
extern unsigned char D_6CF0[];/*NPCStatus*/
extern int16 far *D_6EF4;/*BaseTile*/
extern unsigned char D_6EFC[];/*TalkFlags*/
extern unsigned char far *D_7640;/*TileFlag2*/
extern int16 D_7744[0x20];/*AnimFirstFrame*/
extern struct coord D_78E4[];/*ObjPos*/
extern struct tSurface D_9CE4;/*MapSurface*/
extern unsigned char D_9CF6;/*PartySize*/
extern int16 D_9CF8;/*NumAnimData*/
extern uint16 D_9CFA[];/*ObjShapeType*/
extern unsigned char far *D_B4FA;/*TerrainType*/
extern int16 D_B4FE[0x20];/*AnimTile*/
extern int16 far *D_B53E;/*OrigShapeType*/
extern unsigned char D_B542[0x20];/*AnimFrameShift*/
extern unsigned char D_B566[5];/*HexInputBuf*/
extern unsigned char D_B56B[6];/*DecInputBuf*/
extern unsigned char D_B570[0x20];/*StateAnimData*/
extern int16 D_B5E6;/*AdvanceDir*/
extern struct tSurface D_B5EC;/*MouseArea*/
extern int16 D_B642;/*Selection.obj*/
extern unsigned char D_BD10[11][11];/*Obj_11x11*/
extern int16 D_BD8A[];/*CellObjTile*/
extern unsigned char D_BF8A[];/*CellObjNext*/
extern int16 D_C08A[11][11];/*Tile_11x11*/
extern int16 D_EADE;
extern struct tGr far *D_ED0C;/*GrInfo*/
extern union tHandler D_ED10;/*GrDriver*/

#define MouseOn MouseOn
#define DirIncrX DirIncrX
#define DirIncrY DirIncrY
#define SelectMode SelectMode
#define AimX AimX
#define AimY AimY
#define SelectRange SelectRange
#define FreeAimMode FreeAimMode
#define SlotCursorCol SlotCursorCol
#define SlotCursorRow SlotCursorRow
#define HelpOn HelpOn
#define LastAttackTarget LastAttackTarget
#define StatusMember StatusMember
#define MouseMode MouseMode
#define StatusDisplay StatusDisplay
#define PendingCommand PendingCommand
#define AllowMouseMov AllowMouseMov
#define AllowNPCTeleport AllowNPCTeleport
#define VisibilityDirty VisibilityDirty
#define ScreenFade ScreenFade
#define ColorBlack D_24E8
#define NextSleep NextSleep
#define Time_M Time_M
#define Time_H Time_H
#define Date_D Date_D
#define Date_M Date_M
#define Date_Y Date_Y
#define Active Active
#define AmbientLight AmbientLight
#define MapX MapX
#define MapY MapY
#define MapZ MapZ
#define SpellFx SpellFx
#define HourlyUnused HourlyUnused
#define CaughtFish CaughtFish
#define DrunkCounter DrunkCounter
#define MustRingBell MustRingBell
#define SoundFlag SoundFlag
#define KegFuse KegFuse
#define KegObject KegObject
#define InCombat InCombat
#define SoloMember SoloMember
#define SavedNPCMode SavedNPCMode
#define DiskFullChoice DiskFullChoice
#define Screen D_3268
#define AnimFrameMask D_3276
#define ObjStatus D_329C
#define Mouse D_3E9C
#define MovePts D_44FA
#define NPCFlag D_4602
#define NPCMode D_4706
#define Party D_4EDA
#define Names D_5104
#define Amount D_54F0
#define NPCStatus D_6CF0
#define BaseTile D_6EF4
#define TalkFlags D_6EFC
#define TileFlag2 D_7640
#define AnimFirstFrame D_7744
#define ObjPos D_78E4
#define MapSurface D_9CE4
#define PartySize D_9CF6
#define NumAnimData D_9CF8
#define ObjShapeType D_9CFA
#define TerrainType D_B4FA
#define AnimTile D_B4FE
#define OrigShapeType D_B53E
#define AnimFrameShift D_B542
#define HexInputBuf D_B566
#define DecInputBuf D_B56B
#define StateAnimData D_B570
#define MouseArea D_B5EC
#define Obj_11x11 D_BD10
#define CellObjTile D_BD8A
#define CellObjNext D_BF8A
#define Tile_11x11 D_C08A
#define GrInfo D_ED0C

#define OSI_rand C_2B57_001F
#define MOUSE_SERVE MOUSE_SERVE
#define MOUSE_DRAW C_2ABF_0164
#define MOUSE_SAVE_BG C_2ABF_018B
#define MOUSE_IN_AREA MOUSE_IN_AREA
#define CON_UpdateMousePointer CON_UpdateMousePointer
#define GAME_Restore GAME_Restore
#define GAME_Save GAME_Save
#define DrawSlotCursor DrawSlotCursor
#define CON_getch CON_getch
#define CON_gets CON_gets
#define CON_printf CON_printf
#define NextLoc NextLoc
#define FindLoc FindLoc
#define OBJ_FindTypeAt OBJ_FindTypeAt
#define FindInv FindInv
#define NextInv NextInv
#define FindInvType FindInvType
#define AddMapObj AddMapObj
#define DeleteObj DeleteObj
#define MoveObj MoveObj
#define ShowObjects ShowObjects
#define GetObjectString GetObjectString
#define TALK_GetCharName TALK_GetCharName
#define LoseHP LoseHP
#define GetDex GetStr
#define GetMonsterClass GetMonsterClass

#define GetType(i) (ObjShapeType[i] & 0x3ff)
#define GetFrame(i) (ObjShapeType[i] >> 10)
#define SetFrame(i, v) (ObjShapeType[i] = GetType(i) + ((v) << 10))
#define GetQuan(i) (((unsigned char *)(Amount + (i)))[0])
#define GetQual(i) (((unsigned char *)(Amount + (i)))[1])
#define GetX(i) ((*(int16 *)&(ObjPos[i].raw[0]))&0x3ff)
#define GetY(i) (((*(int16 *)&(ObjPos[i].raw[1]))>>2)&0x3ff)
#define GetZ(i) (ObjPos[i].raw[2] >> 4)
#define TILE_FRAME(i) (BaseTile[GetType(i)] + GetFrame(i))
#define IN_VEHICLE (Active == PartySize)
/*ability flags (U6's MONSTER_xxx) of a monster class: GetMonsterClass() + 1 indexes the table*/
#define MonsterFlag(cls) (MonsterFlags + 1)[cls]

/*0200*/int16 AnimFrameCounter = 0;/*animation frame counter*/
/*0202*/unsigned char StatusDirty = 1;/*status area must be redrawn*/
/*0203*/unsigned char PromptFlag = 0;/*the "<name>>" prompt must be printed again*/
/*0204*/unsigned char CyclopsFlag = 0;/*Ultima VI's pending screen shake; not used by GAME.EXE*/
/*0205*/unsigned char IsArmageddon = 0;/*Ultima VI's; only cleared (GAME_Restore)*/
/*0206*/int16 IdleAnimTick = 0;/*set by the console's idle loop, cleared after each animation*/
/*0208*/int16 MapAnimEnabled = 1;/*map animation/redraw enabled*/
/*020A*/int16 Erupting = 0;/*1: an eruption is pending (see object.c)*/
/*020C*/int16 D_020C = -1;/*step (1 or -1) of the creatures of type 0x149, reversed every 50 ticks*/
/*020E*/int16 D_020E = 30;/*ticks: counts down from 80; type 0x149 moves while <= 30*/
/*0210*/int16 D_0210 = -1;/*direction last shown by the object of type 0x10b*/
/*wall flag (TerrainType) per direction N,NE,E,SE,S,SW,W (Ultima VI's; not used here)*/
/*0212*/unsigned char DirWallFlags[] = {0x80,0,0x40,0,0x20,0,0x10};
/*hour (mod 12) -> direction index for the clock's short hand*/
/*0219*/unsigned char ClockHourToDir[] = {0,1,1,2,3,3,4,5,5,6,7,7};
/*wind direction names (Ultima VI's; not used here)*/
/*0225*/char *WindDirNames[] = {
	"C ",
	"N ",
	"NE",
	"E ",
	"SE",
	"S ",
	"SW",
	"W ",
	"NW"
};
/*0237*/unsigned char LastHourUpdate = -1;/*hour of the last hourly update (-1: none yet)*/
/*commands names, indexed by command code - 0x81*/
/*0238*/static char *CommandNames[] = {
	"Move",
	"Get",
	"Drop",
	"Use",
	"Talk",
	"Look",
	"Attack",
	"Rest",
	"Combat",
	"Save"
};

/*set the animation state of an animated tile
mode: 0 stop, 1 normal, 2 reverse*/
/*C_0359_0006*/SetTileAnimation(int16 tileNum, int16 mode) {
	int16 i;

	for(i = 0; i < NumAnimData; i ++) {
		if(AnimTile[i] == tileNum) {
			StateAnimData[i] = mode;
			break;
		}
	}
}

/*animate the tiles, show the direction given by the object of type 0x10b
carried by the member shown in the status area, and animate some creatures
near the party (each type its own way)*/
/*C_0359_0030*/AnimateTilesAndActors() {
	int16 i, frm;
	int16 x, y, objTyp, chance, obj, dx, dy;

	chance = 0;
	AnimFrameCounter ++;

	/*-- animated tiles --*/
	for(i = 0; i < NumAnimData; i++) {
		frm = AnimFirstFrame[i];
		if(StateAnimData[i] == 1/*normal*/)
			frm += (AnimFrameCounter & AnimFrameMask[i]) >> AnimFrameShift[i];
		else if(StateAnimData[i] == 2/*reverse*/)
			frm += (~AnimFrameCounter & AnimFrameMask[i]) >> AnimFrameShift[i];
		GR_SET_ANIM_FRAME(AnimTile[i], frm);
	}

	/*-- the object of type 0x10b carried by the member shown in the status
	  area points to a place of the current level: its tile shows tile
	  0x158 + direction (0 north .. 7 northwest) --*/
	if((obj = FindInvType(Party[StatusMember], 0x10b, -1)) >= 0) {
		switch(MapZ) {
			case 0: dx = MapX - 0x170; dy = 0x150 - MapY; break;
			case 1: dx = MapX - 0x43; dy = 0x43 - MapY; break;
			case 2: dx = dy = 0; break;
			case 3: dx = MapX - 0x9d; dy = 0x89 - MapY; break;
		}
		if(dx || dy) {
			if(abs(dx) * 2 <= dy)
				frm = 4;
			else if(abs(dx) * 2 <= -dy)
				frm = 0;
			else if(dx > 0) {
				if(dy * 2 >= dx)
					frm = 5;
				else if(dy * 2 >= -dx)
					frm = 6;
				else
					frm = 7;
			} else {
				if(dy * 2 >= -dx)
					frm = 3;
				else if(dy * 2 >= dx)
					frm = 2;
				else
					frm = 1;
			}
			if(frm != D_0210) {
				GR_SET_ANIM_FRAME(TILE_FRAME(obj), frm + 0x158);
				STAT_DrawInventory();
				D_0210 = frm;
			}
		}
	}

	if(OSI_rand(0, 15) == 0)
		chance = 1;
	if(D_020E <= 30) {
		if(D_020C <= 0)
			D_020C = 1;
		else
			D_020C = -1;
		D_020E = 80;
	} else {
		D_020E --;
	}

	/*-- creatures --*/
	for(i = 0; i < 0x100; i ++) {
		objTyp = GetType(i);
		if(GetZ(i) != MapZ)
			continue;
		if(abs(GetX(i) - MapX) >= 6)
			continue;
		if(abs(GetY(i) - MapY) >= 6)
			continue;
		frm = GetFrame(i);
		if(objTyp >= 0x136 && objTyp <= 0x146) {
			if((frm + 1) % 4 && OSI_rand(0, 30) == 0) {
				SetFrame(i, (frm >> 2 << 2) + OSI_rand(0, 2));
				VisibilityDirty = 1;
			}
		} else if(objTyp == 0x123) {
			if(OSI_rand(0, 5) == 0) {
				SetFrame(i, OSI_rand(0, 3));
				VisibilityDirty = 1;
			}
		} else if(
			objTyp == 0x150 || objTyp == 0x151 || objTyp == 0x135 || objTyp == 0x134 ||
			objTyp == 0x133 || objTyp == 0x12e || objTyp == 0x12b
		) {
			if(OSI_rand(0, 30) == 0) {
				SetFrame(i, (frm >> 1 << 1) + OSI_rand(0, 1));
				VisibilityDirty = 1;
			}
		} else if(
			objTyp == 0x14f || objTyp == 0x11b || objTyp == 0x12c || objTyp == 0x147 ||
			objTyp == 0x14a || objTyp == 0x14b || objTyp == 0x14c || objTyp == 0x14d ||
			objTyp == 0x14e || objTyp == 0x14f
		) {
			if(OSI_rand(0, 30) == 0) {
				SetFrame(i, (frm >> 2 << 2) + (OSI_rand(0, 1) << 1) + 1);
				VisibilityDirty = 1;
			}
		} else if(objTyp == 0x128 || objTyp == 0xed) {
			if(OSI_rand(0, 30) == 0) {
				SetFrame(i, (frm >> 3 << 3) + (OSI_rand(0, 1) << 2) + 3);
				VisibilityDirty = 1;
			}
		} else if(objTyp == 0x120) {
			if(OSI_rand(0, 30) == 0) {
				SetFrame(i, frm / 6 * 6 + (OSI_rand(0, 2) << 1) + 1);
				VisibilityDirty = 1;
			}
		} else if(objTyp == 0x149) {
			/*moves one step (D_020C) in the direction of its NPC mode*/
			if(D_020E <= 30) {
				switch(NPCMode[i]) {
					case 0x8b: x = GetX(i); y = GetY(i) - D_020C; break;
					case 0x8c: x = GetX(i) + D_020C; y = GetY(i); break;
					case 0x8d: x = GetX(i); y = GetY(i) + D_020C; break;
					case 0x8e: x = GetX(i) - D_020C; y = GetY(i); break;
					default: x = GetX(i); y = GetY(i);
				}
				if(CanMoveTo(i, x, y)) {
					MoveObj(i, x, y, MapZ);
					VisibilityDirty = 1;
				}
			}
			if(GetQual(i) == 0) {
				if((frm + 1) % 4 == 0)
					SetFrame(i, frm >> 2 << 2);
				else
					SetFrame(i, frm + 1);
				GetQual(i) = D_020E >> 3;
				VisibilityDirty = 1;
			} else {
				GetQual(i) = GetQual(i) - 1;
			}
		} else if(objTyp == 0x11e && (frm & 2)) {
			/*two parts: this one and the one 2 tiles to the west*/
			if(GetQual(i) == 0) {
				VisibilityDirty = 1;
				if(frm == 15) {
					if(chance) {
						SetFrame(i, 3);
						SetFrame(OBJ_FindTypeAt(0x11e, GetX(i) - 2, GetY(i)), 1);
					}
				} else {
					SetFrame(i, frm + 4);
					SetFrame(OBJ_FindTypeAt(0x11e, GetX(i) - 2, GetY(i)), frm + 2);
				}
				GetQual(i) = 5;
			} else {
				GetQual(i) = GetQual(i) - 1;
			}
		} else if(objTyp == 0x11f) {
			if(GetQual(i) == 0) {
				VisibilityDirty = 1;
				if(frm > 11) {
					if(OSI_rand(0, 15) == 0 && FindLoc(GetX(i) + 1, GetY(i), MapZ) == -1) {
						SetFrame(i, 11);
						GetQuan(i) = 1;
						MoveObj(i, GetX(i) + 1, GetY(i), MapZ);
					} else if(frm == 15) {
						SetFrame(i, 12);
					} else {
						SetFrame(i, frm + 1);
					}
				} else if(frm == 11) {
					if(GetQuan(i) == 1) {
						SetFrame(i, 3);
						GetQuan(i) = 0;
					} else {
						MoveObj(i, GetX(i) - 1, GetY(i), MapZ);
						SetFrame(i, 12);
					}
				} else if(OSI_rand(0, 15) == 0) {
					SetFrame(i, 11);
					GetQuan(i) = 0;
				} else if(frm == 3) {
					SetFrame(i, 7);
				} else {
					SetFrame(i, 3);
				}
				GetQual(i) = 10;
			} else {
				GetQual(i) = GetQual(i) - 1;
			}
		} else if(objTyp == 0x148 && frm > 3 && OSI_rand(0, 3) == 0) {
			SetFrame(i, OSI_rand(0, 3) + 4);
			VisibilityDirty = 1;
		}
	}/*end for*/

	IdleAnimTick = 0;
}

/*draw the hands of a clock tile located at screen (x,y) of the map window*/
/*C_0359_0A7E*/DrawClockHands(int16 x, register int16 y) {
	register int16 dirIdx;

	x += 4;
	y += 14;
	GR_SET_COLOR(ColorBlack);
	GrInfo->toBackBuffer = 1;
	/*centre*/
	GR_PUT_PIXEL(x, y);
	/*short hand*/
	dirIdx = ClockHourToDir[Time_H % 12];
	GR_PUT_PIXEL(x + DirIncrX[dirIdx], y + DirIncrY[dirIdx]);
	/*long hand (60 minutes -> 8 directions)*/
	dirIdx = (Time_M * 2) / 15;
	GR_PUT_PIXEL(x + DirIncrX[dirIdx], y + DirIncrY[dirIdx]);
	GrInfo->toBackBuffer = 0;
}

/*draw the 11x11 map window into the back buffer (tiles + objects).
If showOnScreen: also draw the target cursor and the mouse pointer, and
copy (or fade in) the window to the screen.*/
/*C_0359_0B23*/DrawMapWindow(int16 showOnScreen/*0:offScreen,1:onScreen*/) {
	int16 i, j;
	int16 cellObj, scr_x, scr_y, tile, mouseInMap;

	for(j = 0; j < 11; j++) {
		for(i = 0; i < 11; i++) {
			scr_x = TIL2SCR(i);
			scr_y = TIL2SCR(j);
			if(MouseOn) MOUSE_SERVE();
			/*tiles*/
			GR_PUT_TILE(Tile_11x11[j][i], scr_x, scr_y);
			/*objects: linked list of tiles to draw in this cell*/
			for(cellObj = Obj_11x11[j][i]; cellObj; cellObj = CellObjNext[cellObj]) {
				tile = CellObjTile[cellObj];
				GR_PUT_TILE(tile, scr_x, scr_y);
			}
		}
	}
	if(!showOnScreen)
		return;
	if(MouseMode == 0 && !FreeAimMode) {
		DrawSlotCursor(0);
		SelectMode = 0;
	}
	if(SelectMode == 1) {
		/*target cursor*/
		if(SelectRange < 0)
			tile = 0x17c;/*direction*/
		else
			tile = 0x17d;/*select*/
		GR_PUT_TILE(tile, AimX * 16, AimY * 16);
	}
	/*mouse pointer over the map: draw it in the back buffer too*/
	if(MouseOn) {
		MouseArea.left = 8;
		MouseArea.right = 167;
		MouseArea.top = 16;
		MouseArea.bottom = 159;
		mouseInMap = MOUSE_IN_AREA(&MouseArea);
		if(mouseInMap) {
			Mouse.surface = &MapSurface;
			Mouse.posX -= 8;
			Mouse.posY -= 8;
			MOUSE_SAVE_BG();
			MOUSE_DRAW();
			Mouse.posX += 8;
			Mouse.posY += 8;
			Mouse.surface = &Screen;
		}
	}
	/*display the map part*/
	if(ScreenFade) {
		GR_FADE_TO_SCREEN(8, 16, 167, 159, 8, 16);
		ScreenFade = 0;
	} else {
		GR_COPY_TO_SCREEN(8, 16, 167, 159, 8, 16);
	}
}

/*one display "tick": recompute the view if needed, animate, redraw the map
window; the ground shakes near (0x9d,0x89) of level 3 once story flag
TalkFlags[1] & 4 is set*/
/*C_0359_0CD4*/DisplayTick() {
	int16 dx, dy;

	if(VisibilityDirty) {
		VIS_UpdateView();
		VisibilityDirty = 0;
	}
	if(MapAnimEnabled) {
		AnimateTilesAndActors();
		DrawMapWindow(1);
	}
	if((TalkFlags[1] & 4) && MapZ == 3) {
		dx = abs(MapX - 0x9d);
		dy = abs(0x89 - MapY);
		if(OSI_rand(0, ((dx > dy?dx:dy) >> 2) + 1) == 0)
			ShakeScreen(4, 1);
	}
}

/*redraw the map window, with the objects rebuilt; fade it in if mode is 1*/
/*C_0359_0D58*/C_0359_0D58(int16 mode) {
	DrawMapWindow(1);
	if(mode == 1) {
		ShowObjects();
		ScreenFade = 1;
	} else {
		ShowObjects();
		ScreenFade = 0;
	}
	DrawMapWindow(1);
}

/*if the view is dirty, recompute it and redisplay the map now*/
/*C_0359_0D8D*/FlushViewIfDirty() {
	if(VisibilityDirty) {
		VIS_UpdateView();
		VisibilityDirty = 0;
		DisplayTick();
	}
}

/*apply the effect of harmful tile "tileNum" to the creature objNum*/
/*C_0359_0DA8*/TerrainDamage(register int16 objNum, int16 tileNum) {
	int16 isImune;
	int16 objCls, dmg;

	isImune = 0;
	NPCFlag[objNum] |= 8;/*terrain handled for this turn*/
	if(!(NPCStatus[objNum] & 0x10)) {/*not dead*/
		objCls = GetMonsterClass(GetType(objNum));
		/*fire: 1d40, doubled for the fire-sensitive*/
		if(IsLavaTile(tileNum)) {
			if(NPCStatus[objNum] & 1)/*protected*/
				isImune = 1;
			if(MonsterFlag(objCls) & 8)
				isImune = 1;
			if(!isImune) {
				dmg = OSI_rand(1, 40);
				if(MonsterFlag(objCls) & 0x10)
					dmg <<= 1;
				FlushViewIfDirty();
				LoseHP(objNum, dmg);
			}
		}
		switch(tileNum) {
			case 0x3a7:
				if(GetType(objNum) == 0x143 || GetType(objNum) == 0x148)
					LoseHP(objNum, OSI_rand(1, 7));
			break;
			case 0x210: case 0x211: case 0x212: case 0x213: case 0x214:
			case 0x215: case 0x216: case 0x217: case 0x218: case 0x219:
				/*web: stuck unless DEX >= 1d30*/
				if(!(NPCStatus[objNum] & 1)) {
					if(GetDex(objNum) < OSI_rand(1, 30) && GetType(objNum) != 0x150) {
						NPCStatus[objNum] |= 2;/*paralyzed*/
						CON_printf(StuckInWebMsg, GetObjectString(objNum));
						PromptFlag = 1;
					}
				}
			break;
			case 0x383: case 0x406:
			case 0x42b: case 0x42c: case 0x42d: case 0x42e: case 0x42f:
			case 0x434: case 0x435: case 0x436: case 0x437:
				/*1d15, doubled for the fire-sensitive*/
				if(NPCStatus[objNum] & 1)
					isImune = 1;
				if(MonsterFlag(objCls) & 8)
					isImune = 1;
				if(!isImune) {
					dmg = OSI_rand(1, 15);
					if(MonsterFlag(objCls) & 0x10)
						dmg <<= 1;
					FlushViewIfDirty();
					LoseHP(objNum, dmg);
				}
			break;
			case 0x54a: case 0x54b: case 0x54c: case 0x54d: case 0x54e: case 0x54f:
				/*1d5, not for type 0x11a*/
				if(NPCStatus[objNum] & 1)
					isImune = 1;
				if(GetType(objNum) == 0x11a)
					isImune = 1;
				if(!isImune) {
					dmg = OSI_rand(1, 5);
					FlushViewIfDirty();
					LoseHP(objNum, dmg);
				}
			break;
		}
	}
}

/*check whether creature objNum stands on a damaging tile (object in the
same cell, or map tile) and apply it. Type 0xe9, flying ethereal monsters,
creatures already handled this turn, on another level or outside the
loaded 40x40 area are skipped.*/
/*C_0359_1014*/ApplyTerrainEffects(int16 objNum) {
	int16 otherObj;
	int16 tileNum, x, y, area_x, area_y, wrapMask, objTyp, objCls;

	objTyp = GetType(objNum);
	objCls = GetMonsterClass(objTyp);
	if(objTyp == 0xe9 || objCls >= 0 && (MonsterFlag(objCls) & 0x100) && (MonsterFlag(objCls) & 0x400))
		return;
	if(!(NPCFlag[objNum] & 8) && GetZ(objNum) == MapZ) {

		/*coordinates wrap at 1024 on the surface, 256 underground*/
		if(MapZ)
			wrapMask = 0xff;
		else
			wrapMask = 0x3ff;
		area_x = (MapX - 0x10) & 0x3f8;
		area_y = (MapY - 0x10) & 0x3f8;
		x = GetX(objNum);
		y = GetY(objNum);
		if(((x - area_x) & wrapMask) < 40 && ((y - area_y) & wrapMask) < 40) {
			for(otherObj = FindLoc(x, y, MapZ); otherObj >= 0; otherObj = NextLoc()) {
				if(otherObj == objNum)
					continue;
				tileNum = TILE_FRAME(otherObj);
				if(TileFlag2[tileNum] & 4)
					return;
				if((TerrainType[tileNum] & 8) || GetType(otherObj) == 0x124) {
					ObjStatus[otherObj] &= ~2;/*reveal the trap*/
					TerrainDamage(objNum, tileNum);
					return;
				}
			}
			tileNum = GetTileAtXYZ(x, y, MapZ);
			if(TerrainType[tileNum] & 8)
				TerrainDamage(objNum, tileNum);
		}
	}
}

/*world clock: advance the game time by "minutes" (0 = just refresh the
time-dependent state) and process everything that depends on it*/
/*C_0359_11B6*/PassTime(int16 minutes) {
	int16 i, sunTileOrLight;
	int16 minute, newMinutes, invObj, torchLit, wearsStormCloak;
	int16 obj;/*never set (used once, see below)*/

	/*spells*/
	for(i = 0; i < 2; i ++) {
		if(SpellFx[i]) {
			if(SpellFx[i] <= minutes)
				SpellFx[i] = 0;
			else
				SpellFx[i] -= minutes;
		}
	}
	/*explosive's fuse*/
	if(KegFuse > minutes) {
		KegFuse -= minutes;
	} else if(KegFuse) {
		KegFuse = 0;
		Detonate(KegObject);
	}
	/*possible eruption*/
	if(OSI_rand(0, 7) == 0)
		Erupting = 1;
	/* */
	torchLit =
	wearsStormCloak = 0;/*Ultima VI's storm cloak: not used any more*/
	for(i = 0; i < 0x100; i++) {
		if(ObjShapeType[i] == 0)
			continue;

		/*-- lit torches (type 0xd1) of party members --*/
		if(NPCStatus[i] & 0x80) {
			for(invObj = FindInv(i); invObj >= 0; invObj = NextInv()) {
				if((ObjStatus[invObj] & 0x18) != 0x18)
					continue;
				if(GetType(invObj) == 0xd1) {
					if(OSI_rand(0, 1)) {
						if(GetQual(invObj) <= minutes) {
							CON_printf(BurnOutMsg);
							DeleteObj(invObj);
							PromptFlag = 1;
						} else {
							torchLit = 1;
							GetQual(invObj) -= minutes;
						}
					} else {
						torchLit = 1;
					}
				}
			}
		}

		/*-- status effects wearing off, one roll per minute --*/
		for(minute = 0; minute < minutes; minute ++) {
			if(i == D_EADE && i && !InCombat) {
				NPCMode[D_EADE] = 0x13;
				NPCStatus[D_EADE] = (NPCStatus[D_EADE] & ~0x60) | 0x60;
				D_EADE = 0;
			}
			if((NPCStatus[i] & 1) && OSI_rand(0, 25) == 0)
				NPCStatus[i] &= ~1;
			if((ObjStatus[i] & 0x80) && !InCombat)
				ObjStatus[i] &= ~0x80;
			if((ObjStatus[i] & 0x40) && OSI_rand(0, 25) == 0)
				ObjStatus[i] &= ~0x40;
			if((ObjStatus[i] & 4) && OSI_rand(0, 25) == 0)
				COMBAT_stopCharm(i);
			if((NPCStatus[i] & 2) && OSI_rand(0, 3) == 0 && GetDex(i) >= OSI_rand(1, 30))
				NPCStatus[i] &= ~2;
			if(
				(NPCStatus[i] & 4) && NPCMode[i] != 0x91 &&
				OBJ_IsLocLoaded(GetX(i), GetY(i), GetZ(i)) && OSI_rand(0, 20) == 0
			) {
				/*wakes up; the object of type 0xf6 (bed?) under "obj", a
				  variable never set, gets frame 0*/
				NPCStatus[i] &= ~4;
				NPCFlag[i] |= 8;
				for(sunTileOrLight = FindLoc(GetX(obj), GetY(obj), GetZ(obj)); sunTileOrLight >= 0; sunTileOrLight = NextLoc()) {
					if(GetType(sunTileOrLight) == 0xf6) {
						SetFrame(sunTileOrLight, 0);
						MoveObj(obj, GetX(obj), GetY(obj), GetZ(obj));
						break;
					}
				}
				ObjShapeType[i] = OrigShapeType[i];
				if(GetType(i) == 0x126) {
					DeleteObj(OBJ_FindTypeAt(0x118, GetX(i), GetY(i) + 2));
					SetFrame(i, 1);
					sunTileOrLight = AddMapObj(0xd25, 0, i << 8, GetX(i), GetY(i) - 1, MapZ);
					ObjStatus[sunTileOrLight] &= ~0x20;
					sunTileOrLight = AddMapObj(0xd27, 0, i << 8, GetX(i), GetY(i) + 1, MapZ);
					ObjStatus[sunTileOrLight] &= ~0x20;
				}
			}
			if((NPCStatus[i] & 8) && !(NPCStatus[i] & 1) && i != 0x19 && OSI_rand(0, 7) == 0)
				LoseHP(i, 1);
		}

		/*-- damaging terrain --*/
		ApplyTerrainEffects(i);
		NPCFlag[i] &= ~8;
	}

	/*-- calendar: 60 minutes, 24 hours, 28 days, 12 months --*/
	for(newMinutes = Time_M + minutes; newMinutes > 59; ) {
		newMinutes -= 60;
		/*-- +1 hour --*/
		if(++Time_H > 23) {
			Time_H = 0;
			/*-- +1 day --*/
			if(++Date_D > 28) {
				Date_D = 1;
				/*-- +1 month*/
				if(++Date_M > 12) {
					Date_M = 1;
					/*-- +1 year*/
					Date_Y ++;
				}
			}
		}

		if(NextSleep)
			NextSleep --;
		if(DrunkCounter)
			DrunkCounter --;

		/*NPC schedules*/
		UpdateSchedules();
	}
	Time_M = newMinutes;
	minute = Time_H * 60 + Time_M;/*time of day, in minutes*/
	/*hourly updates*/
	if(LastHourUpdate != Time_H) {
		minutes = 15;
		/*some flags (not on the very first call)*/
		if(LastHourUpdate != (unsigned char)-1) {
			HourlyUnused =
			CaughtFish = 0;
			MustRingBell = 1;
		}
		LastHourUpdate = Time_H;
	}
	if(minutes > 10) {
		/*the sky tile: night before 6:40 and after 17:59*/
		if(minute < 400 || minute > 1079)
			sunTileOrLight = 0x143;
		else
			sunTileOrLight = (minute - 400) / 85 + 0x148;
		GR_SET_ANIM_FRAME(BaseTile[0x92] + 3, sunTileOrLight);
		DrawSkyBar();
	}
	/*ambiant light*/
	sunTileOrLight = AmbientLight;
	if(MapZ == 2 && !(TalkFlags[1] & 4))
		AmbientLight = 2;
	else if(MapZ > 0)
		AmbientLight = 0;
	else if(minute < 400 || minute > 1080)
		AmbientLight = 1;
	else if(minute < 440)
		AmbientLight = (minute - 400) / 10 + 2;
	else if(minute > 1040)
		AmbientLight = (1080 - minute) / 10 + 2;
	else
		AmbientLight = 7;
	if(AmbientLight < 2 && (torchLit || SpellFx[0]))
		AmbientLight = 2;
	if(AmbientLight != sunTileOrLight)
		VIS_UpdateView();
}

/*redraw the status area (party list, portrait, equipment or the
conversation portrait) if StatusDirty; in solo mode the displayed member is
forced to the solo character*/
/*C_0359_1806*/RefreshStatus() {
	if(StatusDirty) {
		if(SoloMember >= 0)
			StatusMember = SoloMember;
		switch(StatusDisplay) {
			case 0x91: STAT_ShowParty(0); break;
			case 0x90: STAT_ShowPortrait(StatusMember); break;
			case 0x92: STAT_ShowInventory(StatusMember); break;
			case 0x9e: ShowPortrait(Party[StatusMember]); break;
		}
		DrawSlotCursor(1);
	}
	StatusDirty = 0;
}

/*make party member #partyIdx the controlled character and centre the map
on him (partyIdx == PartySize: the vehicle)*/
/*C_0359_1889*/SetActive(int16 partyIdx) {
	if(Active == partyIdx)
		return;
	Active = partyIdx;
	MapX = GetX(Party[partyIdx]);
	MapY = GetY(Party[partyIdx]);
	MapZ = GetZ(Party[partyIdx]);
	MAP_LoadAround();
	MapAnimEnabled = 1;
	PromptFlag = 1;
}

/*back to party mode: restore the AI mode saved for the solo character,
the leader (Party[0]) is commanded, the others follow*/
/*C_0359_1905*/SetPartyMode() {
	int16 objNum, i;

	if(SoloMember < 0)
		NPCMode[Party[0]] = SavedNPCMode;
	else
		NPCMode[Party[SoloMember]] = SavedNPCMode;
	SavedNPCMode = NPCMode[Party[0]];
	NPCMode[Party[0]] = 2;/*command*/
	for(i = 1; i < PartySize; i ++) {
		objNum = Party[i];
		if(!(ObjStatus[objNum] & 4))
			NPCMode[objNum] = 1;/*follow*/
		if(InCombat && (ObjStatus[objNum] & 0x80))
			NPCMode[objNum] = 6;
	}
	SetActive(0);
	SoloMember = 0xff;/*i.e. -1: party mode*/
	DrawSlotCursor(0);
}

/*read an hexadecimal number of up to "len" (max 4) digits from the
keyboard (debug commands); no validation of the digits*/
/*C_0359_19A6*/ReadHexNumber(int16 len) {
	int16 i, value;

	if(len > 4)
		len = 4;
	CON_gets(HexInputBuf, len);
	value = 0;
	for(i = 0; i < len; i++) {
		if(HexInputBuf[i] == 0)
			break;
		value <<= 4;
		if(HexInputBuf[i] >= '0' && HexInputBuf[i] <= '9')
			value += HexInputBuf[i] - '0';
		else
			value += toupper(HexInputBuf[i]) - ('A' - 10);
	}
	return value;
}

/*read a decimal number of up to "len" (max 5) digits from the keyboard;
stops at the first non digit*/
/*C_0359_1A0C*/ReadDecNumber(int16 len) {
	int16 i, value;

	if(len > 5)
		len = 5;
	CON_gets(DecInputBuf, len);
	value = 0;
	for(i = 0; i < len; i++) {
		if(DecInputBuf[i] == 0 || DecInputBuf[i] < '0' || DecInputBuf[i] > '9')
			break;
		value = DecInputBuf[i] + value * 10 - '0';
	}
	return value;
}

/*game loop:
one iteration per player turn: read a key (CON_getch also returns mouse
generated command codes and runs the animations while waiting), translate
letter keys to commands, execute, then do the end of turn processing.
Exits on <ALT+X>/<CTRL+Q>/<CTRL+E> confirmed (ch == 0x12D).*/
/*C_0359_1A65*/GameLoop() {
	register int16 ch;
	int16 npc;
	int16 key, member;

	/*new game: the introduction conversation (NPC 0x21)*/
	if(!(TalkFlags[0x21] & 1)) {
		TALK_talkTo(Active, 0x21, 0);
		Date_D = 1;
		Date_M = 1;
		Date_Y = 0;
		RefreshStatus();
	}
	HatchingDisabled = 0;
	InitNPCTracker();
	PassTime(0);
	VIS_UpdateView();
	ScreenFade = 1;
	DrawMapWindow(1);
	CON_printf("%s%>", Names[Active]);
	do {
		RefreshStatus();

		if(MouseMode == 0)
			AllowMouseMov = 1;
		ch = CON_getch();
		AllowMouseMov = 0;

		/*keyboard commands: start a target selection (cursor at the
		  centre of the map window, SelectRange < 0 = direction only)*/
		switch(ch) {
			case 'A'/*ttack*/:
				ch = 0x87;
				DrawSlotCursor(0);
				FreeAimMode = 0;
				SelectMode = 1;
				AimX =
				AimY = 5;
				SelectRange = 7;
				/*aim at the previous target if still on screen*/
				if(LastAttackTarget >= 0) {
					AimX = (GetX(LastAttackTarget) & 0xff) - MapX + 5;
					AimY = (GetY(LastAttackTarget) & 0xff) - MapY + 5;
					if(AimX >= 0 && AimX <= 10 && AimY >= 0 && AimY <= 10)
						break;
					AimX =
					AimY = 5;
				}
			break;
			case 'T'/*alk*/:
				ch = 0x85;
				DrawSlotCursor(0);
				FreeAimMode = 0;
				SelectMode = 1;
				AimX =
				AimY = 5;
				SelectRange = 7;
			break;
			case 'L'/*ook*/:
				ch = 0x86;
				DrawSlotCursor(0);
				FreeAimMode = 0;
				SelectMode = 1;
				AimX =
				AimY = 5;
				SelectRange = 7;
			break;
			case 'G'/*et*/:
				ch = 0x82;
				DrawSlotCursor(0);
				FreeAimMode = 0;
				SelectMode = 1;
				AimX =
				AimY = 5;
				SelectRange = -1;
			break;
			case 'D'/*rop*/:
				/*selection in the inventory: show the equipment of the
				  active character*/
				ch = 0x83;
				DrawSlotCursor(0);
				FreeAimMode = 0;
				SelectMode = 2;
				AimX =
				AimY = 5;
				SelectRange = 7;
				SlotCursorCol = 3;
				SlotCursorRow = 0;
				if(!IN_VEHICLE && (StatusDisplay != 0x92 || Active != StatusMember)) {
					StatusDisplay = 0x92;
					StatusMember = Active;
					StatusDirty = 1;
					RefreshStatus();
				}
				DrawSlotCursor(1);
			break;
			case 'M'/*ove*/:
				ch = 0x81;
				DrawSlotCursor(0);
				FreeAimMode = 0;
				SelectMode = 1;
				AimX =
				AimY = 5;
				SelectRange = -1;
			break;
			case 'U'/*se*/:
				ch = 0x84;
				DrawSlotCursor(0);
				FreeAimMode = 0;
				SelectMode = 1;
				AimX =
				AimY = 5;
				SelectRange = -1;
			break;
			case 'R'/*est*/:
				ch = 0x88;
				DrawSlotCursor(0);
				FreeAimMode = 0;
				SelectMode = 1;
				AimX =
				AimY = 5;
				SelectRange = 7;
			break;
			case 19:/*<CTRL+S>*/
				ch = 0xaa;
			break;
			case 'B'/*egin/break combat*/:
				ch = 0x89;
			break;
		}

		/*character switch: '0' = party mode, '1'..'9' = solo mode*/
		if(ch >= '0' && ch <= '9' && ch < PartySize + '1') {
			if(ch == '0') {/*Party Mode*/
				if(IN_VEHICLE) {
					CON_printf(NotOnShipMsg);
				} else if(InCombat) {
					CON_printf(NotInCombatMsg);
				} else {
					/*everybody must be within 8 tiles*/
					for(member = 0; member < PartySize; member ++) {
						if(!CLOSE_ENOUGH(8, MapX, MapY, GetX(Party[member]), GetY(Party[member]))) {
							CON_printf(NotAllHereMsg);
							break;
						}
					}
					if(PartySize == member) {
						CON_printf(PartyModeMsg);
						SetPartyMode();
						MouseMode = 0;
						SelectMode = 0;
						PendingCommand = 0x8f;
						CON_UpdateMousePointer();
					}
				}
			} else {/*Solo Mode*/
				if(
					(NPCStatus[Party[ch - '1']] & 0x16) ||
					(NPCFlag[Party[ch - '1']] & 0x10) ||
					(ObjStatus[Party[ch - '1']] & 4)
				) {
					CON_printf(FailsToRespondMsg, Names[ch - '1']);
				} else if(IN_VEHICLE) {
					CON_printf(NotOnShipMsg);
				} else if(InCombat) {
					CON_printf(NotInCombatMsg);
				} else {
					/*restore the AI mode of the previously commanded
					  character and save the one of the new solo character*/
					if(SoloMember < 0)
						NPCMode[Party[Active]] = SavedNPCMode;
					else
						NPCMode[Party[SoloMember]] = SavedNPCMode;
					SavedNPCMode = NPCMode[Party[ch - '1']];
					/*followers stay where they are*/
					for(member = 0; member < PartySize; member ++) {
						if(NPCMode[Party[member]] == 1)
							NPCMode[Party[member]] = 0;
					}
					SetActive(ch - '1');
					NPCMode[Party[Active]] = 2;
					SoloMember = Active;
					if(StatusDisplay != 0x91)
						StatusDirty = 1;
					CON_printf(SoloModeMsg);
					MouseMode = 0;
					DrawSlotCursor(0);
					SelectMode = 0;
					PendingCommand = 0x8f;
					CON_UpdateMousePointer();
				}
			}
		} else/*the other commands*/
		switch(ch) {
			case 0x80:/*[advance]*/
				PartyAdvance(D_B5E6);
			break;
			case 5: case 17: case 0x12D:/*<CTRL+E> <CTRL+Q> <ALT+X> --"exit to dos?"*/
				CON_printf(ExitToDOSMsg);
				while((key = CON_getch()) != 'Y' && key != 'N');
				CON_printf("%c\n", key);
				if(key == 'N')
					ch = 0;
				else
					ch = 0x12D;
			break;
			case 0x87:/*"attack"*/
				if(IN_VEHICLE) {
					CON_printf(NotOnShipMsg);
					break;
				}
				COMBAT_attack();
				PendingCommand = 0x8f;
			break;
			case 0x89:/*begin/break combat*/
				if(SoloMember >= 0) {
					CON_printf(NotInSoloMsg);
				} else if(InCombat) {/*break off*/
					CON_printf(BreakOffMsg);
					COMBAT_breakOff();
				} else if(IN_VEHICLE) {
					CON_printf(NotOnShipMsg);
				} else {
					CON_printf(CombatMsg);
					COMBAT_begin();
				}
			break;
			case 0x81:
			case 0x82:
			case 0x83:
				/*not allowed aboard a vehicle*/
				if(IN_VEHICLE) {
					CON_printf("%s-", CommandNames[ch - 0x81]);
					CON_printf(NotOnShipMsg);
					break;
				}
			/*break;*/
			case 0x84:
			case 0x85:
			case 0x86:
				/*wait for the target (0x8e = selection done)*/
				MouseMode = 1;
				PendingCommand = ch;
				CON_UpdateMousePointer();
				CON_printf("%s-", CommandNames[ch - 0x81]);
				key = CON_getch();
				if(key == 0x8e) {
					switch(PendingCommand) {
						case 0x86: CmdLook(); break;/*"look"*/
						case 0x83: CmdDrop(); break;/*"drop"*/
						case 0x82: CmdGet(); break;/*"get"*/
						case 0x81: CmdMove(); break;/*"move"*/
						case 0x84: CmdUse(); break;/*"use"*/
						case 0x85:/*"talk"*/
							MouseMode = 0;
							TALK_talkTo(Active, D_B642, 1);
						break;
					}
				} else {
					CON_printf(WhatMsg);
				}
				MouseMode = 0;
				PendingCommand = 0x8f;
				DrawSlotCursor(0);
				SelectMode = 0;
				CON_UpdateMousePointer();
			break;
			case 0x88:/*"rest"*/
				if(IN_VEHICLE) {
					CON_printf(NotOnShipMsg);
					break;
				}
				REST_Command();
				MouseMode = 0;
				DrawSlotCursor(0);
				SelectMode = 0;
				CON_UpdateMousePointer();
			break;
			case 0xaa:/*"save game"*/
				if(IN_VEHICLE) {
					CON_printf(NotOnShipMsg);
					break;
				}
				if(InCombat) {
					CON_printf(NotInCombatMsg);
					break;
				}
				CON_printf(SaveMsg);
				while((key = CON_getch()) != 'Y' && key != 'N');
				CON_printf("%c\n", key);
				if(key == 'Y') {
					GAME_Save();
					GAME_Restore();
					DrawSlotCursor(0);
					MouseMode = 0;
					SelectMode = 0;
					CON_UpdateMousePointer();
				}
			break;
			case 18:/*<CTRL+R> "restore game"*/
				CON_printf("Restore game? ");
				while((key = CON_getch()) != 'Y' && key != 'N');
				CON_printf("%c\n", key);
				if(key == 'Y') {
					GAME_Restore();
					DrawSlotCursor(0);
					MouseMode = 0;
					SelectMode = 0;
					CON_UpdateMousePointer();
				}
			break;
			case 27: case ' ': case 0x8f:/*"pass"*/
				CON_printf(PassMsg);
				MovePts[Party[Active]] = 0;
				DrawSlotCursor(0);
				MouseMode = 0;
				SelectMode = 0;
				CON_UpdateMousePointer();
				MoveFollowers(Party[Active], 1);
			break;
			case 26:/*<CTRL+Z>*/
				if(SoundFlag) {
					SoundFlag = 0;
					MUS_PlaySong(-1);
					C_2701_0684();
					CON_printf(SoundOffMsg);
				} else {
					SoundFlag = 1;
					MUS_PlayAreaSong();
					CON_printf(SoundOnMsg);
				}
			break;
			case 8:/*<CTRL+H>*/
				CON_printf("Help ");
				HelpOn = !HelpOn;
				CON_printf(HelpOn?"on":"off");
				CON_printf("!\n");
			break;
			case 22:/*<CTRL+V> version string*/
				CON_printf(VersionMsg);
			break;
			case 0xd5: case 0xd6: case 0xd7: case 0xde: case 0xe0:
			case 0xe8: case 0xe9: case 0xf2: case 0xfc:/*<alt 2 x x> debug*/
				if(TalkFlags[1] & 0x40) {
					switch(ch) {
						int16 num, num2, isOn;/*after the switch's own temporary*/
						char *name;

						case 0xd5:/*time, position, peer*/
							CON_printf("\nTIME %02d:%02d\nX%03x Y%03x Z%1x\n", Time_H, Time_M, MapX, MapY, MapZ);
							PeerMap();
						break;
						case 0xe9:/*sound effect*/
							if(SoundFlag) {
								CON_printf("SFX number: ");
								while((npc = CON_getch()) < '0' || npc > '9');
								CON_printf("%c", npc);
								while((num = CON_getch()) < '0' || num > '9');
								CON_printf("%c\n", num);
								npc -= '0';
								num -= '0';
								MUS_PlaySfx((npc *= 10) + num);
								break;
							}
						break;
						case 0xf2:/*talk flag bits*/
							CON_printf("NPC: ");
							npc = ReadHexNumber(2);
							name = TALK_GetCharName(npc);
							CON_printf("%s\n", name);
							CON_printf("\nBit: ");
							if((num = ReadHexNumber(1)) < 8)
								CON_printf("%d is ", num);
							else
								CON_printf("WAIT flag ");
							isOn = num < 8 ? TalkFlags[npc] & (1 << num) : D_3298[npc] & 0x80;
							if(isOn)
								CON_printf("on.");
							else
								CON_printf("off.");
							CON_printf("\nNew value? ");
							if((num2 = ReadHexNumber(1)) != 0) {
								if(num < 8)
									TalkFlags[npc] |= 1 << num;
								else
									D_3298[npc] |= 0x80;
								CON_printf("on.");
							} else {
								if(num < 8)
									TalkFlags[npc] &= ~(1 << num);
								else
									D_3298[npc] &= ~0x80;
								CON_printf("off.");
							}
						break;
						case 0xe8:/*show an NPC*/
							CON_printf("NPC: ");
							npc = ReadHexNumber(2);
							num = (int16)TALK_GetCharName(npc);/*the name goes through num*/
							CON_printf("%s\n", num);
							ShowPortrait(npc);
						break;
						case 0xd6:/*go to coordinate (hexadecimal x,y,z)*/
							if(IN_VEHICLE) {
								CON_printf(NotOnShipMsg);
							} else {
								CON_printf("\nGoto x: ");
								num = ReadHexNumber(3);
								CON_printf("\n     y: ");
								npc = ReadHexNumber(3);
								CON_printf("\n     z: ");
								if((num2 = ReadHexNumber(1)) != 0) {
									num &= 0xff;
									npc &= 0xff;
								} else {
									num &= 0x3ff;
									npc &= 0x3ff;
								}
								if(num2 > 5)
									num2 = 5;
								PartyTeleport(num, npc, num2, 0);
								MUS_PlayAreaSong();
							}
						break;
						case 0xe0:/*go to an NPC*/
							if(IN_VEHICLE) {
								CON_printf(NotOnShipMsg);
								break;
							}
							CON_printf("\nNPC: ");
							npc = ReadHexNumber(2);
							if(!(NPCStatus[npc] & 0x10) && npc <= 0x98) {
								num = (int16)TALK_GetCharName(npc);
								CON_printf("%s\n", num);
								PartyTeleport(GetX(npc) + 1, GetY(npc) + 1, GetZ(npc), 0);
								MUS_PlayAreaSong();
							} else {
								CON_printf("invalid");
							}
						break;
						case 0xd7:/*time += 60 minutes*/
							PassTime(60);
							AllowNPCTeleport = 1;
							PTH_SearchPaths();
							AllowNPCTeleport = 0;
						break;
						case 0xfc:/*memory*/
							CON_printf("%dK free for tiles\n\n", D_3296);
						break;
						case 0xde:/*"solid" avatar*/
							if(SpeakGargish) {
								CON_printf("Avatar solidifies!\n");
								SpeakGargish = 0;
							} else {
								CON_printf("Avatar desolidifies!\n");
								SpeakGargish = 1;
							}
						break;
					}
				} else if(ch == 0xd5) {
					CON_printf("\nTIME %02d:%02d\nX%03x Y%03x Z%1x\n", Time_H, Time_M, MapX, MapY, MapZ);
				} else {
					CON_printf(WhatMsg);
					DrawSlotCursor(0);
					MouseMode = 0;
					SelectMode = 0;
					CON_UpdateMousePointer();
				}
			break;
			default:
				CON_printf(WhatMsg);
				DrawSlotCursor(0);
				MouseMode = 0;
				SelectMode = 0;
				CON_UpdateMousePointer();
		}

		/*-- end of turn --*/
		if(ch != 0x12D) {
			/*"disk full" recovery choice*/
			if(DiskFullChoice == 'R')
				GAME_Restore();/*Restore game?*/
			else if(DiskFullChoice == 'G')
				GAME_Save();/*Save game?*/
			/*NPCs and the rest of the world take their turn*/
			RunNPCTurns();
			if(InCombat && (CombatOutcome == 1 || CombatOutcome == -1))
				C_2701_045E();
			if(ch != 0x80)
				PromptFlag = 1;
			if(PromptFlag) {
				CON_printf("\n%s%>", Names[Active]);
				PromptFlag = 0;
			}
		}
	} while(ch != 0x12D);
}
