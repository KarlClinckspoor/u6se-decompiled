/* flags: -d -mm -p -c -Z -O -y */
/*
 * The Savage Empire (Worlds of Ultima) - GAME.EXE
 * segment 1DBD: the object commands (Ultima VI's objcmd.c).
 *
 * The cut scenes of the story (pictures of MIDGAMEn.LZC with the changes
 * they make to the world, the last one running SAVAGE.EXE for the
 * endgame), the object type lists and tests used by the commands, and the
 * commands of the main loop that act on objects: "look" (and searching),
 * "get", "drop", "move" (pushing on the map, giving to party members,
 * moving into/out of containers) and "use", which dispatches on the
 * object type to one handler per usable object.
 */
#include "se.h"

#undef abs

/*text window (Ultima VI's)*/
struct tClipInfo/*size:9*/ {
	unsigned char originX, originY, cols, rows;
	unsigned char curX, curY;
	unsigned char fgColor, bgColor;
	unsigned char attrFlags;
};

/*OSI*/
extern int16 C_2B57_001F(int16, int16);/*OSI_rand(lo, hi)*/
#define OSI_rand C_2B57_001F
/*main*/
extern BuildConfigString(char *);
/*gameloop*/
extern DrawMapWindow(int16);
extern PassTime(int16);
extern TerrainDamage(int16, int16);
extern RefreshStatus(void);
/*console*/
extern int16 MkDirection(int16, int16);
extern CON_UpdateMousePointer(void);
extern int16 mkMouseDir(void);
extern CON_ReadCommand(void);/*reads a command (U6: CON_ReadCommand)*/
extern DrawSlotCursor(int16);
extern int16 CON_getch(void);
extern CON_gets(char *, int16);
extern CON_putch(int16);
extern CON_setClip(struct tClipInfo *);
extern CON_gotoxy(int16, int16);
extern CON_itoa(uint16, char far *, int16, int16);
extern cdecl CON_printf(char far *, ...);
extern int16 C_2ABF_013D(void);/*MOUSE_SHOW*/
extern int16 C_2ABF_014E(void);/*MOUSE_HIDE*/
/*map*/
extern PartyTeleport(int16, int16, int16, int16);
extern MAP_LoadAround(void);
extern C_08CB_0855(int16);/*goes through the entrance objNum*/
extern unsigned char GetTileAtXYZ(int16, int16, int16);
/*visible*/
extern VIS_UpdateView(void);
extern DisplayTick(void);
/*object*/
extern int16 NextLoc(void);
extern int16 NextInv(void);
extern int16 SearchArea(int16, int16, int16, int16);
extern int16 NextArea(void);
extern int16 FindLoc(int16, int16, int16);
extern int16 OBJ_MakeDirFrame(int16, int16);
extern int16 FindInv(int16);
extern int16 C_0A5D_070D(int16);
extern int16 C_0A5D_0613(int16);/*is the object in the 11x9 tiles around the party?*/
extern ShowObject(int16, int16, int16, int16);/*the NPC holding an object, -1 if none*/
extern int16 OBJ_FindTypeAt(int16, int16, int16);
extern int16 FindInvType(int16, int16, int16);
extern int16 AddMapObj(int16, int16, int16, int16, int16, int16);
extern int16 AddObj(int16, struct coord, int16, int16, int16);
extern int16 AddInvObj(int16, int16, int16, int16);
extern int16 InsertObj(int16, int16, int16);
extern int16 C_0A5D_0FD6(int16);
extern int16 C_0A5D_080A(int16, int16);/*an object of type objTyp carried by objNum, -1 if none*//*number of objects directly inside an object*/
extern DeleteObj(int16);
extern int16 OBJ_IsInside(int16, int16);
extern MoveObj(int16, int16, int16, int16);
extern OBJ_DeleteTmpBlocks(void);
extern int16 QuanType(int16);
extern char *GetObjectString(int16);
extern char *GetTileString(int16);
extern int16 TestObj(int16, int16);
extern TakeObj(int16, int16, int16);
extern ShowObjects(void);
/*status*/
extern STAT_DrawInventory(void);
extern STAT_ShowParty(int16);
extern int16 IsWeightPer10Type(int16);
extern int16 GetWeight(int16);
extern int16 Encumbrance(int16);
extern int16 STAT_GetEquipSlot(int16);
extern STAT_ComputeWeights(int16);
extern STAT_RefreshInventory(int16);
extern int16 STAT_FindEquipSlot(int16);
extern int16 STAT_CheckUnready(int16, int16);
extern STAT_StopEquipEffect(int16, int16);
/*talkdr*/
extern char *TALK_GetCharName(int16);
extern int16 IsNpcOnScreen(int16);
/*talk*/
extern TALK_talkTo(int16, int16, int16);
extern JoinParty(int16);
/*npctrack*/
extern MoveFollowers(int16, int16);/*MoveFollowers*/
extern UpdateMoveFrame(int16, int16);
extern SubMov(int16, int16);
extern int16 C_130F_014E(int16);/*is there land next to the vehicle?*/
extern Board(int16);
extern int16 Unboard(int16);
extern int16 CanMoveTo(int16, int16, int16);
/*spell*/
extern SPELL_Invoke(int16);
/*combat*/
extern COMBAT_begin(void);/*COMBAT_begin*/
extern int16 GetStr(int16);
extern LoseHP(int16, int16);
extern COMBAT_displaySeverity(int16);
extern int16 C_1810_0F71(int16);/*is a tile ...?*/
extern int16 COMBAT_getHead(int16);
extern int16 COMBAT_Missile(int16, int16 *, int16 *, int16, int16);
extern C_1810_14EA(int16, int16, int16);/*pushes the boulder obj away from (x, y)*/
extern C_1810_1941(int16, int16);/*explosion of 0x8a at (x, y)*/
extern C_1810_19C1(int16, int16);/*explosion of 0xf1 at (x, y)*/
extern int16 MaxHP(int16);/*MaxHP*/
#define MaxHP MaxHP
extern COMBAT_Kill(int16);
/*egg*/
extern EGG_hatchArea(void);
extern int16 EGG_generate(int16, int16, int16, int16);
/*music*/
extern MUS_PlaySong(int16);
extern MUS_PlayAreaSong(void);
extern MUS_PlaySfx(int16);
/*gfx*/
extern ClearMapWindow(int16);
extern ShowMidgame(int16, int16, unsigned char *);
extern int16 far *Explosion(int16, int16, int16);
extern C_276B_1BAF(void);/*clears the status panel for a portrait?*/
extern DrawPortrait(int16, int16, int16);
/*far messages*/
extern char far NothingMsg[];/*"nothing!\n"*/
extern char far OutOfRangeMsg[];/*"Out of range!\n"*/
extern char far NotPossibleMsg[];/*"Not possible.\n"*/
extern char far TooMuchMsg[];/*"You are carrying too much already.\n"*/
extern char far FailedMsg[];/*"Failed.\n"*/
extern char far NotInSoloMsg[];/*"Not in solo mode!\n"*/
extern char far NoEffectMsg[];/*"No effect.\n"*/
extern char far BlockedMsg[];/*"Blocked!\n"*/
extern char far NotOnShipMsg[];/*"Not while aboard ship!\n"*/
extern char far CantUseMsg[];/*"`%s can't use that!\n"*/
/*disk*/
extern DISK_confirm(int16);
/*library*/
extern int cdecl abs(int);
extern cdecl ShutdownSound(void);
extern cdecl MEM_shutdown(int16);

extern char DirIncrX[];/*DirIncrX*/
extern char DirIncrY[];/*DirIncrY*/
extern char *DirNames[];/*DirNames*/
extern char MouseOn;/*MouseOn*/
extern int16 EndPagingAtPrompt;/*EndPagingAtPrompt*/
extern int16 StatusDisplay;/*StatusDisplay*/
extern int16 GetsAborted;/*GetsAborted*/
extern struct tClipInfo *CurClip;/*CurClip*/
extern char AnimateAtPrompt;/*AnimateAtPrompt*/
extern char SelectMode;/*SelectMode*/
extern char SelectRange;/*SelectRange*/
extern int16 LastNumberPlural;/*LastNumberPlural*/
extern char DirOriginX, DirOriginY;/*DirOriginX, DirOriginY*/
extern int16 AdvanceBlocked;/*AdvanceBlocked*/
extern int16 VisibilityDirty;/*VisibilityDirty*/
extern int16 D_B5E6;/*AdvanceDir*/
extern unsigned char far *D_763C;/*TypeWeight*/
extern unsigned char D_E648;/*InvenSize*/
extern int16 LocPartOffset;/*LocPartOffset*/
extern unsigned char StatusDirty;/*StatusDirty*/
extern int16 StatusMember;/*StatusMember*/
extern int16 MouseMode;/*MouseMode*/
extern unsigned char DrunkCounter;/*DrunkCounter*/
extern char SoloMember;/*SoloMember: -1 in party mode*/
extern int16 KegFuse;/*KegFuse*/
extern int16 KegObject;/*KegObject*/
extern int16 D_E64A;/*InvenOwner*/
extern unsigned char D_E64C[8];/*Party_2Handed*/
extern int16 D_E658[8];/*Equipment*/
extern int16 D_E668;/*WeightEquip*/
extern int16 D_E66A;/*WeightInven*/
extern unsigned char IsInConversation;/*IsInConversation*/
extern int16 ScreenFade;/*ScreenFade*/
extern int16 ColorPaper;/*ColorPaper*/
extern unsigned char Time_M, Time_H, Date_D, Date_M;/*Time_M, Time_H, Date_D, Date_M*/
extern int16 Date_Y;/*Date_Y*/
extern unsigned char Active;/*Active*/
extern char AmbientLight;/*AmbientLight*/
extern int16 MapX, MapY, MapZ;/*MapX, MapY, MapZ*/
extern unsigned char far *D_3298;/*per-NPC flags (0x80: ?)*/
extern unsigned char D_329C[];/*ObjStatus*/
extern unsigned char far *D_4602;/*NPCFlag*/
extern unsigned char far *D_4706;/*NPCMode*/
extern unsigned char D_4EDA[];/*Party*/
extern char D_5104[][14];/*Names*/
extern int16 D_54F0[];/*Amount*/
extern unsigned char D_53E8[];/*STREN*/
extern unsigned char D_6CF0[];/*NPCStatus*/
extern unsigned char D_6DF4[];/*HitPoints*/
extern int16 far *D_6EF4;/*BaseTile*/
extern unsigned char D_6EFC[];/*TalkFlags: per-NPC flags of the conversations ([1]: the story's)*/
extern unsigned char far *D_7640;/*TileFlag2*/
extern struct coord D_78E4[];/*ObjPos*/
extern unsigned char far *D_B4FA;/*TerrainType*/
extern unsigned char D_9CF6;/*PartySize*/
extern uint16 D_9CFA[];/*ObjShapeType*/
extern int16 far *D_B53E;/*OrigShapeType*/
extern struct tClipInfo D_B644[4];/*TextWindows*/
extern int16 D_B6B6;/*lines left before the console pages*/
extern int16 D_B6B8;/*SelectionIsDark*/
extern int16 D_B5E8, D_B5EA;/*PointerX, PointerY*/
extern int16 D_B642;/*Selection.obj*/
extern int16 D_B66A, D_B66C;/*Selection.x, Selection.y*/
extern int16 D_C17C[];/*Link*/
extern union tHandler D_ED10;/*GrDriver*/

#define DirIncrX DirIncrX
#define DirIncrY DirIncrY
#define MouseOn MouseOn
#define StatusDisplay StatusDisplay
#define Amount D_54F0
#define TileFlag2 D_7640
#define AnimateAtPrompt AnimateAtPrompt
#define EndPagingAtPrompt EndPagingAtPrompt
#define GetsAborted GetsAborted
#define StatusMember StatusMember
#define CurClip CurClip
#define StatusDirty StatusDirty
#define IsInConversation IsInConversation
#define ScreenFade ScreenFade
#define ColorPaper ColorPaper
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
#define ObjStatus D_329C
#define NPCFlag D_4602
#define NPCMode D_4706
#define Party D_4EDA
#define Names D_5104
#define NPCStatus D_6CF0
#define HitPoints D_6DF4
#define BaseTile D_6EF4
#define TalkFlags D_6EFC
#define ObjPos D_78E4
#define PartySize D_9CF6
#define ObjShapeType D_9CFA
#define OrigShapeType D_B53E
#define TextWindows D_B644
#define Link D_C17C
#define DirNames DirNames
#define SelectRange SelectRange
#define LastNumberPlural LastNumberPlural
#define DirOriginX DirOriginX
#define DirOriginY DirOriginY
#define AdvanceBlocked AdvanceBlocked
#define VisibilityDirty VisibilityDirty
#define AdvanceDir D_B5E6
#define TypeWeight D_763C
#define InvenSize D_E648
#define MouseMode MouseMode
#define DrunkCounter DrunkCounter
#define SoloMember SoloMember
#define KegFuse KegFuse
#define KegObject KegObject
#define InvenOwner D_E64A
#define Party_2Handed D_E64C
#define Equipment D_E658
#define WeightEquip D_E668
#define WeightInven D_E66A
#define STREN D_53E8
#define SelectMode SelectMode
#define LocPartOffset LocPartOffset
#define TerrainType D_B4FA
#define SelectionIsDark D_B6B8
#define PointerX D_B5E8
#define PointerY D_B5EA
#define SelectionObj D_B642
#define SelectionX D_B66A
#define SelectionY D_B66C
#define GrDriver D_ED10

#define GR_SHUTDOWN()                      (GrDriver.iii.ofs = 0x03, (*GrDriver.f)())
#define GR_SET_COLOR(c)                    (GrDriver.iii.ofs = 0x06, (*GrDriver.f)(c))
#define GR_FILL_RECT(x0,y0,x1,y1)          (GrDriver.iii.ofs = 0x12, (*GrDriver.f)(x0,y0,x1,y1))
#define GR_PUT_TILE_SCREEN(tile,x,y)       (GrDriver.iii.ofs = 0x2d, (*GrDriver.f)(tile,x,y))
#define GR_SET_ANIM_FRAME(a,b)             (GrDriver.iii.ofs = 0x4b, (*GrDriver.f)(a,b))

#define LOCXYZ 0
#define CONTAINED 8
#define INVEN 0x10
#define EQUIP 0x18

#define GetQuan(o) (((unsigned char *)(Amount + (o)))[0])
#define GetQual(o) (((unsigned char *)(Amount + (o)))[1])
#define GetCoordUse(i) (ObjStatus[i] & 0x18)
#define TypeFrame(t, s) ((t) + ((s) << 10))
#define GetType(i) (ObjShapeType[i] & 0x3ff)
#define GetFrame(i) (ObjShapeType[i] >> 10)
#define SetType(i, v) (ObjShapeType[i] = (ObjShapeType[i] & ~0x3ff) + (v))
#define SetFrame(i, v) (ObjShapeType[i] = TypeFrame(GetType(i),(v)))
#define TILE_FRAME(i) (BaseTile[GetType(i)] + GetFrame(i))
#define GetCoordX(o) ((*(int16 *)&((o).raw[0]))&0x3ff)
#define GetCoordY(o) (((*(int16 *)&((o).raw[1]))>>2)&0x3ff)
#define GetCoordZ(o) ((o).raw[2] >> 4)
#define GetCoordAssoc(i) *(uint16 *)(&(i))
#define GetX(i) GetCoordX(ObjPos[i])
#define GetY(i) GetCoordY(ObjPos[i])
#define GetZ(i) GetCoordZ(ObjPos[i])
#define GetAssoc(i) GetCoordAssoc(ObjPos[i])
#define IN_VEHICLE (Active == PartySize)
#define SetQuan(o, v) (GetQuan(o) = (v))
#define SetQual(o, v) (GetQual(o) = (v))
#define SubQuan(o, v) (GetQuan(o) -= (v))
#define QualQuan(quality, quantity) ((quality) << 8 | (quantity))
#define SetLocal(i) ObjStatus[i] |= 0x20
#define ClrLocal(i) ObjStatus[i] &= ~0x20
#define SetCoordXYZ(o, x, y, z) {      \
	(o).raw[2] = ((z) << 4);           \
	*(int16 *)&((o).raw[0]) = (x);       \
	*(int16 *)&((o).raw[1]) |= (y) << 2; \
}
#define IsTileSu(tile) (TileFlag2[tile] & 0x02)
#define IsTerrainImpass(tile) (TerrainType[tile] & 0x02)
#define IsInvisible(i) (ObjStatus[i] & 0x02)
#define IsNPC(i) ((i) < 0x100 && (i) >= 0)
#define IsPlrControl(i) (NPCStatus[i] & 0x80)
#define SCR2TIL(i) ((i) >> 4)
#define TIL2SCR(i) ((i) << 4)
#define IsTileIg(tile) (TileFlag2[tile] & 0x10)
#define HideMouse if(MouseOn) C_2ABF_014E();
#define ShowMouse if(MouseOn) C_2ABF_013D();

/*this module*/
int16 FindTypeAt(int16, int16, int16, int16);
void C_1DBD_5069(int16, int16, int16);

/*the text window under the map, cleared, for a cut scene*/
#define CUTSCENE_START(song) \
	MUS_PlaySong(song); \
	ClearMapWindow(1); \
	CON_setClip(&TextWindows[3]); \
	CurClip->attrFlags |= 1; \
	CurClip->attrFlags |= 0x20; \
	CurClip->attrFlags |= 4; \
	EndPagingAtPrompt = 1; \
	GR_SET_COLOR(ColorPaper); \
	GR_FILL_RECT(0xb8, 0x80, 0x137, 0xbf); \
	CON_gotoxy(0, 0); \
	D_B6B6 = CurClip->rows; \
	GR_PUT_TILE_SCREEN(0x175, 0x40, 0); \
	GR_PUT_TILE_SCREEN(0x176, 0x50, 0);

/*back to the game after a cut scene*/
#define CUTSCENE_END \
	AnimateAtPrompt = 1; \
	VIS_UpdateView(); \
	ScreenFade = 1; \
	DrawMapWindow(0); \
	IsInConversation = 0; \
	StatusDirty ++; \
	RefreshStatus();

/*the NPC holding objNum, or the active party member when no NPC holds it*/
/*C_1DBD_0003*/unsigned char C_1DBD_0003(int16 objNum) {
	int16 holder;

	if((holder = C_0A5D_070D(objNum)) == -1)
		return Party[Active];
	return holder;
}

/*cut scene 1 (MIDGAME1)*/
/*C_1DBD_002E*/void C_1DBD_002E() {
	unsigned char text[4];

	AnimateAtPrompt = 0;
	IsInConversation = 1;
	CUTSCENE_START(0xe)
	text[0] = 2;
	text[1] = text[2] = 0;
	ShowMidgame(1, 3, text);
	D_3298[2] &= ~0x80;
	JoinParty(2);
	MoveObj(0x17, 0x73, 0xa8, MapZ);
	D_3298[0x17] &= ~0x80;
	TalkFlags[0x17] |= 1;
	NPCMode[0x17] = 8;
	TalkFlags[0x10] |= 0x80;
	TalkFlags[1] |= 0x80;
	EGG_hatchArea();
	TalkFlags[1] &= ~0x80;
	TalkFlags[1] |= 1;
	AnimateAtPrompt = 1;
	COMBAT_begin();
	VIS_UpdateView();
	ScreenFade = 1;
	DrawMapWindow(0);
	IsInConversation = 0;
	StatusDirty ++;
	RefreshStatus();
	MUS_PlayAreaSong();
}

/*cut scene 2 (MIDGAME2)*/
/*C_1DBD_01A0*/void C_1DBD_01A0() {
	unsigned char text[4];

	IsInConversation = 1;
	AnimateAtPrompt = 0;
	CUTSCENE_START(0xf)
	if(Time_M) {
		PassTime(60 - Time_M);
		while(Time_H != 12)
			PassTime(60);
	}
	text[0] = text[1] = text[2] = 0;
	ShowMidgame(2, 3, text);
	ObjStatus[AddMapObj(0x4a0, 0, 0, 0xc0, 0x1cc, 0)] &= ~0x20;
	TalkFlags[1] |= 2;
	GR_SET_ANIM_FRAME(0x20a, 0x11c);
	CUTSCENE_END
	MUS_PlayAreaSong();
}

/*cut scene 3 (MIDGAME3)*/
/*C_1DBD_0304*/void C_1DBD_0304() {
	unsigned char text[4];
	int16 x, y, z;
	int16 obj2, obj;
	int16 i, type;

	IsInConversation = 1;
	AnimateAtPrompt = 0;
	CUTSCENE_START(0x10)
	text[0] = 5;
	text[1] = text[2] = text[3] = 0;
	x = GetX(11);
	y = GetY(11);
	z = GetZ(11);
	SetFrame(11, 11);
	for(obj = FindInv(11); obj >= 0; obj = FindInv(11))
		MoveObj(obj, x, y, z);
	AddMapObj(ObjShapeType[11], 0, 0, x, y, z);
	COMBAT_Kill(0x2f);
	COMBAT_Kill(0x1f);
	COMBAT_Kill(0x37);
	COMBAT_Kill(0x23);
	COMBAT_Kill(0xb);
	MoveObj(5, x, y, z);
	NPCStatus[5] &= ~0x10;
	HitPoints[5] = MaxHP(5);
	JoinParty(5);
	MoveFollowers(Party[Active], 1);
	D_3298[0x28] &= ~0x80;
	D_3298[0x2c] &= ~0x80;
	ObjShapeType[0x28] = OrigShapeType[0x28] = 0x142;
	ObjShapeType[0x2c] = OrigShapeType[0x2c] = 0x142;
	TalkFlags[0x28] |= 0x80;
	TalkFlags[1] |= 4;
	TalkFlags[0x31] |= 0x20;
	for(i = 0; i < 24; i ++)
		PassTime(60);
	for(i = 0; i < 0x100; i ++) {
		type = GetType(i);
		if(type == 0x12d || type == 0x129)
			COMBAT_Kill(i);
	}
	for(; i < 0xc00; i ++) {
		type = GetType(i);
		if(type == 0xe6 || type == 0x44) {
			if(type == 0x44) {
				ObjShapeType[i] = 0x26;
			} else {
				obj2 = FindInv(i);
				type = GetType(obj2);
				if(type == 0x129 || type == 0x12d) {
					DeleteObj(obj2);
					DeleteObj(i);
				}
			}
		}
	}
	ShowMidgame(3, 4, text);
	CUTSCENE_END
	GR_SET_ANIM_FRAME(0x20a, 0x11d);
	MUS_PlayAreaSong();
}

/*cut scene 4 (MIDGAME4)*/
/*C_1DBD_05DC*/void C_1DBD_05DC() {
	unsigned char text[4];
	int16 item, obj;

	IsInConversation = 1;
	AnimateAtPrompt = 0;
	CUTSCENE_START(0x11)
	text[0] = text[1] = text[2] = 0;
	ShowMidgame(4, 3, text);
	TalkFlags[1] |= 8;
	PartyTeleport(0xf3, 0xcc, 1, 0);
	if((obj = OBJ_FindTypeAt(0x16, MapX, MapY)) >= 0)
		SetFrame(obj, 1);
	if((item = FindInvType(Party[StatusMember], 0x15, -1)) >= 0) {
		GR_SET_ANIM_FRAME(TILE_FRAME(item), 0x10c);
		STAT_DrawInventory();
	}
	CUTSCENE_END
	MUS_PlayAreaSong();
}

/*the last cut scene (MIDGAME5): saves the game and runs SAVAGE.EXE for
  the endgame, with the configuration and the date as arguments*/
/*C_1DBD_077E*/C_1DBD_077E() {
	unsigned char text[4];
	char config[4];
	char day[4], month[4], year[4];
	char flag[4];

	IsInConversation = 1;
	AnimateAtPrompt = 0;
	CUTSCENE_START(0xc)
	text[0] = text[1] = text[2] = 0;
	ShowMidgame(5, 3, text);
	CON_itoa(Date_D + 99, day, 3, 10);
	CON_itoa(Date_M + 99, month, 3, 10);
	CON_itoa(Date_Y + 100, year, 3, 10);
	day[0] = 'D';
	month[0] = 'B';
	year[0] = 'Y';
	flag[0] = 'L';
	flag[2] = 0;
	if(TalkFlags[2] & 0x20)
		flag[1] = 'A';
	else if(TalkFlags[0x33] & 0x20)
		flag[1] = 'T';
	else
		flag[0] = 0;
	BuildConfigString(config);
	OBJ_DeleteTmpBlocks();
	DISK_confirm(1);
	GR_SHUTDOWN();
	ShutdownSound();
	MEM_shutdown(8);
	for(;;)
		execl("savage.exe", "savage.exe", config, day, month, year, flag, 0L);
}

/*party member object for party slot partyIdx; when the party is in a
 vehicle the whole party is represented by Party[0] (the vehicle)*/
/*C_1DBD_092C*/static GetPartyObjOrVehicle(int16 partyIdx) {
	if(IN_VEHICLE)
		return Party[0];
	else
		return Party[partyIdx];
}

/*X of objNum, or X of object 0 (the party's vehicle) when in a vehicle*/
/*C_1DBD_094A*/static GetObjXOrVehicle(int16 objNum) {
	if(IN_VEHICLE)
		return GetX(0);
	else
		return GetX(objNum);
}

/*Y of objNum, or Y of object 0 (the party's vehicle) when in a vehicle*/
/*C_1DBD_0970*/static GetObjYOrVehicle(int16 objNum) {
	if(IN_VEHICLE)
		return GetY(0);
	else
		return GetY(objNum);
}

/*the object on the map that (directly or not) holds objNum*/
/*C_1DBD_099A*/GetOuterHolder(register int16 objNum) {
	do
		objNum = GetAssoc(objNum);
	while(GetCoordUse(objNum) != LOCXYZ);
	return objNum;
}

/*resistance roll, target strength vs. caster strength:
  returns non-zero if the target resists*/
/*C_1DBD_09BC*/ResistStrVsStr(int16 caster, int16 target) {
	return ((GetStr(target) / 2) + 30 - GetStr(caster)) / 2 > OSI_rand(1, 30);
}

/*container types*/
/*1122*/int16 ContainerTypes[] = {0x3b, 0x3c, 0x102, 0xb5, 0xbc, 0xb7, 0xb6, 0xb8, 0x61};

/*is typeObject a container (one of ContainerTypes)? returns 1/0*/
/*C_1DBD_0A02*/IsContainerType(int16 typeObject) {
	int16 i;

	for(i = 0; i < 9; i ++)
		if(ContainerTypes[i] == typeObject)
			return 1;
	return 0;
}

/*is tileNum one of the tiles 0x20..0x2f, 0xbe (clay to dig; land next to
  the water)?*/
/*C_1DBD_0A29*/C_1DBD_0A29(register int16 tileNum) {
	return (tileNum >= 0x20 && tileNum <= 0x2f) || tileNum == 0xbe;
}

/*is tileNum lava (tiles 0x0f..0x1b, 0xaa, 0xab)?*/
/*C_1DBD_0A4C*/IsLavaTile(register int16 tileNum) {
	return (tileNum >= 0xf && tileNum <= 0x1b) || tileNum == 0xaa || tileNum == 0xab;
}

/*tree tiles*/
/*1134*/int16 TreeTiles[] = {
	0x32, 0x33, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
	0x48, 0xc0, 0xc1, 0xc2, 0xc3, 0xc6, 0xb3, 0xb4, 0x82
};

/*is tile tileNum a tree (one of TreeTiles)?*/
/*C_1DBD_0A75*/IsTreeTile(int16 tileNum) {
	int16 i;

	for(i = 0; i < 19; i ++)
		if(TreeTiles[i] == tileNum)
			return 1;
	return 0;
}

/*115A*/int16 D_115A[] = {0x36, 0x37, 0x38, 0x5d, 0x6c, 0x6d, 0x7b, 0x81, 0x82, 0xcc, 0xa};

/*is typeObject one of D_115A's types (what fits in a bag or a pouch)?*/
/*C_1DBD_0A9C*/C_1DBD_0A9C(int16 typeObject) {
	int16 i;

	for(i = 0; i < 11; i ++)
		if(D_115A[i] == typeObject)
			return 1;
	return 0;
}

/*can itemObj be put into containerObj? (at most 16 objects inside, items
  only; what fits depends on the container type) returns 1/0*/
/*C_1DBD_0AC3*/CanPutInContainer(int16 containerObj, int16 itemObj) {
	int16 contTyp;
	int16 itemTyp, inner;

	if(C_0A5D_0FD6(containerObj) < 0x10 && itemObj >= 0x100) {
		itemTyp = GetType(itemObj);
		contTyp = GetType(containerObj);
		if(IsContainerType(contTyp)) {
			switch(contTyp) {
				case 0xf9:
					itemTyp = itemTyp;/*(a dropped statement: keeps the return from merging)*/
					return 0;
				case 0xbc:
					if(GetFrame(containerObj))
						return 0;
					return 1;
				case 0x3b: case 0x3c:
					if(C_1DBD_0A9C(itemTyp))
						return 1;
					return 0;
				case 0x61:
					if(!C_1DBD_0A9C(itemTyp))
						return 0;
					inner = FindInv(containerObj);
					if(inner != -1 && GetType(inner) != itemTyp)
						return 0;
					return 1;
				default:
					if(IsContainerType(itemTyp))
						return 0;
					return 1;
			}
		}
	}
	return 0;
}

/*types that break when thrown*/
/*1170*/int16 BreakableTypes[] = {0x3, 0x6, 0x19, 0x3b, 0x3c, 0x5f, 0x61, 0x84, 0x85, 0xb5, 0xb8, 0x89, 0x3f};

/*does an object of type typeObject break when thrown (one of BreakableTypes)?*/
/*C_1DBD_0BAB*/IsBreakableType(int16 typeObject) {
	int16 i;

	for(i = 0; i < 13; i ++)
		if(BreakableTypes[i] == typeObject)
			return 1;
	return 0;
}

/*types that burn*/
/*118A*/int16 FlammableTypes[] = {0x4, 0x5, 0xd, 0x7, 0x9, 0x24, 0x27, 0x2a, 0x11, 0x2d, 0x86, 0xb7, 0xb4, 0xbf, 0xd2, 0xab};

/*does an object of type typeObject burn (one of FlammableTypes)?*/
/*C_1DBD_0BD2*/IsFlammableType(int16 typeObject) {
	int16 i;

	for(i = 0; i < 16; i ++)
		if(FlammableTypes[i] == typeObject)
			return 1;
	return 0;
}

/*is objNum (and everything it contains) flagged "ok to take" (ObjStatus
  bit 0)? taking an object without the flag is stealing*/
/*C_1DBD_0BF9*/static IsOkToGet(register int16 objNum) {
	register int16 child;

	if(!(ObjStatus[objNum] & 1))
		return 0;
	for(child = Link[objNum]; child >= 0 && GetCoordUse(child) != LOCXYZ && OBJ_IsInside(child, objNum); child = Link[child]) {
		if(!(ObjStatus[child] & 1))
			return 0;
	}
	return 1;
}

/*set the "ok to take" flag on objNum and all its contents*/
/*C_1DBD_0C48*/void SetOkToGet(register int16 objNum) {
	register int16 child;

	ObjStatus[objNum] |= 1;
	for(child = Link[objNum]; child >= 0 && GetCoordUse(child) != LOCXYZ && OBJ_IsInside(child, objNum); child = Link[child])
		ObjStatus[child] |= 1;
}

/*usable types*/
/*11AA*/int16 UsableTypes[] = {
	0x3e, 0x2f, 0x12e, 0x133, 0xd6, 0x69, 0x6e, 0xd5, 0x67, 0xa,
	0x6a, 0xea, 0x5d, 0x6f, 0xc5, 0xa1, 0x66, 0x68, 0x6b, 0x6c,
	0x6d, 0x2b, 0x71, 0x52, 0x4f, 0x50, 0x51, 0x72, 0xf0, 0x78,
	0x3f, 0x113, 0xcf, 0xc0, 0x3d, 0xbc, 0xd8, 0x83, 0xad, 0xd4,
	0x5e, 0x6, 0x3c, 0xcc, 0xf7, 0xfc, 0xdd, 0x94, 0x2c, 0x34,
	0x19, 0xd9, 0x5f, 0xa5, 0x6e, 0x88, 0x77, 0xbe, 0x3b, 0x31,
	0x33, 0x14, 0x2a, 0x16, 0x89, 0xe9, 0xed, 0x32, 0x1d, 0x5c,
	0x86, 0xbf, 0xd0, 0xd1, 0x56, 0x58, 0x57, 0x8f, 0xb1, 0xe5,
	0x101, 0xf2, 0xba, 0xfe
};

/*can objects of type typeObject be "use"d (one of UsableTypes)?*/
/*C_1DBD_0C8C*/IsUsableType(int16 typeObject) {
	int16 i;

	for(i = 0; i < 84; i ++)
		if(UsableTypes[i] == typeObject)
			return 1;
	return 0;
}

/*parse a number typed by the player.
 coordMode 0: plain number; returns the value, -1 for an empty line
   accepted with <Enter>, -2 for an escaped line or a bad character.
 coordMode 1: "<n>S" (positive) or "<n>N" (negative),
 coordMode 2: "<n>E" (positive) or "<n>W" (negative);
   returns 100 when the letter is missing/wrong.*/
/*C_1DBD_0CB3*/ParseNumberInput(unsigned char *str, int16 coordMode) {
	int16 i, value;

	value = 0;
	for(i = 0; i < strlen(str); i ++) {
		if(str[i] >= '0' && str[i] <= '9') {
			value = value * 10 + (str[i] - '0');
			continue;
		}
		if(coordMode == 1) {
			if(toupper(str[i]) == 'S')
				return value;
			else if(toupper(str[i]) == 'N')
				return -value;
			else
				return 100;
		}
		if(coordMode == 2) {
			if(toupper(str[i]) == 'E')
				return value;
			else if(toupper(str[i]) == 'W')
				return -value;
			else
				return 100;
		}
		return -2;
	}
	if(coordMode)
		return 100;
	/*GetsAborted is set by CON_gets when the input was escaped*/
	if(i == 0 && GetsAborted == 0)
		return -1;
	if(i == 0 && GetsAborted)
		return -2;
	return value;
}

/*show the portrait of objNum in the status panel with its name (1000:
  only the picture); waits for a key, outside a conversation*/
/*C_1DBD_0D8F*/void ShowPortrait(register int16 objNum) {
	int16 unused2, unused4, unused6;
	int16 savedStatus;
	struct tClipInfo *savedClip;
	char *name;

	unused2 = 0;
	unused4 = 0;
	unused6 = 0;
	if(objNum == 1000) {
		HideMouse
		DrawPortrait(objNum, 0xd2, 7);
		ShowMouse
		return;
	}
	if(IsInConversation && objNum >= 0xfd) {
		HideMouse
		DrawPortrait(objNum, 0xd2, 7);
		ShowMouse
		return;
	}
	if(objNum >= 0xc0) {
		switch(GetType(objNum)) {
			case 0x123: case 0x13c: case 0x13d: case 0x140:
				break;
			default:
				return;
		}
	} else if(objNum >= 0x98) {
		return;
	}
	savedClip = CurClip;
	HideMouse
	savedStatus = StatusDisplay;
	C_276B_1BAF();
	DrawPortrait(objNum, 0xd2, 7);
	if(objNum >= 0xb0) {
		switch(ObjShapeType[objNum]) {
			case 0x13c: case 0x140:
				name = "guard";
				break;
			case 0x13d:
				name = "parrot";
				break;
			case 0x123:
				name = "wisp";
				break;
			default:
				name = GetObjectString(objNum);
				break;
		}
	} else if(!(TalkFlags[objNum] & 1)) {
		name = GetObjectString(objNum);
	} else {
		name = TALK_GetCharName(objNum);
	}
	CON_setClip(&TextWindows[1]);
	CON_gotoxy((14 - strlen(name)) / 2 + 1, 12);
	CON_printf("%s\n", name);
	StatusDisplay = 0x9e;
	if(!IsInConversation) {
		CON_setClip(&TextWindows[3]);
		CON_ReadCommand();
	}
	CON_setClip(savedClip);
	ShowMouse
	StatusDisplay = savedStatus;
	if(!IsInConversation) {
		StatusDirty = 1;
		RefreshStatus();
	}
}

/*print the article ("a ", "an " or "the ") of tile numTile*/
/*C_1DBD_0F6C*/void PrintTileArticle(int16 numTile) {
	switch(TileFlag2[numTile] & 0xc0) {
		case 0x40: CON_printf("a "); break;
		case 0x80: CON_printf("an "); break;
		case 0xc0: CON_printf("the "); break;
	}
}

/*print the count (stackables, gold) or the article of objNum before
 its name; returns the count (1 for a single object)*/
/*C_1DBD_0FAB*/PrintObjCountOrArticle(register int16 objNum) {
	int16 quanType;
	register int16 count;

	/*QuanType: 4 = the whole Amount is the count, 2 = stackable (low
	byte), 0 = single object*/
	quanType = QuanType(GetType(objNum));
	if(quanType == 4) {
		count = Amount[objNum];
		CON_printf("%d ", count);
	} else if(quanType == 2) {
		count = GetQuan(objNum);
		CON_printf("%d ", count);
	} else {
		count = 1;
		PrintTileArticle(TILE_FRAME(objNum));
	}
	return count;
}

/*the first object at objNum's location, after objNum, that is not
 "ignored" (TileFlag2 0x10) and is not an NPC; -1 if none*/
/*C_1DBD_1030*/FindNonIgnoredObjAt(register int16 objNum) {
	register int16 other;

	for(other = FindLoc(GetX(objNum), GetY(objNum), MapZ); other >= 0; other = NextLoc()) {
		if(other == objNum)
			continue;
		if(!(TileFlag2[TILE_FRAME(other)] & 0x10)) {
			if(other > 0xff)
				break;
		}
	}
	return other;
}

/*search objNum at (x, y) (after looking at something adjacent): empties
 an open container on the ground and lists what was found*/
/*C_1DBD_10BA*/void SearchObj(int16 objNum, int16 x, int16 y) {
	register int16 item;
	int16 actor, actorX, actorY, objTyp, objFrm;
	int16 found, nextItem;

	found = 0;
	actor = Party[Active];
	actorX = GetX(actor);
	actorY = GetY(actor);
	if(IsTileIg(TILE_FRAME(objNum)))
		objNum = FindNonIgnoredObjAt(objNum);
	objTyp = GetType(objNum);
	objFrm = GetFrame(objNum);
	if(!CLOSE_ENOUGH(1, x, y, actorX, actorY))
		return;
	if(!IsContainerType(objTyp) && objTyp != 0xf9)
		return;
	if(objTyp == 0xbc && GetFrame(objNum))
		return;
	if(objTyp == 0x61)
		return;
	if((item = FindInv(objNum)) < 0) {
		CON_printf("\nSearching here, you find nothing.\n");
		return;
	}
	CON_printf("\nSearching here, you find ");
	while(item >= 0) {
		objTyp = GetType(item);
		MoveObj(item, GetX(objNum), GetY(objNum), MapZ);
		MoveObj(actor, actorX, actorY, MapZ);
		VIS_UpdateView();
		DisplayTick();
		nextItem = FindInv(objNum);
		if(found && nextItem >= 0)
			CON_printf(", ");
		else if(found && nextItem == -1)
			CON_printf(" and ");
		if(QuanType(objTyp) && GetQuan(item) == 0)
			SetQuan(item, 1);
		PrintObjCountOrArticle(item);
		CON_printf(GetObjectString(item));
		found = 1;
		item = nextItem;
	}
	CON_printf(".\n");
}

/*"look" command: describe the selected object or tile (the time on a
 clock, what a pot holds, a special note), show the portrait of a person,
 then search the place*/
/*C_1DBD_12BD*/void CmdLook() {
	unsigned char numTile;
	int16 x, y, count, readSomething;
	int16 objTyp, dir;

	readSomething = 0;
	CurClip->attrFlags |= 0x10;
	if(QuanType(GetType(SelectionObj)) && GetQuan(SelectionObj) == 0)
		SetQuan(SelectionObj, 1);
	CON_printf("You see ");
	dir = MkDirection(PointerX, PointerY);
	if(dir != -1 && !IN_VEHICLE) {
		UpdateMoveFrame(Party[Active], dir);
		VIS_UpdateView();
	}
	if(SelectionIsDark) {
		CON_printf("darkness.\n");
		EndPagingAtPrompt = 1;
		return;
	}
	x = GetObjXOrVehicle(Party[Active]);
	y = GetObjYOrVehicle(Party[Active]);
	if(SelectionObj == -1 || IsInvisible(SelectionObj) || IsTileIg(TILE_FRAME(SelectionObj))) {
		/*no object: describe the map tile under the pointer*/
		if(PointerX < 168 && PointerY < 168) {
			SelectionX = SCR2TIL(PointerX) + MapX - 5;
			SelectionY = SCR2TIL(PointerY) + MapY - 5;
			numTile = GetTileAtXYZ(SelectionX, SelectionY, MapZ);
			PrintTileArticle(numTile);
			CON_printf(GetTileString(numTile));
		} else {
			CON_printf("nothing");
		}
		CON_printf(".\n");
		EndPagingAtPrompt = 1;
		return;
	}
	objTyp = GetType(SelectionObj);
	if(SelectionObj < 0x100 && SelectionObj >= 0) {
		/*a person or a creature: name and portrait*/
		if(!IsPlrControl(SelectionObj) || IN_VEHICLE)
			PrintObjCountOrArticle(SelectionObj);
		CON_printf(GetObjectString(SelectionObj));
		if(objTyp == 0x126 && TalkFlags[SelectionObj] == 0xff)
			CON_printf(" with a bell around its neck.\n");
		else
			CON_printf(".\n");
		if(!IN_VEHICLE)
			ShowPortrait(SelectionObj);
	} else {
		count = PrintObjCountOrArticle(SelectionObj);
		CON_printf(GetObjectString(SelectionObj));
		if(objTyp == 0xf7 && GetQual(SelectionObj) == 0) {
			CurClip->attrFlags |= 0x10;
			CON_printf("\nSearching here, you find a piece of paper with notes written on it:\n\n");
			CON_printf("\"The statue is of the same materials and ");
			CON_printf("workmanship as that golden head Fritz found.  It appears to be ");
			CON_printf("a mechanism of some sort.  I will have to talk ");
			CON_printf("to Fritz about this...&");
			EndPagingAtPrompt = 1;
		}
		if(objTyp == 0x88 && GetFrame(SelectionObj) > 0) {
			CON_printf(" holding ");
			switch(GetFrame(SelectionObj)) {
				case 1: CON_printf("%d pint\\s of tar", TestObj(SelectionObj, 0x8b)); break;
				case 2: CON_printf("%d pint\\s of water", TestObj(SelectionObj, 0x8c)); break;
			}
		}
		CON_printf(".\n");
		if(objTyp == 0x92 && Time_H >= 5 && Time_H < 20) {
			/*sundial: daytime only*/
			CON_printf("\n\nThe time is ");
			if(Time_H == 0)
				CON_printf("12:%02d A.M.\n", Time_M);
			else if(Time_H < 12)
				CON_printf("%d:%02d A.M.\n", Time_H, Time_M);
			else if(Time_H == 12)
				CON_printf("12:%02d P.M.\n", Time_M);
			else
				CON_printf("%d:%02d P.M.\n", Time_H - 12, Time_M);
		}
		if(IsContainerType(objTyp) || objTyp == 0xf9 && (objTyp != 0xbc || GetFrame(SelectionObj) == 0))
			SearchObj(SelectionObj, SelectionX, SelectionY);
	}
	EndPagingAtPrompt = 1;
}

/*is there an object at (x, y) that things can be put on (a "supporting"
 tile, or 0xbc in frame 1)? (stops at an impassable object)*/
/*C_1DBD_1677*/static HasSupportAt(int16 x, int16 y) {
	int16 objNum;

	for(objNum = FindLoc(x, y, MapZ); objNum >= 0; objNum = NextLoc()) {
		if(IsTileSu(TILE_FRAME(objNum)))
			return 1;
		if(GetType(objNum) == 0xbc && GetFrame(objNum) == 1)
			return 1;
		if(IsTerrainImpass(TILE_FRAME(objNum)))
			return 0;
	}
	return 0;
}

/*ask "How many?" for a stack of objNum (not when readied);
 returns the quantity chosen (all on empty input, 0 if invalid),
 1 for a single object*/
/*C_1DBD_172C*/AskHowMany(int16 objNum) {
	int16 answer, available;
	char buf[5];
	int16 quanType, cursorY, savedSel, savedSelMode;

	quanType = QuanType(GetType(objNum));
	if(quanType && GetCoordUse(objNum) != EQUIP) {
		available = quanType == 2?GetQuan(objNum):Amount[objNum];
		if(available != 1) {
			savedSel = SelectionObj;
			cursorY = CurClip->curY;
			CON_printf("How many? ");
			savedSelMode = SelectMode;
			DrawSlotCursor(0);
			SelectMode = 0;
			CON_gets(buf, 4);
			SelectMode = savedSelMode;
			SelectionObj = savedSel;
			answer = ParseNumberInput(buf, 0);
			if(answer == -1) {
				CON_gotoxy(10, cursorY);
				CON_printf("%d\n", available);
				return available;
			}
			if(answer > available || answer == 0 || answer == -2) {
				CON_gotoxy(10, cursorY);
				CON_printf("0   \n");
				return 0;
			}
			CON_putch('\n');
		} else {
			answer = 1;
		}
	} else {
		answer = 1;
	}
	return answer;
}

/*is (x, y) impassable: the map tile and every object there are
 impassable (terrain bit 0x02) with none of the bits 0x10, 0x20, 0x40,
 0x80? returns 0 when it is, 1 when it is not (U6: IsBlockedAt)*/
/*C_1DBD_1848*/C_1DBD_1848(int16 x, int16 y) {
	int16 flags, obj;

	flags = TerrainType[GetTileAtXYZ(x, y, MapZ)];
	if((flags & 2) && !(flags & 0x10) && !(flags & 0x40) && !(flags & 0x80) && !(flags & 0x20))
		return 0;
	for(obj = FindLoc(x, y, MapZ); obj >= 0; obj = NextLoc()) {
		flags = TerrainType[TILE_FRAME(obj) - LocPartOffset];
		if((flags & 2) && !(flags & 0x10) && !(flags & 0x40) && !(flags & 0x80) && !(flags & 0x20))
			return 0;
	}
	return 1;
}

/*is (x, y) water or a similar tile (terrain bits 0x01 and 0x02,
 IsLavaTile or C_1810_0F71) with no object on it?*/
/*C_1DBD_18FD*/C_1DBD_18FD(int16 x, int16 y) {
	int16 flags, tile;

	flags = TerrainType[tile = GetTileAtXYZ(x, y, MapZ)];
	return (((flags & 1) && (flags & 2)) || IsLavaTile(tile) || C_1810_0F71(tile)) && FindLoc(x, y, MapZ) == -1;
}

/*"drop" command: drop or throw the selected inventory object of the
 displayed party member to a location within 8 tiles: onto water
 (it is lost), into a container, or on the ground; the breakable types
 (IsBreakableType) break when thrown farther than an adjacent tile*/
/*C_1DBD_1960*/void CmdDrop() {
	int16 objTyp, destX, destY, newObj, actor, target, objFrm, targetTyp;
	struct coord xyz;
	int16 qtyCode, count, quanType, cantDrop, unused;
	int16 obj, holder;

	unused = 0;
	if(SelectionObj == -1 || GetCoordUse(SelectionObj) == LOCXYZ || SelectionObj == InvenOwner) {
		CON_printf(NothingMsg);
		return;
	}
	CON_printf(GetObjectString(SelectionObj));
	CON_putch('\n');
	objTyp = GetType(SelectionObj);
	quanType = QuanType(objTyp);
	actor = Party[StatusMember];
	holder = GetAssoc(SelectionObj);
	obj = SelectionObj;
	objFrm = GetFrame(obj);
	cantDrop = STAT_CheckUnready(obj, actor);
	if(cantDrop == 0) {
		count = AskHowMany(SelectionObj);
		if(count == 0)
			return;
		CON_printf("Location: ");
		MouseMode = 1;
		if(SelectMode != 0) {
			DrawSlotCursor(0);
			SelectMode = 1;
		}
		CON_getch();
		CON_putch('\n');
		target = FindLoc(SelectionX, SelectionY, MapZ);
		if(SelectionX != -1 && SelectionY != -1 && COMBAT_Missile(actor, &SelectionX, &SelectionY, 8, 0)) {
			if(C_1DBD_18FD(SelectionX, SelectionY)) {
				/*into the water: lost*/
				if(obj >= 0x100 || obj < 0) {
					COMBAT_Missile(actor, &SelectionX, &SelectionY, 8, TILE_FRAME(obj));
					if(GetQuan(obj) > count) {
						SubQuan(obj, count);
					} else {
						if(GetType(holder) == 0x61)
							SetFrame(holder, 0);
						DeleteObj(obj);
					}
					if(StatusDisplay == 0x92)
						STAT_DrawInventory();
					return;
				} else {
					CON_printf(NotPossibleMsg);
					return;
				}
			}
		} else {
			CON_printf(NotPossibleMsg);
			return;
		}
		if(!C_1DBD_1848(SelectionX, SelectionY) && !HasSupportAt(SelectionX, SelectionY)) {
			if(target == -1 || !CanPutInContainer(target, obj)) {
				CON_printf(NotPossibleMsg);
				return;
			}
		}
		/*unready it*/
		STAT_StopEquipEffect(obj, actor);
		if(GetCoordUse(obj) == EQUIP) {
			Equipment[STAT_FindEquipSlot(obj)] = 0;
			if(STAT_GetEquipSlot(obj) == 8) {
				Party_2Handed[StatusMember] = 0;
				Equipment[5] = 0;
			}
		}
		STAT_ComputeWeights(actor);
		SubMov(actor, 3);
		/*animate the throw; destX/destY = where it lands*/
		destX = SelectionX;
		destY = SelectionY;
		COMBAT_Missile(actor, &destX, &destY, 8, TILE_FRAME(obj));
		if(!CLOSE_ENOUGH(1, SelectionX, SelectionY, GetX(actor), GetY(actor)) && IsBreakableType(objTyp)) {
			if(quanType) {
				qtyCode = QualQuan(GetQual(obj), count);
				if(GetQuan(obj) > count) {
					SubQuan(obj, count);
				} else {
					if(GetType(holder) == 0x61)
						SetFrame(holder, 0);
					DeleteObj(obj);
				}
			} else {
				if(IsContainerType(objTyp))
					MoveObj(obj, destX, destY, MapZ);
				DeleteObj(obj);
				if(GetType(holder) == 0x61)
					SetFrame(holder, 0);
			}
			if(objTyp == 0x8a) {
				KegFuse = 0;
				KegObject = -1;
				C_1810_1941(destX, destY);
			} else if(objTyp == 0x84) {
				AddMapObj(0xc0, 0, 1, destX, destY, MapZ);
				CON_printf("\nThe soft clay pot went squish.\n");
			} else {
				CON_printf("\nIt broke!\n");
			}
			VIS_UpdateView();
			StatusDirty = 0;
			STAT_RefreshInventory(InvenOwner);
			if(StatusDisplay == 0x92)
				STAT_DrawInventory();
			return;
		}
		targetTyp = GetType(target);
		if(IsContainerType(targetTyp) && CanPutInContainer(target, obj)) {
			/*into the container at the destination*/
			if(quanType) {
				qtyCode = QualQuan(GetQual(obj), count);
				if((newObj = AddInvObj(objTyp | objFrm << 10, CONTAINED, qtyCode, target)) != -1) {
					if(GetQuan(obj) > count) {
						SubQuan(obj, count);
					} else {
						DeleteObj(obj);
						if(GetType(holder) == 0x61)
							SetFrame(holder, 0);
					}
				}
			} else {
				if(InsertObj(obj, target, CONTAINED) != -1) {
					SetOkToGet(obj);
				} else {
					MoveObj(obj, destX, destY, MapZ);
					if(GetType(holder) == 0x61)
						SetFrame(holder, 0);
				}
			}
		} else {
			/*on the ground (a new stack for a partial quantity)*/
			if(quanType) {
				qtyCode = QualQuan(GetQual(obj), count);
				SetCoordXYZ(xyz, destX, destY, MapZ);
				newObj = AddObj(0, xyz, objTyp | objFrm << 10, 0, qtyCode);
				SetOkToGet(newObj);
				if(GetQuan(obj) > count) {
					SubQuan(obj, count);
				} else {
					if(GetType(holder) == 0x61)
						SetFrame(holder, 0);
					DeleteObj(obj);
				}
			} else if(objTyp == 0xf1) {
				KegFuse = 0;
				KegObject = -1;
				C_1810_19C1(destX, destY);
				DeleteObj(obj);
			} else {
				MoveObj(obj, destX, destY, MapZ);
				if(GetType(holder) == 0x61)
					SetFrame(holder, 0);
				SetOkToGet(obj);
			}
		}
		if(objTyp == 0x4e)
			SetLocal(obj);
		VIS_UpdateView();
		StatusDirty = 0;
	}
	STAT_RefreshInventory(InvenOwner);
}

/*"get" command: pick up the selected adjacent object (taking the whole
 content of 0xf9, a lit 0xd1 into a free hand, some types turned into
 others)*/
/*C_1DBD_1FEA*/void CmdGet() {
	int16 weight, strength, actorX, actorY, dir, room, caught, other;
	int16 item, nextItem, found, x, y, objFrm, qty;
	int16 actor, objTyp;

	caught = 0;
	nextItem = -1;
	if(IN_VEHICLE) {
		actor = 1;
		actorX = GetX(1);
		actorY = GetY(1);
	} else {
		actor = Party[Active];
		actorX = GetX(actor);
		actorY = GetY(actor);
	}
	/*pointing at an NPC or an "ignore" tile: take what is under it*/
	if((SelectionObj <= 0xff && SelectionX != -1) || IsTileIg(TILE_FRAME(SelectionObj)))
		SelectionObj = FindNonIgnoredObjAt(SelectionObj);
	if(SelectionObj == -1 || GetCoordUse(SelectionObj) != LOCXYZ) {
		CON_printf(NothingMsg);
		return;
	}
	dir = MkDirection(PointerX, PointerY);
	if(dir != -1) {
		UpdateMoveFrame(actor, dir);
		VIS_UpdateView();
	}
	if(QuanType(GetType(SelectionObj)) && GetQuan(SelectionObj) == 0)
		SetQuan(SelectionObj, 1);
	PrintObjCountOrArticle(SelectionObj);
	CON_printf(GetObjectString(SelectionObj));
	CON_putch('\n');
	if(!CLOSE_ENOUGH(1, SelectionX, SelectionY, actorX, actorY)) {
		CON_printf(OutOfRangeMsg);
		return;
	}
	objTyp = GetType(SelectionObj);
	objFrm = GetFrame(SelectionObj);
	if(objTyp == 0xf9)
		weight = GetWeight(SelectionObj);
	else
		weight = GetWeight(SelectionObj) + Encumbrance(SelectionObj);
	strength = STREN[Party[Active]];
	if(Active != StatusMember || StatusDisplay != 0x92)
		STAT_ComputeWeights(actor);
	/*weight 0 (except the types weighed per 10) or 255 means "can not be
	taken"; a character carries 2 x STR stones (weights are in tenths of
	stones)*/
	if((weight == 0 && !IsWeightPer10Type(objTyp)) || weight == 0xff || (SelectionObj < 0x100 && SelectionObj >= 0)) {
		CON_printf(NotPossibleMsg);
	} else if(strength * 20 < weight + WeightInven + WeightEquip) {
		CON_printf("\nThe total is too heavy.\n");
	} else {
		SubMov(actor, 3);
		if(QuanType(objTyp)) {
			/*stackable: merge into the inventory*/
			if(AddInvObj(GetType(SelectionObj) | objFrm << 10, INVEN, GetQuan(SelectionObj), actor) == -1)
				CON_printf(TooMuchMsg);
			else
				DeleteObj(SelectionObj);
		} else if(objTyp == 0xf9) {
			/*take the content too*/
			if((item = FindInv(SelectionObj)) >= 0) {
				CON_printf("\nSearching the body, you find ");
				x = GetX(SelectionObj);
				y = GetY(SelectionObj);
				while(item >= 0) {
					objTyp = GetType(item);
					MoveObj(item, x, y, MapZ);
					MoveObj(actor, actorX, actorY, MapZ);
					VIS_UpdateView();
					DisplayTick();
					nextItem = FindInv(SelectionObj);
					if(found && nextItem >= 0)
						CON_printf(", ");
					else if(found && nextItem == -1)
						CON_printf(" and ");
					if(QuanType(objTyp) && GetQuan(item) == 0)
						SetQuan(item, 1);
					PrintObjCountOrArticle(item);
					CON_printf(GetObjectString(item));
					found = 1;
					item = nextItem;
				}
				CON_printf(".\n");
			} else {
				CON_printf("\nSearching the body, you find nothing.\n");
			}
			if(InsertObj(SelectionObj, actor, INVEN) == -1) {
				CON_printf(TooMuchMsg);
				return;
			}
		} else if(objTyp == 0xd1) {
			/*goes directly into a free hand, lit*/
			if(Party[StatusMember] == actor) {
				if(Equipment[2] == 0) {
					Equipment[2] = SelectionObj;
				} else if(Equipment[5] == 0) {
					Equipment[5] = SelectionObj;
				} else {
					CON_printf("Your hands are full.\n");
					return;
				}
			}
			if(InsertObj(SelectionObj, actor, EQUIP) == -1) {
				CON_printf(TooMuchMsg);
				return;
			}
			SetQual(SelectionObj, 100);
			PassTime(0);
		} else if(objTyp == 0x6f) {
			if(AddInvObj(0x6a, INVEN, 1, actor) != -1)
				DeleteObj(SelectionObj);
			else
				CON_printf(TooMuchMsg);
		} else if(objTyp == 0xf5) {
			if(AddInvObj(0x5c, INVEN, 1, actor) != -1)
				DeleteObj(SelectionObj);
			else
				CON_printf(TooMuchMsg);
		} else if(objTyp == 0x47) {
			/*becomes 3 to 11 objects of type 0x48, as many as can be carried*/
			room = strength * 20 - WeightInven - WeightEquip;
			qty = OSI_rand(3, 11);
			if(room * 10 >= qty) {
				if(AddInvObj(0x48, INVEN, qty, actor) != -1)
					DeleteObj(SelectionObj);
				else
					CON_printf(TooMuchMsg);
			} else {
				if(AddInvObj(0x48, INVEN, room * 10, actor) != -1)
					DeleteObj(SelectionObj);
				else
					CON_printf(TooMuchMsg);
			}
		} else if(objTyp == 0x4e) {
			other = OBJ_FindTypeAt(0xe6, GetX(SelectionObj), GetY(SelectionObj));
			if(InsertObj(SelectionObj, actor, INVEN) == -1) {
				CON_printf(TooMuchMsg);
				return;
			}
			if(other >= 0 && GetType(FindInv(other)) == 0x124) {
				DeleteObj(FindInv(other));
				DeleteObj(other);
			}
		} else {
			if(InsertObj(SelectionObj, actor, INVEN) == -1) {
				CON_printf(TooMuchMsg);
				return;
			}
		}
		if(Party[StatusMember] == actor && StatusDisplay == 0x92)
			STAT_RefreshInventory(actor);
		VIS_UpdateView();
	}
	if(Party[StatusMember] != actor)
		STAT_ComputeWeights(actor);
	MouseMode = 0;
}

/*is the push of objNum from (x, y) in direction dir blocked?
 the destination must be passable or hold a supporting object; for a
 diagonal push one of the orthogonal neighbours must be free too.
 returns 1 if blocked*/
/*C_1DBD_261D*/IsPushBlocked(int16 objNum, int16 x, int16 y, register int16 dir) {
	int16 destX;
	int16 destY, sideX1, sideY1, sideX2, sideY2;

	destX = x + DirIncrX[dir];
	destY = y + DirIncrY[dir];
	if(CanMoveTo(objNum, destX, destY) || HasSupportAt(destX, destY)) {
		/*odd directions are the diagonals*/
		if(dir & 1) {
			sideX1 = x + DirIncrX[dir-1];
			sideY1 = y + DirIncrY[dir-1];
			sideX2 = x + DirIncrX[dir+1];
			sideY2 = y + DirIncrY[dir+1];
			if(
				CanMoveTo(objNum, sideX1, sideY1) || HasSupportAt(sideX1, sideY1) ||
				CanMoveTo(objNum, sideX2, sideY2) || HasSupportAt(sideX2, sideY2)
			)
				return 0;
			else
				return 1;
		} else {
			return 0;
		}
	} else {
		return 1;
	}
}

/*"move" command: push an object on the map by one tile (into a
 container there, or lost in the water), or move an inventory object to
 another party member, onto the gem stand, or into/out of a container*/
/*C_1DBD_26E8*/void CmdMove() {
	unsigned char ch;
	int16 quanType, destX, destY, objX, objY, dir;
	int16 weight, count, qtyCode, srcMember, target, objFrm, actor, cantMove, unused;
	int16 itemTyp, dirMoved;
	int16 obj, objTyp;

	unused = 0;
	if(SelectionObj == -1 || SelectionObj == InvenOwner) {
		CON_printf(NothingMsg);
		return;
	}
	LastNumberPlural = 0;
	CON_printf(GetObjectString(SelectionObj));
	CON_putch('\n');
	if(SelectionObj == 0x2f || SelectionObj == 0x37 || SelectionObj == 0x1f) {
		CON_printf(NotPossibleMsg);
		return;
	}
	objTyp = GetType(SelectionObj);
	objFrm = GetFrame(SelectionObj);
	quanType = QuanType(objTyp);
	if(GetCoordUse(SelectionObj) == LOCXYZ) {
		if(!CLOSE_ENOUGH(1, SelectionX, SelectionY, MapX, MapY)) {
			CON_printf(OutOfRangeMsg);
			return;
		}
		dir = MkDirection(PointerX, PointerY);
		if(dir != -1) {
			UpdateMoveFrame(Party[Active], dir);
			VIS_UpdateView();
		}
		objX = GetX(SelectionObj);
		objY = GetY(SelectionObj);
		MouseMode = 0;
		/*center the mouse direction cursor on the object
		(88,88 = center of the map window)*/
		DirOriginX = TIL2SCR(objX - MapX) + 88;
		DirOriginY = TIL2SCR(objY - MapY) + 88;
		CON_printf("To ");
		if(CON_getch() != 0x80) {
			CON_printf("nowhere.\n");
			MouseMode = 0;
			DirOriginX = 88;
			DirOriginY = 88;
			CON_UpdateMousePointer();
			return;
		}
		DirOriginX = 88;
		DirOriginY = 88;
		CON_UpdateMousePointer();
		AdvanceBlocked = 1;
		dirMoved = AdvanceDir;
		AdvanceDir = mkMouseDir();
		CON_printf("%s.\n", DirNames[dirMoved]);
		actor = Party[Active];
		weight = GetWeight(SelectionObj) + Encumbrance(SelectionObj);
		if(TypeWeight[GetType(SelectionObj)] == 0) {
			CON_printf(NotPossibleMsg);
		} else {
			/*an NPC resists being pushed (strength contest), can't push
			oneself; (frame & 3) == 3: lying NPC?*/
			if(SelectionObj < 0x100 && SelectionObj >= 0 && (ResistStrVsStr(actor, SelectionObj) || actor == SelectionObj ||
				((GetFrame(SelectionObj) & 3) == 3 && GetType(SelectionObj) != 0xf9))) {
				CON_printf(FailedMsg);
				MouseMode = 0;
				return;
			}
			SubMov(Party[Active], 5);
			destX = objX + DirIncrX[dirMoved];
			destY = objY + DirIncrY[dirMoved];
			if(C_1DBD_18FD(destX, destY)) {
				/*pushed into the water*/
				if(SelectionObj >= 0x100 || SelectionObj < 0) {
					DeleteObj(SelectionObj);
					return;
				} else {
					CON_printf(FailedMsg);
					return;
				}
			}
			target = FindLoc(destX, destY, MapZ);
			/*pushed onto a container: put it inside*/
			if(target != -1 && CanPutInContainer(target, SelectionObj) && weight != 0xff) {
				if(InsertObj(SelectionObj, target, CONTAINED) == -1) {
					MoveObj(SelectionObj, destX, destY, MapZ);
					return;
				}
			} else {
				if(IsPushBlocked(SelectionObj, objX, objY, dirMoved)) {
					CON_printf(BlockedMsg);
					MouseMode = 0;
					return;
				}
				MoveObj(SelectionObj, destX, destY, MapZ);
			}
			VIS_UpdateView();
		}
	} else {
		/*inventory object of the displayed member StatusMember*/
		cantMove = STAT_CheckUnready(SelectionObj, Party[StatusMember]);
		if(cantMove) {
			STAT_RefreshInventory(InvenOwner);
			return;
		}
		if(!IsNpcOnScreen(Party[StatusMember])) {
			CON_printf("\nThat person isn't here!\n");
			return;
		}
		count = AskHowMany(SelectionObj);
		if(!count)
			return;
		CON_printf("To ");
		obj = SelectionObj;
		itemTyp = GetType(obj);
		srcMember = StatusMember;
		if(SelectMode != 0) {
			SelectRange = 7;
			DrawSlotCursor(0);
			SelectMode = 1;
		}
		ch = CON_getch();
		if(ch != 0x8e || SelectionObj == -1) {
			CON_printf("nobody.\n");
			return;
		}
		if(GetCoordUse(SelectionObj) == LOCXYZ && (
			(SelectionObj < 0x100 && SelectionObj >= 0 && !IsNpcOnScreen(SelectionObj)) ||
			((SelectionObj >= 0x100 || SelectionObj < 0) && !CLOSE_ENOUGH(1, SelectionX, SelectionY, MapX, MapY)))) {
			CON_printf(OutOfRangeMsg);
			return;
		}
		if(GetType(SelectionObj) == 0x42) {
			/*the gem stand*/
			MoveObj(obj, GetX(SelectionObj) - 1, GetY(SelectionObj) - 1, MapZ);
			CON_printf("\nYou place the gem on the stand.\n");
			if(StatusDisplay == 0x92)
				STAT_DrawInventory();
			if(!(TalkFlags[1] & 2)) {
				C_1DBD_01A0();
				StatusDirty = 0;
				MouseMode = 0;
				return;
			}
			StatusDirty = 0;
			MouseMode = 0;
			return;
		} else {
			if(SelectionObj < 0x100 && SelectionObj >= 0) {
				/*give to another party member*/
				if(Party[srcMember] != SelectionObj) {
					CON_printf(GetObjectString(SelectionObj));
					CON_printf(".\n");
					if(!IsNpcOnScreen(SelectionObj)) {
						CON_printf("\nThat person is not around!\n");
						return;
					}
					if(!IsPlrControl(SelectionObj)) {
						CON_printf("\nOnly within the party!\n");
						return;
					}
					STAT_ComputeWeights(SelectionObj);
					if(quanType) {
						if(IsWeightPer10Type(objTyp))
							weight = TypeWeight[objTyp] * count / 10;
						else
							weight = TypeWeight[objTyp] * count;
					} else {
						weight = GetWeight(obj) + Encumbrance(obj);
					}
					if(STREN[SelectionObj] * 20 < weight + WeightInven + WeightEquip || C_0A5D_0FD6(SelectionObj) >= 0x10) {
						CON_printf("%s is carrying too much already.\n", GetObjectString(SelectionObj));
						STAT_ComputeWeights(Party[StatusMember]);
						return;
					}
					if(GetCoordUse(obj) == EQUIP && Party[srcMember] != SelectionObj) {
						Equipment[STAT_FindEquipSlot(obj)] = 0;
						if(STAT_GetEquipSlot(obj) == 8) {
							Party_2Handed[srcMember] = 0;
							Equipment[5] = 0;
						}
					}
					if(quanType) {
						qtyCode = QualQuan(GetQual(obj), count);
						if(AddInvObj(objTyp | objFrm << 10, INVEN, qtyCode, SelectionObj) != -1) {
							if(GetQuan(obj) > qtyCode) {
								SubQuan(obj, qtyCode);
							} else {
								if(GetType(GetAssoc(obj)) == 0x61)
									SetFrame(GetAssoc(obj), 0);
								DeleteObj(obj);
							}
						} else {
							CON_printf("%s is carrying too much already.\n", GetObjectString(SelectionObj));
						}
					} else {
						if(InsertObj(obj, SelectionObj, INVEN) == -1) {
							CON_printf("%s is carrying too much already.\n", GetObjectString(SelectionObj));
							return;
						}
					}
					STAT_ComputeWeights(Party[StatusMember]);
				} else {
					CON_printf("yourself.\n");
				}
				dir = MkDirection(PointerX, PointerY);
				if(dir != -1) {
					UpdateMoveFrame(Party[Active], dir);
					VIS_UpdateView();
				}
				SubMov(Party[Active], 5);
			} else {
				/*into (or out of) a container of the same owner*/
				if(GetCoordUse(SelectionObj) == LOCXYZ) {
					CON_printf("nobody.\n");
					return;
				}
				if(!CanPutInContainer(SelectionObj, obj)) {
					CON_printf(GetObjectString(SelectionObj));
					CON_printf("\n\nYou can't do that!\n");
					return;
				}
				if(GetOuterHolder(SelectionObj) != GetOuterHolder(obj)) {
					CON_printf(GetObjectString(SelectionObj));
					CON_printf("\n\nThat container belongs to someone else!\n");
					return;
				}
				if(C_0A5D_0FD6(SelectionObj) >= 0x10) {
					CON_printf(GetObjectString(SelectionObj));
					CON_printf("\n\nThat container is already full.\n");
					return;
				}
				if(GetCoordUse(obj) == EQUIP) {
					CON_printf(GetObjectString(SelectionObj));
					CON_printf("\n\nYou have to unready it first.\n");
					return;
				}
				if(SelectionObj == obj) {
					CON_printf("\n\nHow can a container go into itself!\n");
					return;
				}
				if(GetAssoc(obj) == SelectionObj) {
					/*already in it: move it out to the container's holder*/
					CON_printf("out of ");
					CON_printf(GetObjectString(SelectionObj));
					CON_putch('\n');
					SubMov(Party[Active], 5);
					if(quanType) {
						qtyCode = QualQuan(GetQual(obj), count);
						if(GetQuan(obj) > qtyCode) {
							SubQuan(obj, qtyCode);
						} else {
							if(GetType(SelectionObj) == 0x61)
								SetFrame(SelectionObj, 0);
							DeleteObj(obj);
						}
						if(AddInvObj(objTyp | objFrm << 10, INVEN, qtyCode, GetAssoc(SelectionObj)) == -1) {
							AddMapObj(objTyp | objFrm << 10, 0, qtyCode, GetX(GetAssoc(SelectionObj)), GetY(GetAssoc(SelectionObj)), MapZ);
							return;
						}
					} else {
						if(InsertObj(obj, GetAssoc(SelectionObj), CONTAINED) == -1) {
							MoveObj(obj, GetX(GetAssoc(SelectionObj)), GetY(GetAssoc(SelectionObj)), MapZ);
							return;
						}
					}
					InvenSize --;
				} else {
					CON_printf("into ");
					CON_printf(GetObjectString(SelectionObj));
					CON_putch('\n');
					SubMov(Party[Active], 5);
					if(quanType) {
						if(AddInvObj(objTyp | objFrm << 10, CONTAINED, count, SelectionObj) == -1)
							CON_printf("That container is full.\n");
						else if(GetQuan(obj) == count)
							DeleteObj(obj);
						else
							SubQuan(obj, count);
					} else {
						if(InsertObj(obj, SelectionObj, CONTAINED) == -1) {
							CON_printf("That container is full.\n");
							return;
						}
					}
					/*the frame of 0x61 shows what it holds*/
					if(GetType(SelectionObj) == 0x61) {
						switch(itemTyp) {
							case 0x5d: case 0x6c: case 0x7b:
								SetFrame(SelectionObj, 1);
								break;
							case 0xa: case 0x82:
								SetFrame(SelectionObj, 2);
								break;
							case 0x36: case 0x37: case 0x38: case 0x6d: case 0x81: case 0xcc:
								SetFrame(SelectionObj, 3);
								break;
							default:
								SetFrame(SelectionObj, 0);
								break;
						}
					}
					InvenSize --;
				}
			}
			STAT_RefreshInventory(InvenOwner);
		}
	}
	StatusDirty = 0;
	MouseMode = 0;
}

/*a check against a characteristic (unused): true when it fails*/
/*C_1DBD_3275*/C_1DBD_3275(int16 value) {
	return OSI_rand(1, 30) < ((45 - value) / 2);
}

/*does a strength check fail?*/
/*C_1DBD_32A2*/FailsStrCheck(int16 strength) {
	return OSI_rand(1, 30) < ((45 - strength) / 2);
}

/*a check against a characteristic (unused): true when it fails*/
/*C_1DBD_32CF*/C_1DBD_32CF(int16 value) {
	return OSI_rand(1, 30) < ((45 - value) / 2);
}

/*use a stack of torches objNum: one of them, lit, goes into a free hand*/
/*C_1DBD_32FC*/void UseTorch(register int16 objNum) {
	int16 quan;
	register int16 torch;

	quan = GetQuan(objNum);
	if(Equipment[2] && Equipment[5]) {
		CON_printf("\nYour hands are full.\n");
	} else {
		if(quan > 1)
			SetQuan(objNum, quan - 1);
		else
			DeleteObj(objNum);
		torch = AddInvObj(0xd1, EQUIP, 0, Party[StatusMember]);
		if(Equipment[2] == 0)
			Equipment[2] = torch;
		else if(Equipment[5] == 0)
			Equipment[5] = torch;
	}
	/*(the torch is used even when none was made)*/
	SetQual(torch, 100);
	PassTime(0);
	if(StatusDisplay == 0x92)
		STAT_DrawInventory();
}

/*use a brazier (lighting it burns the people around) or a campfire:
 light or douse it (frame bit 0)*/
/*C_1DBD_33A4*/void UseLightSource(register int16 objNum) {
	int16 objTyp;
	int16 far *list;
	register int16 objFrm;

	objTyp = GetType(objNum);
	objFrm = GetFrame(objNum);
	CON_putch('\n');
	switch(objTyp) {
		case 0xba:
			CON_printf("brazier");
			for(list = Explosion(GetX(objNum), GetY(objNum), 0x18a); *list >= 0; list ++) {
				if(*list < 0x100 && *list >= 0) {
					LoseHP(*list, OSI_rand(1, 20));
					CON_putch('\n');
					COMBAT_displaySeverity(*list);
				}
			}
			break;
		case 0xcf:
			CON_printf("campfire");
			break;
	}
	/*frame bit 0: lit; bit 1 (variant) is kept*/
	if(objFrm & 1) {
		SetFrame(objNum, objFrm & 2);
		CON_printf(" is doused.\n");
	} else {
		SetFrame(objNum, objFrm | 1);
		CON_printf(" is lit.\n");
	}
}

/*use the grinding container objNum: corn (0x6c) alone gives corn flour
 (0x5d); 0x81, 0x7b and 0x82 together give gunpowder (0xcc), as much
 as the smallest of the three*/
/*C_1DBD_34DD*/void UseGrinder(int16 objNum) {
	int16 corn = -1;
	unsigned char qty = 0;
	int16 ingr[3] = {-1, -1, -1};
	register int16 item;
	register int16 i = 0;

	CON_putch('\n');
	if((item = FindInv(objNum)) < 0) {
		CON_printf("\nThere's nothing to grind!\n");
		return;
	}
	for(; item >= 0; item = NextInv()) {
		switch(GetType(item)) {
			case 0x81: ingr[0] = item; break;
			case 0x7b: ingr[1] = item; break;
			case 0x82: ingr[2] = item; break;
			case 0x6c: corn = item; break;
			default:
				CON_printf("\nYou can't grind that!\n");
				return;
		}
		if(corn != -1)
			break;
		qty = (GetQuan(item) > qty && qty != 0)?qty:GetQuan(item);
	}
	if(corn != -1) {
		if(ingr[0] == -1 && ingr[1] == -1 && ingr[2] == -1) {
			CON_printf("\nYou grind the corn into corn flour.\n");
			if((qty = GetQuan(corn)) == 0)
				qty = 1;
			DeleteObj(corn);
			AddInvObj(0x5d, CONTAINED, qty, objNum);
		} else {
			CON_printf("\nThe mixture in the %s is wrong.\n", GetObjectString(objNum));
			return;
		}
	} else {
		for(; i < 3; i ++) {
			if(ingr[i] < 0) {
				CON_printf("\nYou're missing some ingredients.\n");
				return;
			}
			if(GetQuan(ingr[i]) > qty)
				SetQuan(ingr[i], GetQuan(ingr[i]) - qty);
			else
				DeleteObj(ingr[i]);
		}
		AddInvObj(0xcc, CONTAINED, qty, objNum);
		CON_printf("\nYou mix a batch of gunpowder.\n");
	}
	if(StatusDisplay == 0x92)
		STAT_DrawInventory();
}

/*unused direction tables*/
/*1258*/signed char D_1258[] = {0, 1, 0, -1};
/*125C*/signed char D_125C[] = {-1, 0, 1, 0};

/*use the bell objNum on a dinosaur (0x118 in frame 0x0d, or in frame 0x11
 standing for the NPC in its quality)*/
/*C_1DBD_36C3*/void UseBell(int16 objNum) {
	int16 ch, targetTyp, x, y;
	int16 frm, user;

	user = C_1DBD_0003(objNum);
	x = GetX(user);
	y = GetY(user);
	CON_printf("On ");
	if(SelectMode != 0) {
		DrawSlotCursor(0);
		SelectMode = 1;
	}
	ch = CON_getch();
	if(ch == 0x8e && SelectionObj != -1) {
		CON_printf(GetObjectString(SelectionObj));
		CON_putch('\n');
		targetTyp = GetType(SelectionObj);
		frm = GetFrame(SelectionObj);
		if(GetCoordUse(SelectionObj) == LOCXYZ && !CLOSE_ENOUGH(1, SelectionX, SelectionY, x, y)) {
			CON_printf(OutOfRangeMsg);
		} else if(targetTyp == 0x118 && (frm == 0xd || frm == 0x11)) {
			if(frm == 0x11)
				SelectionObj = GetQual(SelectionObj);
			if(!(NPCStatus[SelectionObj] & 4)) {
				CON_printf(NoEffectMsg);
			} else if(TalkFlags[SelectionObj] & 0xff) {
				CON_printf("\nIt already has a bell on its neck.\n");
			} else {
				DeleteObj(objNum);
				TalkFlags[SelectionObj] |= 0xff;
				CON_printf("\nYou put the bell on the dinosaur's neck.\n");
				if(SelectionObj == 0xb5)
					TalkFlags[0x14] |= 0x80;
				if(StatusDisplay == 0x92)
					STAT_DrawInventory();
			}
		} else {
			CON_printf(NoEffectMsg);
		}
	} else {
		CON_printf(NothingMsg);
	}
}

/*use objNum (a tool) on a rock (0xac: a piece flakes off) or on an
 unlit campfire (0xcf: makes charcoal, 0x81)*/
/*C_1DBD_3866*/void C_1DBD_3866(int16 objNum) {
	int16 ch, targetTyp, x, y;
	unsigned char qty;
	int16 user, item;

	user = C_1DBD_0003(objNum);
	CON_printf("On ");
	if(SelectMode != 0) {
		DrawSlotCursor(0);
		SelectMode = 1;
	}
	ch = CON_getch();
	if(ch == 0x8e && SelectionObj != -1) {
		CON_printf(GetObjectString(SelectionObj));
		CON_putch('\n');
		targetTyp = GetType(SelectionObj);
		x = GetX(user);
		y = GetY(user);
		if(GetCoordUse(SelectionObj) == LOCXYZ && !CLOSE_ENOUGH(1, SelectionX, SelectionY, x, y)) {
			CON_printf(OutOfRangeMsg);
		} else if(targetTyp == 0xac) {
			if(FailsStrCheck(GetStr(user))) {
				CON_printf("\nGlanced off!\n");
			} else {
				CON_printf("\nA piece of rock flakes off.\n");
				if(AddInvObj(0x3a, INVEN, 0, user) < 0)
					AddMapObj(0x3a, 0, 0, x, y, MapZ);
				if(StatusDisplay == 0x92)
					STAT_DrawInventory();
			}
		} else if(targetTyp == 0xcf && GetFrame(SelectionObj) == 0) {
			qty = OSI_rand(1, GetStr(user));
			for(item = FindInv(user); item != -1; item = NextInv()) {
				if(GetType(item) == 0x81 && GetQuan(item) + qty <= 0xff)
					break;
			}
			if(item == -1) {
				CON_printf("\nYou made some charcoal.\n");
				if(AddInvObj(0x81, INVEN, qty, user) < 0)
					AddMapObj(0x81, 0, 0, x, y, MapZ);
			} else {
				SetQuan(item, GetQuan(item) + qty);
			}
			if(StatusDisplay == 0x92)
				STAT_DrawInventory();
		} else {
			CON_printf(NoEffectMsg);
		}
	} else {
		CON_printf(NothingMsg);
	}
}

/*use a grenade (0x89) or a device (0xf1): light it (it becomes 0x8a /
 stays 0xf1, and the fuse, KegFuse, starts)*/
/*C_1DBD_3AA1*/void UseExplosive(register int16 objNum) {
	int16 holder = -1;
	int16 owner = -1, isGrenade = 0, newTyp, status;

	if(GetCoordUse(objNum) != LOCXYZ) {
		owner = holder = GetAssoc(objNum);
		while(holder >= 0x100 || holder < 0)
			holder = GetAssoc(holder);
	}
	if(GetType(objNum) == 0x89)
		isGrenade = 1;
	if(GetFrame(objNum) == 0) {
		if(KegFuse) {
			if(isGrenade)
				CON_printf("\nA grenade has already been lit.\n");
			else
				CON_printf("\nA device has already been activated.\n");
		} else {
			if(GetQuan(objNum) > 1)
				SetQuan(objNum, GetQuan(objNum) - 1);
			else
				DeleteObj(objNum);
			if(isGrenade)
				newTyp = 0x8a;
			else
				newTyp = 0xf1;
			if(owner >= 0) {
				if(holder == owner)
					status = INVEN;
				else
					status = CONTAINED;
				if((objNum = AddInvObj(newTyp, status, 1, owner)) == -1)
					objNum = AddMapObj(newTyp, 0, 1, GetX(holder), GetY(holder), MapZ);
			} else {
				objNum = AddMapObj(newTyp, 0, 1, GetX(objNum), GetY(objNum), MapZ);
			}
			KegFuse = 3;
			KegObject = objNum;
			if(isGrenade)
				CON_printf("\nGrenade lit!\n");
			else
				CON_printf("\nDevice activated.\n");
			if(GetCoordUse(objNum) == LOCXYZ) {
				VIS_UpdateView();
				DisplayTick();
			} else {
				STAT_DrawInventory();
			}
		}
	} else {
		CON_printf(NoEffectMsg);
	}
}

/*the lit explosive objNum (on the map or in an inventory) explodes*/
/*C_1DBD_3C68*/void Detonate(register int16 objNum) {
	int16 x, y, objTyp;
	register int16 holder;

	if(objNum < 0)
		return;
	if(GetCoordUse(objNum) == LOCXYZ) {
		x = GetX(objNum);
		y = GetY(objNum);
	} else {
		holder = objNum;
		do
			holder = GetAssoc(holder);
		while(GetCoordUse(holder) != LOCXYZ);
		x = GetX(holder);
		y = GetY(holder);
	}
	objTyp = GetType(objNum);
	DeleteObj(objNum);
	if(objTyp == 0xf1)
		C_1810_19C1(x, y);
	else
		C_1810_1941(x, y);
	DeleteObj(objNum);
	KegFuse = 0;
	KegObject = -1;
}

/*after an explosion at (x, y): around it, the grenades (0x89) explode,
 the boulders (0xa7) are pushed away, 0x107 is hurt and 0x11 destroyed*/
/*C_1DBD_3D25*/void AfterExplosion(int16 x, int16 y) {
	int16 i;
	int16 lastBoulder = -1, last107 = -1, last11 = -1;
	int16 newObj;
	int16 objNum, j;

	for(i = x - 1; i <= x + 1; i ++) {
		for(j = y - 1; j <= y + 1; j ++) {
			if((objNum = FindTypeAt(i, j, MapZ, 0x89)) >= 0) {
				newObj = AddMapObj(0x8a, 0, 1, GetX(objNum), GetY(objNum), MapZ);
				if(GetQuan(objNum) > 1)
					SetQuan(objNum, GetQuan(objNum) - 1);
				else
					DeleteObj(objNum);
				VIS_UpdateView();
				DisplayTick();
				Detonate(newObj);
			}
			if((objNum = FindTypeAt(i, j, MapZ, 0xa7)) >= 0 && lastBoulder != objNum) {
				C_1810_14EA(x, y, objNum);
				lastBoulder = objNum;
			}
			if((objNum = FindTypeAt(i, j, MapZ, 0x107)) >= 0 && last107 != objNum) {
				LoseHP(objNum, 30);
				last107 = objNum;
			}
			if((objNum = FindTypeAt(i, j, MapZ, 0x11)) >= 0 && last11 != objNum) {
				DeleteObj(objNum);
				last11 = objNum;
			}
		}
	}
}

/*first object of type objTyp at (x, y, z), -1 if none*/
/*C_1DBD_3E75*/FindTypeAt(int16 x, int16 y, int16 z, int16 objTyp) {
	int16 objNum;

	for(objNum = FindLoc(x, y, z); objNum >= 0 && GetType(objNum) != objTyp; objNum = NextLoc());
	return objNum;
}

/*ground tiles on which a lowered drawbridge ends (U6's list)*/
/*1260*/unsigned char BridgeEndTiles[] = {0x10, 0x1c, 0x20, 0x2c, 0xd6};

/*is tileNum one of the BridgeEndTiles tiles?*/
/*C_1DBD_3EAB*/IsBridgeEndTile(int16 tileNum) {
	int16 i;

	for(i = 0; i < 5; i ++) {
		if(BridgeEndTiles[i] == tileNum)
			return 1;
	}
	return 0;
}

/*create a permanent (not LOCAL) object of type typeObject/frame
 shapeNum at (x, y, MapZ); returns its number*/
/*C_1DBD_3ED0*/static AddPermanentObj(int16 x, int16 y, int16 typeObject, int16 shapeNum) {
	int16 numObject;
	struct coord xyz;

	SetCoordXYZ(xyz, x, y, MapZ);
	numObject = AddObj(0, xyz, typeObject, shapeNum, 0);
	ClrLocal(numObject);

	return numObject;
}

/*looks at the eggs (0xe6) within 20 tiles of (x, y), and does nothing
 more (U6: DestroyGargoyleEggs)*/
/*C_1DBD_3F24*/static void C_1DBD_3F24(int16 x, int16 y) {
	int16 egg, content;

	for(egg = SearchArea(x - 20, y - 20, x + 20, y + 20); egg >= 0; egg = NextArea()) {
		if(GetType(egg) == 0xe6)
			content = FindInv(egg);
	}
}

/*use the fishing pole objNum next to deep water (tile 0x0c)*/
/*C_1DBD_3F78*/void UseFishingPole(int16 objNum) {
	int16 actor, x, y, done;
	int16 i, j;

	done = 0;
	actor = C_1DBD_0003(objNum);
	x = GetObjXOrVehicle(actor);
	y = GetObjYOrVehicle(actor);
	for(i = 0; i < 3; i ++) {
		for(j = 0; j < 3; j ++) {
			if(i == 1 && j == 1)
				continue;
			/*(i for both coordinates: only the diagonal is looked at)*/
			if(GetTileAtXYZ(x + i - 1, y + i - 1, MapZ) == 0xc) {
				if(OSI_rand(0, 4) == 0) {
					if(AddInvObj(0xd5, INVEN, 1, actor) != -1)
						CON_printf("\nYou caught a fish!\n");
					else
						CON_printf("\nYou didn't get a fish.\n");
				} else {
					CON_printf("\nYou didn't get a fish.\n");
				}
				done = 1;
				break;
			}
		}
		if(done)
			break;
	}
	if(!done)
		CON_printf("\nYou need to stand next to deep water.\n");
	if(StatusDisplay == 0x92)
		STAT_DrawInventory();
}

/*drink objNum*/
/*C_1DBD_405D*/void UseDrink(int16 objNum) {
	DeleteObj(objNum);
	DrunkCounter ++;
	CON_printf("\nYou drink it.\n");
	if(StatusDisplay == 0x92)
		STAT_DrawInventory();
}

/*unused*/
/*1265*/char *FoodNames[] = {"Food", "Mutton", "Fruit"};

/*the 8 directions*/
/*126B*/signed char NeighbourDX[] = {0, 1, 1, 1, 0, -1, -1, -1};
/*1273*/signed char NeighbourDY[] = {-1, -1, 0, 1, 1, 1, 0, -1};

/*can the party (Party[0]) move to one of the 8 tiles around (x, y)?*/
/*C_1DBD_4089*/C_1DBD_4089(int16 x, int16 y, int16 unused) {
	int16 i;

	for(i = 0; i < 8; i ++) {
		if(CanMoveTo(Party[0], x + NeighbourDX[i], y + NeighbourDY[i]))
			return 1;
	}
	return 0;
}

/*use the raft objNum: board it (4 party members carrying paddles, 0x0c,
 in party mode) or leave it next to land*/
/*C_1DBD_40C8*/void UseVehicle(register int16 objNum) {
	int16 x, y, actor, objTyp, actorTyp, paddlers;
	register int16 i;

	paddlers = 0;
	objNum = COMBAT_getHead(objNum);
	objTyp = GetType(objNum);
	actor = Party[Active];
	actorTyp = GetType(actor);
	x = GetX(objNum);
	y = GetY(objNum);
	if(SoloMember != -1) {
		CON_printf(NotInSoloMsg);
		return;
	}
	if(PartySize < 4) {
		CON_printf("You don't have enough people to paddle.\n");
		return;
	}
	for(i = 0; i < PartySize; i ++) {
		if(C_0A5D_080A(Party[i], 0xc) >= 0) {
			paddlers ++;
			if(paddlers >= 4)
				break;
		}
	}
	if(IN_VEHICLE && actor == objNum) {
		if(C_130F_014E(objNum))
			objNum = Unboard(1);
		else
			CON_printf("\nOnly next to land.\n");
	} else if(paddlers < 4) {
		CON_printf("You would need at least four people carrying ");
		CON_printf("paddles to operate the raft effectively.\n");
	} else if(MapX - x > 1 || MapY - y > 1 || MapX - x < -2 || MapY - y < -2) {
		CON_printf(OutOfRangeMsg);
	} else {
		InsertObj(actor, objNum, INVEN);
		Board(objNum);
	}
}

/*eat objNum*/
/*C_1DBD_4225*/void UseFood(register int16 objNum) {
	register int16 inInventory;

	inInventory = 0;
	CON_printf("\nYou eat the food.\n");
	if(GetCoordUse(objNum) != LOCXYZ)
		inInventory = 1;
	if(QuanType(GetType(objNum)) && GetQuan(objNum) > 1)
		SetQuan(objNum, GetQuan(objNum) - 1);
	else
		DeleteObj(objNum);
	if(inInventory)
		STAT_DrawInventory();
}

/*always 0*/
/*C_1DBD_428E*/C_1DBD_428E(int16 objNum) {
	return 0;
}

/*load the flintlock (frame 1) of npc with one gunpowder (0xcc) it carries*/
/*C_1DBD_4297*/void LoadFlintlock(int16 flintlock, register int16 npc) {
	int16 actor, frm;
	register int16 powder;

	actor = Party[Active];
	if((frm = GetFrame(flintlock)) == 1) {
		CON_printf("\nFlintlock already loaded.\n");
		return;
	}
	if((powder = C_0A5D_080A(npc, 0xcc)) == -1) {
		if(npc == actor) {
			CON_printf("\nYou don't have any gunpowder.\n");
			return;
		} else {
			CON_printf("\n%s doesn't have any gunpowder.\n", GetObjectString(npc));
			return;
		}
	}
	if(GetQuan(powder) > 1)
		SetQuan(powder, GetQuan(powder) - 1);
	else
		DeleteObj(powder);
	SetFrame(flintlock, 1);
	if(npc == actor)
		CON_printf("\nFlintlock loaded.\n");
	else
		CON_printf("\n%s: Flintlock loaded.\n", GetObjectString(npc));
	if(Party[StatusMember] == npc && StatusDisplay == 0x92)
		STAT_DrawInventory();
}

/*use the shovel objNum next to a clay tile (C_1DBD_0A29): get clay (0xc0)*/
/*C_1DBD_4390*/void UseShovel(int16 objNum) {
	int16 actor, x, y, done;
	register int16 i, j;

	done = 0;
	actor = C_1DBD_0003(objNum);
	x = GetObjXOrVehicle(actor);
	y = GetObjYOrVehicle(actor);
	for(i = 0; i < 3; i ++) {
		for(j = 0; j < 3; j ++) {
			if(i == 1 && j == 1)
				continue;
			if(C_1DBD_0A29(GetTileAtXYZ(x + i - 1, y + j - 1, MapZ))) {
				CON_printf("\nYou get some clay.\n");
				done = 1;
				if(AddInvObj(0xc0, INVEN, 1, actor) == -1)
					AddMapObj(0xc0, 0, 1, x + i - 1, y + j - 1, MapZ);
				break;
			}
		}
		if(done)
			break;
	}
	if(!done)
		CON_printf("\nThere is nothing to dig!\n");
	if(StatusDisplay == 0x92)
		STAT_DrawInventory();
}

/*use the clay objNum: shape a soft pot (0x84)*/
/*C_1DBD_447A*/void UseClay(register int16 objNum) {
	register int16 user;

	user = C_1DBD_0003(objNum);
	if(OBJ_IsInside(objNum, user)) {
		if(GetQuan(objNum) > 1)
			SetQuan(objNum, GetQuan(objNum) - 1);
		else
			DeleteObj(objNum);
		if(AddInvObj(0x84, INVEN, 1, user) == -1)
			AddMapObj(0x84, 0, 1, GetX(user), GetY(user), MapZ);
	} else {
		if(GetQuan(objNum) > 1)
			SetQuan(objNum, GetQuan(objNum) - 1);
		else
			DeleteObj(objNum);
		AddMapObj(0x84, 0, 1, GetX(objNum), GetY(objNum), MapZ);
	}
	CON_printf("\nYou shaped a small, soft pot from the clay.\n");
	if(StatusDisplay == 0x92)
		STAT_DrawInventory();
}

/*use fire (a lit object of the active member) on something: fire a
 soft pot (0x84 -> 0x85), cook corn flour into tortillas (0x5d -> 0x66),
 make charcoal (0xce -> 10 x 0x81), burn the types of IsFlammableType, or set
 off gunpowder (0xcc)*/
/*C_1DBD_458F*/void C_1DBD_458F() {
	int16 ch, x, y;
	unsigned char qty;
	int16 actor, targetTyp;

	actor = GetPartyObjOrVehicle(Active);
	x = GetX(actor);
	y = GetY(actor);
	CON_printf("On ");
	if(SelectMode != 0) {
		DrawSlotCursor(0);
		SelectMode = 1;
	}
	ch = CON_getch();
	if(ch == 0x8e && SelectionObj != -1) {
		CON_printf(GetObjectString(SelectionObj));
		CON_putch('\n');
		targetTyp = GetType(SelectionObj);
		actor = Party[StatusMember];
		if(GetCoordUse(SelectionObj) == LOCXYZ && !CLOSE_ENOUGH(1, SelectionX, SelectionY, x, y)) {
			CON_printf(OutOfRangeMsg);
		} else if(targetTyp == 0x84) {
			CON_printf("\nYou fired the pot.\n");
			if(GetQuan(SelectionObj) > 1)
				SetQuan(SelectionObj, GetQuan(SelectionObj) - 1);
			else
				DeleteObj(SelectionObj);
			if(AddInvObj(0x85, INVEN, 1, actor) == -1)
				AddMapObj(0x85, 0, 1, GetX(actor), GetY(actor), MapZ);
		} else if(targetTyp == 0x5d) {
			qty = GetQuan(SelectionObj);
			if(!OBJ_IsInside(SelectionObj, actor) || AddInvObj(0x66, INVEN, qty, actor) == -1)
				AddMapObj(0x66, 0, qty, SelectionX, SelectionY, MapZ);
			DeleteObj(SelectionObj);
			CON_printf("\nYou made corn tortillas.\n");
		} else if(targetTyp == 0xce) {
			CON_printf("\nYou made some charcoal.\n");
			if(GetQuan(SelectionObj) > 1)
				SetQuan(SelectionObj, GetQuan(SelectionObj) - 1);
			else
				DeleteObj(SelectionObj);
			if(AddInvObj(0x81, INVEN, 10, actor) == -1)
				AddMapObj(0x81, 0, 10, GetX(actor), GetY(actor), MapZ);
		} else if(IsFlammableType(targetTyp)) {
			CON_printf("\nYou incinerated the %s.\n", GetObjectString(SelectionObj));
			DeleteObj(SelectionObj);
		} else if(targetTyp == 0xcc) {
			if(OBJ_IsInside(SelectionObj, actor))
				C_1810_1941(GetX(actor), GetY(actor));
			else
				C_1810_1941(SelectionX, SelectionY);
			DeleteObj(SelectionObj);
		} else {
			CON_printf(NoEffectMsg);
		}
	} else {
		CON_printf(NothingMsg);
	}
	if(StatusDisplay == 0x92)
		STAT_DrawInventory();
}

/*use the knife objNum: cut cloth (0xb4) into strips, cut up the dead
 creatures (0x118, by frame) or the dead birds (0xf9, frame 0x0f)*/
/*C_1DBD_48B8*/void UseKnife(int16 objNum) {
	int16 ch, targetTyp, x, y, actor, frm, qty;
	int16 tx, ty;

	actor = C_1DBD_0003(objNum);
	CON_printf("On ");
	if(SelectMode != 0) {
		DrawSlotCursor(0);
		SelectMode = 1;
	}
	ch = CON_getch();
	if(ch == 0x8e && SelectionObj != -1) {
		CON_printf(GetObjectString(SelectionObj));
		CON_putch('\n');
		targetTyp = GetType(SelectionObj);
		x = GetX(actor);
		y = GetY(actor);
		if(GetCoordUse(SelectionObj) == LOCXYZ && !CLOSE_ENOUGH(1, SelectionX, SelectionY, x, y)) {
			CON_printf(OutOfRangeMsg);
		} else if(targetTyp == 0xb4) {
			actor = Party[StatusMember];
			if(!OBJ_IsInside(SelectionObj, actor) || AddInvObj(0x86, INVEN, 10, actor) == -1)
				AddMapObj(0x86, 0, 10, x, y, MapZ);
			if(GetQuan(SelectionObj) > 1)
				SetQuan(SelectionObj, GetQuan(SelectionObj) - 1);
			else
				DeleteObj(SelectionObj);
			CON_printf("\nYou made some cloth strips.\n");
			if(StatusDisplay == 0x92)
				STAT_DrawInventory();
		} else if(targetTyp == 0x118) {
			frm = GetFrame(SelectionObj);
			tx = GetX(SelectionObj);
			ty = GetY(SelectionObj);
			switch(frm) {
				case 1:
					CON_printf("\nYou ghoul!\n");
					break;
				case 3: case 4: case 6: case 8: case 9: case 11: case 15: case 17:
				case 27: case 28: case 30: case 31: case 33: case 35: case 43:
					CON_printf("\nThe hide is too tough.\n");
					break;
				case 21: case 39: case 41: case 45:
					AddMapObj(0x452, 0, 0, tx, ty, MapZ);
					AddMapObj(0xf8 | OSI_rand(0, 2) << 10, 0, 0, tx, ty, MapZ);
					AddMapObj(0xf8 | OSI_rand(0, 2) << 10, 0, 0, tx - 1, ty, MapZ);
					DeleteObj(SelectionObj);
					CON_printf("\nYou cut up some meat.\n");
					qty = OSI_rand(2, 7);
					if(AddInvObj(0x6e, INVEN, qty, actor) == -1)
						AddMapObj(0x6e, 0, qty, tx, ty, MapZ);
					break;
				case 47:
					AddMapObj(0x50, 0, 0, tx, ty, MapZ);
					AddMapObj(0xf8 | OSI_rand(0, 2) << 10, 0, 0, tx, ty, MapZ);
					DeleteObj(SelectionObj);
					CON_printf("\nYou cut up some meat.\n");
					qty = OSI_rand(2, 7);
					if(AddInvObj(0x6e, INVEN, qty, actor) == -1)
						AddMapObj(0x6e, 0, qty, tx, ty, MapZ);
					break;
				case 37:
					AddMapObj(0x452, 0, 0, tx, ty, MapZ);
					AddMapObj(0xf8 | OSI_rand(0, 2) << 10, 0, 0, tx, ty, MapZ);
					AddMapObj(0xf8 | OSI_rand(0, 2) << 10, 0x20, 0, tx - 1, ty, MapZ);
					DeleteObj(SelectionObj);
					CON_printf("\nYou skinned the big cat.\n");
					qty = OSI_rand(2, 7);
					if(AddInvObj(0x6e, INVEN, qty, actor) == -1)
						AddMapObj(0x6e, 0, qty, tx, ty, MapZ);
					if(AddInvObj(0x5c, INVEN, 0, actor) == -1)
						AddMapObj(0x5c, 0, 0, tx, ty, MapZ);
					break;
				case 23: case 24: case 25:
					CON_printf("\nClank!\n");
					break;
				case 46:
					CON_printf("\nYou make a mess.\n");
					AddMapObj(0xf8, 0, 0, tx, ty, MapZ);
					break;
			}
		} else if(targetTyp == 0xf9) {
			frm = GetFrame(SelectionObj);
			tx = GetX(SelectionObj);
			ty = GetY(SelectionObj);
			if(frm == 0xf) {
				AddMapObj(0xf8 | OSI_rand(0, 2) << 10, 0, 0, tx, ty, MapZ);
				DeleteObj(SelectionObj);
				CON_printf("\nYou remove the feathers.\n");
				qty = OSI_rand(5, 20);
				if(AddInvObj(0x30, INVEN, qty, actor) == -1)
					AddMapObj(0x30, 0, qty, tx, ty, MapZ);
			} else if(frm == 0xb) {
				CON_printf("\nYou make a mess.\n");
				AddMapObj(0xf8, 0, 0, tx, ty, MapZ);
			} else {
				CON_printf("\nYou ghoul!\n");
			}
		} else {
			CON_printf(NoEffectMsg);
		}
	} else {
		CON_printf(NothingMsg);
	}
}

/*the fire fireObj burns at (x, y): hurts the NPCs there, burns the
 objects of the types of IsFlammableType and the eggs (0xe6); 0x11 and 0xab
 spread the fire (0xf2 of quality q) to the 0x11 around; then the fire
 goes out*/
/*C_1DBD_4E58*/void C_1DBD_4E58(int16 x, int16 y, int16 fireObj, int16 q) {
	int16 found, objTyp, i, item;
	int16 objNum, j;

	for(objNum = FindLoc(x, y, MapZ); objNum >= 0; objNum = NextLoc()) {
		objTyp = GetType(objNum);
		if(objNum <= 0xff)
			TerrainDamage(objNum, TILE_FRAME(fireObj));
		if(IsFlammableType(objTyp) || objTyp == 0xe6) {
			if(objTyp == 0x11 || objTyp == 0xab) {
				DeleteObj(objNum);
				for(i = -1; i < 2; i ++) {
					for(j = -1; j < 2; j ++) {
						if(i == 0 && j == 0)
							continue;
						if((found = OBJ_FindTypeAt(0x11, x + i, y + j)) != -1)
							AddMapObj(0xf2, 0, q << 8, x + i, y + j, MapZ);
					}
				}
			} else {
				if(objTyp == 0xe6) {
					for(item = FindInv(objNum); item >= 0; item = NextInv())
						DeleteObj(item);
				}
				DeleteObj(objNum);
			}
		}
	}
	DeleteObj(fireObj);
}

/*the fire obj (quality 0) at (x, y) burns; then every fire of quality 0
 burns, then the first one of quality 1 (C_1DBD_5069)*/
/*C_1DBD_4F8E*/void C_1DBD_4F8E(int16 x, int16 y, int16 obj) {
	int16 i;

	VisibilityDirty = 1;
	DisplayTick();
	C_1DBD_4E58(x, y, obj, 1);
	for(i = 0xc00; i > 0xff; i --) {
		if(GetType(i) == 0xf2 && GetQual(i) == 0)
			C_1DBD_4E58(GetX(i), GetY(i), i, 1);
	}
	for(i = 0xc00; i > 0xff; i --) {
		if(GetType(i) == 0xf2 && GetQual(i) == 1) {
			C_1DBD_5069(GetX(i), GetY(i), i);
			break;
		}
	}
	VisibilityDirty = 1;
	DisplayTick();
}

/*the same for a fire of quality 1*/
/*C_1DBD_5069*/void C_1DBD_5069(int16 x, int16 y, int16 obj) {
	int16 i;

	VisibilityDirty = 1;
	DisplayTick();
	C_1DBD_4E58(x, y, obj, 0);
	for(i = 0xc00; i > 0xff; i --) {
		if(GetType(i) == 0xf2 && GetQual(i) == 1)
			C_1DBD_4E58(GetX(i), GetY(i), i, 0);
	}
	for(i = 0xc00; i > 0xff; i --) {
		if(GetType(i) == 0xf2 && GetQual(i) == 0) {
			C_1DBD_4F8E(GetX(i), GetY(i), i);
			break;
		}
	}
	VisibilityDirty = 1;
	DisplayTick();
}

/*use the pot of tar objNum (it holds TestObj(objNum, 0x8b) tar): tar
 cloth strips (0x86 -> 0xbf); set fire (0xf2) to a pteranodon nest
 (0xab, or the egg 0x6f in it), to a dinosaur egg, to a web (0x11) or
 to the ground; the pot is emptied (frame 0) when the tar runs out*/
/*C_1DBD_5141*/void UseTar(register int16 objNum) {
	int16 ch, targetTyp, tile, x, y, actor, empty, unused, tx, ty, done, nest, isNest, fire, tar;
	int16 n;

	empty = 0;
	unused = 0;
	done = 0;
	isNest = 0;
	actor = C_1DBD_0003(objNum);
	CON_printf("On ");
	if(SelectMode != 0) {
		DrawSlotCursor(0);
		SelectMode = 1;
	}
	ch = CON_getch();
	targetTyp = GetType(SelectionObj);
	x = GetX(actor);
	y = GetY(actor);
	tile = GetTileAtXYZ(SelectionX, SelectionY, MapZ);
	if(SelectionObj != -1 && GetCoordUse(SelectionObj) == LOCXYZ && !CLOSE_ENOUGH(1, SelectionX, SelectionY, x, y)) {
		CON_printf(OutOfRangeMsg);
	} else if(targetTyp == 0x86 && SelectionObj != -1) {
		actor = Party[StatusMember];
		CON_printf("cloth strips\n");
		tar = TestObj(objNum, 0x8b);
		n = GetQuan(SelectionObj);
		CON_printf("\nYou tarred some cloth strips.\n");
		done = 1;
		TakeObj(objNum, 0x8b, n);
		if(!OBJ_IsInside(SelectionObj, actor) || AddInvObj(0xbf, INVEN, tar > n?n:tar, actor) == -1)
			AddMapObj(0xbf, 0, tar > n?n:tar, GetX(SelectionObj), GetY(SelectionObj), MapZ);
		if(tar >= n)
			DeleteObj(SelectionObj);
		else
			SubQuan(SelectionObj, tar);
		if(tar <= n)
			empty = 1;
	} else if((targetTyp == 0xab || targetTyp == 0x6f) && SelectionObj != -1) {
		tx = GetX(SelectionObj);
		ty = GetY(SelectionObj);
		if(targetTyp == 0xab) {
			isNest = 1;
		} else if(targetTyp == 0x6f) {
			if((nest = OBJ_FindTypeAt(0xab, tx, ty)) >= 0) {
				isNest = 1;
				SelectionObj = nest;
			}
			if(!isNest) {
				CON_printf("dinosaur egg\n");
				done = 1;
				fire = AddMapObj(0xf2, 0, 0, SelectionX, SelectionY, MapZ);
				C_1DBD_4F8E(SelectionX, SelectionY, fire);
				ShowObjects();
			}
		}
		if(isNest) {
			CON_printf("pteranodon nest\n");
			fire = AddMapObj(0xf2, 0, 0, tx, ty, MapZ);
			AddMapObj(0xc4c, 0, 0, tx, ty, MapZ);
			C_1DBD_4F8E(tx, ty, fire);
			done = 1;
			if(TestObj(objNum, 0x8b) > 4)
				TakeObj(objNum, 0x8b, 1);
			else
				empty = 1;
		}
	} else if(targetTyp == 0x11 && SelectionObj != -1) {
		CON_printf("web\n");
		AddMapObj(0xf2, 0, 0x200, SelectionX, SelectionY, MapZ);
		fire = AddMapObj(0xf2, 0, 0, SelectionX, SelectionY, MapZ);
		C_1DBD_4F8E(SelectionX, SelectionY, fire);
		done = 1;
	} else {
		if(SelectionX == -1 || SelectionY == -1) {
			CON_printf(NothingMsg);
			return;
		}
		if(!CLOSE_ENOUGH(1, SelectionX, SelectionY, x, y)) {
			CON_printf(OutOfRangeMsg);
			return;
		}
		if(SelectionObj >= 0 && GetCoordUse(SelectionObj) == LOCXYZ)
			CON_printf(GetObjectString(SelectionObj));
		else
			CON_printf(GetTileString(tile));
		CON_putch('\n');
		if((tar = TestObj(objNum, 0x8b)) > 1)
			TakeObj(objNum, 0x8b, 1);
		else
			empty = 1;
		if(!(TerrainType[tile] & 1)) {
			done = 1;
			AddMapObj(0xf2, 0, 0x200, SelectionX, SelectionY, MapZ);
			fire = AddMapObj(0xf2, 0, 0, SelectionX, SelectionY, MapZ);
			C_1DBD_4F8E(SelectionX, SelectionY, fire);
			ShowObjects();
		}
	}
	if(empty) {
		TakeObj(objNum, 0x8b, TestObj(objNum, 0x8b));
		SetFrame(objNum, 0);
	} else if(!done) {
		CON_printf(NoEffectMsg);
	}
	if(StatusDisplay == 0x92)
		STAT_DrawInventory();
}

/*the active member (or the one shown) tars cloth strips (0x86 -> 0xbf)*/
/*C_1DBD_5641*/void C_1DBD_5641() {
	int16 ch, targetTyp, x, y;
	int16 actor, n;

	actor = StatusDisplay == 0x92?Party[StatusMember]:Party[Active];
	CON_printf("On ");
	if(SelectMode != 0) {
		DrawSlotCursor(0);
		SelectMode = 1;
	}
	ch = CON_getch();
	targetTyp = GetType(SelectionObj);
	x = GetX(actor);
	y = GetY(actor);
	if(GetCoordUse(SelectionObj) == LOCXYZ && !CLOSE_ENOUGH(1, SelectionX, SelectionY, x, y)) {
		CON_printf(OutOfRangeMsg);
	} else if(targetTyp == 0x86 && SelectionObj != -1) {
		n = GetQuan(SelectionObj);
		CON_printf("cloth strips\n");
		if(!OBJ_IsInside(SelectionObj, actor) || AddInvObj(0xbf, INVEN, n, actor) == -1)
			AddMapObj(0xbf, 0, n, SelectionX, SelectionY, MapZ);
		DeleteObj(SelectionObj);
	} else {
		CON_printf(NoEffectMsg);
	}
	if(StatusDisplay == 0x92)
		STAT_DrawInventory();
}

/*use the bucket of water objNum: put a fire out (0xf2, 0xfe, or the
 lit campfire 0xcf), else pour the water out; the bucket is emptied
 (frame 0)*/
/*C_1DBD_5794*/void UseBucketOfWater(register int16 objNum) {
	int16 ch, x, y, user;
	register int16 targetTyp;

	user = C_1DBD_0003(objNum);
	CON_printf("On ");
	if(SelectMode != 0) {
		DrawSlotCursor(0);
		SelectMode = 1;
	}
	ch = CON_getch();
	if(ch == 0x8e && SelectionObj != -1) {
		CON_printf(GetObjectString(SelectionObj));
		CON_putch('\n');
		targetTyp = GetType(SelectionObj);
		x = GetX(user);
		y = GetY(user);
		if(GetCoordUse(SelectionObj) == LOCXYZ && !CLOSE_ENOUGH(1, SelectionX, SelectionY, x, y)) {
			CON_printf(OutOfRangeMsg);
		} else if(targetTyp == 0xf2 || targetTyp == 0xfe) {
			TakeObj(objNum, 0x8c, TestObj(objNum, 0x8c));
			SetFrame(objNum, 0);
			CON_printf("\nYou put the fire out.\n");
			DeleteObj(SelectionObj);
		} else if(targetTyp == 0xcf && GetFrame(SelectionObj) == 2) {
			TakeObj(objNum, 0x8c, TestObj(objNum, 0x8c));
			SetFrame(objNum, 0);
			CON_printf("\nYou put the fire out.\n");
			SetFrame(SelectionObj, 0);
		} else {
			TakeObj(objNum, 0x8c, TestObj(objNum, 0x8c));
			SetFrame(objNum, 0);
			CON_printf("\nYou pour the water out.\n");
		}
		if(StatusDisplay == 0x92)
			STAT_DrawInventory();
	} else if(SelectionX == -1 || SelectionY == -1) {
		CON_printf(NothingMsg);
	} else {
		CON_printf(GetTileString(GetTileAtXYZ(SelectionX, SelectionY, MapZ)));
		CON_putch('\n');
		TakeObj(objNum, 0x8c, TestObj(objNum, 0x8c));
		SetFrame(objNum, 0);
		CON_printf("\nYou pour the water out.\n");
	}
}

/*use the bucket objNum: with tar (frame 1) as UseTar, with water (frame
 2) as UseBucketOfWater; empty, fill it with tar (from 0xe5) or water
 next to the user*/
/*C_1DBD_59D9*/void UseBucket(register int16 objNum) {
	int16 i, j, user, x, y, done;
	register int16 typ;

	done = 0;
	user = C_1DBD_0003(objNum);
	x = GetObjXOrVehicle(user);
	y = GetObjYOrVehicle(user);
	if(GetFrame(objNum) == 1) {
		UseTar(objNum);
	} else if(GetFrame(objNum) == 2) {
		UseBucketOfWater(objNum);
	} else {
		for(i = 0; i < 3; i ++) {
			for(j = 0; j < 3; j ++) {
				if(i == 1 && j == 1)
					continue;
				/*(the types of the first object, then the numbers of the
				next ones)*/
				for(typ = GetType(FindLoc(x + i - 1, y + j - 1, MapZ)); typ > -1; typ = NextLoc()) {
					if(typ == 0xe5) {
						if(AddInvObj(0x8b, CONTAINED, 10, objNum) > -1) {
							CON_printf("\nYou got some tar in the bucket.\n");
							SetFrame(objNum, 1);
							done = 1;
						}
						break;
					}
					if((TerrainType[GetTileAtXYZ(x + i - 1, y - i + j, MapZ)] & 1) ||
						typ == 0xa2 || typ == 0xc8 || typ == 0xa6 || typ == 0x96 || typ == 0x97) {
						if(AddInvObj(0x8c, CONTAINED, 10, objNum) > -1) {
							CON_printf("\nYou got some water in the bucket.\n");
							SetFrame(objNum, 2);
							done = 1;
						}
						break;
					}
				}
				if(done)
					break;
			}
			if(done)
				break;
		}
		if(!done)
			CON_printf("\nThere is nothing to get in the bucket.\n");
		if(StatusDisplay == 0x92)
			STAT_DrawInventory();
	}
}

/*use the cloth strip objNum on a stick (0xce: makes a torch, 0xd0) or
 on a fired pot (0x85: with five gunpowders, makes a grenade, 0x89)*/
/*C_1DBD_5BA7*/void UseClothStrip(register int16 objNum) {
	int16 ch, targetTyp, x, y, powder, unusedC, unusedE;
	unsigned char unusedF;
	register int16 user;

	user = C_1DBD_0003(objNum);
	CON_printf("On ");
	if(SelectMode != 0) {
		DrawSlotCursor(0);
		SelectMode = 1;
	}
	ch = CON_getch();
	if(ch == 0x8e && SelectionObj != -1) {
		CON_printf(GetObjectString(SelectionObj));
		CON_putch('\n');
		targetTyp = GetType(SelectionObj);
		x = GetX(user);
		y = GetY(user);
		if(GetCoordUse(SelectionObj) == LOCXYZ && !CLOSE_ENOUGH(1, SelectionX, SelectionY, x, y)) {
			CON_printf(OutOfRangeMsg);
		} else if(targetTyp == 0xce) {
			CON_printf("\nYou make a torch\n");
			if(GetQuan(objNum) > 1)
				SetQuan(objNum, GetQuan(objNum) - 1);
			else
				DeleteObj(objNum);
			if(OBJ_IsInside(SelectionObj, user)) {
				if(GetQuan(SelectionObj) > 1)
					SetQuan(SelectionObj, GetQuan(SelectionObj) - 1);
				else
					DeleteObj(SelectionObj);
				if(AddInvObj(0xd0, INVEN, 1, user) == -1)
					AddMapObj(0xd0, 0, 1, x, y, MapZ);
			} else {
				AddMapObj(0xd0, 0, 1, SelectionX, SelectionY, MapZ);
				if(GetQuan(SelectionObj) > 1)
					SetQuan(SelectionObj, GetQuan(SelectionObj) - 1);
				else
					DeleteObj(SelectionObj);
			}
			if(StatusDisplay == 0x92)
				STAT_DrawInventory();
		} else if(targetTyp == 0x85) {
			unusedF = 0;
			unusedC = -1;
			unusedE = 0;
			CON_putch('\n');
			if((powder = C_0A5D_080A(user, 0xcc)) < 0 || GetQuan(powder) < 5) {
				CON_printf("\nYou need five pinches of gunpowder to make a grenade.\n");
			} else {
				if(GetQuan(powder) == 5)
					DeleteObj(powder);
				else
					SetQuan(powder, GetQuan(powder) - 5);
				if(GetQuan(objNum) < 2)
					DeleteObj(objNum);
				else
					SetQuan(objNum, GetQuan(objNum) - 1);
				CON_printf("\nYou made a grenade.\n");
				if(OBJ_IsInside(SelectionObj, user)) {
					if(GetQuan(SelectionObj) > 1)
						SetQuan(SelectionObj, GetQuan(SelectionObj) - 1);
					else
						DeleteObj(SelectionObj);
					if(AddInvObj(0x89, INVEN, 1, user) == -1)
						AddMapObj(0x89, 0, 1, GetX(user), GetY(user), MapZ);
				} else {
					AddMapObj(0x89, 0, 1, SelectionX, SelectionY, MapZ);
					if(GetQuan(SelectionObj) > 1)
						SetQuan(SelectionObj, GetQuan(SelectionObj) - 1);
					else
						DeleteObj(SelectionObj);
				}
				if(StatusDisplay == 0x92)
					STAT_DrawInventory();
			}
		} else {
			CON_printf(NoEffectMsg);
		}
	} else {
		CON_printf(NothingMsg);
	}
}

/*use the sieve objNum next to sulphur (0xe7): makes sulphur powder (0x7b)*/
/*C_1DBD_5F40*/void UseSieve(int16 objNum) {
	int16 user, found, x, y, done;
	register int16 i, j;

	done = 0;
	user = C_1DBD_0003(objNum);
	x = GetX(user);
	y = GetY(user);
	for(i = 0; i < 3; i ++) {
		for(j = 0; j < 3; j ++) {
			if(i == 1 && j == 1)
				continue;
			if((found = OBJ_FindTypeAt(0xe7, x + i - 1, y + j - 1)) != -1) {
				CON_printf("\nYou made some sulphur powder.\n");
				if(AddInvObj(0x7b, INVEN, 10, user) == -1)
					AddMapObj(0x7b, 0, 10, x, y, MapZ);
				done = 1;
				break;
			}
		}
		if(done)
			break;
	}
	if(!done)
		CON_printf("\nThere is nothing to sift.\n");
	if(StatusDisplay == 0x92)
		STAT_DrawInventory();
}

/*use the bandage objNum on a wounded NPC: 1 to 5 hit points back*/
/*C_1DBD_6036*/void UseBandage(int16 objNum) {
	int16 ch, x, y, heal, unused;
	unsigned char n;
	int16 targetTyp, user;

	unused = 0;
	user = C_1DBD_0003(objNum);
	CON_printf("On ");
	if(SelectMode != 0) {
		DrawSlotCursor(0);
		SelectMode = 1;
	}
	ch = CON_getch();
	targetTyp = GetType(SelectionObj);
	x = GetX(user);
	y = GetY(user);
	if(GetCoordUse(SelectionObj) == LOCXYZ && !CLOSE_ENOUGH(1, SelectionX, SelectionY, x, y)) {
		CON_printf(OutOfRangeMsg);
	} else if((SelectionObj >= 0 && IsNPC(SelectionObj)) || targetTyp == 0xf9 || targetTyp == 0xf8) {
		n = GetQuan(objNum);
		if(targetTyp != 0xf9 && targetTyp != 0xf8) {
			CON_printf(GetObjectString(SelectionObj));
			CON_printf("\n");
			if(MaxHP(SelectionObj) > HitPoints[SelectionObj]) {
				heal = OSI_rand(1, 5);
				if(MaxHP(SelectionObj) > HitPoints[SelectionObj] + heal)
					HitPoints[SelectionObj] = HitPoints[SelectionObj] + heal;
				else
					HitPoints[SelectionObj] = MaxHP(SelectionObj);
				CON_printf("\n%s's wounds are bound.\n", GetObjectString(SelectionObj));
				if(n > 1)
					SetQuan(objNum, n - 1);
				else
					DeleteObj(objNum);
			} else {
				CON_printf("\n%s doesn't need any help.\n", GetObjectString(SelectionObj));
			}
		} else {
			CON_printf("a body.\n");
			CON_printf("\nThat person is beyond help.\n");
		}
	} else {
		CON_printf("nothing\n");
	}
	if(StatusDisplay == 0x92)
		STAT_DrawInventory();
}

/*use a tree: a branch (0xce) for the active member*/
/*C_1DBD_622F*/void UseTree() {
	int16 actor;

	actor = GetPartyObjOrVehicle(Active);
	if(AddInvObj(0xce, INVEN, 1, actor) >= 0) {
		CON_printf("\nYou pulled a branch from the tree.\n");
		if(StatusDisplay == 0x92)
			STAT_DrawInventory();
	} else {
		CON_printf(TooMuchMsg);
	}
}

/*use a bean plant: beans (0x6d)*/
/*C_1DBD_6282*/void UseBeanPlant() {
	int16 actor;

	actor = GetPartyObjOrVehicle(Active);
	if(AddInvObj(0x6d, INVEN, 1, actor) >= 0) {
		CON_printf("\nYou got some beans from the plant.\n");
		if(StatusDisplay == 0x92)
			STAT_DrawInventory();
	} else {
		CON_printf(TooMuchMsg);
	}
}

/*use a corn plant: corn (0x6c)*/
/*C_1DBD_62D5*/void UseCornPlant() {
	int16 actor;

	actor = GetPartyObjOrVehicle(Active);
	if(AddInvObj(0x6c, INVEN, 1, actor) >= 0) {
		CON_printf("\nYou got some corn from the plant.\n");
		if(StatusDisplay == 0x92)
			STAT_DrawInventory();
	} else {
		CON_printf(TooMuchMsg);
	}
}

/*use a yucca plant: flax (0xd2)*/
/*C_1DBD_6328*/void UseYuccaPlant() {
	int16 actor;

	actor = GetPartyObjOrVehicle(Active);
	if(AddInvObj(0xd2, INVEN, 1, actor) >= 0) {
		CON_printf("\nYou got some flax from the yucca plant.\n");
		if(StatusDisplay == 0x92)
			STAT_DrawInventory();
	} else {
		CON_printf(TooMuchMsg);
	}
}

/*use a skeleton: rib bones (0xd3)*/
/*C_1DBD_637B*/void UseSkeleton() {
	int16 actor;

	actor = GetPartyObjOrVehicle(Active);
	if(AddInvObj(0xd3, INVEN, 1, actor) >= 0) {
		CON_printf("\nYou got some rib bones.\n");
		if(StatusDisplay == 0x92)
			STAT_DrawInventory();
	} else {
		CON_printf(TooMuchMsg);
	}
}

/*use the loom: weave cloth (0xb4) from flax (0xd2)*/
/*C_1DBD_63CE*/void UseLoom() {
	int16 ch, targetTyp, x;
	int16 actor, y;

	actor = GetPartyObjOrVehicle(Active);
	x = GetX(actor);
	y = GetY(actor);
	CON_printf("On ");
	if(SelectMode != 0) {
		DrawSlotCursor(0);
		SelectMode = 1;
	}
	ch = CON_getch();
	if(ch == 0x8e && SelectionObj != -1) {
		CON_printf(GetObjectString(SelectionObj));
		CON_putch('\n');
		targetTyp = GetType(SelectionObj);
		if(GetCoordUse(SelectionObj) == LOCXYZ && !CLOSE_ENOUGH(1, SelectionX, SelectionY, x, y)) {
			CON_printf(OutOfRangeMsg);
		} else if(targetTyp == 0xd2) {
			CON_printf("\nYou wove cloth from the flax.\n");
			if(GetCoordUse(SelectionObj) != LOCXYZ) {
				if(AddInvObj(0xb4, INVEN, 1, actor) == -1)
					AddMapObj(0xb4, 0, 1, x, y, MapZ);
			} else {
				AddMapObj(0xb4, 0, 1, SelectionX, SelectionY, MapZ);
			}
			if(GetQuan(SelectionObj) > 1)
				SetQuan(SelectionObj, GetQuan(SelectionObj) - 1);
			else
				DeleteObj(SelectionObj);
			if(StatusDisplay == 0x92)
				STAT_DrawInventory();
		} else {
			CON_printf(NoEffectMsg);
		}
	} else {
		CON_printf(NothingMsg);
	}
}

/*use the statue's head objNum next to the headless statue (0xf7): the
 statue becomes NPC 11, who joins the party*/
/*C_1DBD_6577*/void UseStatueHead(int16 objNum) {
	int16 user, x, y, done, found;
	register int16 i, j;

	done = 0;
	user = C_1DBD_0003(objNum);
	x = GetObjXOrVehicle(user);
	y = GetObjYOrVehicle(user);
	for(i = 0; i < 3; i ++) {
		for(j = 0; j < 3; j ++) {
			if(i == 1 && j == 1)
				continue;
			if((found = OBJ_FindTypeAt(0xf7, x + i - 1, y + j - 1)) != -1) {
				DeleteObj(objNum);
				DeleteObj(found);
				MoveObj(11, x, y, MapZ);
				JoinParty(11);
				TalkFlags[11] |= 0x40;
				CON_printf("\nYou replace the statue's missing head.\n");
				done = 1;
			}
		}
		if(done)
			break;
	}
	if(!done)
		CON_printf(NoEffectMsg);
	if(StatusDisplay == 0x92)
		STAT_DrawInventory();
	else if(StatusDisplay == 0x91)
		STAT_ShowParty(0);
}

/*use the headless statue objNum: with the head (0xbe), it becomes NPC
 11, who joins the party*/
/*C_1DBD_666A*/void UseHeadlessStatue(int16 objNum) {
	int16 actor, x, unused;
	register int16 y, head;

	unused = 0;
	actor = GetPartyObjOrVehicle(Active);
	x = GetX(objNum);
	y = GetY(objNum);
	if((head = FindInvType(actor, 0xbe, -1)) != -1) {
		DeleteObj(head);
		DeleteObj(objNum);
		MoveObj(11, x, y, MapZ);
		JoinParty(11);
		TalkFlags[11] |= 0x40;
		CON_printf("\nYou replace the statue's missing head.\n");
	} else {
		CON_printf(NoEffectMsg);
	}
	if(StatusDisplay == 0x92)
		STAT_DrawInventory();
	else if(StatusDisplay == 0x91)
		STAT_ShowParty(0);
}

/*use the gem objNum next to the gem stand (0x42): put it on the stand
 (the second cut scene, the first time)*/
/*C_1DBD_6732*/void UseGem(int16 objNum) {
	int16 user, x, y, done, found;
	register int16 i, j;

	done = 0;
	user = C_1DBD_0003(objNum);
	x = GetX(user);
	y = GetY(user);
	for(i = 0; i < 3; i ++) {
		for(j = 0; j < 3; j ++) {
			if((found = OBJ_FindTypeAt(0x42, x + i - 1, y + j - 1)) != -1) {
				MoveObj(objNum, x + i - 2, y + j - 2, MapZ);
				CON_printf("\nYou place the gem on the stand.\n");
				STAT_DrawInventory();
				if(!(TalkFlags[1] & 2))
					C_1DBD_01A0();
				done = 1;
			}
		}
		if(done)
			break;
	}
	if(!done)
		CON_printf(NoEffectMsg);
	if(StatusDisplay == 0x92)
		STAT_DrawInventory();
}

/*use a container of water objNum: put a fire out, or harden lava (the
 tiles of IsLavaTile, not on level 3) into rock (0x39, the frame giving
 the tile's shape)*/
/*C_1DBD_6819*/void UseWater(int16 objNum) {
	int16 ch, x, y, user, tile;
	register int16 targetTyp, frm;

	user = C_1DBD_0003(objNum);
	x = GetX(user);
	y = GetY(user);
	CON_printf("On ");
	if(SelectMode != 0) {
		DrawSlotCursor(0);
		SelectMode = 1;
	}
	ch = CON_getch();
	if(ch == 0x8e && SelectionObj != -1) {
		CON_printf(GetObjectString(SelectionObj));
		CON_putch('\n');
		targetTyp = GetType(SelectionObj);
		if(GetCoordUse(SelectionObj) == LOCXYZ && !CLOSE_ENOUGH(1, SelectionX, SelectionY, x, y)) {
			CON_printf(OutOfRangeMsg);
		} else if(targetTyp == 0xf2 || targetTyp == 0xfe) {
			MUS_PlaySfx(0x1d);
			CON_printf("\nYou put the fire out.\n");
			DeleteObj(SelectionObj);
		} else if(targetTyp == 0xcf && GetFrame(SelectionObj) == 2) {
			MUS_PlaySfx(0x1d);
			CON_printf("\nYou put the fire out.\n");
			SetFrame(SelectionObj, 0);
		} else {
			CON_printf(NoEffectMsg);
		}
	} else if(SelectionObj == -1) {
		if(!CLOSE_ENOUGH(1, SelectionX, SelectionY, x, y)) {
			CON_printf(OutOfRangeMsg);
		} else if(IsLavaTile(tile = GetTileAtXYZ(SelectionX, SelectionY, MapZ))) {
			if(MapZ == 3) {
				CON_printf("\nThe lava is too far below you.\n");
			} else {
				MUS_PlaySfx(0x1d);
				CON_printf("Lava\n");
				CON_printf("You harden some of the lava.\n");
				switch(tile) {
					case 0xf: frm = 0; break;
					case 0x10: frm = 1; break;
					case 0x11: frm = 2; break;
					case 0x12: frm = 3; break;
					case 0x13: frm = 4; break;
					case 0x14: frm = 5; break;
					case 0x15: frm = 6; break;
					case 0x16: frm = 7; break;
					case 0x17: frm = 8; break;
					case 0x18: frm = 9; break;
					case 0x19: frm = 10; break;
					case 0x1a: frm = 11; break;
					case 0x1b: frm = 12; break;
					case 0xaa: frm = 13; break;
					case 0xab: frm = 14; break;
				}
				AddMapObj(0x39 | frm << 10, 0, 0, SelectionX, SelectionY, MapZ);
			}
		} else {
			CON_printf(NothingMsg);
		}
	}
}

/*use 0x113 objNum: switch it (frame 1 <-> 3); switched to 3 near NPC 2,
 the first time, it starts the first cut scene*/
/*C_1DBD_6AD8*/void C_1DBD_6AD8(register int16 objNum) {
	int16 dx, dy, dz;
	register int16 frm;

	frm = GetFrame(objNum);
	dx = GetX(objNum) - GetX(2);
	dy = GetY(objNum) - GetY(2);
	dz = GetZ(objNum) - GetZ(2);
	if(frm == 1) {
		SetFrame(objNum, 3);
		if(dx <= 2 && dy <= 2 && dz == 0 && !(TalkFlags[1] & 1))
			C_1DBD_002E();
	} else if(frm == 3) {
		SetFrame(objNum, 1);
	}
}

/*use the turtle bait objNum next to water: a turtle (0x12e) comes to
 one of the tiles around, if the user is in the turtles' area*/
/*C_1DBD_6BB3*/void UseTurtleBait(register int16 objNum) {
	int16 i, j, user, x, y, done, water, tx, ty;
	register int16 turtle;

	done = 0;
	water = 0;
	user = C_1DBD_0003(objNum);
	x = GetObjXOrVehicle(user);
	y = GetObjYOrVehicle(user);
	turtle = EGG_generate(0x12e, MapX, MapY, MapZ);
	NPCStatus[turtle] = (NPCStatus[turtle] & 0x9f) | 0x40;
	NPCMode[turtle] = 0;
	for(i = 0; i < 3; i ++) {
		for(j = 0; j < 3; j ++) {
			if(i == 1 && j == 1)
				continue;
			tx = x + i - 1;
			ty = y + j - 1;
			if(!water && (TerrainType[GetTileAtXYZ(tx, ty, MapZ)] & 1))
				water = 1;
			if(CanMoveTo(turtle, tx, ty)) {
				MoveObj(turtle, tx, ty, MapZ);
				NPCStatus[turtle] = (NPCStatus[turtle] & 0x9f) | 0x40;
				NPCMode[turtle] = 0;
				done = 1;
			}
			if(done)
				break;
		}
		if(done)
			break;
	}
	if(!done) {
		DeleteObj(turtle);
		if(!water) {
			CON_printf("\nYou must be near water.\n");
		} else {
			if(GetQuan(objNum) > 1)
				SetQuan(objNum, GetQuan(objNum) - 1);
			else
				DeleteObj(objNum);
			if(StatusDisplay == 0x92)
				STAT_DrawInventory();
			CON_printf("\nNothing takes the bait.\n");
		}
	} else {
		if(x < 0x335 || x > 0x3ed || y < 0xe7 || y > 0x1e7) {
			DeleteObj(turtle);
			CON_printf("\nNothing takes the bait.\n");
		}
		VIS_UpdateView();
		DisplayTick();
		if(GetQuan(objNum) > 1)
			SetQuan(objNum, GetQuan(objNum) - 1);
		else
			DeleteObj(objNum);
		if(StatusDisplay == 0x92)
			STAT_DrawInventory();
	}
}

/*the party gets on the turtle: npc moves to (x, y), becomes a rider
 (0x133) facing frame, and the map follows*/
/*C_1DBD_6D95*/void C_1DBD_6D95(int16 npc, int16 x, int16 y, int16 frame, int16 unused) {
	MoveObj(npc, x, y, MapZ);
	ObjShapeType[npc] = 0x133 | frame << 10;
	MapX = x;
	MapY = y;
	MAP_LoadAround();
	DisplayTick();
}

/*npc gets off the turtle onto the land next to it (the tiles of
 C_1DBD_0A29); the turtle (0x12e) stays*/
/*C_1DBD_6DD9*/void C_1DBD_6DD9(int16 npc) {
	int16 dirFrm, typ, x, y, dir, tx, ty, turtle, done;
	register int16 i, j;

	done = 0;
	dir = GetFrame(npc) & 6;
	typ = OrigShapeType[npc] & 0x3ff;
	dirFrm = OBJ_MakeDirFrame(typ, NPCFlag[npc] & 7);
	x = GetX(npc);
	y = GetY(npc);
	for(i = 0; i < 3; i ++) {
		for(j = 0; j < 3; j ++) {
			if(i == 1 && j == 1)
				continue;
			tx = x + i - 1;
			ty = y + j - 1;
			if(C_1DBD_0A29(GetTileAtXYZ(tx, ty, MapZ))) {
				MoveObj(npc, tx, ty, MapZ);
				MapX = tx;
				MapY = ty;
				done = 1;
				break;
			}
		}
		if(done)
			break;
	}
	if(!done) {
		CON_printf("\nOnly next to land!\n");
	} else {
		ObjShapeType[npc] = typ | dirFrm << 10;
		turtle = EGG_generate(0x12e, x, y, MapZ);
		NPCStatus[turtle] = (NPCStatus[turtle] & 0x9f) | 0x40;
		NPCMode[turtle] = 0x8f;
		MAP_LoadAround();
		DisplayTick();
	}
}

/*use the turtle objNum: the active member (or, when he already rides
 one, a party member next to it) gets on it*/
/*C_1DBD_6F30*/void UseTurtle(int16 objNum) {
	int16 x, y, frm, px, py;
	register int16 actor, i;

	actor = Party[Active];
	x = GetX(objNum);
	y = GetY(objNum);
	frm = GetFrame(objNum);
	if(GetType(actor) == 0x133) {
		for(i = 0; i < PartySize; i ++) {
			actor = Party[i];
			px = GetX(actor);
			py = GetY(actor);
			if(DELTA(x, px) <= 1 && DELTA(y, py) <= 1 && GetType(actor) != 0x133) {
				DeleteObj(objNum);
				C_1DBD_6D95(actor, x, y, frm, 1);
				return;
			}
		}
		CON_printf("\nYou're already on a turtle!\n");
	} else {
		DeleteObj(objNum);
		C_1DBD_6D95(actor, x, y, frm, 1);
	}
}

/*use the gunpowder objNum on a flintlock (0x32): load it (frame 1)*/
/*C_1DBD_703A*/void UseGunpowder(register int16 objNum) {
	int16 ch, targetTyp, x, y, frm;
	register int16 user;

	user = C_1DBD_0003(objNum);
	x = GetX(user);
	y = GetY(user);
	CON_printf("On ");
	if(SelectMode != 0) {
		DrawSlotCursor(0);
		SelectMode = 1;
	}
	ch = CON_getch();
	if(ch == 0x8e && SelectionObj != -1) {
		CON_printf(GetObjectString(SelectionObj));
		CON_putch('\n');
		targetTyp = GetType(SelectionObj);
		if(GetCoordUse(SelectionObj) == LOCXYZ && !CLOSE_ENOUGH(1, SelectionX, SelectionY, x, y)) {
			CON_printf(OutOfRangeMsg);
		} else if(targetTyp == 0x32) {
			if((frm = GetFrame(SelectionObj)) == 1) {
				CON_printf("\nFlintlock already loaded.\n");
			} else {
				if(GetQuan(objNum) > 1)
					SetQuan(objNum, GetQuan(objNum) - 1);
				else
					DeleteObj(objNum);
				SetFrame(SelectionObj, 1);
				CON_printf("\nFlintlock loaded.\n");
				if(StatusDisplay == 0x92)
					STAT_DrawInventory();
			}
		} else {
			CON_printf(NoEffectMsg);
		}
	} else {
		CON_printf(NothingMsg);
	}
}

/*read Jimmy's notes: only Jimmy (0x138) can*/
/*C_1DBD_71B8*/void ReadJimmysNotes() {
	int16 actor;

	actor = Party[StatusMember];
	if(OrigShapeType[actor] == 0x138) {
		CON_printf("\nJimmy says, \"Hey, boss, you'll never be able to figure out my ");
		CON_printf("shorthand.\"\n\n\"Why don't you just ask me what ");
		CON_printf("you want to know?\"\n");
	} else {
		CON_printf("\nYou can't read Jimmy's shorthand.\n");
	}
}

/*a flash of light at (x, y); the first flash near NPC 0x19, when he is
 on screen, makes him talk*/
/*C_1DBD_7204*/void Flash(register int16 x, register int16 y) {
	int16 unused, savedLight;

	unused = 0;
	savedLight = AmbientLight;
	VIS_UpdateView();
	ShowObject(0x108, x - MapX + 5, y - MapY + 5, 2);
	DrawMapWindow(1);
	AmbientLight = 7;
	VIS_UpdateView();
	DrawMapWindow(1);
	AmbientLight = savedLight;
	VIS_UpdateView();
	DrawMapWindow(1);
	if(!(TalkFlags[3] & 2) && !(TalkFlags[0x35] & 0x80) && C_0A5D_0613(0x19)) {
		if(x - GetX(0x19) > 1 || y - GetY(0x19) > 1 || x - GetX(0x19) < -2 || y - GetY(0x19) < -2) {
			CON_printf("\nYou hear a barely audible voice whisper, \"Closer...\".\n");
		} else {
			TalkFlags[0x19] = 0x28;
			TalkFlags[0x35] |= 0x80;
			TALK_talkTo(1, 0x19, 0);
		}
	}
}

/*use the camera objNum: a picture (its quality is the film left) and a
 flash*/
/*C_1DBD_730F*/void UseCamera(int16 objNum) {
	int16 film;
	register int16 actor;

	if(GetCoordUse(objNum) != LOCXYZ)
		actor = C_1DBD_0003(objNum);
	else
		actor = objNum;
	if((film = GetQual(objNum)) > 0) {
		SetQual(objNum, film - 1);
		CON_printf("\nYou take a picture.\n");
	} else {
		CON_printf("\nThe camera's out of film, ");
		CON_printf("but the flash still works.\n");
	}
	Flash(GetX(actor), GetY(actor));
}

/*use the flash powder objNum: a flash*/
/*C_1DBD_739B*/void UseFlashPowder(register int16 objNum) {
	register int16 user;

	user = C_1DBD_0003(objNum);
	if(GetQuan(objNum) == 1)
		DeleteObj(objNum);
	else
		SetQuan(objNum, GetQuan(objNum) - 1);
	Flash(GetX(user), GetY(user));
	if(StatusDisplay == 0x92)
		STAT_DrawInventory();
}

/*use the rope objNum on 0xa8: a rope (0x14) is put there, with its
 quality*/
/*C_1DBD_740E*/void UseRope(int16 objNum) {
	int16 ch, targetTyp, x, y;
	register int16 newObj, user;

	user = C_1DBD_0003(objNum);
	x = GetX(user);
	y = GetY(user);
	CON_printf("On ");
	if(SelectMode != 0) {
		DrawSlotCursor(0);
		SelectMode = 1;
	}
	ch = CON_getch();
	if(ch == 0x8e && SelectionObj != -1) {
		CON_printf(GetObjectString(SelectionObj));
		CON_putch('\n');
		targetTyp = GetType(SelectionObj);
		if(GetCoordUse(SelectionObj) == LOCXYZ && !CLOSE_ENOUGH(1, SelectionX, SelectionY, x, y)) {
			CON_printf(OutOfRangeMsg);
		} else if(targetTyp == 0xa8) {
			DeleteObj(objNum);
			newObj = AddMapObj(0x14, 0, GetQual(SelectionObj) << 8, GetX(SelectionObj) - 1, GetY(SelectionObj) - 1, MapZ);
			ClrLocal(newObj);
		} else {
			CON_printf("\nYou can't use a rope here!\n");
		}
	} else {
		CON_printf(NothingMsg);
	}
	if(StatusDisplay == 0x92)
		STAT_DrawInventory();
}

/*use the door objNum: open it (frame 1 or 3 -> 5 or 7: it moves one
 tile, unless something is in the way) or close it; the first opening
 of the door at (0x184, 0x235) brings NPC 4 into the party*/
/*C_1DBD_7580*/void UseDoor(register int16 objNum) {
	int16 x, y, blocked, obj, typ;
	register int16 frm;

	blocked = 0;
	frm = GetFrame(objNum);
	x = GetX(objNum);
	y = GetY(objNum);
	if(frm == 1 || frm == 3) {
		for(obj = FindLoc(x + (frm == 1), y + (frm == 3), MapZ); obj >= 0; obj = NextLoc()) {
			typ = GetType(obj);
			if(typ != 0xea && typ != 0xec && typ != 0xe2 && !(TileFlag2[TILE_FRAME(obj)] & 0x10)) {
				blocked = 1;
				break;
			}
		}
		if(!blocked) {
			MoveObj(objNum, GetX(objNum) + (frm == 1), GetY(objNum) + (frm == 3), MapZ);
			SetFrame(objNum, frm + 4);
		} else {
			CON_printf("\nThe door is blocked.\n");
		}
	} else if(frm == 5) {
		MoveObj(objNum, GetX(objNum) - 1, GetY(objNum), MapZ);
		SetFrame(objNum, 1);
		if(x == 0x184 && y == 0x235 && MapZ == 0 && (TalkFlags[4] & 0x10) == 0 && C_0A5D_0613(4)) {
			if(NPCStatus[4] & 4)
				NPCStatus[4] &= ~4;
			TalkFlags[4] |= 0x10;
			JoinParty(4);
			TALK_talkTo(Active, 4, 1);
			StatusDirty = 1;
		}
	} else if(frm == 7) {
		MoveObj(objNum, GetX(objNum), GetY(objNum) - 1, MapZ);
		SetFrame(objNum, 3);
	}
}

/*the doors of objNum open when Yunapotli (NPC 11) is in the party and
 near*/
/*C_1DBD_7800*/void C_1DBD_7800(int16 objNum) {
	int16 opened;

	if((NPCStatus[11] & 0x80) && C_0A5D_0613(11)) {
		CON_printf("\nYunapotli closes its eyes and appears to concentrate.");
		CON_printf("  There is a grinding noise froom the direction of the doors... ");
		DeleteObj(objNum);
		opened = 1;
		DisplayTick();
		CON_printf(" and they slide open!\n");
	} else {
		CON_printf("You can't budge it!\n");
	}
}

/*play the drum (with a 0x5b): with all the tribes allied, the fourth
 cut scene, once*/
/*C_1DBD_785B*/void PlayDrum() {
	int16 actor;

	actor = Party[Active];
	if(FindInvType(actor, 0x5b, -1)) {
		if(!(TalkFlags[1] & 8) && (TalkFlags[0x10] & 0x80) && (TalkFlags[0x11] & 0x80) && (TalkFlags[0x13] & 0x80) &&
			(TalkFlags[0x14] & 0x80) && (TalkFlags[0x1b] & 0x80) && (TalkFlags[0x1d] & 0x80) && (TalkFlags[0x20] & 0x80) &&
			(TalkFlags[0x22] & 0x80) && (TalkFlags[0x28] & 0x80) && (TalkFlags[0x30] & 0x80) && (TalkFlags[0x35] & 0x80))
			C_1DBD_05DC();
	} else {
		CON_printf("You have nothing with which to play the drum.\n");
	}
}

/*use objNum on 0x1a: both become a 0x01*/
/*C_1DBD_78EC*/C_1DBD_78EC(int16 objNum) {
	int16 ch, targetTyp, x, unused8, unusedA, inside;
	register int16 actor, y;

	unused8 = 0;
	unusedA = 0;
	actor = C_1DBD_0003(objNum);
	CON_printf("On ");
	if(SelectMode != 0) {
		DrawSlotCursor(0);
		SelectMode = 1;
	}
	ch = CON_getch();
	if(ch == 0x8e && SelectionObj != -1) {
		CON_printf(GetObjectString(SelectionObj));
		CON_putch('\n');
		targetTyp = GetType(SelectionObj);
		x = GetX(actor);
		y = GetY(actor);
		if(GetCoordUse(SelectionObj) == LOCXYZ && !CLOSE_ENOUGH(1, SelectionX, SelectionY, x, y)) {
			CON_printf(OutOfRangeMsg);
		} else if(targetTyp == 0x1a) {
			actor = Party[StatusMember];
			x = GetX(actor);
			y = GetY(actor);
			inside = OBJ_IsInside(SelectionObj, actor);
			if(GetQuan(SelectionObj) > 1)
				SetQuan(SelectionObj, GetQuan(SelectionObj) - 1);
			else
				DeleteObj(SelectionObj);
			if(!inside || AddInvObj(1, INVEN, 0, actor) == -1)
				AddMapObj(1, 0, 0, x, y, MapZ);
			DeleteObj(objNum);
		} else {
			CON_printf(NoEffectMsg);
		}
	} else {
		CON_printf(NothingMsg);
	}
	if(StatusDisplay == 0x92)
		STAT_DrawInventory();
}

/*use the fire objNum (a lit torch?) on something: set fire (0xf2) to a
 pteranodon nest, a dinosaur egg or a web, burn the types of
 IsFlammableType, or set off gunpowder (0xcc)*/
/*C_1DBD_7AB6*/void UseFire(int16 objNum) {
	int16 ch, tile, x, y, user, unusedC, unusedE, tx, ty, done, nest, isNest;
	register int16 targetTyp, fire;

	unusedC = 0;
	unusedE = 0;
	done = 0;
	isNest = 0;
	user = C_1DBD_0003(objNum);
	CON_printf("On ");
	if(SelectMode != 0) {
		DrawSlotCursor(0);
		SelectMode = 1;
	}
	ch = CON_getch();
	if(ch == 0x8e && SelectionObj != -1) {
		CON_printf(GetObjectString(SelectionObj));
		CON_putch('\n');
		targetTyp = GetType(SelectionObj);
		x = GetX(user);
		y = GetY(user);
		tile = GetTileAtXYZ(SelectionX, SelectionY, MapZ);
		if(GetCoordUse(SelectionObj) == LOCXYZ && !CLOSE_ENOUGH(1, SelectionX, SelectionY, x, y)) {
			CON_printf(OutOfRangeMsg);
		} else if(targetTyp == 0xab || targetTyp == 0x6f) {
			tx = GetX(SelectionObj);
			ty = GetY(SelectionObj);
			if(targetTyp == 0xab) {
				isNest = 1;
			} else if(targetTyp == 0x6f) {
				if((nest = OBJ_FindTypeAt(0xab, tx, ty)) >= 0) {
					isNest = 1;
					SelectionObj = nest;
				}
				if(!isNest) {
					CON_printf("dinosaur egg\n");
					done = 1;
					fire = AddMapObj(0xf2, 0, 0, SelectionX, SelectionY, MapZ);
					C_1DBD_4F8E(SelectionX, SelectionY, fire);
					ShowObjects();
				}
			}
			if(isNest) {
				CON_printf("pteranodon nest\n");
				fire = AddMapObj(0xf2, 0, 0, tx, ty, MapZ);
				AddMapObj(0xc4c, 0, 0, tx, ty, MapZ);
				C_1DBD_4F8E(tx, ty, fire);
			}
		} else if(targetTyp == 0x11) {
			CON_printf("web\n");
			fire = AddMapObj(0xf2, 0, 0, SelectionX, SelectionY, MapZ);
			C_1DBD_4F8E(SelectionX, SelectionY, fire);
		} else if(IsFlammableType(targetTyp)) {
			CON_printf("\nYou incinerated the %s.\n", GetObjectString(SelectionObj));
			DeleteObj(SelectionObj);
		} else if(targetTyp == 0xcc) {
			if(C_0A5D_070D(SelectionObj) >= 0)
				LoseHP(C_0A5D_070D(SelectionObj), OSI_rand(1, 30));
			C_1810_1941(SelectionX, SelectionY);
			DeleteObj(SelectionObj);
		} else {
			CON_printf(NoEffectMsg);
		}
	} else {
		CON_printf(NothingMsg);
	}
	if(StatusDisplay == 0x92)
		STAT_DrawInventory();
}

/*use the healing objNum on a wounded NPC: all his hit points back, and
 he is cured of the poison (NPCStatus 0x08)*/
/*C_1DBD_7DA2*/void UseHealing(register int16 objNum) {
	int16 ch, x, y, user, unused;
	unsigned char n;
	register int16 targetTyp;

	unused = 0;
	user = C_1DBD_0003(objNum);
	CON_printf("On ");
	if(SelectMode != 0) {
		DrawSlotCursor(0);
		SelectMode = 1;
	}
	ch = CON_getch();
	targetTyp = GetType(SelectionObj);
	x = GetX(user);
	y = GetY(user);
	if(GetCoordUse(SelectionObj) == LOCXYZ && !CLOSE_ENOUGH(1, SelectionX, SelectionY, x, y)) {
		CON_printf(OutOfRangeMsg);
	} else if(SelectionObj != -1 && (IsNPC(SelectionObj) || targetTyp == 0xf9 || targetTyp == 0xf8)) {
		n = GetQuan(objNum);
		if(targetTyp != 0xf9 && targetTyp != 0xf8) {
			CON_printf(GetObjectString(SelectionObj));
			CON_printf("\n");
			if(MaxHP(SelectionObj) > HitPoints[SelectionObj]) {
				HitPoints[SelectionObj] = MaxHP(SelectionObj);
				NPCStatus[SelectionObj] &= ~8;
				CON_printf("\n%s is healed.\n", GetObjectString(SelectionObj));
				if(n > 1)
					SetQuan(objNum, GetQuan(objNum) - 1);
				else
					DeleteObj(objNum);
			} else {
				CON_printf("\n%s Doesn't need any help.\n", GetObjectString(SelectionObj));
			}
		} else {
			CON_printf("a body.\n");
			CON_printf("\nThat person is beyond help.\n");
		}
	} else {
		CON_printf("nothing\n");
	}
	if(StatusDisplay == 0x92)
		STAT_DrawInventory();
}

/*use the machine objNum: put in one of the gems (0x48, 0x49, 0x4a)
 carried; it makes a 0xc5*/
/*C_1DBD_7F74*/void UseMachine(int16 objNum) {
	int16 x, y, actor;
	register int16 item, typ;

	x = GetX(objNum);
	y = GetY(objNum);
	actor = GetPartyObjOrVehicle(Active);
	for(item = FindInv(actor); item >= 0; item = NextInv()) {
		typ = GetType(item);
		if(typ == 0x48 || typ == 0x4a || typ == 0x49) {
			if(GetQuan(item) > 1)
				SetQuan(item, GetQuan(item) - 1);
			else
				DeleteObj(item);
			break;
		}
	}
	if(item >= 0) {
		MUS_PlaySfx(0x20);
		CON_printf("\nYou put a gem in the machine.\n");
		AddMapObj(0xc5, 0, 0, x, y, MapZ);
		if(StatusDisplay == 0x92)
			STAT_DrawInventory();
	} else {
		CON_printf("\nYou don't have any gems.\n");
	}
}

/*use the folded skin objNum on the ground: it unfolds (0xcf5) over the
 three tiles up and left of it, when they are free*/
/*C_1DBD_805E*/void UseSkin(int16 objNum) {
	int16 x, i, blocked;
	register int16 y, j;

	blocked = 0;
	x = GetX(objNum);
	y = GetY(objNum);
	for(i = 0; i < 2; i ++) {
		for(j = 0; j < 2; j ++) {
			if(FindLoc(x - 1, y - j, MapZ) != -1) {
				blocked = 1;
				break;
			}
		}
	}
	if(GetCoordUse(objNum) == LOCXYZ &&
		!(TerrainType[GetTileAtXYZ(x - 1, y, MapZ)] & 2) &&
		!(TerrainType[GetTileAtXYZ(x, y - 1, MapZ)] & 2) &&
		!(TerrainType[GetTileAtXYZ(x - 1, y - 1, MapZ)] & 2) &&
		!blocked) {
		SetType(objNum, 0xcf5);
	} else {
		CON_printf("\nThere is not enough room to unfold the skin.\n");
	}
	if(StatusDisplay == 0x92)
		STAT_DrawInventory();
}

/*use a crystal: Rafkin (NPC 7) or Johann (NPC 5), in the party, awake
 and near, identifies potassium nitrate (10 x 0x82)*/
/*C_1DBD_8184*/void UseCrystal() {
	int16 who;

	who = -1;
	if((NPCStatus[7] & 0x80) && !(NPCStatus[7] & 0x10) && !(NPCStatus[7] & 4) && C_0A5D_0613(7))
		who = 7;
	else if((NPCStatus[5] & 0x80) && !(NPCStatus[5] & 0x10) && !(NPCStatus[5] & 4) && C_0A5D_0613(5))
		who = 5;
	if(who != -1) {
		if(AddInvObj(0x82, INVEN, 10, Party[Active]) != -1) {
			if(who == 7)
				CON_printf("\nRafkin ");
			else
				CON_printf("\nJohann ");
			CON_printf("identifies some potassium nitrate, ");
			CON_printf("and you scrape a quantity from the crystal.\n");
			if(StatusDisplay == 0x92)
				STAT_DrawInventory();
		} else {
			CON_printf(TooMuchMsg);
		}
	} else {
		CON_printf("\nThere is nobody in the party who can identify which ");
		CON_printf("crystals are useful.\n");
	}
}

/*"use" command: on a map tile without object (a tree, lava, a yucca
 plant), or on an object: dispatches on its type to its handler*/
/*C_1DBD_8276*/void CmdUse() {
	int16 frm, x, y, under, underTyp, dir;
	register int16 tile, objTyp;

	CurClip->attrFlags |= 0x10;
	SelectionObj = COMBAT_getHead(SelectionObj);
	objTyp = GetType(SelectionObj);
	/*pointing at an NPC or an "ignore" tile: use what is under it*/
	if((SelectionObj <= 0xff && SelectionX != -1 && !IN_VEHICLE && objTyp != 0x12e && objTyp != 0x133) ||
		IsTileIg(TILE_FRAME(SelectionObj)))
		SelectionObj = FindNonIgnoredObjAt(SelectionObj);
	if(SelectionObj == -1 && SelectionX != -1 && SelectionY != -1) {
		/*a map tile*/
		x = GetX(Party[Active]);
		y = GetY(Party[Active]);
		if(!CLOSE_ENOUGH(1, SelectionX, SelectionY, x, y)) {
			CON_printf(OutOfRangeMsg);
			EndPagingAtPrompt = 1;
			return;
		}
		if(IsTreeTile(tile = GetTileAtXYZ(SelectionX, SelectionY, MapZ))) {
			CON_printf("Tree\n");
			UseTree();
			SubMov(Party[Active], 5);
		} else if(IsLavaTile(tile)) {
			CON_printf("Lava\n");
			C_1DBD_458F();
			SubMov(Party[Active], 5);
		} else if(tile == 0x34) {
			CON_printf("Yucca plant\n");
			UseYuccaPlant();
			SubMov(Party[Active], 5);
		} else {
			CON_printf(NothingMsg);
		}
		EndPagingAtPrompt = 1;
		return;
	}
	SelectionObj = COMBAT_getHead(SelectionObj);
	if(objTyp == 0x133) {
		CON_printf("turtle\n");
	} else {
		LastNumberPlural = 0;
		CON_printf(GetObjectString(SelectionObj));
		CON_putch('\n');
	}
	objTyp = GetType(SelectionObj);
	if(!IsUsableType(objTyp)) {
		CON_printf(NotPossibleMsg);
		return;
	}
	if(IN_VEHICLE && C_1DBD_428E(SelectionObj)) {
		CON_printf("\n");
		CON_printf(NotOnShipMsg);
		EndPagingAtPrompt = 1;
		return;
	}
	if(GetCoordUse(SelectionObj) == LOCXYZ) {
		dir = MkDirection(PointerX, PointerY);
		if(dir != -1) {
			UpdateMoveFrame(Party[Active], dir);
			VIS_UpdateView();
		}
		x = GetX(Party[Active]);
		y = GetY(Party[Active]);
		if(!CLOSE_ENOUGH(1, SelectionX, SelectionY, x, y)) {
			CON_printf(OutOfRangeMsg);
			EndPagingAtPrompt = 1;
			return;
		}
	}
	under = FindLoc(GetX(SelectionObj), GetY(SelectionObj), MapZ);
	underTyp = GetType(under);
	frm = GetFrame(SelectionObj);
	switch(objTyp) {
		case 0x5f:
			UseDrink(SelectionObj);
			break;
		case 0x3f:
			C_1DBD_78EC(SelectionObj);
			break;
		case 0xbc:
			if(frm == 1)
				SetFrame(SelectionObj, 0);
			else
				CON_printf(NotPossibleMsg);
			SearchObj(SelectionObj, SelectionX, SelectionY);
			break;
		case 0x2b:
			UseBell(SelectionObj);
			break;
		case 0xba:
			if(objTyp == 0xba && (frm & 2))
				CON_printf(NotPossibleMsg);
			else
				C_1DBD_458F(SelectionObj);
			break;
		case 0x4f: case 0x50: case 0x51: case 0x52:
			UseSkeleton();
			break;
		case 0x3e:
			UseBeanPlant();
			break;
		case 0x3d:
			UseCornPlant();
			break;
		case 0xcf:
			switch(GetFrame(SelectionObj)) {
				case 0:
					CON_printf(NotPossibleMsg);
					break;
				case 2:
					C_1DBD_458F();
					break;
				case 1:
					SetFrame(SelectionObj, 2);
					CON_printf("\nYou started a fire.\n");
					break;
			}
			break;
		case 0xea:
			UseDoor(SelectionObj);
			break;
		case 0xa1:
			C_1DBD_7800(SelectionObj);
			break;
		case 0x5d: case 0x66: case 0x67: case 0x68: case 0x69: case 0x6a: case 0x6b: case 0x6c:
		case 0x6d: case 0x6e: case 0x6f: case 0x71: case 0x72: case 0x78: case 0xc5: case 0xd5: case 0xd6:
			UseFood(SelectionObj);
			break;
		case 0x3b: case 0x3c:
			UseGrinder(SelectionObj);
			break;
		case 0xa:
			UseFlashPowder(SelectionObj);
			break;
		case 0x113:
			C_1DBD_6AD8(SelectionObj);
			break;
		case 0xc0:
			UseClay(SelectionObj);
			break;
		case 0xd8:
			UseCrystal();
			break;
		case 0x2f:
			UseTurtleBait(SelectionObj);
			break;
		case 0x83:
			UseShovel(SelectionObj);
			break;
		case 0x2a:
			UseRope(SelectionObj);
			break;
		case 0x14:
			if(SoloMember == -1)
				C_08CB_0855(SelectionObj);
			else
				CON_printf(NotInSoloMsg);
			break;
		case 0x16:
			if(SoloMember == -1 && GetFrame(SelectionObj) == 1)
				C_08CB_0855(SelectionObj);
			else if(GetFrame(SelectionObj) == 0)
				CON_printf("You need a rope dropped from above.\n");
			else
				CON_printf(NotInSoloMsg);
			break;
		case 0xd4:
			UseFishingPole(SelectionObj);
			break;
		case 0x5e:
			UseWater(SelectionObj);
			break;
		case 0xa5:
			UseLoom(SelectionObj);
			break;
		case 0xcc:
			UseGunpowder(SelectionObj);
			break;
		case 0xfc:
			UseHealing(SelectionObj);
			break;
		case 0x19: case 0x2c: case 0x34:
			UseKnife(SelectionObj);
			break;
		case 0x94:
			PlayDrum();
			break;
		case 0xe5:
			C_1DBD_5641();
			break;
		case 0x88:
			UseBucket(SelectionObj);
			break;
		case 0xbe:
			UseStatueHead(SelectionObj);
			break;
		case 0x31:
			ReadJimmysNotes();
			break;
		case 0x33:
			UseCamera(SelectionObj);
			break;
		case 0xf7:
			UseHeadlessStatue(SelectionObj);
			break;
		case 0x1d: case 0x77:
			C_1DBD_3866(SelectionObj);
			break;
		case 0x6:
			UseGem(SelectionObj);
			break;
		case 0x89: case 0xf0:
			UseExplosive(SelectionObj);
			break;
		case 0x32:
			LoadFlintlock(SelectionObj, Party[StatusMember]);
			break;
		case 0xad: case 0xf2: case 0xfe: case 0x101:
			C_1DBD_458F();
			break;
		case 0xbf:
			UseClothStrip(SelectionObj);
			break;
		case 0x86:
			UseBandage(SelectionObj);
			break;
		case 0xd0:
			UseTorch(SelectionObj);
			break;
		case 0xd1:
			UseFire(SelectionObj);
			break;
		case 0xe9: case 0xed:
			UseVehicle(SelectionObj);
			break;
		case 0x5c:
			UseSkin(SelectionObj);
			break;
		case 0x56: case 0x57: case 0x58:
			if(OBJ_IsInside(SelectionObj, Party[StatusMember]) && Party[StatusMember] != 9)
				CON_printf(CantUseMsg, Names[StatusMember]);
			else if(Party[Active] != 9 && !OBJ_IsInside(SelectionObj, Party[StatusMember]))
				CON_printf(CantUseMsg, Names[StatusMember]);
			else
				SPELL_Invoke(SelectionObj);
			break;
		case 0x12e:
			UseTurtle(SelectionObj);
			break;
		case 0x133:
			C_1DBD_6DD9(SelectionObj);
			break;
		case 0x8f:
			UseMachine(SelectionObj);
			break;
		case 0xb1:
			UseSieve(SelectionObj);
			break;
	}
	SubMov(Party[Active], 5);
	EndPagingAtPrompt = 1;
	VIS_UpdateView();
	DisplayTick();
}
