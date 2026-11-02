/* flags: -d -mm -p -c -Z -O -y */
/*
 * The Savage Empire (Worlds of Ultima) - GAME.EXE
 * Rest module, from Ultima VI's rest.c.
 *
 * The "Rest" command (REST_Command) and its helpers. Resting is refused in
 * combat, with foes near (a hostile NPC within 39 tiles), when it is too
 * noisy (an NPC within 5 tiles for which C_1810_0CA4, U6 COMBAT_canSee?, holds),
 * in solo mode, aboard a ship, or on a turtle. The party rests until
 * sunrise or sunset, or for 1-9 hours. The party gathers around the
 * leader (CampOfsX/CampOfsY/CampFacing); after more than 4 hours everybody
 * (but NPC 11) eats one piece of food (EdibleTypes) and, when NextSleep
 * allows it, gets 5 hit points back. Everybody (but NPC 11) sleeps on a
 * bedroll (object 0xF6) while the time passes (20 minutes per step); a long
 * enough rest lets the members who have the experience for it go up a
 * level through a dream (REST_LevelUp). At the end, in the morning, the
 * dawn song plays (C_2701_023F).
 */
#include "se.h"

#undef abs
extern int cdecl abs(int);

/*text window (Ultima VI's)*/
struct tClipInfo/*size:9*/ {
	unsigned char originX, originY, cols, rows;
	unsigned char curX, curY;
	unsigned char fgColor, bgColor;
	unsigned char attrFlags;/*1 wrap, 2 center, 4 capitalize, 8 runes, 0x10 paginate, 0x20 scroll*/
};

/*OSI routines (asm)*/
extern cdecl C_2ABF_00A7();/*MOUSE_SERVE*/

/*other game modules*/
extern C_0268_0574();/*PaletteAnimation*/
extern C_0359_0B23(int16);/*DrawMapWindow*/
extern C_0359_0CD4();/*DisplayTick*/
extern C_0359_11B6(int16);/*PassTime(minutes)*/
extern int16 C_05D0_0004(int16, int16, int16, int16);/*CON_mouseOffAt(x0, y0, x1, y1)*/
extern C_05D0_004D(int16);/*CON_mouseOn*/
extern int16 C_05D0_22C8();/*CON_getch*/
extern C_05D0_2494(char *, int16);/*CON_gets(buffer, max length)*/
extern C_05D0_25AD(struct tClipInfo *);/*CON_setClip*/
extern C_05D0_25E4(int16, int16);/*CON_gotoxy*/
extern cdecl C_05D0_2DB9(char far *, ...);/*CON_printf*/
extern C_08CB_053A();/*MAP_LoadAround*/
extern C_09DA_0327();/*VIS_UpdateView*/
extern int16 C_0A5D_0677(int16, int16, int16);/*OBJ_FindTypeAt(type, x, y)*/
extern int16 C_0A5D_0EB6(int16, int16, int16, int16, int16, int16);/*AddMapObj(type, status, quantity, x, y, z)*/
extern C_0A5D_1506(int16);/*DeleteObj*/
extern int16 C_0A5D_1BF3(int16, int16);/*OBJ_IsInside(objNum, container)*/
extern C_0A5D_22F7(int16, int16, int16);/*TakeObj(objNum, type, quantity)*/
extern C_0A5D_3577();/*ShowObjects*/
extern C_0E13_0BA1();/*STAT_DrawInventory*/
extern char *C_0F8D_0094(int16);/*TALK_GetNpcName*/
extern C_130F_11F4(int16, int16);/*turns an NPC to a direction (U6: UpdateMoveFrame)*/
extern int16 C_130F_3AB8(int16, int16, int16);/*TryMoveTo(npc, x, y): 0 if moved*/
extern int16 C_1810_062A(int16);/*MaxHP*/
extern C_1810_0664(int16);/*puts an NPC to sleep (U6: SetTypeUnconscious?)*/
extern int16 C_1810_0CA4(int16, int16);/*(U6: COMBAT_canSee)*/
extern C_1DBD_0D8F(int16);
extern C_2701_023F();/*plays the morning song*/
extern C_276B_23CA(int16);
extern char far C_2F65_020D[];/*NotOnShipMsg*/
extern char far C_2F65_029D[];/*NotInSoloMsg*/
extern char far C_2F65_02B0[];/*NotInCombatMsg*/

extern unsigned char D_00D8;/*MouseOn*/
extern char D_0202;/*StatusDirty*/
extern int16 D_0206;/*IdleAnimTick*/
extern char D_03A4;/*SelectMode*/
extern char D_03AE;/*AnimateAtPrompt*/
extern int16 D_03B1;
extern int16 D_03C5;/*StatusMember*/
extern int16 D_03CD;/*StatusDisplay: 0x90 a member, 0x91 the party, 0x92 the equipment*/
extern struct tClipInfo *D_03EC;/*CurClip*/
extern char D_08A2;
extern char D_0A39;
extern int16 D_24D6;/*ScreenFade*/
extern int16 D_24E6;/*ColorText*/
extern int16 D_24F0;/*text color of the dream's question*/
extern unsigned char D_27E9;/*NextSleep: hours before the party can eat and heal again*/
extern unsigned char D_27EB;/*Time_H*/
extern unsigned char D_27F2;/*Active*/
extern int16 D_27F4, D_27F6, D_27F8;/*MapX, MapY, MapZ*/
extern int16 D_2810;/*sound on*/
extern char D_2860;/*InCombat*/
extern char D_2861;/*SoloMember*/
extern unsigned char D_329C[];/*ObjStatus*/
extern unsigned char D_44FA[];/*MovePts*/
extern unsigned char D_4606[];/*DEXTE*/
extern unsigned char D_4EDA[];/*Party*/
extern unsigned char far *D_4EEC;/*Level*/
extern char D_5104[][14];/*Names*/
extern uint16 D_51E4[];/*ExpPoints*/
extern unsigned char D_53E8[];/*STREN*/
extern unsigned char D_6CF0[];/*NPCStatus*/
extern unsigned char D_6DF4[];/*HitPoints*/
extern unsigned char D_7644[];/*INTEL*/
extern struct coord D_78E4[];/*ObjPos*/
extern unsigned char D_9CF6;/*PartySize*/
extern uint16 D_9CFA[];/*ObjShapeType*/
extern int16 far *D_B53E;/*OrigShapeType*/
extern struct tClipInfo D_B644[4];/*TextWindows*/
extern int16 D_B6B6;
extern int16 D_C17C[];/*Link*/
extern union tHandler D_ED10;/*GrDriver*/

/*Savage Empire's library*/
extern int16 cdecl soundCard;

#define MouseOn D_00D8
#define StatusDirty D_0202
#define IdleAnimTick D_0206
#define SelectMode D_03A4
#define AnimateAtPrompt D_03AE
#define StatusMember D_03C5
#define StatusDisplay D_03CD
#define CurClip D_03EC
#define ScreenFade D_24D6
#define ColorText D_24E6
#define NextSleep D_27E9
#define Time_H D_27EB
#define Active D_27F2
#define MapX D_27F4
#define MapY D_27F6
#define MapZ D_27F8
#define InCombat D_2860
#define SoloMember D_2861
#define ObjStatus D_329C
#define MovePts D_44FA
#define DEXTE D_4606
#define Party D_4EDA
#define Level D_4EEC
#define Names D_5104
#define ExpPoints D_51E4
#define STREN D_53E8
#define NPCStatus D_6CF0
#define HitPoints D_6DF4
#define INTEL D_7644
#define ObjPos D_78E4
#define PartySize D_9CF6
#define ObjShapeType D_9CFA
#define OrigShapeType D_B53E
#define TextWindows D_B644
#define Link D_C17C
#define GrDriver D_ED10

#define PaletteAnimation C_0268_0574
#define DrawMapWindow C_0359_0B23
#define DisplayTick C_0359_0CD4
#define PassTime C_0359_11B6
#define CON_mouseOffAt C_05D0_0004
#define CON_mouseOn C_05D0_004D
#define CON_getch C_05D0_22C8
#define CON_gets C_05D0_2494
#define CON_setClip C_05D0_25AD
#define CON_gotoxy C_05D0_25E4
#define CON_printf C_05D0_2DB9
#define MAP_LoadAround C_08CB_053A
#define VIS_UpdateView C_09DA_0327
#define OBJ_FindTypeAt C_0A5D_0677
#define AddMapObj C_0A5D_0EB6
#define DeleteObj C_0A5D_1506
#define OBJ_IsInside C_0A5D_1BF3
#define TakeObj C_0A5D_22F7
#define ShowObjects C_0A5D_3577
#define STAT_DrawInventory C_0E13_0BA1
#define TALK_GetNpcName C_0F8D_0094
#define UpdateMoveFrame C_130F_11F4
#define TryMoveTo C_130F_3AB8
#define MaxHP C_1810_062A
#define MOUSE_SERVE C_2ABF_00A7

#define GR_PUT_TILE_SCREEN(tile,x,y) (GrDriver.iii.ofs = 0x2d, (*GrDriver.f)(tile,x,y))

#define GetType(i) (ObjShapeType[i] & 0x3ff)
#define GetCoordUse(i) (ObjStatus[i] & 0x18)
#define GetX(i) ((*(int16 *)&(ObjPos[i].raw[0]))&0x3ff)
#define GetY(i) (((*(int16 *)&(ObjPos[i].raw[1]))>>2)&0x3ff)
#define GetZ(i) (ObjPos[i].raw[2] >> 4)
#define SetCoordXYZ(o, x, y, z) {      \
	(o).raw[2] = ((z) << 4);           \
	*(int16 *)&((o).raw[0]) = (x);       \
	*(int16 *)&((o).raw[1]) |= (y) << 2; \
}

/*an attribute goes up by one, up to 30 (an expression whose value is not used:
  the original loads it in AL)*/
#define RAISE(a) ((a) >= 30 ? (a) : (a)++)

/*edible items*/
/*286C*/int16 EdibleTypes[] = {
	0x3D, 0x3E, 0x5D, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C,
	0x6D, 0x6E, 0x6F, 0x71, 0x72, 0x78, 0xC5, 0xD5, 0xD6
};

/*returns 1 if objType is one of the 19 edible types of EdibleTypes*/
static /*C_29D8_0000*/IsEdible(int16 objType) {
	int16 i;

	for(i = 0; i < 19; i ++) {
		if(EdibleTypes[i] == objType)
			return 1;
	}
	return 0;
}

/*look for food in the inventory of party member objNum only;
  if found, one unit of that food type is consumed (TakeObj) and 1 is returned*/
static /*C_29D8_0027*/TryEatExclusive(int16 objNum) {
	int16 item, itemType;

	for(item = Link[objNum]; item >= 0 && GetCoordUse(item) && OBJ_IsInside(item, objNum); item = Link[item]) {
		itemType = GetType(item);
		if(IsEdible(itemType)) {
			TakeObj(objNum, itemType, 1);
			return 1;
		}
	}
	return 0;
}

/*party member #partyNum eats: first from his own pack,
  else from the first party member who has some food.
  returns 1 if something was eaten*/
static /*C_29D8_0081*/TryEat(int16 partyNum) {
	int16 i;

	if(TryEatExclusive(Party[partyNum]))
		return 1;
	for(i = 0; i < PartySize; i ++) {
		if(TryEatExclusive(Party[i]))
			return 1;
	}
	return 0;
}

/*check foes near:
  returns 1 if a hostile NPC (NPCStatus 0x20) is on this level within
  39 tiles of the party*/
/*C_29D8_00C2*/static REST_FoesNear() {
	int16 objNum;

	for(objNum = 0x10; objNum < 0x100; objNum ++) {
		if(
			ObjShapeType[objNum] != 0 &&
			(NPCStatus[objNum] & 0x20) &&
			CLOSE_ENOUGH(39, GetX(objNum), GetY(objNum), MapX, MapY) &&
			GetZ(objNum) == MapZ
		) return 1;
	}
	return 0;
}

/*check others near ("It's too noisy"):
  returns 1 if an NPC (NPCStatus without 0x10, 0x04, 0x80) is within
  5 tiles on this level and C_1810_0CA4 says yes*/
/*C_29D8_014B*/static REST_OthersNear() {
	int16 objNum;

	for(objNum = 1; objNum < 0x100; objNum ++) {
		if(
			!(NPCStatus[objNum] & 0x10) &&
			!(NPCStatus[objNum] & 0x04) &&
			!(NPCStatus[objNum] & 0x80) &&
			CLOSE_ENOUGH(5, GetX(objNum), GetY(objNum), MapX, MapY) &&
			GetZ(objNum) == MapZ &&
			C_1810_0CA4(1, objNum)
		) return 1;
	}
	return 0;
}

/*while resting, NPC npc goes up a level if it has the experience for it:
  in a dream the player chooses the bear (strength), the jaguar (dexterity)
  or the monkey (intelligence); any other answer raises the highest of the
  three. Attributes go up to 30*/
/*C_29D8_01F8*/REST_LevelUp(int16 npc) {
	char pronoun[5] = "He";
	int16 savedDisplay, savedMember, savedB6B6;
	struct tClipInfo *savedClip;
	unsigned char answer[13] = "Winona Ryder";
	uint16 expTable[9] = {0, 100, 200, 400, 800, 1600, 3200, 6400, 0x7fff};
	char *name;

	if(expTable[Level[npc]] > ExpPoints[npc])
		return;
	Level[npc] ++;
	name = TALK_GetNpcName(npc);
	if(npc == 2) {
		pronoun[2] = 'e';
		pronoun[1] = 'h';
		pronoun[0] = 'S';
	}
	D_0A39 = 1;
	AnimateAtPrompt = 0;
	C_276B_23CA(1);
	C_1DBD_0D8F(npc);
	savedDisplay = StatusDisplay;
	savedMember = StatusMember;
	savedB6B6 = D_B6B6;
	savedClip = CurClip;
	CON_setClip(&(TextWindows[2]));
	CurClip->attrFlags |= 0x20;
	CurClip->attrFlags |= 0x10;
	CON_gotoxy(0, 0);
	D_B6B6 = CurClip->rows;
	GR_PUT_TILE_SCREEN(0x175, 0x40, 0);
	GR_PUT_TILE_SCREEN(0x176, 0x50, 0);
	CON_printf("%s has a dream...\n\n", name);
	CON_printf("%s chases three animals through the jungle: a bear, a jaguar and a monkey.\n\n", pronoun);
	CON_printf("The path branches, each animal going a separate way.\n\n");
	CON_printf("Does %s follow the @bear, the @jaguar or the @monkey?\n", name);
	CurClip->fgColor = D_24F0;
	CON_printf("Your choice:");
	CON_gets(answer, 13);
	CON_printf("\n");
	switch(answer[0]) {
		case 'B': case 'b':/*bear*/
			RAISE(STREN[npc]);
			break;
		case 'J': case 'j':/*jaguar*/
			RAISE(DEXTE[npc]);
			break;
		case 'M': case 'm':/*monkey*/
			RAISE(INTEL[npc]);
			break;
		default:
			if(STREN[npc] > DEXTE[npc]) {
				if(STREN[npc] > INTEL[npc])
					RAISE(STREN[npc]);
				else
					RAISE(INTEL[npc]);
			} else {
				if(DEXTE[npc] > INTEL[npc])
					RAISE(DEXTE[npc]);
				else
					RAISE(INTEL[npc]);
			}
			break;
	}
	CurClip->fgColor = ColorText;
	D_0A39 = 0;
	CON_setClip(savedClip);
	D_03B1 = 1;
	D_B6B6 = savedB6B6;
	AnimateAtPrompt = 1;
	GR_PUT_TILE_SCREEN(0x164, 0x40, 0);
	GR_PUT_TILE_SCREEN(0x165, 0x50, 0);
	VIS_UpdateView();
	ScreenFade = 1;
	DrawMapWindow(0);
	if(StatusMember == PartySize) {
		StatusMember = 0;
		D_08A2 = 0;
		StatusDisplay = 0x91;
	} else {
		StatusDisplay = savedDisplay;
		StatusMember = savedMember;
	}
	StatusDirty ++;
}

/*places around the leader for party members #0..#7:
  CampOfsX/CampOfsY = x/y offset, CampFacing = direction the member
  faces (0=N 2=E 4=S 6=W)*/
/*28B6*/char CampOfsX[] = { 0, 0, 1,-1, 1,-1, 1,-1};
/*28BE*/char CampOfsY[] = {-1, 1, 0, 0,-1, 1, 1,-1};
/*28C6*/char CampFacing[] = { 4, 0, 6, 2, 4, 0, 6, 2};

/*"rest" command*/
/*C_29D8_050B*/REST_Command() {
	int16 i, npc;
	struct coord xyz;
	int16 ch, sleepTime;
	int16 someoneCannotMove, cannotMove, gatherTries, ate, obj;
	int16 eaterIdx, mouseState, dream, wasSelectMode;

	someoneCannotMove = 1;
	gatherTries = 20;
	obj = 0x100;
	dream = 0;
	CON_printf("Rest");
	wasSelectMode = SelectMode;
	if(InCombat) {
		CON_printf("\n");
		CON_printf(C_2F65_02B0);
		return;
	}
	if(REST_FoesNear()) {
		CON_printf("\n");
		CON_printf("Not while foes are near!\n");
		return;
	}
	if(REST_OthersNear()) {
		CON_printf("\nIt's too noisy to sleep here!\n");
		return;
	}
	if(SoloMember != -1) {
		CON_printf("\n");
		CON_printf(C_2F65_029D);
		return;
	}
	if(Active == PartySize) {
		CON_printf("\n");
		CON_printf(C_2F65_020D);
		return;
	}
	for(i = 0; i < PartySize; i ++) {
		if(GetType(Party[i]) == 0x133) {
			CON_printf("-Not on a turtle!\n");
			return;
		}
	}

	CON_printf("\nRest until ");
	if(Time_H < 7 || Time_H > 17) {
		CON_printf("sunrise? ");
		sleepTime = (Time_H < 7)?(7 - Time_H):(24 - Time_H + 7);
	} else {
		CON_printf("sunset? ");
		sleepTime = 18 - Time_H;
	}
	ch = CON_getch();
	if(ch == 'N' || ch == 'n') {
		sleepTime = 0;
		CON_printf("N\nHow many hours? ");
		ch = CON_getch();
		if(ch < '1' || ch > '9') {
			CON_printf("0\n");
			return;
		}
		CON_printf("%c\n", ch);
		sleepTime = ch - '0';
	} else {
		CON_printf("Y\n");
	}

	mouseState = CON_mouseOffAt(0, 0, 311, 191);
	SetCoordXYZ(xyz, MapX, MapY, MapZ);

	/*gather around the leader (at most 20 steps)*/
	while(someoneCannotMove && gatherTries) {
		someoneCannotMove = 0;
		gatherTries --;
		for(i = 0; i < PartySize; i ++) {
			npc = Party[i];
			cannotMove = TryMoveTo(npc, MapX + CampOfsX[i], MapY + CampOfsY[i]);
			if(!cannotMove)
				UpdateMoveFrame(npc, CampFacing[i]);
			if(cannotMove)
				someoneCannotMove = 1;
		}
		ShowObjects();
		DisplayTick();
	}

	/*more than 4 hours: a meal; with NextSleep 0 it heals 5 hit points*/
	if(sleepTime > 4) {
		ate = 0;
		for(eaterIdx = 0; eaterIdx < PartySize; eaterIdx ++) {
			CON_printf(Names[eaterIdx]);
			if(Party[eaterIdx] == 11) {
				CON_printf(" does not eat.\n");
			} else if(TryEat(eaterIdx)) {
				CON_printf(" has food.\n");
				if(!NextSleep) {
					npc = Party[eaterIdx];
					if(MaxHP(npc) >= HitPoints[npc] + 5) {
						HitPoints[npc] += 5;
						ate = 1;
						if(StatusDisplay == 0x91 || (StatusDisplay == 0x90 && Party[StatusMember] == npc))
							StatusDirty ++;
					}
				}
			} else {
				CON_printf(" has no food.\n");
			}
		}
		if(ate)
			NextSleep = 12;
	}

	if(StatusDisplay == 0x92)
		STAT_DrawInventory();
	GR_PUT_TILE_SCREEN(0x164, 0x40, 0);
	GR_PUT_TILE_SCREEN(0x165, 0x50, 0);

	/*everybody (but NPC 11) sleeps on a bedroll*/
	for(i = 0; i < PartySize; i ++) {
		if(Party[i] != 11) {
			C_1810_0664(Party[i]);
			obj = AddMapObj(0xf6, 0x20, 0, GetX(Party[i]), GetY(Party[i]), GetZ(Party[i]));
			ObjShapeType[obj] = GetType(obj) + 0x400;
		}
	}
	ShowObjects();

	/*20 minutes per step; while dreaming, member #i may go up a level*/
	dream = sleepTime * 3 > PartySize;
	for(i = 0; i < sleepTime * 3; i ++) {
		PassTime(20);
		ShowObjects();
		IdleAnimTick = 1;
		DisplayTick();
		PaletteAnimation();
		if(MouseOn)
			MOUSE_SERVE();
		if(dream && i < PartySize)
			REST_LevelUp(Party[i]);
	}

	/*wake up: shapes back, bedrolls removed, movement points reset*/
	for(i = 0; i < PartySize; i ++) {
		ObjShapeType[Party[i]] = OrigShapeType[Party[i]];
		obj = OBJ_FindTypeAt(0xf6, GetX(Party[i]), GetY(Party[i]));
		if(obj != -1)
			DeleteObj(obj);
		MovePts[Party[i]] = DEXTE[Party[i]];
	}
	GR_PUT_TILE_SCREEN(0x173, 0x40, 0);
	GR_PUT_TILE_SCREEN(0x174, 0x50, 0);

	MapX = GetX(Party[Active]);
	MapY = GetY(Party[Active]);
	ShowObjects();
	MAP_LoadAround();
	DisplayTick();
	CON_mouseOn(mouseState);
	if(D_2810 && soundCard != 3)
		C_2701_023F();
}
