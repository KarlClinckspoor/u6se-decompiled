/* flags: -d -mm -p -c -Z -O -y */
/*
 * The Savage Empire (Worlds of Ultima) - GAME.EXE
 * segment 1810 (from 1810:000D, after npctrack's last bytes): combat
 * (Ultima VI's combat.c, much rewritten).
 *
 * - combat mode on/off for the party (COMBAT_begin, COMBAT_beginAttacked
 *   when a party member is attacked, COMBAT_breakOff),
 * - derived attributes (GetStr/GetDex/GetInt with curse (ObjStatus 0x40)
 *   and ObjStatus 0x80 modifiers, MaxHP, armour and weapon tables),
 * - the "Attack" command of the player (COMBAT_attack, cycling through
 *   every equipped weapon),
 * - resolving one attack (__TryHit -> __AttackTest -> __DoHit -> LoseHP),
 *   with ranged and thrown weapons, ammunition and explosives
 *   (COMBAT_FireRangedWeapon), and the special weapon/target pairs,
 * - death (COMBAT_Kill, SetTypeUnconscious: dead bodies, creatures made of
 *   several tiles), breakable objects with their effects (LoseHP),
 * - the creatures made of segments (types 0x124/0x11A, C_1810_22F0,
 *   __TryGrowVine, COMBAT_AI_Tangle) and the boulders pushed around
 *   (type 0xA7, C_1810_14EA and its helpers),
 * - the combat AI modes (assault, range, berserk, retreat, like,
 *   unfriendly, immobile, tangle) and the choice of weapons and targets.
 * Objects 0..0xff are the actors; objects >= 0x100 are plain items.
 * Object, tile and mode numbers are Savage Empire's (not named yet).
 */
#include "se.h"

/*text window (Ultima VI's)*/
struct tClipInfo/*size:9*/ {
	unsigned char originX, originY, cols, rows;
	unsigned char curX, curY;
	unsigned char fgColor, bgColor;
	unsigned char attrFlags;
};

#undef abs

/*far tables (monsdata.c, segment 2F43, indexed by GetMonsterClass() + 1) and far messages (msgs.c, segment 2F65)*/
extern unsigned char far MonsterStr[];
extern unsigned char far MonsterDex[];
extern unsigned char far MonsterArmor[];
extern unsigned char far MonsterDamage[];
extern unsigned char far MonsterHP[];
extern int16 far MonsterFlags[];
extern char far CriticalMsg[];/*CriticalMsg*/
extern char far HeavilyMsg[];/*HeavilyMsg*/
extern char far LightlyMsg[];/*LightlyMsg*/
extern char far BarelyMsg[];/*BarelyMsg*/
extern char far WoundedMsg[];/*WoundedMsg*/
extern char far GrazedMsg[];/*GrazedMsg*/
extern char far KilledMsg[];/*KilledMsg*/
extern char far FallsMsg[];/*FallsMsg*/
extern char far BareHandsMsg[];/*BareHandsMsg*/
extern char far WithMsg[];/*WithMsg*/
extern char far OutOfArrowsMsg[];/*OutOfArrowsMsg*/
extern char far OutOfAmmoMsg[];/*OutOfAmmoMsg*/
extern char far OutOfDartsMsg[];/*OutOfDartsMsg*/
extern char far OutOfSpearsMsg[];/*OutOfSpearsMsg*/
extern char far FlintlockNotLoadedMsg[];/*FlintlockNotLoadedMsg*/
extern char far AttackSelfPassMsg[];/*AttackSelfPassMsg*/
extern char far NothingMsg[];/*NothingMsg*/
extern char far AttackWithMsg[];/*AttackWithMsg*/
extern char far AttackMsg[];/*AttackMsg*/
extern char far WhatMsg[];/*WhatMsg*/
extern char far PoisonedMsg[];/*PoisonedMsg*/
extern char far OutOfRangeMsg[];/*OutOfRangeMsg*/
extern char far NotOnShipMsg[];/*NotOnShipMsg*/
extern char far AttacksMsg[];/*AttacksMsg*/
extern char far NotPossibleMsg[];/*NotPossibleMsg*/
/*variables of other modules*/
extern char DirIncrX[];/*DirIncrX*/
extern char DirIncrY[];/*DirIncrY*/
extern unsigned char StatusDirty;/*StatusDirty*/
extern unsigned char PromptFlag;/*PromptFlag*/
extern unsigned char SelectMode;/*SelectMode*/
extern char AimX, AimY;/*AimX, AimY*/
extern char SelectRange;/*SelectRange*/
extern int16 LastNumberPlural;/*LastNumberPlural*/
extern int16 TextOn;/*TextOn*/
extern int16 LastAttackTarget;/*LastAttackTarget*/
extern int16 SkipNewlines;/*SkipNewlines*/
extern int16 StatusMember;/*StatusMember*/
extern int16 MouseMode;/*MouseMode*/
extern int16 StatusDisplay;/*StatusDisplay: 0x92 when the equipment view is shown*/
extern int16 PendingCommand;/*PendingCommand*/
extern struct tClipInfo *CurClip;/*CurClip*/
extern unsigned char PartyListTop;/*PartyListTop*/
extern unsigned char IsInConversation;/*IsInConversation*/
extern int16 VisibilityDirty;/*VisibilityDirty*/
extern int16 EnemyWarningGiven;/*EnemyWarningGiven*/
extern int16 ColorRed;/*ColorRed*/
extern int16 ColorGreen;/*ColorGreen*/
extern int16 D_24E6;/*ColorText*/
extern unsigned char Active;/*Active*/
extern int16 MapX, MapY, MapZ;/*MapX, MapY, MapZ*/
extern char InCombat;/*InCombat*/
extern char SoloMember;/*SoloMember: -1 in party mode*/
extern unsigned char SavedNPCMode;/*SavedNPCMode*/
extern unsigned char D_329C[];/*ObjStatus*/
extern char D_44FA[];/*MovePts*/
extern unsigned char far *D_45FA;/*SchedIndex*/
extern unsigned char far *D_4602;/*NPCFlag*/
extern unsigned char D_4606[];/*DEXTE*/
extern unsigned char far *D_4706;/*NPCMode*/
extern unsigned char D_4EDA[];/*Party*/
extern unsigned char far *D_4EEC;/*Level*/
extern unsigned char far *D_5100;/*Leader*/
extern char D_5104[][14];/*Names*/
extern uint16 D_51E4[];/*ExpPoints*/
extern unsigned char far *D_53E4;/*NPCComMode*/
extern unsigned char D_53E8[];/*STREN*/
extern int16 D_54F0[];/*Amount*/
extern unsigned char D_6CF0[];/*NPCStatus*/
extern unsigned char D_6DF4[];/*HitPoints*/
extern int16 far *D_6EF4;/*BaseTile*/
extern unsigned char D_6EFC[];/*TalkFlags*/
extern unsigned char far *D_7640;/*TileFlag2*/
extern unsigned char D_7644[];/*INTEL*/
extern struct coord D_78E4[];/*ObjPos*/
extern unsigned char D_9CF6;/*PartySize*/
extern uint16 D_9CFA[];/*ObjShapeType*/
extern unsigned char far *D_B4FA;/*TerrainType*/
extern int16 far *D_B53E;/*OrigShapeType*/
extern int16 D_B642;/*Selection.obj*/
extern struct tClipInfo D_B644[4];/*TextWindows*/
extern int16 D_B66A, D_B66C;/*Selection.x, Selection.y*/
extern int16 D_B6B8;/*SelectionIsDark*/
extern int16 D_E658[];/*Equipment: object in each slot (8: two-handed)*/
extern int16 D_EBCC;/*MissileHitObj*/
extern int16 D_EBCE[2];/*MissileEnd*/
extern int16 D_EC52[8];/*EquipWeapons*/
extern union tHandler D_ED10;/*GrDriver*/
/*functions of other modules*/
extern AnimateTilesAndActors(void);/*AnimateTilesAndActors*/
extern DrawMapWindow(int16);/*DrawMapWindow*/
extern DisplayTick(void);/*DisplayTick*/
extern int16 C_0359_0D58(int16);
extern PassTime(int16);/*PassTime*/
extern RefreshStatus(void);/*RefreshStatus*/
extern SetPartyMode(void);/*SetPartyMode*/
extern int16 CON_mouseOffAt(int16, int16, int16, int16);/*CON_mouseOffAt*/
extern CON_mouseOn(int16);/*CON_mouseOn*/
extern CON_UpdateMousePointer(void);/*CON_UpdateMousePointer*/
extern int16 CON_getch(void);/*CON_getch*/
extern CON_setClip(struct tClipInfo *);/*CON_setClip*/
extern CON_gotoxy(int16, int16);/*CON_gotoxy*/
extern cdecl CON_printf(char far *, ...);/*CON_printf*/
extern unsigned char GetTileAtXYZ(int16, int16, int16);/*GetTileAtXYZ*/
extern VIS_UpdateView(void);/*VIS_UpdateView*/
extern int16 NextLoc(void);/*NextLoc*/
extern int16 FindLoc(int16, int16, int16);/*FindLoc*/
extern int16 OBJ_FindTypeAt(int16, int16, int16);/*OBJ_FindTypeAt(type, x, y)*/
extern int16 FindInv(int16);/*FindInv*/
extern int16 NextInv(void);/*NextInv*/
extern int16 C_0A5D_080A(int16, int16);/*an object of a type, not equipped, inside an object*/
extern int16 C_0A5D_0DEF(int16);/*OBJ_PartHeadType: type of the head of a multi-tile creature part, 0 if none*/
extern int16 AddMapObj(int16, int16, int16, int16, int16, int16);/*AddMapObj(shape, status, amount, x, y, z)*/
extern int16 AddInvObj(int16, int16, int16, int16);/*AddInvObj(shapeType, status, amount, owner)*/
extern DeleteObj(int16);/*DeleteObj*/
extern OBJ_MakeDeadBody(int16);/*OBJ_MakeDeadBody*/
extern MoveObj(int16, int16, int16, int16);/*MoveObj*/
extern int16 QuanType(int16);/*QuanType*/
extern ShowObject(int16, int16, int16, int16);/*ShowObject(tile, x, y, mode)*/
extern ShowObjects(void);/*ShowObjects*/
extern char *GetTileString(int16);/*GetTileString*/
extern char *GetObjectString(int16);/*GetObjectString*/
extern STAT_ShowParty(int16);/*STAT_ShowParty*/
extern STAT_refreshComMode(void);/*STAT_refreshComMode*/
extern STAT_DrawInventory(void);/*STAT_DrawInventory*/
extern STAT_ShowInventory(int16);/*STAT_ShowInventory*/
extern int16 C_130F_0002(int16);
extern int16 C_130F_004D(int16);
extern int16 C_130F_0070(int16);
extern int16 C_130F_00A7(int16);
extern int16 C_130F_00CA(int16);
extern int16 C_130F_0136(int16);
extern int16 CanMoveTo(int16, int16, int16);/*CanMoveTo*/
extern int16 C_130F_1199(int16);
extern SubMov(int16, int16);/*SubMov*/
extern int16 TryStraightMove(int16, int16, int16);/*TryStraightMove(npc, dir, ?)*/
extern int16 TryMoveTo(int16, int16, int16);/*TryMoveTo(npc, x, y): 0 if not moved*/
extern WanderRandomly(int16);/*WanderRandomly*/
extern CheckActiveMember(void);
extern C_1DBD_0304(void);
extern C_1DBD_077E(void);
extern int16 IsTreeTile(int16);
extern int16 C_1DBD_18FD(int16, int16);
extern AfterExplosion(int16, int16);
extern int16 FindTypeAt(int16, int16, int16, int16);
extern LoadFlintlock(int16, int16);
extern C_1DBD_4F8E(int16, int16, int16);
extern C_1DBD_6DD9(int16);
extern int16 GetMonsterClass(int16);/*GetMonsterClass*/
extern C_2701_02D6(void);
extern C_2701_0355(int16);
extern C_2701_04D7(void);
extern MUS_PlaySfx(int16);/*MUS_PlaySfx*/
extern int16 Missiles(int16, int16, int16, int16 *, int16 *, int16, int16, int16);/*Missiles*/
extern int16 far *Explosion(int16, int16, int16);/*Explosion(x, y, tile): list of the objects hit, -1 terminated*/
extern int16 OSI_rand(int16, int16);/*OSI_rand(lo, hi)*/

#define DirIncrX DirIncrX
#define DirIncrY DirIncrY
#define StatusDirty StatusDirty
#define PromptFlag PromptFlag
#define SelectMode SelectMode
#define AimX AimX
#define AimY AimY
#define SelectRange SelectRange
#define LastNumberPlural LastNumberPlural
#define TextOn TextOn
#define LastAttackTarget LastAttackTarget
#define SkipNewlines SkipNewlines
#define StatusMember StatusMember
#define MouseMode MouseMode
#define StatusDisplay StatusDisplay
#define PendingCommand PendingCommand
#define CurClip CurClip
#define PartyListTop PartyListTop
#define IsInConversation IsInConversation
#define VisibilityDirty VisibilityDirty
#define EnemyWarningGiven EnemyWarningGiven
#define ColorRed ColorRed
#define ColorGreen ColorGreen
#define ColorText D_24E6
#define Active Active
#define MapX MapX
#define MapY MapY
#define MapZ MapZ
#define InCombat InCombat
#define SoloMember SoloMember
#define SavedNPCMode SavedNPCMode
#define ObjStatus D_329C
#define MovePts D_44FA
#define SchedIndex D_45FA
#define NPCFlag D_4602
#define DEXTE D_4606
#define NPCMode D_4706
#define Party D_4EDA
#define Level D_4EEC
#define Leader D_5100
#define Names D_5104
#define ExpPoints D_51E4
#define NPCComMode D_53E4
#define STREN D_53E8
#define Amount D_54F0
#define NPCStatus D_6CF0
#define HitPoints D_6DF4
#define BaseTile D_6EF4
#define TalkFlags D_6EFC
#define TileFlag2 D_7640
#define INTEL D_7644
#define ObjPos D_78E4
#define PartySize D_9CF6
#define ObjShapeType D_9CFA
#define TerrainType D_B4FA
#define OrigShapeType D_B53E
#define SelectionObj D_B642
#define TextWindows D_B644
#define SelectionX D_B66A
#define SelectionY D_B66C
#define SelectionIsDark D_B6B8
#define Equipment D_E658
#define MissileHitObj D_EBCC
#define MissileEnd D_EBCE
#define EquipWeapons D_EC52
#define GrDriver D_ED10

#define AnimateTilesAndActors AnimateTilesAndActors
#define DrawMapWindow DrawMapWindow
#define DisplayTick DisplayTick
#define PassTime PassTime
#define RefreshStatus RefreshStatus
#define SetPartyMode SetPartyMode
#define CON_mouseOffAt CON_mouseOffAt
#define CON_mouseOn CON_mouseOn
#define CON_UpdateMousePointer CON_UpdateMousePointer
#define CON_getch CON_getch
#define CON_setClip CON_setClip
#define CON_gotoxy CON_gotoxy
#define CON_printf CON_printf
#define GetTileAtXYZ GetTileAtXYZ
#define VIS_UpdateView VIS_UpdateView
#define NextLoc NextLoc
#define FindLoc FindLoc
#define OBJ_FindTypeAt OBJ_FindTypeAt
#define FindInv FindInv
#define NextInv NextInv
#define OBJ_PartHeadType C_0A5D_0DEF
#define AddMapObj AddMapObj
#define AddInvObj AddInvObj
#define DeleteObj DeleteObj
#define OBJ_MakeDeadBody OBJ_MakeDeadBody
#define MoveObj MoveObj
#define QuanType QuanType
#define ShowObject ShowObject
#define ShowObjects ShowObjects
#define GetTileString GetTileString
#define GetObjectString GetObjectString
#define STAT_ShowParty STAT_ShowParty
#define STAT_refreshComMode STAT_refreshComMode
#define STAT_DrawInventory STAT_DrawInventory
#define STAT_ShowInventory STAT_ShowInventory
#define CanMoveTo CanMoveTo
#define SubMov SubMov
#define TryStraightMove TryStraightMove
#define TryMoveTo TryMoveTo
#define WanderRandomly WanderRandomly
#define GetMonsterClass GetMonsterClass
#define MUS_PlaySfx MUS_PlaySfx
#define Missiles Missiles
#define Explosion Explosion
#define OSI_rand OSI_rand
#define CriticalMsg CriticalMsg
#define HeavilyMsg HeavilyMsg
#define LightlyMsg LightlyMsg
#define BarelyMsg BarelyMsg
#define WoundedMsg WoundedMsg
#define GrazedMsg GrazedMsg
#define KilledMsg KilledMsg
#define FallsMsg FallsMsg
#define BareHandsMsg BareHandsMsg
#define WithMsg WithMsg
#define OutOfArrowsMsg OutOfArrowsMsg
#define OutOfAmmoMsg OutOfAmmoMsg
#define OutOfDartsMsg OutOfDartsMsg
#define OutOfSpearsMsg OutOfSpearsMsg
#define FlintlockNotLoadedMsg FlintlockNotLoadedMsg
#define AttackSelfPassMsg AttackSelfPassMsg
#define NothingMsg NothingMsg
#define AttackWithMsg AttackWithMsg
#define AttackMsg AttackMsg
#define WhatMsg WhatMsg
#define PoisonedMsg PoisonedMsg
#define OutOfRangeMsg OutOfRangeMsg
#define NotOnShipMsg NotOnShipMsg
#define AttacksMsg AttacksMsg
#define NotPossibleMsg NotPossibleMsg

#define GR_PUT_TILE_SCREEN(tile,x,y) (GrDriver.iii.ofs = 0x2d, (*GrDriver.f)(tile,x,y))

#define TILE_AT(x, y) GetTileAtXYZ(x, y, MapZ)
#define GR_PUT_TILE_SCREEN(tile,x,y) (GrDriver.iii.ofs = 0x2d, (*GrDriver.f)(tile,x,y))
#define GetType(i) (ObjShapeType[i] & 0x3ff)
#define GetFrame(i) (ObjShapeType[i] >> 10)
#define TILE_FRAME(i) (BaseTile[GetType(i)] + GetFrame(i))
#define GetCoordX(o) ((*(int16 *)&((o).raw[0]))&0x3ff)
#define GetCoordY(o) (((*(int16 *)&((o).raw[1]))>>2)&0x3ff)
#define GetCoordZ(o) ((o).raw[2] >> 4)
#define GetX(i) GetCoordX(ObjPos[i])
#define GetY(i) GetCoordY(ObjPos[i])
#define GetZ(i) GetCoordZ(ObjPos[i])
#define GetQuan(o) (((unsigned char *)(Amount + (o)))[0])
#define SubQuan(o, v) (GetQuan(o) -= (v))
#define GetQual(o) (((unsigned char *)(Amount + (o)))[1])
#define SubQual(o, v) (GetQual(o) -= (v))
#define GetDirection(i) (NPCFlag[i] & 7)
#define SetHurtThisTurn(i) (NPCFlag[i] |= 8)
#define ClrInvisible(i) ObjStatus[i] &= ~2
#define ClrAsleep(i) (NPCStatus[i] &= ~4)
#define SetDead(i) (NPCStatus[i] |= 0x10)
#define IsPoisoned(i) (NPCStatus[i] & 8)
#define SetPoisoned(i) (NPCStatus[i] |= 8)
#define SetAsleep(i) (NPCStatus[i] |= 4)
#define GetAlignment(i) (NPCStatus[i] & 0x60)
#define Is_ATKPLR(i) (GetAlignment(i) & 0x20)
#define Is_ATKMON(i) (GetAlignment(i) & 0x40)
#define SetFrame(i, v) (ObjShapeType[i] = GetType(i) + ((v) << 10))
#define SetDirection(i, v) (NPCFlag[i] = (NPCFlag[i] & ~7) | (v))
#define SetType(i, v) (ObjShapeType[i] = (ObjShapeType[i] & ~0x3ff) + (v))
#define SetQuan(o, v) (GetQuan(o) = (v))
#define SetQual(o, v) (GetQual(o) = (v))
#define IsNPC(i) ((i) < 0x100 && (i) >= 0)
#define GetCoordUse(i) (ObjStatus[i] & 0x18)
#define IsInvisible(i) (ObjStatus[i] & 2)
#define IsTileIg(tile) (TileFlag2[tile] & 0x10)
#define IsProtected(i) (NPCStatus[i] & 1)
#define IsCharmed(i) (ObjStatus[i] & 4)
#define IsCursed(i) (ObjStatus[i] & 0x40)
#define ClrCharmed(i) ObjStatus[i] &= ~4
#define SetAlignment(i, v) (NPCStatus[i] = (NPCStatus[i] & ~0x60) | (v))
#define GetBKAlignment(i) (NPCFlag[i] & 0x60)
#define IsIncapacitated(i) (NPCStatus[i] & 0x16)
#define IsAsleep(i) (NPCStatus[i] & 4)
#define IsDead(i) (NPCStatus[i] & 0x10)
#define IsPlrControl(i) (NPCStatus[i] & 0x80)
#define IsDraggedUnder(i) (NPCFlag[i] & 0x10)

/*this module's functions used before their definition*/
C_1810_14EA(int16, int16, int16);
COMBAT_Kill(int16);
LoseHP(int16, int16);
COMBAT_displaySeverity(int16);
COMBAT_AttackRange(int16, int16, int16);
COMBAT_beginAttacked(void);

/*when 0, the next COMBAT_showHit() call skips the hit flash*/
/*0DE0*/int16 ShowHitFlash = 1;
/*object firing the current missile (-1: none); lets Missiles() find
  the object hit at the impact tile (MissileHitObj)*/
/*0DE2*/int16 MissileShooter = -1;
/*how the current combat goes, for the music at its end (C_2701_0355):
  -1 a party member died, 1 an enemy died, 0 nothing yet*/
/*0DE4*/int16 CombatOutcome = 0;

/*range table: RangeTable[|dx|][|dy|] is the "combat distance" between two
  tiles (roughly max + min/2); everything 8 or more tiles away in one
  axis is treated as 9 by COMBAT_range()*/
/*0DE6*/unsigned char RangeTable[][8] = {
	0,1,2,3,4,5,6,7,
	1,1,2,3,4,5,6,7,
	2,2,2,3,4,5,6,7,
	3,3,3,4,5,6,7,7,
	4,4,4,5,6,7,7,8,
	5,5,5,6,7,7,8,8,
	6,6,6,7,7,8,8,8,
	7,7,7,7,8,8,8,8
};

/*unused (Ultima VI's, for the triple crossbow):
  SpreadDirTable[dy + 5][dx + 5] gives, for a target at offset (dx,dy), the two
  directions (high nibble, low nibble) in which the side missiles are
  spread around the aimed tile (roughly perpendicular to the shot)*/
/*0E26*/unsigned char SpreadDirTable[][11] = {
/*	0x42,0x52,0x52,0x52,0x52,0x62,0x63,0x63,0x63,0x63,0x64,
	0x41,0x42,0x52,0x52,0x52,0x62,0x63,0x63,0x63,0x64,0x74,
	0x41,0x41,0x42,0x52,0x52,0x62,0x63,0x63,0x64,0x74,0x74,
	0x41,0x41,0x41,0x42,0x52,0x62,0x63,0x64,0x74,0x74,0x74,
	0x41,0x41,0x41,0x41,0x42,0x62,0x64,0x74,0x74,0x74,0x74,
	0x40,0x40,0x40,0x40,0x40,0x00,0x04,0x04,0x04,0x04,0x04,
	0x30,0x30,0x30,0x30,0x20,0x26,0x06,0x05,0x05,0x05,0x05,
	0x30,0x30,0x30,0x20,0x27,0x26,0x16,0x06,0x05,0x05,0x05,
	0x30,0x30,0x20,0x27,0x27,0x26,0x16,0x16,0x06,0x05,0x05,
	0x30,0x20,0x27,0x27,0x27,0x26,0x16,0x16,0x16,0x06,0x05,
	0x20,0x27,0x27,0x27,0x27,0x26,0x16,0x16,0x16,0x16,0x06*/
#define MD(dir_0,dir_1) (((dir_0)<<4)|(dir_1))
	MD(4,2),MD(5,2),MD(5,2),MD(5,2),MD(5,2),MD(6,2),MD(6,3),MD(6,3),MD(6,3),MD(6,3),MD(6,4),
	MD(4,1),MD(4,2),MD(5,2),MD(5,2),MD(5,2),MD(6,2),MD(6,3),MD(6,3),MD(6,3),MD(6,4),MD(7,4),
	MD(4,1),MD(4,1),MD(4,2),MD(5,2),MD(5,2),MD(6,2),MD(6,3),MD(6,3),MD(6,4),MD(7,4),MD(7,4),
	MD(4,1),MD(4,1),MD(4,1),MD(4,2),MD(5,2),MD(6,2),MD(6,3),MD(6,4),MD(7,4),MD(7,4),MD(7,4),
	MD(4,1),MD(4,1),MD(4,1),MD(4,1),MD(4,2),MD(6,2),MD(6,4),MD(7,4),MD(7,4),MD(7,4),MD(7,4),
	MD(4,0),MD(4,0),MD(4,0),MD(4,0),MD(4,0),MD(0,0),MD(0,4),MD(0,4),MD(0,4),MD(0,4),MD(0,4),
	MD(3,0),MD(3,0),MD(3,0),MD(3,0),MD(2,0),MD(2,6),MD(0,6),MD(0,5),MD(0,5),MD(0,5),MD(0,5),
	MD(3,0),MD(3,0),MD(3,0),MD(2,0),MD(2,7),MD(2,6),MD(1,6),MD(0,6),MD(0,5),MD(0,5),MD(0,5),
	MD(3,0),MD(3,0),MD(2,0),MD(2,7),MD(2,7),MD(2,6),MD(1,6),MD(1,6),MD(0,6),MD(0,5),MD(0,5),
	MD(3,0),MD(2,0),MD(2,7),MD(2,7),MD(2,7),MD(2,6),MD(1,6),MD(1,6),MD(1,6),MD(0,6),MD(0,5),
	MD(2,0),MD(2,7),MD(2,7),MD(2,7),MD(2,7),MD(2,6),MD(1,6),MD(1,6),MD(1,6),MD(1,6),MD(0,6)
#undef MD
};

/*0E9F*/static int16 NumEquipWeapons = 0;/*number of equiped weapons in EquipWeapons*/
/*0EA1*/static int16 CurEquipWeapon = 0;/*index in EquipWeapons of the weapon currently attacking*/

/*actor type ==> dead body (type | frame << 10); 0-terminated pairs*/
/*0EA3*/uint16 DeadShape[] = {
	0x136,0x0F9,0x137,0x4F9,0x138,0x8F9,0x139,0xCF9,
	0x13A,0x10F9,0x13B,0x14F9,0x13C,0x18F9,0x13E,0x1CF9,
	0x140,0x20F9,0x141,0x24F9,0x142,0x28F9,0x143,0x2CF9,
	0x144,0x30F9,0x145,0x34F9,0x146,0x38F9,0x13D,0x3CF9,
	0x11B,0x518,0x11C,0xD18,0x120,0x1918,0x121,0x2118,
	0x126,0x3518,0x128,0x5518,0x129,0x5D18,0x12D,0x6518,
	0x12F,0x6D18,0x131,0x7918,0x147,0x8518,0x14A,0x8D18,
	0x14B,0x9518,0x14C,0x9D18,0x14D,0xA518,0x14E,0xAD18,
	0x14F,0xB518,0x150,0xB918,0x151,0xBD18,0x12B,0x6B,
	0,0
};
/*wall flags (TerrainType) on the side of each direction (N, E, S, W)*/
/*0F37*/unsigned char WallDirFlags[] = {0x80,0,0x40,0,0x20,0,0x10};

/*actors that cannot be hurt (LoseHP keeps them at 255 hit points, no
  poison): the ones of original type 0x11E/0x11F/0x126 and NPCs 11, 25,
  31, 33, 35, 47, 55*/
/*C_1810_000D*/IsInvulnerable(int16 objNum) {
	int16 objType;

	objType = OrigShapeType[objNum] & 0x3ff;
	if(objType == 0x126 || objType == 0x11f || objType == 0x11e)
		return 1;
	if(
		objNum == 0x1f || objNum == 0x2f || objNum == 0x37 || objNum == 0x21 ||
		objNum == 0x0b || objNum == 0x19 || objNum == 0x23
	) return 1;
	return 0;
}

/*distance in tiles (max(|dx|,|dy|)) between an object and a location*/
/*C_1810_0068*/COMBAT_TileDistance(int16 objNum, int16 x, int16 y) {
	int16 delta_x, delta_y;

	delta_x = abs(GetX(objNum) - x);
	delta_y = abs(GetY(objNum) - y);
	if(delta_x > delta_y)
		return delta_x;
	else
		return delta_y;
}

/*end the "charm" effect: restore the original alignment (kept in
  NPCFlag) and give the NPC back its normal mode*/
/*C_1810_00BB*/COMBAT_stopCharm(int16 objNum) {
	ClrCharmed(objNum);
	SetAlignment(objNum, GetBKAlignment(objNum));
	if(IsPlrControl(objNum))
		SetAlignment(objNum, 0x40);
	if(InCombat)
		NPCMode[objNum] = NPCComMode[objNum];
	else if(SoloMember < 0)/*party mode*/
		NPCMode[objNum] = 1;
	else/*solo mode*/
		NPCMode[objNum] = 0;
}

/*enter combat mode: every party member takes its combat mode
  (NPCComMode), the controlled member (first able one in party mode, the
  solo member otherwise) gets mode 2 (command), the combat icon is drawn
  and the combat music starts*/
/*C_1810_012A*/COMBAT_begin() {
	int16 partyIdx, mouseSave;

	InCombat = 1;
	CombatOutcome = 0;
	for(partyIdx = 0; partyIdx < PartySize; partyIdx ++) {
		if(!IsCharmed(Party[partyIdx]))
			NPCMode[Party[partyIdx]] = NPCComMode[Party[partyIdx]];
		Leader[Party[partyIdx]] = Party[partyIdx];
	}
	if(SoloMember < 0) {
		for(
			partyIdx = 0;
			(IsIncapacitated(Party[partyIdx]) || IsDraggedUnder(Party[partyIdx])) && partyIdx < PartySize;
			partyIdx ++
		);
	} else {
		partyIdx = SoloMember;
	}
	SavedNPCMode = NPCMode[Party[partyIdx]];
	NPCMode[Party[partyIdx]] = 2;
	mouseSave = CON_mouseOffAt(64, 0, 95, 15);
	GR_PUT_TILE_SCREEN(0x177, 64, 0);
	GR_PUT_TILE_SCREEN(0x178, 80, 0);
	CON_mouseOn(mouseSave);
	C_2701_04D7();
}

/*leave combat mode: party members follow again (or stay put in solo
  mode), the combat icon is cleared, the music says how the fight went
  (CombatOutcome) and time passes*/
/*C_1810_023B*/COMBAT_breakOff() {
	int16 mouseSave;
	int16 partyIdx, newMode;

	InCombat =
	EnemyWarningGiven = 0;
	if(SoloMember < 0)
		newMode = 1;
	else
		newMode = 0;
	for(partyIdx = 0; partyIdx < PartySize; partyIdx ++) {
		if(IsDead(Party[partyIdx])) {
			NPCMode[Party[partyIdx]] = 0;
		} else if(!IsCharmed(Party[partyIdx])) {
			NPCMode[Party[partyIdx]] = newMode;
			if(newMode == 1)
				Leader[Party[partyIdx]] = partyIdx;
		}
	}
	if(SoloMember < 0)
		partyIdx = 0;
	else
		partyIdx = SoloMember;
	SavedNPCMode = NPCMode[Party[partyIdx]];
	NPCMode[Party[partyIdx]] = 2;
	mouseSave = CON_mouseOffAt(64, 0, 95, 15);
	GR_PUT_TILE_SCREEN(0x173, 64, 0);
	GR_PUT_TILE_SCREEN(0x174, 80, 0);
	CON_mouseOn(mouseSave);
	C_2701_0355(CombatOutcome);
	CombatOutcome = 0;
	PassTime(1);
	if(StatusDisplay == 0x92) {
		STAT_refreshComMode();
		STAT_ShowInventory(StatusMember);
	}
}

/*move an object to a random free spot at most 3 tiles away (8 tries);
  with onScreenNotOk the spot must be outside the visible 11x11 area.
  returns 1 if the object was moved*/
/*C_1810_0394*/COMBAT_TryTeleport(int16 objNum, int16 onScreenNotOk) {
	int16 tries, x, y, z, new_x, new_y;

	if(objNum < 0)
		return 0;
	x = GetX(objNum);
	y = GetY(objNum);
	z = GetZ(objNum);
	for(tries = 0; tries < 8; tries ++) {
		new_x = OSI_rand(1, 4) + OSI_rand(1, 4) + x - 5;
		new_y = OSI_rand(1, 4) + OSI_rand(1, 4) + y - 5;
		if(
			onScreenNotOk &&
			MapX - 5 <= new_x && new_x <= MapX + 5 &&
			MapY - 5 <= new_y && new_y <= MapY + 5
		) continue;
		if(CanMoveTo(objNum, new_x, new_y)) {
			MoveObj(objNum, new_x, new_y, z);
			return 1;
		}
	}
	return 0;
}

/*effective strength: when cursed, -3 (minimum 1), then +3 (maximum 30)
  with ObjStatus 0x80*/
/*C_1810_04A0*/GetStr(int16 objNum) {
	int16 value = STREN[objNum];
	if(IsCursed(objNum)) {
		if((value -= 3) < 1)
			value = 1;
		if(ObjStatus[objNum] & 0x80) {
			if((value += 3) > 30)
				value = 30;
		}
	}
	return value;
}

/*effective dexterity: -3 when cursed (minimum 1), +3 with ObjStatus 0x80
  (maximum 30); 1 when asleep*/
/*C_1810_04E0*/GetDex(int16 objNum) {
	int16 value = DEXTE[objNum];
	if(IsCursed(objNum)) {
		if((value -= 3) < 1)
			value = 1;
	}
	if(ObjStatus[objNum] & 0x80) {
		if((value += 3) > 30)
			value = 30;
	}
	if(IsAsleep(objNum))
		value = 1;
	return value;
}

/*effective intelligence: -3 (minimum 1) when cursed or with ObjStatus
  0x80 (but for NPC 1)*/
/*C_1810_052A*/GetInt(int16 objNum) {
	int16 value = INTEL[objNum];
	if(IsCursed(objNum) || ((ObjStatus[objNum] & 0x80) && objNum != 1)) {
		if((value -= 3) < 1)
			value = 1;
	}
	return value;
}

/*intelligence contest for spells: returns 1 when the target resists
  (type 0x143: 1 in 10; type 0x148: never)*/
/*C_1810_0562*/COMBAT_ResistsSpell(int16 casterObj, int16 targetObj) {
	if(GetType(targetObj) == 0x143)
		return OSI_rand(1, 10) == 1;
	else if(GetType(targetObj) == 0x148)
		return 0;
	else
		return GetInt(targetObj) - GetInt(casterObj) > OSI_rand(-12, 12);
}

/*level from experience: 1 + number of halvings to get 100xp or less*/
/*C_1810_05D4*/ComputeLevel(int16 objNum) {
	uint16 lvl, xp;

	xp = ExpPoints[objNum];
	lvl = 1;
	while(xp > 99) {
		lvl ++;
		xp >>= 1;
	}
	return lvl;
}

/*add experience points (capped at 9999)*/
/*C_1810_05F7*/AddXP(int16 objNum, int16 xp) {
	if(ExpPoints[objNum] < 9999 - xp)
		ExpPoints[objNum] += xp;
	else
		ExpPoints[objNum] = 9999;
}

/*maximum hit points: 2*STR + 4*level, between 1 and 255*/
/*C_1810_062A*/MaxHP(int16 objNum) {
	int16 hp;

	hp = GetStr(objNum) * 2 + Level[objNum] * 4;
	if(hp == 0)
		hp = 1;
	if(hp > 255)
		return 255;
	return hp;
}

/*set to "unconscious" type: called when an actor falls asleep or dies.
  Actors take their dead body shape from DeadShape (NPCs 11, 31, 47, 55
  and the "vanishing" monster classes have none); for the creatures of
  several tiles (C_130F_00CA/C_130F_0070) the original shape is kept in
  OrigShapeType, the parts around are removed and the body made of
  several objects (with blood when dead). Monsters of class flag 0x2000
  bleed. returns 1 if the actor has a body shape (0: it must disappear)*/
/*C_1810_0664*/SetTypeUnconscious(int16 objNum) {
	register int16 newObj;
	int16 dir, objType, objCls, obj, isDead, x, y, i, j, terrain, terrain2;
	int16 idx, deadShape;

	idx = 0;
	objType = GetType(objNum);
	isDead = IsDead(objNum);
	if(isDead && (objType == 0xf9 || objType == 0x118))
		return 1;
	if(isDead && objType == 0x133) {
		C_1DBD_6DD9(objNum);
		objType = GetType(objNum);
	}
	if(C_130F_00CA(objNum) || C_130F_0070(objNum))
		OrigShapeType[objNum] = ObjShapeType[objNum];
	if(objNum == 0x1f || objNum == 0x2f || objNum == 0x37 || objNum == 0x0b) {
		deadShape = 0;
		objCls = -1;
	} else {
		while(DeadShape[idx] > 0 && DeadShape[idx] != objType)
			idx += 2;
		deadShape = DeadShape[idx + 1];
		objCls = GetMonsterClass(objType);
	}
	if(deadShape && !(MonsterFlags[objCls + 1] & 0x1000) && !(MonsterFlags[objCls + 1] & 0x800)) {
		if(!C_130F_00CA(objNum) && !C_130F_0070(objNum)) {
			ObjShapeType[objNum] = deadShape;
		} else {
			MUS_PlaySfx(0x1f);
			x = GetX(objNum);
			y = GetY(objNum);
			terrain = TerrainType[TILE_FRAME(objNum)];
			for(dir = 0; dir < 8; dir ++) {
				if(dir & 1)
					continue;
				if(!(WallDirFlags[dir] & terrain))
					continue;
				for(obj = FindLoc(x + DirIncrX[dir], y + DirIncrY[dir], MapZ); obj >= 0; obj = NextLoc()) {
					terrain2 = TerrainType[TILE_FRAME(obj)];
					if(
						(obj >= 0x100 || obj < 0) &&
						GetQual(obj) == objNum &&
						abs(GetType(obj) - objType) <= 1 &&
						(WallDirFlags[dir ^ 4] & terrain2)
					) break;
				}
				if(obj >= 0)
					DeleteObj(obj);
			}
			ObjShapeType[objNum] = deadShape;
			switch(objType) {
				case 0x126:
					newObj = AddMapObj(0x4518, 0, objNum << 8, x, y + 2, MapZ);
					ObjStatus[newObj] = (newObj >= 0 && objNum < 0xc0)?ObjStatus[newObj] & ~0x20:ObjStatus[newObj] | 0x20;
					for(i = 0; i < 2; i ++)
						for(j = 0; j < 4; j ++)
							if(OSI_rand(0, 1) && isDead)
								AddMapObj(0xf8 | (OSI_rand(0, 2) << 10), 0x20, 0, x - i, y + 2 - j, MapZ);
				break;
				case 0x11c: case 0x121: case 0x131:
					newObj = AddMapObj(deadShape + 0x400, 0x20, objNum << 8, x + 1, y, MapZ);
					ObjStatus[newObj] = (newObj >= 0 && objNum < 0xc0)?ObjStatus[newObj] & ~0x20:ObjStatus[newObj] | 0x20;
					for(i = 0; i < 3; i ++)
						if(OSI_rand(0, 1) && isDead)
							AddMapObj(0xf8 | (OSI_rand(0, 2) << 10), 0x20, 0, x + 1 - i, y, MapZ);
				break;
				case 0x12f:
					newObj = AddMapObj(deadShape + 0x400, 0x20, objNum << 8, x, y + 1, MapZ);
					ObjStatus[newObj] = (newObj >= 0 && objNum < 0xc0)?ObjStatus[newObj] & ~0x20:ObjStatus[newObj] | 0x20;
					for(j = 0; j < 3; j ++)
						if(OSI_rand(0, 1) && isDead)
							AddMapObj(0xf8 | (OSI_rand(0, 2) << 10), 0x20, 0, x, y + 1 - j, MapZ);
				break;
				case 0x129:
					newObj = AddMapObj(deadShape + 0x400, 0x20, objNum << 8, x + 1, y, MapZ);
					if(newObj >= 0 && objNum < 0xc0)
						ObjStatus[newObj] &= ~0x20;
					else
						ObjStatus[newObj] |= 0x20;
				break;
			}
		}
	}
	if(isDead && objCls >= 0) {
		if((MonsterFlags[objCls + 1] & 0x800) || (MonsterFlags[objCls + 1] & 0x1000)) {
			deadShape = 0;
			if((MonsterFlags[objCls + 1] & 0x1000) && CLOSE_ENOUGH(5, GetX(objNum), GetY(objNum), MapX, MapY))
				C_0359_0D58(1);
		}
		if((MonsterFlags[objCls + 1] & 0x2000) && objNum >= 0x10 && !C_130F_00CA(objNum) && !C_130F_0070(objNum)) {
			if((obj = AddMapObj(0xf8, 0x20, 0, GetX(objNum), GetY(objNum), MapZ)) >= 0)
				ObjShapeType[obj] = GetType(obj) + (OSI_rand(0, 2) << 10);
		}
	}
	VisibilityDirty = 1;
	return deadShape != 0;
}

/*can viewerObj see (and so target) targetObj? not if it is invisible
  (unless both are party members), dragged under, an "ignore" tile or on
  another level*/
/*C_1810_0CA4*/COMBAT_canSee(int16 viewerObj, int16 targetObj) {
	int16 isVisible;

	isVisible = !IsInvisible(targetObj);

	if(
		IsPlrControl(viewerObj) &&
		!isVisible && IsNPC(targetObj) && IsPlrControl(targetObj)
	) isVisible = 1;

	if(IsNPC(targetObj) && IsDraggedUnder(targetObj))
		isVisible = 0;

	if(IsTileIg(TILE_FRAME(targetObj)))
		isVisible = 0;

	if(GetZ(viewerObj) != GetZ(targetObj))
		isVisible = 0;

	return isVisible;
}

/*pick the object targeted at (x,y) by viewerObj: the first visible
  living actor, else the first visible object, else the first hidden
  object; -1 if the tile is empty. A part of a multi-tile creature gives
  the creature (its number is in the part's quality)*/
/*C_1810_0D5B*/COMBAT_GetTargetAt(int16 viewerObj, int16 x, int16 y) {
	int16 result;
	int16 objType;
	int16 liveNpc, firstHidden;

	firstHidden = -1;
	/*first "canSee" object*/
	for(result = FindLoc(x, y, MapZ); result >= 0; result = NextLoc()) {
		if(COMBAT_canSee(viewerObj, result))
			break;
		if(firstHidden < 0)
			firstHidden = result;
	}
	/*first "canSee" notDead NPC*/
	for(liveNpc = result; liveNpc >= 0; liveNpc = NextLoc()) {
		if(COMBAT_canSee(viewerObj, liveNpc)) {
			if(IsNPC(liveNpc) && !IsDead(liveNpc))
				break;
			continue;
		}
		if(firstHidden < 0)
			firstHidden = liveNpc;
	}
	/* */
	if(liveNpc >= 0)
		result = liveNpc;
	else if(firstHidden >= 0 && result < 0)
		result = firstHidden;
	if(!IsNPC(result)) {
		objType = GetType(result);
		if(GetQual(result) && OBJ_PartHeadType(objType) && OBJ_PartHeadType(objType) != objType)
			result = GetQual(result);
	}

	return result;
}

/*armours ... and their protection (ArmorValues)*/
/*0F3E*/int16 ArmorTypes[] = {7,8,9,0x43,2,0xd,0xe,0xf};
/*0F4E*/unsigned char ArmorValues[] = {1,3,2,7,4,1,2,3};

/*protection given by an armour object (0 if not an armour)*/
/*C_1810_0E39*/GetArmorPoints(int16 objNum) {
	int16 i, points;

	points = 0;
	for(i = 0; i < 8; i ++) {
		if(ArmorTypes[i] == GetType(objNum)) {
			points = ArmorValues[i];
			break;
		}
	}
	return points;
}

/*total protection of an actor: equipped armours, -3 cursed,
  +3 "protected"*/
/*C_1810_0E74*/static __GetProtection(int16 objNum) {
	int16 protection, invObj;

	protection = 0;
	for(invObj = FindInv(objNum); invObj >= 0; invObj = NextInv()) {
		if(GetCoordUse(invObj) == 0x18)
			protection += GetArmorPoints(invObj);
	}
	if(IsCursed(objNum))
		protection -= 3;
	if(IsProtected(objNum))
		protection += 3;

	return protection;
}

/*weapons with a range > 1 ... and their range (WeaponRanges)*/
/*0F56*/int16 RangedWeaponTypes[] = {0x1a,0x1b,0x32,0x89,0x8a,0x1f,0x20,0x25,0x27,0x28,0x01,0x2c,0xf0,0xf1};
/*0F72*/unsigned char WeaponRanges[] = {6,4,7,6,6,6,4,7,4,8,5,4,6,6};

/*range of a weapon (or of an actor's natural attack): 1 by default;
  7 for object 0x15 with quality 1*/
/*C_1810_0EC7*/static __GetWeaponRange(int16 objNum) {
	int16 i, range;
	int16 objType;

	range = 1;
	objType = GetType(objNum);
	for(i = 0; i < 14; i ++) {
		if(RangedWeaponTypes[i] == objType) {
			range = WeaponRanges[i];
			break;
		}
	}
	if(objType == 0x15 && GetQual(objNum) == 1)
		range = 7;
	return range;
}

/*fire one missile (tile numTile, 0: invisible) from objNum toward
  *pX_dst,*pY_dst; on return they hold where it stopped*/
/*C_1810_0F19*/COMBAT_Missile(int16 objNum, int16 *pX_dst, int16 *pY_dst, int16 max_range, int16 numTile) {
	register int16 ret;

	MissileShooter = objNum;
	ret = Missiles(GetX(objNum), GetY(objNum), 1, pX_dst, pY_dst, max_range, numTile, 0);
	MissileShooter = -1;
}

/*tile classes (terrain tiles)*/
/*0F80*/int16 TileClass1[] = {
	0x5,0x6,0x8,0x9,0xA,0xB,0xD,0xE,0x1E,0x31,0x32,0x35,0x36,0x37,0x3D,0x3E,
	0x3F,0x62,0x63,0x64,0x65,0x67,0x68,0x69,0x70,0x71,0x73,0x74,0x75,0x76,0x77,0x79,
	0x7A,0x7B,0x7C,0x7D,0x7E,0x7F,0x80,0x83,0x84,0x85,0x87,0x88,0x89,0x8A,0x8B,0x8C,
	0x8D,0x8E,0x8F,0x93,0x94,0x96,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD9,0xDA,0xDB,
	0xDC,0xDD,0xDE,0xDF
};
/*1008*/int16 TileClass2[] = {
	0xE,0xB,0xD0,0x1C,0x1D,0x1E,0x32,0x33,0x60,0x61,0x62,0x6B,0x72,0x78,0x7B,0xD3,
	0xD5,0x8C,0xDF
};
/*102E*/int16 TileClass3[] = {
	0x5,0x6,0x8,0xA,0x1C,0x1F,0x31,0x33,0x35,0x36,0x60,0x63,0x66,0x74,0x75,0x83,
	0xDD,0xDE,0xCB,0xCC,0xCD,0xCE,0xCF,0xD0
};

/*is the tile in TileClass1? (a boulder falls in: pit, chasm?)*/
/*C_1810_0F71*/C_1810_0F71(int16 tile) {
	int16 i;

	for(i = 0; i < 0x44; i ++)
		if(TileClass1[i] == tile)
			return 1;
	return 0;
}

/*is the tile in TileClass2? (blocks a boulder moving south?)*/
/*C_1810_0F98*/C_1810_0F98(int16 tile) {
	int16 i;

	for(i = 0; i < 0x13; i ++)
		if(TileClass2[i] == tile)
			return 1;
	return 0;
}

/*is the tile in TileClass3? (blocks a boulder moving east?)*/
/*C_1810_0FBF*/C_1810_0FBF(int16 tile) {
	int16 i;

	for(i = 0; i < 0x18; i ++)
		if(TileClass3[i] == tile)
			return 1;
	return 0;
}

/*does the tile stop a pushed boulder?*/
/*C_1810_0FE6*/C_1810_0FE6(int16 tile) {
	if(
		tile == 0x90 || tile == 0x91 || tile == 0x92 || tile == 0x95 ||
		(tile >= 0x98 && tile <= 0xa9) ||
		(tile >= 0xac && tile <= 0xb6) ||
		(tile >= 0xba && tile <= 0xc5) ||
		IsTreeTile(tile)
	) return 1;
	return 0;
}

/*is the 2x2 square whose bottom right corner is (x,y) blocked for a
  boulder (by the terrain or by some object types)?*/
/*C_1810_103F*/C_1810_103F(int16 x, int16 y) {
	int16 obj, j;
	int16 i, objType;

	for(i = 0; i < 2; i ++) {
		for(j = 0; j < 2; j ++) {
			if(C_1810_0FE6(TILE_AT(x + i - 1, y + j - 1)))
				return 1;
			for(obj = FindLoc(x + i - 1, y - i + j, MapZ); obj >= 0; obj = NextLoc()) {
				objType = GetType(obj);
				if(
					objType == 0x107 || objType == 0x108 || objType == 0x109 || objType == 0x10a ||
					objType == 0x104 || objType == 0x105 || objType == 0x106 || objType == 0x96 ||
					objType == 0x97
				) return 1;
			}
		}
	}
	return 0;
}

/*a boulder (selfObj) lands on the 2x2 square whose bottom right corner is
  (x,y): another boulder there is pushed first (returns 0 if it can't be);
  then actors (or the creatures a part belongs to) are crushed (mode 1)
  or pushed aside and hurt, other objects destroyed. returns 1*/
/*C_1810_1101*/C_1810_1101(int16 x, int16 y, int16 selfObj, int16 mode) {
	int16 tx, ty, i;
	int16 dx, dy, found, objType;
	int16 obj, j;

	for(i = 0; i < 2; i ++) {
		for(j = 0; j < 2; j ++) {
			tx = x - i;
			ty = y - j;
			for(obj = FindLoc(tx, ty, MapZ); obj >= 0; obj = NextLoc()) {
				objType = GetType(obj);
				if(obj == selfObj)
					continue;
				if(objType == 0xa7 && !C_1810_14EA(tx, ty, obj))
					return 0;
			}
		}
	}
	for(i = 0; i < 2; i ++) {
		for(j = 0; j < 2; j ++) {
			tx = x - i;
			ty = y - j;
			for(obj = FindLoc(tx, ty, MapZ); obj >= 0; obj = NextLoc()) {
				objType = GetType(obj);
				if(obj == selfObj)
					continue;
				if(IsNPC(obj) || OBJ_PartHeadType(GetType(obj))) {
					if(OBJ_PartHeadType(GetType(obj)))
						obj = GetQual(obj);
					if(mode == 1) {
						COMBAT_Kill(obj);
					} else {
						dx = 0;
						dy = 0;
						found = 0;
						while(dx < 3 && !found) {
							while(dy < 3 && !found) {
								if((dx == 1) & (dy == 1))
									dy ++;
								if(CanMoveTo(obj, tx + dx - 1, ty + dy - 1))
									found = 1;
								else
									dy ++;
							}
							if(!found)
								dx ++;
						}
						if(!found) {
							MUS_PlaySfx(0x1f);
							COMBAT_Kill(obj);
						} else {
							MoveObj(obj, tx + dx - 1, ty + dy - 1, MapZ);
							MUS_PlaySfx(0x1f);
							LoseHP(obj, OSI_rand(1, 30));
						}
					}
				} else if(!(TileFlag2[obj] & 0x20) && objType != 0xf9 && objType != 0x118) {
					DeleteObj(obj);
				}
			}
		}
	}
	return 1;
}

/*does a boulder fall in water? if a tile of its 2x2 square is 0x0C or
  0x0F, it stops being temporary (returns 1)*/
/*C_1810_1327*/C_1810_1327(int16 objNum) {
	int16 x, y, tile;
	int16 i, j;

	x = GetX(objNum);
	y = GetY(objNum);
	for(i = 0; i < 2; i ++) {
		for(j = 0; j < 2; j ++) {
			tile = TILE_AT(x - i + 1, y + j - 1);
			if(tile == 0xc || tile == 0xf) {
				ObjStatus[objNum] &= ~0x20;
				return 1;
			}
		}
	}
	return 0;
}

/*move a boulder (quality 1: stuck) to (x,y), unless the place is blocked
  (and force is not 1); then a boulder dropped at the right place removes
  the objects 0x96/0x97/0xC3/0x98 around (and otherObj)*/
/*C_1810_13B1*/C_1810_13B1(int16 obj, int16 x, int16 y, int16 mode, int16 force, int16 otherObj) {
	if(!((GetQual(obj) == 1 || C_1810_103F(x, y)) && force != 1)) {
		if(C_1810_1101(x, y, obj, mode) == 1 || force == 1) {
			MoveObj(obj, x, y, MapZ);
			VIS_UpdateView();
			DisplayTick();
			if(OBJ_FindTypeAt(0x96, x, y + 2) != -1) {
				MUS_PlaySfx(0x1f);
				GetQual(obj) = 1;
				DeleteObj(OBJ_FindTypeAt(0x96, x, y + 2));
				DeleteObj(OBJ_FindTypeAt(0x97, x + 2, y + 3));
				DeleteObj(OBJ_FindTypeAt(0xc3, x + 1, y + 3));
				DeleteObj(OBJ_FindTypeAt(0xc3, x, y + 3));
				DeleteObj(OBJ_FindTypeAt(0x98, x + 2, y + 4));
				if(otherObj >= 0)
					DeleteObj(otherObj);
			}
			if(C_1810_1327(obj)) {
				MUS_PlaySfx(0x1f);
				GetQual(obj) = 1;
			}
			return 1;
		} else
			return 0;
	} else
		return 0;
}

/*push the boulder obj away from (px,py), the pusher's position*/
/*C_1810_14EA*/C_1810_14EA(int16 px, int16 py, int16 obj) {
	int16 dy, dx, done, dirs, pushN, pushW, inPit, i, j, moved, otherObj;
	int16 x, y;

	done = 0;
	dirs = 0;
	pushN = 0;
	pushW = 0;
	inPit = 0;
	otherObj = FindTypeAt(GetX(obj), GetY(obj), MapZ, 0xe6);
	if(GetQual(obj) == 1) {
		MUS_PlaySfx(0x1c);
		return 0;
	}
	x = GetX(obj);
	y = GetY(obj);
	dx = px - x;
	dy = py - y;
	if(dx == 1 || (dx == 0 && (dy == 0 || dy == -1))) {
		x --;
		dirs |= 0x10;
	} else if((dx == -2 || (dx == -1 && (dy == 0 || dy == -1))) && !C_1810_0FBF(TILE_AT(x + 1, y))) {
		x ++;
		dirs |= 0x40;
	}
	if(dy == 1 || (dy == 0 && (dx == 0 || dx == -1))) {
		y --;
		dirs |= 0x80;
	} else if((dy == -2 || (dy == -1 && (dx == 0 || dx == -1))) && !C_1810_0F98(TILE_AT(x + 1, y))) {
		y ++;
		dirs |= 0x20;
	}
	if((dirs & 0x10) && C_1810_0FBF(TILE_AT(x, y))) {
		pushW = 1;
		C_1810_13B1(obj, x --, y, 0, 0, otherObj);
		C_1810_13B1(obj, x --, y, 1, 0, otherObj);
		C_1810_13B1(obj, x, y, 1, 0, otherObj);
	} else if((dirs & 0x80) && C_1810_0F98(TILE_AT(x, y))) {
		pushN = 1;
		C_1810_13B1(obj, x, y --, 0, 0, otherObj);
		C_1810_13B1(obj, x, y --, 1, 0, otherObj);
		C_1810_13B1(obj, x, y, 1, 0, otherObj);
	} else if(!C_1810_13B1(obj, x, y, 0, 1, otherObj)) {
		return 0;
	}
	while(!done && GetQual(obj) != 1) {
		done = 1;
		if(
			C_1810_0F71(TILE_AT(x, y)) || C_1810_0F71(TILE_AT(x - 1, y)) ||
			C_1810_0F71(TILE_AT(x, y - 1)) || C_1810_0F71(TILE_AT(x - 1, y - 1))
		) {
			inPit = 1;
			if(!C_1810_0FBF(TILE_AT(x + 1, y))) {
				x ++;
				done = 0;
			}
			if(!C_1810_0F98(TILE_AT(x, y + 1))) {
				y ++;
				done = 0;
			}
			if(!done)
				C_1810_13B1(obj, x, y, 1, 0, otherObj);
		} else if(!C_1810_13B1(obj, x, y, 1, 1, otherObj)) {
			if(inPit || pushN || pushW) {
				moved = 0;
				for(i = 0; i < 3; i ++) {
					for(j = 0; j < 3; j ++) {
						if(i == 1 && j == 1)
							continue;
						if(pushN && j == 2)
							continue;
						if(pushW && i == 2)
							continue;
						if(inPit && i == 0 && j == 0)
							continue;
						if(j == 2 && C_1810_0F98(TILE_AT(x + i - 1, y + j - 1)))
							continue;
						if(i == 2 && C_1810_0FBF(TILE_AT(x + i - 1, y + j - 1)))
							continue;
						if(C_1810_13B1(obj, x + i - 1, y + j - 1, 1, 1, otherObj)) {
							moved = 1;
							break;
						}
					}
				}
				if(!moved) {
					if(inPit) {
						C_1810_13B1(obj, ++x, ++y, 1, 0, otherObj);
						done = 0;
					} else if(pushN) {
						C_1810_13B1(obj, x, --y, 1, 0, otherObj);
						done = 0;
					} else if(pushW) {
						C_1810_13B1(obj, --x, y, 1, 0, otherObj);
						done = 0;
					}
				}
			}
		}
	}
}

/*explosion of a fire bomb? (object 0x89/0x8A) at (x,y): hurts every
  actor around (1d20)*/
/*C_1810_1941*/C_1810_1941(int16 x, int16 y) {
	int16 far *list;

	MUS_PlaySfx(6);
	for(list = Explosion(x, y, 0x105); *list >= 0; list ++)
		if(!QuanType(GetType(*list)))
			LoseHP(*list, OSI_rand(1, 20));
	AfterExplosion(x, y);
	VIS_UpdateView();
	DisplayTick();
}

/*explosion of object 0xF0/0xF1 at (x,y): hurts the actors of types
  0x143/0x148 around (1d35)*/
/*C_1810_19C1*/C_1810_19C1(int16 x, int16 y) {
	int16 far *list;
	int16 objType;

	MUS_PlaySfx(0x12);
	MUS_PlaySfx(0x1d);
	for(list = Explosion(x, y, 0x109); *list >= 0; list ++) {
		objType = GetType(*list);
		if(!QuanType(objType) && (objType == 0x143 || objType == 0x148))
			LoseHP(*list, OSI_rand(1, 35));
	}
	AfterExplosion(x, y);
	VIS_UpdateView();
	DisplayTick();
}

/*range weapon management: show the missile of a ranged attack and
  handle its consequences: ammunition used, thrown weapon dropped at the
  impact tile (or lost), explosives. *pX_dest,*pY_dest are updated to the
  impact tile. returns 1*/
/*C_1810_1A5A*/static COMBAT_FireRangedWeapon(int16 att_objNum, int16 *pX_dest, int16 *pY_dest, int16 weap_objNum) {
	int16 typ;
	int16 itemObj, invChanged, range, dist_x;
	int16 dist_y, missileTile, numUsed, slot, dest_x, dest_y;

	typ = GetType(weap_objNum);
	range = __GetWeaponRange(weap_objNum);
	dest_x = *pX_dest;
	dest_y = *pY_dest;

	if(typ == 0x1f)
		missileTile = BaseTile[0x2d];
	else if(typ == 0x25)
		missileTile = BaseTile[0x1a];
	else if(typ == 0x20)
		missileTile = BaseTile[0x24];
	else if(typ == 0x32 || typ == 0x28)
		missileTile = 0x107;
	else if(typ == 0x89 || typ == 0x8a)
		missileTile = BaseTile[0x8a];
	else if(typ == 0xf0 || typ == 0xf1)
		missileTile = BaseTile[0xf1];
	else if(typ == 0x15)
		missileTile = 0x10a;
	else if(att_objNum != weap_objNum)
		missileTile = TILE_FRAME(weap_objNum);
	else
		missileTile = 0;

	switch(typ) {
		case 1: case 0x1a: case 0x1b: case 0x1f: case 0x25: case 0x2c: case 0xf0: case 0xf1:
			MUS_PlaySfx(2);
		break;
		case 0x89: case 0x8a:
			MUS_PlaySfx(0x1e);
		break;
		case 0x20:
			MUS_PlaySfx(5);
		break;
		case 0x27:
			MUS_PlaySfx(3);
		break;
		case 0x28: case 0x32:
			MUS_PlaySfx(4);
		break;
		case 0x15:
			MUS_PlaySfx(0x12);
			MUS_PlaySfx(0x1e);
			range = 7;
		break;
	}
	COMBAT_Missile(att_objNum, pX_dest, pY_dest, range, missileTile);
	invChanged = 0;
	switch(typ) {
		case 0x1f: case 0x20: case 0x25: case 0x28: case 0x32:
			if(typ == 0x1f)
				itemObj = C_0A5D_080A(att_objNum, 0x2d);
			else if(typ == 0x28)
				itemObj = C_0A5D_080A(att_objNum, 0x29);
			else if(typ == 0x20)
				itemObj = C_0A5D_080A(att_objNum, 0x24);
			else if(typ == 0x25)
				itemObj = C_0A5D_080A(att_objNum, 0x1a);
			else
				itemObj = -1;
			numUsed = 1;
			if(itemObj >= 0) {
				if(GetQuan(itemObj) > numUsed) {
					SubQuan(itemObj, numUsed);
				} else if(typ != 0x25) {
					DeleteObj(itemObj);
				} else {
					if(C_1DBD_18FD(MissileEnd[0], MissileEnd[1]))
						DeleteObj(itemObj);
					else
						MoveObj(itemObj, MissileEnd[0], MissileEnd[1], MapZ);
					C_0359_0D58(0);
				}
				invChanged = 1;
			} else if(typ == 0x32) {
				ObjShapeType[weap_objNum] = GetType(weap_objNum);
				invChanged = 1;
			}
		break;
		case 1: case 0x1a: case 0x1b: case 0x2c:
			dist_x = *pX_dest;
			dist_y = *pY_dest;
			dist_x = abs(dist_x - GetX(att_objNum));
			dist_y = abs(dist_y - GetY(att_objNum));
			if(dest_x > 1 || dest_y > 1) {/*thrown*/
				if((itemObj = C_0A5D_080A(att_objNum, typ)) >= 0) {
					if(Equipment[2] == itemObj)
						slot = 2;
					else if(Equipment[5] == itemObj)
						slot = 5;
					else if(Equipment[8] == itemObj)
						slot = 8;
					if(QuanType(typ) && GetQuan(itemObj) > 1)
						SubQuan(itemObj, 1);
					else
						DeleteObj(itemObj);
					Equipment[slot] = AddInvObj(typ, 0x18, 1, att_objNum);
				}
				if(MissileHitObj >= 0) {
					MoveObj(weap_objNum, MissileEnd[0], MissileEnd[1], MapZ);
				} else {
					if(C_1DBD_18FD(MissileEnd[0], MissileEnd[1]))
						DeleteObj(weap_objNum);
					else
						MoveObj(weap_objNum, MissileEnd[0], MissileEnd[1], MapZ);
					C_0359_0D58(0);
				}
			}
		break;
		case 0x89: case 0x8a: case 0xf0: case 0xf1:
			dist_x = *pX_dest;
			dist_y = *pY_dest;
			invChanged = 1;
			DeleteObj(weap_objNum);
			if(typ == 0xf0 || typ == 0xf1)
				C_1810_19C1(dist_x, dist_y);
			else
				C_1810_1941(dist_x, dist_y);
			if(typ == 0x8a)
				typ = 0x89;
			if(typ == 0xf1)
				typ = 0xf0;
			if((itemObj = C_0A5D_080A(att_objNum, typ)) >= 0) {
				if(Equipment[2] == itemObj)
					slot = 2;
				else if(Equipment[5] == itemObj)
					slot = 5;
				else if(Equipment[8] == itemObj)
					slot = 8;
				if(QuanType(typ) && GetQuan(itemObj) > 1)
					SubQuan(itemObj, 1);
				else
					DeleteObj(itemObj);
				Equipment[slot] = AddInvObj(typ, 0x18, 1, att_objNum);
			}
		break;
	}
	if(invChanged && StatusDisplay == 0x92 && Party[StatusMember] == att_objNum)
		StatusDirty = 1;
	return 1;
}

/*weapons ... and their damage (WeaponDamages)*/
/*105E*/int16 WeaponTypes[] = {
	0x01,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1f,0x20,0x21,0x22,0x25,0x27,0x28,0x2c,0x32,
	0x76,0x77,0x80,0x89,0x8a,0xaf,0xf0,0xf1,0x15,0xd1,0x5e
};
/*1094*/unsigned char WeaponDamages[] = {
	10, 7, 5,10, 6, 8, 6, 8, 2,12,11,14, 4,35, 7,25,
	15, 6, 3,15,15, 3, 3, 3, 5, 5, 6
};

/*maximum damage of a weapon, or of an actor's natural attack (from its
  monster class); 40 for object 0x15 with quality 1; -1 if the object is
  not a weapon*/
/*C_1810_1F3D*/GetDamagePoints(int16 objNum) {
	int16 i, damage;
	int16 objCls;

	damage = -1;
	if(IsNPC(objNum))
		objCls = GetMonsterClass(GetType(objNum));
	else
		objCls = -1;
	if(objCls >= 0) {
		damage = MonsterDamage[objCls + 1];
	} else if(GetType(objNum) == 0x15 && GetQual(objNum) == 1) {
		damage = 40;
	} else {
		for(i = 0; i < 27; i ++)
			if(GetType(objNum) == WeaponTypes[i]) {
				damage = WeaponDamages[i];
				break;
			}
	}
	return damage;
}

/*copy the state of an actor into another one*/
/*C_1810_1FD9*/CloneObject(int16 src_objNum, int16 dst_objNum) {
	ObjStatus[dst_objNum] = ObjStatus[src_objNum];
	ObjShapeType[dst_objNum] = ObjShapeType[src_objNum];
	Amount[dst_objNum] = Amount[src_objNum];
	NPCStatus[dst_objNum] = NPCStatus[src_objNum];
	STREN[dst_objNum] = STREN[src_objNum];
	DEXTE[dst_objNum] = DEXTE[src_objNum];
	INTEL[dst_objNum] = INTEL[src_objNum];
	MovePts[dst_objNum] = 0;
	ExpPoints[dst_objNum] = ExpPoints[src_objNum];
	HitPoints[dst_objNum] = HitPoints[src_objNum];
	NPCMode[dst_objNum] = NPCMode[src_objNum];
	NPCComMode[dst_objNum] = NPCComMode[src_objNum];
	SchedIndex[dst_objNum] = SchedIndex[src_objNum];
	Leader[dst_objNum] = Leader[src_objNum];
	NPCFlag[dst_objNum] = NPCFlag[src_objNum];
}

/*flash the "hit" tile over an object (if on screen and ShowHitFlash set) and
  wake it up if it was asleep*/
/*C_1810_2081*/COMBAT_showHit(int16 objNum) {
	if(ShowHitFlash) {
		int16 x = GetX(objNum) - MapX + 5;
		int16 y = GetY(objNum) - MapY + 5;
		if(x >= 0 && x < 11 && y >= 0 && y < 11) {
			/*display hit*/
			AnimateTilesAndActors();
			if(VisibilityDirty)
				VIS_UpdateView();
			ShowObjects();
			ShowObject(0x101, x, y, 1);
			DrawMapWindow(1);
			MUS_PlaySfx(!IsPlrControl(objNum)?7:8);
			VisibilityDirty = 1;
		}
		if(IsNPC(objNum))
			SetHurtThisTurn(objNum);
	}
	if(IsNPC(objNum) && IsAsleep(objNum) && (OrigShapeType[objNum] & 0x3ff) != 0x126) {
		ClrAsleep(objNum);
		ObjShapeType[objNum] = OrigShapeType[objNum];
		VisibilityDirty = 1;
	}
	ShowHitFlash = 1;
}

/*the 2x5 area hit (D_10AF says which tiles) when the object 0x107 is
  destroyed: the actors there are pushed aside and hurt (2d10), or killed
  if there is no room*/
/*10AF*/int16 D_10AF[5][2] = {{1,1},{1,1},{1,1},{1,1},{0,1}};

/*C_1810_219F*/C_1810_219F(int16 x, int16 y) {
	int16 tx, ty, i, j, dx, found;
	int16 obj, dy;

	for(i = 0; i < 2; i ++) {
		for(j = 0; j < 5; j ++) {
			if(D_10AF[j][i] == 0)
				continue;
			tx = x + i;
			ty = y + j - 6;
			for(obj = FindLoc(tx, ty, MapZ); obj != -1; obj = NextLoc()) {
				if(!IsNPC(obj))
					continue;
				dx = 0;
				dy = 0;
				found = 0;
				while(dx < 3 && !found) {
					while(dy < 3 && !found) {
						if((dx == 1) & (dy == 1))
							dy ++;
						if(CanMoveTo(obj, tx + dx - 1, ty + dy - 1))
							found = 1;
						else
							dy ++;
					}
					if(!found)
						dx ++;
				}
				if(!found) {
					LoseHP(obj, HitPoints[obj]);
				} else {
					MoveObj(obj, tx + dx - 1, ty + dy - 1, MapZ);
					LoseHP(obj, OSI_rand(1, 10) + OSI_rand(1, 10));
				}
			}
		}
	}
}

/*next direction of the segments of a multi-tile creature, by frame and
  direction / 2*/
/*10C3*/unsigned char D_10C3[][4] = {
	{9,2,9,6},{0,9,4,9},{6,4,9,9},{9,0,6,9},{9,9,2,0},{2,9,9,4}
};

/*damage to a creature made of segments (types 0x124 and 0x11A): the
  head (0x124, an NPC) passes half the damage to every segment; a
  segment that dies removes the parts that follow it; when the last one
  is gone the creature dies. returns 10 (experience) when it dies*/
/*C_1810_22F0*/C_1810_22F0(int16 objNum, int16 damage) {
	int16 objType, i, x, y, obj, dir;
	int16 head;

	objType = GetType(objNum);
	x = GetX(objNum);
	y = GetY(objNum);
	if(objType == 0x124) {
		COMBAT_showHit(objNum);
		if(IsNPC(objNum)) {
			for(i = 0x10; i < 0x100; i ++)
				if(GetType(i) == 0x11a)
					C_1810_22F0(i, damage >> 1);
		} else {
			C_1810_22F0(GetQual(objNum), damage);
		}
		return 0;
	} else if(objType == 0x11a) {
		if(HitPoints[objNum] <= damage) {
			head = GetQual(objNum);
			SubQual(head, 1);
			dir = GetDirection(objNum);
			COMBAT_Kill(objNum);
			if(CLOSE_ENOUGH(5, x, y, MapX, MapY))
				C_0359_0D58(1);
			do {
				switch(dir) {
					case 0: y ++; break;
					case 2: x --; break;
					case 4: y --; break;
					case 6: x ++; break;
				}
				for(obj = FindLoc(x, y, MapZ); obj >= 0; obj = NextLoc()) {
					if(GetType(obj) == 0x124 && GetQual(obj) == objNum)
						break;
				}
				dir = D_10C3[GetFrame(obj) - 2][dir >> 1];
				DeleteObj(obj);
				if(CLOSE_ENOUGH(5, x, y, MapX, MapY)) {
					VisibilityDirty = 1;
					DisplayTick();
				}
			} while(obj >= 0 && !IsNPC(obj));
			if(GetQual(head) == 0 || GetQual(head) == 0xff) {
				AddMapObj(0x4e, 0x20, 0, GetX(head), GetY(head), MapZ);
				COMBAT_Kill(head);
				if(CLOSE_ENOUGH(5, GetX(head), GetY(head), MapX, MapY))
					C_0359_0D58(1);
				return 10;
			}
		} else {
			HitPoints[objNum] -= damage;
		}
	}
}

/*an actor dies: experience for a hostile monster, removed from the
  party, dead body (or removed from the map). returns the experience*/
/*C_1810_25AD*/COMBAT_Kill(int16 objNum) {
	int16 objCls, xp, noShape, x, y;
	int16 i;

	xp = 0;
	if(objNum == 0x17)
		TalkFlags[0x35] |= 0x20;
	else if(objNum == 0xb4)
		TalkFlags[0x30] |= 0x80;
	if(LastAttackTarget == objNum)
		LastAttackTarget = -1;
	if(StatusDisplay != 0x91 && Party[StatusMember] == objNum) {
		SelectMode = 0;
		StatusDisplay = 0x91;
		StatusMember = 0;
		STAT_ShowParty(0);
	}
	if(InCombat) {
		if(IsPlrControl(objNum))
			CombatOutcome = -1;
		else if((NPCStatus[objNum] & 0x20) && CombatOutcome <= 0)
			CombatOutcome = 1;
	}
	if(HitPoints[objNum] >= 0) {
		HitPoints[objNum] = 0;
		SetDead(objNum);
		if(NPCStatus[objNum] & 0x20) {
			if((objCls = GetMonsterClass(GetType(objNum))) >= 0) {
				if(OrigShapeType[objNum] & 0) {
					xp = 500;
				} else {
					if(MonsterFlags[objCls + 1] & 0x20)
						xp = MonsterStr[objCls + 1] * MonsterDamage[objCls + 1];
					else
						xp = MonsterDex[objCls + 1] * MonsterDamage[objCls + 1];
					xp += (MonsterHP[objCls + 1] + MonsterArmor[objCls + 1]) * MonsterDex[objCls + 1];
					xp >>= 4;
				}
			}
		}
		ClrInvisible(objNum);
		if(GetType(objNum) != 0x11a && !IsInConversation) {
			if(objNum >= 0x10)
				CON_printf(KilledMsg, GetObjectString(objNum));
			else
				CON_printf(FallsMsg, GetObjectString(objNum));
		}
		noShape = !SetTypeUnconscious(objNum);
		NPCMode[objNum] = 0;
		if(IsPlrControl(objNum) && Party[0] != objNum) {
			for(i = 1; i <= PartySize; i ++) {
				if(Party[i] == objNum) {
					if(SoloMember >= i)
						SoloMember --;
					if(Active == i)
						SetPartyMode();
					if(Active > i)
						Active --;
					for(; i < PartySize; i ++) {
						Party[i] = Party[i + 1];
						strcpy(Names[i], Names[i + 1]);
					}
					PartySize --;
					StatusDirty = 1;
					break;
				}
			}
		}
		if(!noShape) {
			OBJ_MakeDeadBody(objNum);
		} else {
			x = GetX(objNum);
			y = GetY(objNum);
			MoveObj(objNum, MapX, MapY, 6);
			if(objNum >= 0xc0)
				ObjShapeType[objNum] = 0;
			if(objNum == 0x36)
				DeleteObj(OBJ_FindTypeAt(0x148, x, y));
		}
		VisibilityDirty = 1;
	}
	if(Party[Active] == objNum)
		CheckActiveMember();
	return xp;
}

/*inflict damage to an object. Actors: the raft (0xED) passes the damage
  to a random party member, the immortal ones keep 255 hit points, others
  die (COMBAT_Kill) or lose hit points (shown in the party list).
  Breakable items lose "quantity" points and are destroyed at 0, some with
  effects. returns the experience gained by killing a hostile monster*/
/*C_1810_289B*/LoseHP(int16 objNum, int16 damage) {
	int16 far *list;
	int16 i, x, member, objType, mouseSave, xp, savedTextOn;
	int16 y;

	if(GetZ(objNum) != MapZ)
		return 0;
	x = GetX(objNum);
	y = GetY(objNum);
	if(damage < 0)
		return 0;
	xp = 0;
	objType = GetType(objNum);
	if(objType == 0x124 || objType == 0x11a) {
		COMBAT_showHit(objNum);
		return C_1810_22F0(objNum, damage);
	}
	if(objType == 0x148)
		objNum = 0x36;
	savedTextOn = TextOn;
	TextOn = 1;
	if(IsNPC(objNum)) {
		if(objType == 0xed) {
			member = Party[0];
			if(PartySize > 1)
				member = Party[OSI_rand(0, PartySize - 1)];
			LoseHP(member, damage);
			COMBAT_displaySeverity(member);
		} else if(IsInvulnerable(objNum)) {
			HitPoints[objNum] = 0xff;
			return 0;
		} else if(HitPoints[objNum] <= damage) {
			COMBAT_showHit(objNum);
			xp = COMBAT_Kill(objNum);
		} else {
			COMBAT_showHit(objNum);
			HitPoints[objNum] -= damage;
			if(NPCMode[objNum] == 9)
				NPCMode[objNum] = 8;
		}
		if(IsPlrControl(objNum)) {
			if(StatusDisplay == 0x90 && Party[StatusMember] == objNum) {
				StatusDirty = 1;
			} else if(StatusDisplay == 0x91) {
				for(i = PartyListTop; i < PartySize && i < PartyListTop + 5; i ++) {
					if(Party[i] == objNum) {
						mouseSave = CON_mouseOffAt(183, 6, 314, 118);
						CON_setClip(&(TextWindows[1]));
						CON_gotoxy(13, (i - PartyListTop) * 2);
						if(IsPoisoned(objNum))
							CurClip->fgColor = ColorGreen;
						else if(HitPoints[objNum] < 10)
							CurClip->fgColor = ColorRed;
						CON_printf("%3d\n", HitPoints[objNum]);
						CurClip->fgColor = ColorText;
						CON_setClip(&(TextWindows[3]));
						CON_mouseOn(mouseSave);
					}
				}
			}
		}
	} else if(!QuanType(GetType(objNum)) && GetQuan(objNum)) {
		COMBAT_showHit(objNum);
		if(GetQuan(objNum) > damage) {
			SubQuan(objNum, damage);
		} else {
			PromptFlag = 1;
			if(objType == 0x7f) {
				DeleteObj(objNum);
				AddMapObj(0x80, 0x20, 0, x, y, MapZ);
				VisibilityDirty = 1;
			} else if(objType == 0x107) {
				DeleteObj(objNum);
				DeleteObj(OBJ_FindTypeAt(0x108, x - 2, y - 1));
				DeleteObj(OBJ_FindTypeAt(0x109, x, y - 2));
				DeleteObj(OBJ_FindTypeAt(0x10a, x - 2, y - 3));
				MUS_PlaySfx(0x1f);
				ObjStatus[AddMapObj(0x504, 0, 0, x, y, MapZ)] &= ~0x20;
				ObjStatus[AddMapObj(0x505, 0, 0, x - 1, y - 2, MapZ)] &= ~0x20;
				ObjStatus[AddMapObj(0x1505, 0, 0, x + 1, y - 2, MapZ)] &= ~0x20;
				ObjStatus[AddMapObj(0x2505, 0, 0, x + 1, y - 4, MapZ)] &= ~0x20;
				ObjStatus[AddMapObj(0xd06, 0, 0, x + 1, y - 5, MapZ)] &= ~0x20;
				C_1810_219F(x, y);
				VisibilityDirty = 1;
			} else if(objType == 0xde) {
				MUS_PlaySfx(0);
				for(list = Explosion(x, y, 0x106); *list >= 0; list ++)
					if(!QuanType(GetType(*list)))
						LoseHP(*list, OSI_rand(1, 30));
				DeleteObj(objNum);
				VIS_UpdateView();
				DisplayTick();
				for(i = 0xc00; i > 0xff; i --)
					if(GetType(i) == 0x100)
						ObjShapeType[i] = (ObjShapeType[i] & 0xfc00) + 0xdf;
				VisibilityDirty = 1;
				DisplayTick();
				if(!(TalkFlags[1] & 4))
					C_1DBD_0304();
				VisibilityDirty = 1;
			} else if(objType == 0x93) {
				MUS_PlaySfx(0);
				Explosion(x, y, 0x104);
				Explosion(x, y, 0x104);
				DisplayTick();
				MUS_PlaySfx(0);
				Explosion(x, y, 0x106);
				Explosion(x, y, 0x106);
				DisplayTick();
				MUS_PlaySfx(0);
				Explosion(x, y, 0x102);
				Explosion(x, y, 0x102);
				DisplayTick();
				DeleteObj(objNum);
				VisibilityDirty = 1;
				C_1DBD_077E();
			} else {
				DeleteObj(objNum);
				VisibilityDirty = 1;
			}
		}
	}
	TextOn = savedTextOn;
	return xp;
}

/*does the attack hit? always against an object; some weapon/target pairs
  always hit; dexterity (or strength) of the attacker against the
  defender's dexterity (20 for types 0x143/0x148)*/
/*C_1810_2E13*/static __AttackTest(int16 objNum_att, int16 objNum_def, int16 objNum_weapon) {
	int16 objTyp, attackStat;
	int16 defDex, objCls, strOverDex;
	int16 defTyp;

	if(!IsNPC(objNum_def))
		return 1;
	objTyp = GetType(objNum_weapon);
	defTyp = GetType(objNum_def);
	if(objTyp == 1 && defTyp == 0x126)
		return 1;
	if(objTyp == 0xd1 && defTyp == 0x11)
		return 1;
	if(objTyp == 0x15 && (defTyp == 0x143 || defTyp == 0x148)) {
		if(OSI_rand(0, 7) == 0)
			return 0;
		return 1;
	}
	if(objNum_weapon == objNum_att) {
		objCls = GetMonsterClass(objTyp);
		strOverDex = MonsterFlags[objCls + 1] & 0x20;
	} else {
		strOverDex = (
			objTyp == 0x18 || objTyp == 0x1d || objTyp == 0x22 || objTyp == 0x76 ||
			objTyp == 0x21 || objTyp == 0x77 || objTyp == 0x5e
		);
	}
	if(strOverDex)
		attackStat = GetStr(objNum_att);
	else
		attackStat = GetDex(objNum_att);
	defDex = GetDex(objNum_def);
	if(defTyp == 0x143 || defTyp == 0x148)
		defDex = 20;

	return OSI_rand(1, 30) >= (defDex + 30 - attackStat) / 2;
}

/*print how badly an actor is wounded (in quarters of its max HP);
  a critically wounded intelligent NPC in a mobile combat mode flees*/
/*C_1810_2F42*/COMBAT_displaySeverity(int16 objNum) {
	int16 severity;

	if(GetType(objNum) == 0x124)
		return;
	if(GetType(objNum) == 0x11a)
		return;
	if(HitPoints[objNum] == 0)
		return;
	severity = (HitPoints[objNum] << 2) / MaxHP(objNum);
	if(severity < 4) {
		CON_printf("`%s ", GetObjectString(objNum));
		PromptFlag = 1;
	}
	if(severity == 0) {/*critical*/
		CON_printf(CriticalMsg);
		if(
			GetInt(objNum) >= 5 &&
			NPCMode[objNum] != 6 &&
			NPCMode[objNum] != 0xe &&
			NPCMode[objNum] != 2 &&
			NPCMode[objNum] != 1 &&
			NPCMode[objNum] != 0
		) NPCMode[objNum] = 7;
	} else if(severity == 1) {/*heavily*/
		CON_printf(HeavilyMsg);
	} else if(severity == 2) {/*lightly*/
		CON_printf(LightlyMsg);
	} else if(severity == 3) {/*barely*/
		CON_printf(BarelyMsg);
	}
	if(severity != 0 && severity < 4)
		CON_printf(WoundedMsg);
}

/*uncalled: left from Ultima VI (an item of ownerObj that acid or a thief
  takes), now always -1*/
/*C_1810_3043*/static COMBAT_PickVulnerableItem(int16 ownerObj, int16 wantFood) {
	return -1;
}

/*a neutral actor (not in the party) is attacked: "<name> attacks!" when
  it is of a fighting class, a scheduled NPC hit by the player gets mode
  0x9B, else it turns chaotic (if attacked by a good one); temporary
  monsters get mode 8*/
/*C_1810_304D*/COMBAT_ProvokeNeutral(int16 victim, int16 attacker, int16 damage) {
	int16 objCls;

	if(!IsNPC(victim))
		return;
	if(GetAlignment(victim) != 0)
		return;
	if(!IsPlrControl(victim)) {
		objCls = GetMonsterClass(GetType(victim));
		if(NPCMode[attacker] == 2 && objCls >= 0 && (MonsterFlags[objCls + 1] & 0x4000) && damage > 0) {
			CON_printf(AttacksMsg, GetObjectString(victim));
			PromptFlag = 1;
		}
		if(victim < 0xc0 && NPCMode[victim] >= 0x80) {
			if(NPCMode[attacker] == 2)
				NPCMode[victim] = 0x9b;
			return;
		}
		if(GetAlignment(attacker) == 0x40 && !Is_ATKPLR(victim))
			SetAlignment(victim, 0x60);
		if(victim >= 0xc0)
			NPCMode[victim] = 8;
	}
}

/*a hit landed: roll the damage (1..hitPts; 255 is a sure kill), subtract
  1..armour, apply it, print the messages and poison*/
/*C_1810_3134*/static __DoHit(int16 attacker, int16 defender, int16 hitPts) {
	char *pObjStr;
	int16 armorPts, objCls, defTyp;

	defTyp = GetType(defender);
	if(defender < 0)
		return;
	pObjStr = GetObjectString(defender);

	if(hitPts == -1)
		hitPts = 1;
	else if(hitPts > 1 && hitPts < 255)
		hitPts = OSI_rand(1, hitPts);

	if(IsNPC(defender))
		armorPts = __GetProtection(defender);
	else
		armorPts = 0;
	if(armorPts > 0 && hitPts < 255)
		hitPts -= OSI_rand(1, armorPts);

	if(hitPts <= 0) {
		COMBAT_showHit(defender);
		CON_printf(GrazedMsg, pObjStr);
		PromptFlag = 1;
	} else {
		if(Is_ATKPLR(attacker) && IsPlrControl(defender) && !InCombat && SoloMember == -1)
			COMBAT_beginAttacked();
		AddXP(attacker, LoseHP(defender, hitPts));
	}
	if(!IsNPC(defender))
		return;

	if(defender == SelectionObj)
		COMBAT_ProvokeNeutral(defender, attacker, hitPts);
	if(HitPoints[defender] == 0 && defTyp != 0xed) {
		PromptFlag = 1;
		if(StatusDisplay == 0x90 && Party[StatusMember] == attacker)
			StatusDirty = 1;
		return;
	}
	objCls = GetMonsterClass(GetType(attacker));
	if(
		IsNPC(defender) && (MonsterFlags[objCls + 1] & 0x40) && OSI_rand(0, 3) == 0 &&
		!IsProtected(defender) && !IsInvulnerable(defender)
	) {
		CON_printf(PoisonedMsg, pObjStr);
		PromptFlag = 1;
		SetPoisoned(defender);
		if(
			StatusDisplay == 0x91 ||
			(StatusDisplay == 0x90 && Party[StatusMember] == defender)
		) StatusDirty = 1;
	}
	if(!IsDead(defender) && hitPts > 0 && defTyp != 0x124 && defTyp != 0x11a)
		COMBAT_displaySeverity(defender);
}

/*return the actor owning a part of a multi-tile creature (number in the
  part's quality); the object itself otherwise*/
/*C_1810_3329*/COMBAT_getHead(int16 objNum) {
	int16 objType;

	if(objNum < 0)
		return -1;
	objType = GetType(objNum);
	if(OBJ_PartHeadType(objType))
		objNum = (objNum < 0x100 || !GetQual(objNum))?objNum:GetQual(objNum);
	return objNum;
}

/*attack (x,y) with a weapon: range, ammunition, missile, hit test,
  damage; some weapon/target special cases.
  returns 0: done, 2: out of range, 3: not with that weapon*/
/*C_1810_337D*/static __TryHit(int16 att_objNum, int16 x, int16 y, int16 weap_objNum) {
	int16 weap_typ, opp_objNum;
	int16 damage, didHit;
	int16 isRanged, objTyp, range;

	if(GetType(att_objNum) == 0x133)
		weap_objNum = att_objNum;
	objTyp = GetType(weap_objNum);
	if(objTyp == 0x15 && (GetType(SelectionObj) == 0x143 || GetType(SelectionObj) == 0x148))
		SetQual(weap_objNum, 1);
	else if(objTyp == 0x15)
		SetQual(weap_objNum, 0);
	if((range = COMBAT_AttackRange(x, y, att_objNum)) > __GetWeaponRange(weap_objNum))
		return 2;
	if(GetType(SelectionObj) == 0x11 && objTyp != 0xd1 && objTyp != 0x89 && objTyp != 0x8a)
		return 3;
	if(GetType(SelectionObj) == 0x107 && objTyp != 0x89 && objTyp != 0x8a && objTyp != 0x76)
		return 3;
	/*ammunition*/
	if(objTyp == 0x1f && C_0A5D_080A(att_objNum, 0x2d) < 0)
		return 0;
	if(objTyp == 0x28 && C_0A5D_080A(att_objNum, 0x29) < 0)
		return 0;
	if(objTyp == 0x20 && C_0A5D_080A(att_objNum, 0x24) < 0)
		return 0;
	if(objTyp == 0x25 && C_0A5D_080A(att_objNum, 0x1a) < 0)
		return 0;
	if(objTyp == 0x32 && GetFrame(weap_objNum) == 0) {
		LoadFlintlock(weap_objNum, att_objNum);
		return 0;
	}

	MissileHitObj = SelectionObj;
	weap_typ = GetType(weap_objNum);
	isRanged = 0;
	if(range > 1)
		isRanged = 1;
	if(
		weap_typ == 0x1f || weap_typ == 0x32 || weap_typ == 0x28 || weap_typ == 0x25 ||
		weap_typ == 0x20 || weap_typ == 0x89 || weap_typ == 0xf0 ||
		(weap_typ == 0x15 && GetQual(weap_objNum) == 1) ||
		weap_typ == 0x27 || weap_typ == 0x8a || weap_typ == 0xf1
	) isRanged = 1;

	damage = GetDamagePoints(weap_objNum);
	if(damage < 0)
		damage = 1;

	didHit = -1;
	opp_objNum = COMBAT_getHead(MissileHitObj);
	if(MissileHitObj >= 0 && IsNPC(opp_objNum))
		didHit = __AttackTest(att_objNum, opp_objNum, weap_objNum);
	if(isRanged && !COMBAT_FireRangedWeapon(att_objNum, &x, &y, weap_objNum))
		didHit = -1;
	opp_objNum = COMBAT_getHead(MissileHitObj);
	if(!isRanged && COMBAT_TileDistance(att_objNum, MapX, MapY) < 6)
		MUS_PlaySfx(0x1f);/*melee sound*/
	if(didHit == -1) {
		didHit = 1;
		if(MissileHitObj >= 0 && IsNPC(opp_objNum))
			didHit = __AttackTest(att_objNum, opp_objNum, weap_objNum);
	}

	if(didHit != -1 && MissileHitObj >= 0 && opp_objNum != att_objNum) {
		if(weap_typ == 1) {
			SetType(weap_objNum, 0x1a);
			SetQuan(weap_objNum, 1);
			if(GetType(opp_objNum) != 0x126) {
				if(IsNPC(opp_objNum))
					LoseHP(opp_objNum, HitPoints[opp_objNum]);
				else
					__DoHit(att_objNum, opp_objNum, damage);
			} else {
				COMBAT_showHit(opp_objNum);
				SetAsleep(opp_objNum);
				SetTypeUnconscious(opp_objNum);
			}
			if(StatusDisplay == 0x92 && Party[StatusMember] == att_objNum)
				STAT_DrawInventory();
		} else if(weap_typ == 0x20) {
			if(IsNPC(opp_objNum) && OSI_rand(0, 1) && !IsProtected(opp_objNum) && !IsInvulnerable(opp_objNum)) {
				CON_printf(PoisonedMsg, GetObjectString(opp_objNum));
				PromptFlag = 1;
				SetPoisoned(opp_objNum);
				if(
					StatusDisplay == 0x91 ||
					(StatusDisplay == 0x90 && Party[StatusMember] == opp_objNum)
				) StatusDirty = 1;
			} else {
				__DoHit(att_objNum, opp_objNum, damage);
			}
		} else if(weap_typ == 0xd1 && GetType(opp_objNum) == 0x11) {
			C_1DBD_4F8E(GetX(opp_objNum), GetY(opp_objNum), AddMapObj(0xf2, 0x20, 0, GetX(opp_objNum), GetY(opp_objNum), MapZ));
			C_0359_0D58(0);
		} else {
			__DoHit(att_objNum, opp_objNum, damage);
		}
	}
	/*returns*/
	if(weap_typ == 0x27)
		Missiles(GetX(att_objNum), GetY(att_objNum), 1, &x, &y, 100, BaseTile[0x27], 1);

	return 0;
}

/*reset equiped weapon list: collect the (up to 8) equipped weapons of
  the active party member into EquipWeapons and print the first one*/
/*C_1810_3840*/static COMBAT_InitWeaponList() {
	int16 invObj, attacker;

	attacker = Party[Active];
	NumEquipWeapons =
	CurEquipWeapon = 0;
	for(invObj = FindInv(attacker); invObj >= 0 && NumEquipWeapons < 8; invObj = NextInv()) {
		if(GetCoordUse(invObj) == 0x18 && GetDamagePoints(invObj) >= 0) {
			EquipWeapons[NumEquipWeapons] = invObj;
			NumEquipWeapons ++;
		}
	}
	if(GetType(attacker) == 0x133)
		CON_printf("with turtle");
	else if(NumEquipWeapons == 0)
		CON_printf(BareHandsMsg);
	else
		CON_printf(WithMsg, GetObjectString(EquipWeapons[0]));
}

/*next equiped weapon list: attack the selected target with the current
  weapon (EquipWeapons[CurEquipWeapon], bare hands if none), then prepare the
  targeting of the next equipped weapon, or end the command*/
/*C_1810_38E7*/static COMBAT_AttackWithNextWeapon() {
	int16 attacker, weapTyp;
	int16 weapObj, hitResult;

	attacker = Party[Active];
	if(NumEquipWeapons == 0)
		weapObj = attacker;
	else
		weapObj = EquipWeapons[CurEquipWeapon];
	weapTyp = GetType(weapObj);
	if(weapTyp == 0x1f && C_0A5D_080A(attacker, 0x2d) < 0) {
		CON_printf(OutOfArrowsMsg);
	} else if(weapTyp == 0x28 && C_0A5D_080A(attacker, 0x29) < 0) {
		CON_printf(OutOfAmmoMsg);
	} else if(weapTyp == 0x20 && C_0A5D_080A(attacker, 0x24) < 0) {
		CON_printf(OutOfDartsMsg);
	} else if(weapTyp == 0x25 && C_0A5D_080A(attacker, 0x1a) < 0) {
		CON_printf(OutOfSpearsMsg);
	} else if(weapTyp == 0x32 && GetFrame(weapObj) == 0) {
		CON_printf(FlintlockNotLoadedMsg);
	} else {
		hitResult = 0;
		if(SelectionObj >= 0) {
			if(SelectionObj == attacker)
				CON_printf(AttackSelfPassMsg);
			else
				CON_printf("%s.\n", GetObjectString(SelectionObj));
		} else {
			if(SelectionIsDark)
				CON_printf(NothingMsg);
			else
				CON_printf("%s.\n", GetTileString(GetTileAtXYZ(SelectionX, SelectionY, MapZ)));
		}
		if(SelectionObj != attacker) {
			CON_printf("\n");
			SkipNewlines = 1;
			if(SelectionObj < 0)
				SelectionObj = COMBAT_GetTargetAt(attacker, SelectionX, SelectionY);
			hitResult = __TryHit(attacker, SelectionX, SelectionY, weapObj);
			if(hitResult == 2) {
				CON_printf(OutOfRangeMsg);
				LastAttackTarget = -1;
				MUS_PlaySfx(0x1c);
			} else if(hitResult == 3) {
				CON_printf(NotPossibleMsg);
				LastAttackTarget = -1;
			}
		}
	}
	CurEquipWeapon ++;
	if(CurEquipWeapon < NumEquipWeapons) {
		SelectMode = 1;
		if(GetType(EquipWeapons[CurEquipWeapon]) == 0x15)
			SelectRange = 7;
		else
			SelectRange = __GetWeaponRange(EquipWeapons[CurEquipWeapon]);
		if(COMBAT_AttackRange(AimX + MapX - 5, AimY + MapY - 5, attacker) > SelectRange) {
			AimX =
			AimY = 5;
		}
		LastNumberPlural = 0;
		CON_printf(AttackWithMsg, GetObjectString(EquipWeapons[CurEquipWeapon]));
	} else {
		MouseMode = 0;
		SelectMode = 0;
		CON_UpdateMousePointer();
	}
}

/*the player's "Attack" command: select a target for each equipped
  weapon in turn*/
/*C_1810_3B65*/COMBAT_attack() {
	int16 attacker;
	int16 ch, weapon_objNum;

	attacker = Party[Active];
	CON_printf(AttackMsg);
	if(Active == PartySize) {
		CON_printf("-");
		CON_printf(NotOnShipMsg);
		return;
	}
	CON_printf(" ");
	MouseMode = 1;
	PendingCommand = 0x87;
	CON_UpdateMousePointer();
	COMBAT_InitWeaponList();
	if(NumEquipWeapons == 0)
		weapon_objNum = attacker;
	else
		weapon_objNum = EquipWeapons[CurEquipWeapon];
	if(GetType(weapon_objNum) == 0x15)
		SelectRange = 7;
	else
		SelectRange = __GetWeaponRange(weapon_objNum);
	if(COMBAT_AttackRange(AimX + MapX - 5, AimY + MapY - 5, attacker) > SelectRange) {
		AimX =
		AimY = 5;
	}
	ch = CON_getch();
	LastAttackTarget = SelectionObj;
	if(ch == 0x8e && (SelectionObj < 0 || GetCoordUse(SelectionObj) == 0)) {
		do {
			COMBAT_AttackWithNextWeapon();
			if(MouseMode == 1) {
				RefreshStatus();
				ch = CON_getch();
				if(ch != 0x8e) {
					CON_printf(WhatMsg);
					break;
				}
			}
		} while(MouseMode == 1);
	} else {
		CON_printf(WhatMsg);
	}
	SubMov(Party[Active], 10);
	MouseMode = 0;
	SelectMode = 0;
	CON_UpdateMousePointer();
}

/*combat distance between two locations (RangeTable; 9 if too far)*/
/*C_1810_3CC2*/COMBAT_range(int16 x0, int16 y0, int16 x1, int16 y1) {
	int16 delta_x, delta_y;
	int16 range;

	delta_x = abs(x0 - x1);
	delta_y = abs(y0 - y1);
	if(delta_x < 8 && delta_y < 8)
		range = RangeTable[delta_x][delta_y];
	else
		range = 9;
	return range;
}

/*combat distance from an attacker to (x,y): COMBAT_range for ordinary
  actors; for the creatures made of several tiles (C_130F_xxxx kinds), 1
  if (x,y) touches the creature's body (which depends on its facing,
  frame / 4, 8, 16 or 6), else 9*/
/*C_1810_3D13*/COMBAT_AttackRange(int16 x, int16 y, int16 objNum) {
	int16 dir, frame, range;
	int16 xo, yo;

	range = 9;
	xo = GetX(objNum);
	yo = GetY(objNum);
	if(C_130F_0136(objNum))
		return COMBAT_range(x, y, xo, yo);
	frame = GetFrame(objNum);
	if(C_130F_0002(objNum)) {
		dir = frame >> 2;
		switch(dir) {
			case 0:
				if(y >= yo - 2 && y <= yo - 1 && x >= xo - 1 && x <= xo + 1)
					range = 1;
			break;
			case 1:
				if(y >= yo - 1 && y <= yo + 1 && x >= xo && x <= xo + 1)
					range = 1;
			break;
			case 2:
				if(y >= yo && y <= yo + 1 && x >= xo - 1 && x <= xo + 1)
					range = 1;
			break;
			case 3:
				if(y >= yo - 1 && y <= yo + 1 && x >= xo - 2 && x <= xo - 1)
					range = 1;
			break;
		}
	} else if(C_130F_004D(objNum)) {
		if(C_130F_1199(objNum))
			dir = frame / 6;
		else
			dir = frame >> 2;
		switch(dir) {
			case 0:
				if(y >= yo - 1 && x >= xo - 1 && x <= xo)
					range = 1;
			break;
			case 1:
				if(y >= yo - 1 && y <= yo && x <= xo + 1)
					range = 1;
			break;
			case 2:
				if(y <= yo + 1 && x >= xo - 1 && x <= xo)
					range = 1;
			break;
			case 3:
				if(y >= yo - 1 && y <= yo && x >= xo - 1)
					range = 1;
			break;
		}
	} else if(C_130F_0070(objNum)) {
		if(C_130F_1199(objNum))
			dir = frame / 6;
		else
			dir = frame >> 2;
		switch(dir) {
			case 0:
				if(y >= yo - 2 && y <= yo && x >= xo - 1 && x <= xo + 1)
					range = 1;
			break;
			case 1:
				if(y >= yo - 1 && y <= yo + 1 && x >= xo && x <= xo + 2)
					range = 1;
			break;
			case 2:
				if(y >= yo && y <= yo + 2 && x >= xo - 1 && x <= xo + 1)
					range = 1;
			break;
			case 3:
				if(y >= yo - 1 && y <= yo + 1 && x >= xo - 2 && x <= xo)
					range = 1;
			break;
		}
	} else if(C_130F_00A7(objNum)) {
		dir = frame >> 3;
		switch(dir) {
			case 0:
				if(y >= yo - 2 && y <= yo - 1 && x >= xo - 2 && x <= xo + 1)
					range = 1;
			break;
			case 1:
				if(y >= yo - 2 && y <= yo + 1 && x >= xo && x <= xo + 1)
					range = 1;
			break;
			case 2:
				if(y >= yo && y <= yo + 1 && x >= xo - 2 && x <= xo + 1)
					range = 1;
			break;
			case 3:
				if(y >= yo - 2 && y <= yo + 1 && x >= xo - 2 && x <= xo - 1)
					range = 1;
			break;
		}
	} else if(C_130F_00CA(objNum)) {
		if(C_130F_1199(objNum))
			dir = frame / 6;
		else
			dir = frame >> 4;
		switch(dir) {
			case 0:
				if(y >= yo - 3 && y <= yo - 1 && x >= xo - 2 && x <= xo + 1)
					range = 1;
			break;
			case 1:
				if(y >= yo - 2 && y <= yo + 1 && x >= xo && x <= xo + 2)
					range = 1;
			break;
			case 2:
				if(y >= yo && y <= yo + 2 && x >= xo - 2 && x <= xo + 1)
					range = 1;
			break;
			case 3:
				if(y >= yo - 2 && y <= yo + 1 && x >= xo - 3 && x <= xo - 1)
					range = 1;
			break;
		}
	}
	return range;
}

/*choose what an NPC attacks with: the equipped weapon with the best
  damage that reaches the opponent, else its natural attack (att_objNum
  itself)*/
/*C_1810_4277*/static __PickWeapon(int16 att_objNum, int16 opp_objNum) {
	int16 ret = -1, i;
	int16 opp_x, opp_y, att_x, att_y;
	int16 range, dmg, max_dmg, itemTyp;

	ret = att_objNum;
	max_dmg = 0;
	opp_x = GetX(opp_objNum);
	opp_y = GetY(opp_objNum);
	att_x = GetX(att_objNum);
	att_y = GetY(att_objNum);
	range = COMBAT_AttackRange(opp_x, opp_y, att_objNum);
	for(i = FindInv(att_objNum); i >= 0; i = NextInv()) {
		itemTyp = GetType(i);
		if(itemTyp == 0x15 && (GetType(opp_objNum) == 0x143 || GetType(opp_objNum) == 0x148))
			SetQual(i, 1);
		else if(itemTyp == 0x15)
			SetQual(i, 0);
		if(GetCoordUse(i) == 0x18) {
			dmg = GetDamagePoints(i);
			if(dmg > max_dmg && __GetWeaponRange(i) >= range) {
				max_dmg = dmg;
				ret = i;
			}
		}
	}
	return ret;
}

/*closest (euclidian) visible enemy within 11 tiles, according to the
  alignments (GOOD attacks the ones hostile to the party, EVIL the ones
  hostile to monsters, CHAOTIC everything but chaotic or neutral,
  NEUTRAL anything not neutral); the strongest attack wins a tie.
  Retreating actors off-screen are ignored. returns -1 if none*/
/*C_1810_4391*/COMBAT_pickOpponent(int16 objNum) {
	int16 i;
	int16 x, y, opp_x, opp_y, objAln;
	int16 dst2, ret, min_dst2;

	ret = -1;
	objAln = GetAlignment(objNum);
	x = GetX(objNum);
	y = GetY(objNum);
	min_dst2 = 0x7fff;
	for(i = 0; i < 0x100; i++) {
		if(ObjShapeType[i] == 0)
			continue;
		if(IsDead(i))
			continue;
		if(i == objNum)
			continue;
		if(!COMBAT_canSee(objNum, i))
			continue;
		if(objAln == 0 && GetAlignment(i) == 0)
			continue;
		if(objAln == 0x60 && (GetAlignment(i) == 0x60 || GetAlignment(i) == 0))
			continue;
		if(objAln == 0x40 && !Is_ATKPLR(i))
			continue;
		if(objAln == 0x20 && !Is_ATKMON(i))
			continue;
		opp_x = GetX(i);
		opp_y = GetY(i);
		if(COMBAT_TileDistance(objNum, opp_x, opp_y) > 11)
			continue;
		if(NPCMode[i] == 7 && !CLOSE_ENOUGH(5, opp_x, opp_y, MapX, MapY))
			continue;
		dst2 = (opp_x - x) * (opp_x - x) + (opp_y - y) * (opp_y - y);
		if(
			dst2 < min_dst2 ||
			(dst2 == min_dst2 && MonsterDamage[GetMonsterClass(GetType(i)) + 1] > MonsterDamage[GetMonsterClass(GetType(ret)) + 1])
		) {
			min_dst2 = dst2;
			ret = i;
		}
	}
	return ret;
}

/*the best target for an explosive thrown by objNum: the tile of the
  11x9 area around the active party member with the most enemies in
  the 3x3 square around it (the centre counts twice); with notAllies 0
  (fire bombs?) the square must not hold the thrower or a party ally,
  with 1 only the types 0x143/0x148 count. returns an object at the
  centre, -1 if none*/
/*C_1810_4561*/C_1810_4561(int16 objNum, int16 notAllies) {
	int16 i, j, x, y, px, py, tx, ty, best, center, leader, bestCount, dx, dy, bad;
	int16 obj, count;

	leader = Party[Active];
	px = GetX(leader);
	py = GetY(leader);
	best = -1;
	x = GetX(objNum);
	y = GetY(objNum);
	bestCount = 0;
	for(i = 0; i < 11; i ++) {
		for(j = 0; j < 9; j ++) {
			tx = px + i - 5;
			ty = py + j - 4;
			if((center = FindLoc(tx, ty, MapZ)) < 0)
				continue;
			if(IsDead(center))
				continue;
			count = 0;
			bad = 0;
			for(dx = -1; dx < 2; dx ++) {
				for(dy = -1; dy < 2; dy ++) {
					for(obj = FindLoc(tx + dx, ty + dy, MapZ); obj >= 0; obj = NextLoc()) {
						if(((IsNPC(obj) && GetAlignment(obj) == 0x40 && !IsDead(obj)) || obj == objNum) && notAllies == 0) {
							bad = 1;
							break;
						}
						if(
							IsNPC(obj) && !IsDead(obj) &&
							(GetAlignment(obj) == 0x20 || GetAlignment(obj) == 0x60) &&
							(notAllies == 0 || GetType(obj) == 0x143 || GetType(obj) == 0x148)
						) {
							count ++;
							if(dx == 0 && dy == 0)
								count ++;
						}
					}
					if(bad == 1)
						break;
				}
				if(bad == 1)
					break;
			}
			if(bad == 1)
				continue;
			if(count > bestCount || (count == bestCount && OSI_rand(0, 1))) {
				bestCount = count;
				best = center;
			}
		}
	}
	return best;
}

/*AI_ASSAULT (also the fallback of the other modes): attack the closest
  enemy if in range and in line of fire, else sidestep (90 degrees);
  then move toward it*/
/*C_1810_476D*/COMBAT_AI_Assault(int16 objNum) {
	int16 weapObj, opp_x, opp_y, x, y, range, dist;
	int16 weapTyp;

	SelectionObj = COMBAT_pickOpponent(objNum);
	weapObj = __PickWeapon(objNum, SelectionObj);
	weapTyp = GetType(weapObj);
	if(weapTyp == 0x89 || weapTyp == 0x8a)
		SelectionObj = C_1810_4561(objNum, 0);
	else if(weapTyp == 0xf0 || weapTyp == 0xf1)
		SelectionObj = C_1810_4561(objNum, 1);
	if(SelectionObj >= 0) {
		opp_x = GetX(SelectionObj);
		opp_y = GetY(SelectionObj);
		x = GetX(objNum);
		y = GetY(objNum);
		if(weapTyp == 0x15 && (GetType(SelectionObj) == 0x143 || GetType(SelectionObj) == 0x148))
			SetQual(weapObj, 1);
		else if(weapTyp == 0x15)
			SetQual(weapObj, 0);
		range = __GetWeaponRange(weapObj);
		if((dist = COMBAT_AttackRange(opp_x, opp_y, objNum)) <= range) {
			if(dist <= 1 || COMBAT_Missile(objNum, &opp_x, &opp_y, range, 0)) {
				__TryHit(objNum, opp_x, opp_y, weapObj);
				SubMov(objNum, 10);
			} else {
				/*no line of fire: step perpendicular to the target*/
				if(OSI_rand(0, 1)) {
					opp_x = x - (GetY(SelectionObj) - y);
					opp_y = y + (GetX(SelectionObj) - x);
				} else {
					opp_x = x + (GetY(SelectionObj) - y);
					opp_y = y - (GetX(SelectionObj) - x);
				}
				TryMoveTo(objNum, opp_x, opp_y);
			}
		}
		TryMoveTo(objNum, opp_x, opp_y);
	} else {
		WanderRandomly(objNum);
	}
}

/*choose a ranged weapon (range 3 or more) for an NPC that keeps its
  distance: the longest range until one reaches the opponent, then the
  best damage; -1 if none*/
/*C_1810_4968*/C_1810_4968(int16 att_objNum, int16 opp_objNum) {
	int16 opp_x, opp_y, att_x, att_y, ret, dmg, max_dmg, itemTyp, range, wrange, oppTyp;
	int16 maxRange, i;

	maxRange = 0;
	ret = -1;
	max_dmg = 0;
	opp_x = GetX(opp_objNum);
	opp_y = GetY(opp_objNum);
	att_x = GetX(att_objNum);
	att_y = GetY(att_objNum);
	oppTyp = GetType(opp_objNum);
	range = COMBAT_AttackRange(opp_x, opp_y, att_objNum);
	i = FindInv(att_objNum);
	while(i >= 0) {
		itemTyp = GetType(i);
		if(itemTyp == 0x15 && (oppTyp == 0x143 || oppTyp == 0x148))
			SetQual(i, 1);
		else if(itemTyp == 0x15)
			SetQual(i, 0);
		if(__GetWeaponRange(i) < 3 || GetCoordUse(i) != 0x18) {
			i = NextInv();
			continue;
		}
		if(maxRange < range) {
			if((wrange = __GetWeaponRange(i)) > maxRange) {
				maxRange = wrange;
				max_dmg = GetDamagePoints(i);
				ret = i;
			}
		} else {
			if((dmg = GetDamagePoints(i)) > max_dmg) {
				maxRange = __GetWeaponRange(i);
				max_dmg = dmg;
				ret = i;
			}
		}
		i = NextInv();
	}
	return ret;
}

/*AI_RANGE: keep away (3 tiles) from the closest enemy and shoot it with
  a ranged weapon; stay near the party*/
/*C_1810_4AA9*/COMBAT_AI_Range(int16 objNum) {
	int16 weapObj, opp_x, opp_y, x, range, dist, weapTyp;
	int16 y;

	x = GetX(objNum);
	y = GetY(objNum);
	SelectionObj = COMBAT_pickOpponent(objNum);
	weapObj = C_1810_4968(objNum, SelectionObj);
	if(SelectionObj >= 0 && abs(GetX(SelectionObj) - x) < 3 && abs(GetY(SelectionObj) - y) < 3) {
		TryMoveTo(objNum, x - GetX(SelectionObj), y - GetY(SelectionObj));
	} else if(weapObj == -1) {
		CON_printf("\n");
		CON_printf(GetObjectString(objNum));
		CON_printf(" has no ranged weapons!\n");
	} else if(abs(x - MapX) > 5 || abs(y - MapY) > 4) {
		TryMoveTo(objNum, MapX, MapY);
	} else {
		weapTyp = GetType(weapObj);
		if(weapTyp == 0x89 || weapTyp == 0x8a)
			SelectionObj = C_1810_4561(objNum, 0);
		else if(weapTyp == 0xf0 || weapTyp == 0xf1)
			SelectionObj = C_1810_4561(objNum, 1);
		if(SelectionObj >= 0) {
			opp_x = GetX(SelectionObj);
			opp_y = GetY(SelectionObj);
			if(weapTyp == 0x15 && (GetType(SelectionObj) == 0x143 || GetType(SelectionObj) == 0x148))
				SetQual(weapObj, 1);
			else if(weapTyp == 0x15)
				SetQual(weapObj, 0);
			range = __GetWeaponRange(weapObj);
			if((dist = COMBAT_AttackRange(opp_x, opp_y, objNum)) <= range) {
				if(COMBAT_Missile(objNum, &opp_x, &opp_y, range, 0)) {
					__TryHit(objNum, opp_x, opp_y, weapObj);
					SubMov(objNum, 10);
				} else {
					/*no line of fire: step perpendicular to the target*/
					if(OSI_rand(0, 1)) {
						opp_x = x - (GetY(SelectionObj) - y);
						opp_y = y + (GetX(SelectionObj) - x);
					} else {
						opp_x = x + (GetY(SelectionObj) - y);
						opp_y = y - (GetX(SelectionObj) - x);
					}
					TryMoveTo(objNum, opp_x, opp_y);
				}
			} else {
				TryMoveTo(objNum, opp_x, opp_y);
			}
		} else {
			WanderRandomly(objNum);
		}
	}
}

/*AI_BERSERK: attack the strongest (STR+DEX+INT) visible enemy within 8
  tiles*/
/*C_1810_4D85*/COMBAT_AI_Berserk(int16 objNum) {
	int16 i;
	int16 weapObj, bestOpp, objAln, x, y, oppStats, maxStats;
	int16 oppX, oppY, range, delta_x, delta_y, mustMove, savedMovePts;
	int16 weapTyp, dist;

	bestOpp = -1;
	mustMove = 0;
	objAln = GetAlignment(objNum);
	x = GetX(objNum);
	y = GetY(objNum);
	maxStats = 0;
	for(i = 0; i < 0x100; i ++) {
		if(
			ObjShapeType[i] &&
			IsDead(i) == 0 &&
			i != objNum &&
			COMBAT_TileDistance(i, x, y) <= 8 &&
			COMBAT_canSee(objNum, i) &&
			(objAln != 0x40 || GetAlignment(i) == 0x20) &&
			(objAln != 0x20 || GetAlignment(i) == 0x40) &&
			(objAln != 0x60 || (GetAlignment(i) != 0x60 && GetAlignment(i) != 0))
		) {
			oppStats = STREN[i] + DEXTE[i] + INTEL[i];
			if(oppStats > maxStats) {
				maxStats = oppStats;
				bestOpp = i;
			}
		}
	}
	if(bestOpp >= 0) {
		oppX = GetX(bestOpp);
		oppY = GetY(bestOpp);
		delta_x = abs(oppX - x);
		delta_y = abs(oppY - y);
		weapObj = __PickWeapon(objNum, bestOpp);
		weapTyp = GetType(weapObj);
		if(weapTyp == 0x89 || weapTyp == 0x8a || weapTyp == 0xf0 || weapTyp == 0xf1) {
			if(weapTyp == 0x89 || weapTyp == 0x8a)
				bestOpp = C_1810_4561(objNum, 0);
			else
				bestOpp = C_1810_4561(objNum, 1);
			oppX = GetX(bestOpp);
			oppY = GetY(bestOpp);
		}
		if(weapTyp == 0x15 && (GetType(bestOpp) == 0x143 || GetType(bestOpp) == 0x148))
			SetQual(weapObj, 1);
		else if(weapTyp == 0x15)
			SetQual(weapObj, 0);
		range = __GetWeaponRange(weapObj);
		if((dist = COMBAT_AttackRange(oppX, oppY, objNum)) <= range) {
			SelectionObj = bestOpp;
			if(dist <= 1 || COMBAT_Missile(objNum, &oppX, &oppY, range, 0)) {
				__TryHit(objNum, oppX, oppY, weapObj);
				SubMov(objNum, 10);
			} else {
				if(OSI_rand(0, 1)) {
					oppX = x - (GetY(SelectionObj) - y);
					oppY = y + (GetX(SelectionObj) - x);
				} else {
					oppX = x + (GetY(SelectionObj) - y);
					oppY = y - (GetX(SelectionObj) - x);
				}
				mustMove = 1;
			}
		} else {
			mustMove = 1;
		}
		if(mustMove) {
			savedMovePts = MovePts[objNum];
			if(!TryMoveTo(objNum, oppX, oppY)) {
				MovePts[objNum] = savedMovePts;
				COMBAT_AI_Assault(objNum);
			}
		}
	} else {
		COMBAT_AI_Assault(objNum);
	}
	SubMov(objNum, 5);
}

/*AI_RETREAT: flee from the closest opponent (when within 8 tiles); good
  ones go back to the party. A retreating NPC not under the player's
  control regains 1 HP 1 time in 4 and goes back to its combat mode when
  no longer critical*/
/*C_1810_50CA*/COMBAT_AI_Retreat(int16 objNum) {
	int16 oppObjNum;
	int16 delta_x, delta_y, savedMovePts;

	if(COMBAT_TileDistance(objNum, MapX, MapY) > 7) {
		MovePts[objNum] = 0;
		return;
	}
	oppObjNum = COMBAT_pickOpponent(objNum);
	if(oppObjNum < 0) {
		WanderRandomly(objNum);
		return;
	}
	delta_x = GetX(oppObjNum) - GetX(objNum);
	delta_y = GetY(oppObjNum) - GetY(objNum);
	if(CLOSE_ENOUGH_S(8, delta_x, delta_y, 0, 0)) {
		savedMovePts = MovePts[objNum];
		if(!TryMoveTo(objNum, GetX(objNum) - delta_x, GetY(objNum) - delta_y))
			MovePts[objNum] = savedMovePts;
	} else if(GetAlignment(objNum) == 0x40) {
		TryMoveTo(objNum, MapX, MapY);
	} else {
		WanderRandomly(objNum);
	}
	if(!IsPlrControl(objNum) && OSI_rand(0, 3) == 0) {
		if(MaxHP(objNum) > HitPoints[objNum])
			HitPoints[objNum] ++;
		if(HitPoints[objNum] * 4 / MaxHP(objNum) > 0)
			NPCMode[objNum] = NPCComMode[objNum];
	}
}

/*AI_LIKE: approach the party now and then when close, else wander*/
/*C_1810_523E*/COMBAT_AI_Like(int16 objNum) {
	int16 i;
	int16 x, y, nearParty;

	nearParty = 0;
	x = GetX(objNum);
	y = GetY(objNum);
	for(i = 0; i < PartySize; i ++) {
		if(CLOSE_ENOUGH_S(3, GetX(Party[i]), GetY(Party[i]), x, y)) {
			nearParty = 1;
			break;
		}
	}
	if(nearParty && OSI_rand(0, 1) == 0) {
		TryMoveTo(objNum, MapX, MapY);

		return;
	}
	if(OSI_rand(0, 1)) {
		if(OSI_rand(0, 1))
			x += OSI_rand(0, 1)?10:-10;
		else
			y += OSI_rand(0, 1)?10:-10;
		TryMoveTo(objNum, x, y);
		return;
	}
	SubMov(objNum, 5);
}

/*AI_UNFRIENDLY: wander; when close to the party, 1 in 8 to turn into
  AI_ASSAULT*/
/*C_1810_5370*/COMBAT_AI_Unfriendly(int16 objNum) {
	int16 i;
	int16 x, y, nearParty;

	nearParty = 0;
	x = GetX(objNum);
	y = GetY(objNum);
	for(i = 0; i < PartySize; i ++) {
		if(CLOSE_ENOUGH_S(3, GetX(Party[i]), GetY(Party[i]), x, y)) {
			nearParty = 1;
			break;
		}
	}
	if(nearParty && OSI_rand(0, 7) == 0) {
		NPCMode[objNum] = 8;
		COMBAT_AI_Assault(objNum);
		return;
	}
	if(OSI_rand(0, 1)) {
		if(OSI_rand(0, 1))
			x += OSI_rand(0, 1)?10:-10;
		else
			y += OSI_rand(0, 1)?10:-10;
		TryMoveTo(objNum, x, y);
		return;
	}
	SubMov(objNum, 5);
}

/*AI_IMMOBILE: never moves; tries 16 random tiles within 4 (but the
  target is picked once, before the loop, at a position never set) or
  throws an explosive at the best place*/
/*C_1810_54A9*/COMBAT_AI_Immobile(int16 objNum) {
	int16 i;
	int16 objAln, opp_x, opp_y;
	int16 weapObj;

	SelectionObj = COMBAT_GetTargetAt(objNum, opp_x, opp_y);
	weapObj = __PickWeapon(objNum, SelectionObj);
	objAln = GetAlignment(objNum);
	if(objAln != 0) {
		if(
			GetType(weapObj) != 0x89 && GetType(weapObj) != 0x8a &&
			GetType(weapObj) != 0xf0 && GetType(weapObj) != 0xf1
		) {
			for(i = 0; i < 16; i ++) {
				opp_x = GetX(objNum) + OSI_rand(0, 8) - 4;
				opp_y = GetY(objNum) + OSI_rand(0, 8) - 4;
				if(
					SelectionObj >= 0 && IsNPC(SelectionObj) && COMBAT_canSee(objNum, SelectionObj) &&
					GetAlignment(SelectionObj) != objAln && GetAlignment(SelectionObj) != 0
				) {
					__TryHit(objNum, opp_x, opp_y, weapObj);
					SubMov(objNum, 10);
					return;
				}
			}
		} else {
			if(GetType(weapObj) == 0x89 || GetType(weapObj) == 0x8a)
				SelectionObj = C_1810_4561(objNum, 0);
			else
				SelectionObj = C_1810_4561(objNum, 1);
			opp_x = GetX(SelectionObj);
			opp_y = GetY(SelectionObj);
			__TryHit(objNum, opp_x, opp_y, weapObj);
			SubMov(objNum, 10);
			return;
		}
	}
	SubMov(objNum, 5);
}

/*segment frames: VineFrames[prev_dir >> 1][dir >> 1] is the frame of the
  segment left behind (corner/straight)*/
/*10DB*/unsigned char VineFrames[][4] = {
	{3,5,3,6},
	{7,2,6,2},
	{3,4,3,7},
	{4,2,5,2}
};

/*the head of a segmented creature (objNum, its tail in its quality) moves
  one tile in direction dir (0,2,4,6), leaving a segment (0x124, quality
  = objNum) behind; no going backward nor more than 5 tiles (4 north-south)
  away from the tail. returns 1 on success*/
/*C_1810_564E*/static __TryGrowVine(int16 objNum, int16 dir) {
	int16 moved, x, prev_dir, vineObj;
	int16 y;

	prev_dir = 0;
	dir &= 6;
	x = GetX(objNum);
	y = GetY(objNum);
	prev_dir = GetDirection(objNum);
	if((dir ^ prev_dir) == 4)
		return 0;
	if(
		(x - GetX(GetQual(objNum)) > 5 && dir == 2) ||
		(x - GetX(GetQual(objNum)) < -5 && dir == 6) ||
		(y - GetY(GetQual(objNum)) > 4 && dir == 4) ||
		(y - GetY(GetQual(objNum)) < -4 && dir == 0)
	) return 0;
	moved = TryStraightMove(objNum, dir, 1);
	if(moved == 0)
		return 0;
	vineObj = AddMapObj(0x124, 0x20, objNum << 8, x, y, MapZ);
	if(vineObj >= 0) {
		SetFrame(vineObj, VineFrames[prev_dir >> 1][dir >> 1]);
		SetDirection(objNum, dir);
		return 1;
	}
	MoveObj(objNum, x, y, MapZ);
	SetDirection(objNum, prev_dir);

	return 0;
}

/*AI_TANGLE (segmented creature; objNum's quality is its tail, whose frame
  alternates): hit an adjacent opponent, or grow toward one within 4
  tiles, or grow at random (1 in 4)*/
/*C_1810_57F9*/COMBAT_AI_Tangle(int16 objNum) {

	int16 dir;
	int16 opp_x, opp_y, tangle_x, tangle_y, tail;

	tail = GetQual(objNum);
	SetFrame(tail, !GetFrame(tail));
	SelectionObj = COMBAT_pickOpponent(tail);
	if(SelectionObj >= 0) {
		opp_x = GetX(SelectionObj);
		opp_y = GetY(SelectionObj);
		tangle_x = GetX(objNum);
		tangle_y = GetY(objNum);
		if(CLOSE_ENOUGH_S(2, opp_x, opp_y, tangle_x, tangle_y)) {
			__TryHit(objNum, opp_x, opp_y, objNum);
			SubMov(objNum, 10);
			return;
		}
		if(CLOSE_ENOUGH_S(5, opp_x, opp_y, tangle_x, tangle_y)) {
			opp_x -= tangle_x;
			opp_y -= tangle_y;
			if(abs(opp_x) > abs(opp_y)) {
				dir = (opp_x > 0)?2:6;
				if(!__TryGrowVine(objNum, dir)) {
					dir = (opp_y > 0)?4:0;
					__TryGrowVine(objNum, dir);
					return;
				}
			} else {
				dir = (opp_y > 0)?4:0;
				if(!__TryGrowVine(objNum, dir)) {
					dir = (opp_x > 0)?2:6;
					__TryGrowVine(objNum, dir);
					return;
				}
			}
		}
	}
	if(OSI_rand(0, 3) == 0) {
		dir = OSI_rand(0, 3) << 1;
		if(dir == GetDirection(objNum))
			dir ^= 4;
		__TryGrowVine(objNum, dir);
	} else {
		MovePts[objNum] = 0;
	}
}

/*enter combat mode when a party member is attacked (as COMBAT_begin,
  with another music)*/
/*C_1810_59C8*/COMBAT_beginAttacked() {
	int16 partyIdx, mouseSave;

	InCombat = 1;
	for(partyIdx = 0; partyIdx < PartySize; partyIdx ++) {
		if(!IsCharmed(Party[partyIdx]))
			NPCMode[Party[partyIdx]] = NPCComMode[Party[partyIdx]];
		Leader[Party[partyIdx]] = Party[partyIdx];
	}
	if(SoloMember < 0) {
		for(
			partyIdx = 0;
			(IsIncapacitated(Party[partyIdx]) || IsDraggedUnder(Party[partyIdx])) && partyIdx < PartySize;
			partyIdx ++
		);
	} else {
		partyIdx = SoloMember;
	}
	SavedNPCMode = NPCMode[Party[partyIdx]];
	NPCMode[Party[partyIdx]] = 2;
	mouseSave = CON_mouseOffAt(64, 0, 95, 15);
	GR_PUT_TILE_SCREEN(0x177, 64, 0);
	GR_PUT_TILE_SCREEN(0x178, 80, 0);
	CON_mouseOn(mouseSave);
	C_2701_02D6();
}
