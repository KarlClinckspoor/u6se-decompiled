/* flags: -d -mm -p -c -Z -O -y */
/*
 * The Savage Empire (Worlds of Ultima) - GAME.EXE
 * segment 0E13: status, equipment and inventory panel (Ultima VI's status.c).
 *
 * Draws the status panel (top right of the screen, TextWindows[1]): the
 * party list, a character's portrait and statistics, and the equipment /
 * inventory view (8 equipment slots around the character figure, a 4x4
 * grid of carried objects, no scrolling). Also computes carried weights
 * (in tenths of stones) and fills Equipment[] (object in each slot; 1 in
 * the left hand means "taken by a two-handed weapon"), maps mouse clicks
 * on the panel to the selection, and implements "Ready" / "Unready".
 *
 * Equipment slots: 0 head, 1 neck, 2 right hand, 3 right finger, 4 chest,
 * 5 left hand, 6 left finger, 7 feet; STAT_GetEquipSlot also returns
 * 8 for a two-handed weapon and 9 for a ring.
 * Object positions (ObjStatus & 0x18): 0x00 on the map, 0x08 contained,
 * 0x10 in an inventory, 0x18 equipped.
 */
#include "se.h"

/*text window (Ultima VI's)*/
struct tClipInfo/*size:9*/ {
	unsigned char originX, originY, cols, rows;
	unsigned char curX, curY;
	unsigned char fgColor, bgColor;
	unsigned char attrFlags;
};

/*console*/
extern int16 CON_mouseOffAt(int16, int16, int16, int16);/*CON_mouseOffAt(x0, y0, x1, y1): hide the mouse if over the rectangle*/
extern CON_mouseOn(int16);/*CON_mouseOn: show the mouse again (CON_mouseOffAt's result)*/
extern DrawInvObject(int16, int16, int16);/*DrawInvObject(objNum, x, y)*/
extern DrawSlotTile(int16, int16, int16, int16);/*DrawSlotTile(tile, background tile, x, y)*/
extern CON_setClip(struct tClipInfo *);/*CON_setClip*/
extern CON_gotoxy(int16, int16);/*CON_gotoxy*/
extern cdecl CON_printf(char far *, ...);/*CON_printf*/
/*object*/
extern int16 OBJ_MakeDirFrame(int16, int16);/*the frame of an NPC type facing a direction (U6: OBJ_MakeDirFrame)*/
extern int16 FindInv(int16);/*FindInv: first object of an inventory*/
extern int16 NextInv(void);/*NextInv*/
extern int16 AddInvObj(int16, int16, int16, int16);/*add an object (type and frame, position, quantity, owner) to an inventory; -1 if no room (U6: AddInvObj)*/
extern DeleteObj(int16);/*DeleteObj*/
extern int16 OBJ_IsInside(int16, int16);/*OBJ_IsInside(objNum, container)*/
extern int16 QuanType(int16);/*QuanType: 4 quantity on 16 bits, 2 on 8 bits, else 0*/
extern char *GetObjectString(int16);/*GetObjectString*/
/*other modules*/
extern int16 MaxHP(int16);/*MaxHP*/
extern int16 IsContainerType(int16);/*IsContainerType*/
extern DrawSkyBar(void);/*draws the sky bar above the party list (U6: DrawSkyBar)*/
extern DrawPortrait(int16, int16, int16);/*draws an NPC's portrait (npcNum, x, y) (U6: DrawPortrait)*/
extern DrawPaperDoll(int16, int16, int16);/*draws the figure of the equipment view (x, y, npcNum) (U6: DrawPaperDoll)*/
extern char far BurnOutMsg[];/*"A torch burned out.\n" (far messages)*/

extern int16 LastNumberPlural;/*console state set by CON_printf (cleared here before "Ready-"; meaning unknown)*/
extern int16 StatusMember;/*StatusMember: party index shown in the portrait/equipment view*/
extern int16 StatusDisplay;/*StatusDisplay: 0x92 when the equipment view is shown*/
extern struct tClipInfo *CurClip;/*CurClip: current text window*/
extern int16 ColorRed;/*ColorRed*/
extern int16 ColorGreen;/*ColorGreen*/
extern int16 ColorBlue;/*ColorBlue*/
extern int16 ColorBlack;/*color set before drawing a portrait*/
extern int16 ColorPaper;/*ColorPaper*/
extern int16 D_24E6;/*ColorText*/
extern unsigned char Active;/*Active: party index of the active member*/
extern int16 MapX, MapY;/*MapX, MapY*/
extern unsigned char D_329C[];/*ObjStatus: 0x18 position bits; 0x80 on an NPC: out of the party's control? (blue hit points, "WILD" combat mode)*/
extern unsigned char D_4606[];/*DEXTE*/
extern unsigned char D_4EDA[];/*Party*/
extern unsigned char far *D_4EEC;/*Level*/
extern char D_5104[][14];/*Names: of the party members*/
extern uint16 D_51E4[];/*ExpPoints*/
extern unsigned char far *D_53E4;/*NPCComMode*/
extern unsigned char D_53E8[];/*STREN*/
extern int16 D_54F0[];/*Amount: quantity*/
extern unsigned char D_6CF0[];/*NPCStatus: 0x08 poisoned*/
extern unsigned char D_6DF4[];/*HitPoints*/
extern int16 far *D_6EF4;/*BaseTile: first tile of each object type*/
extern unsigned char far *D_763C;/*TypeWeight: weight of each object type, in tenths of stones*/
extern unsigned char D_7644[];/*INTEL*/
extern struct coord D_78E4[];/*ObjPos*/
extern unsigned char D_9CF6;/*PartySize*/
extern uint16 D_9CFA[];/*ObjShapeType*/
extern int16 far *D_B53E;/*OrigShapeType: shape of each NPC before any change*/
extern int16 D_B5E8, D_B5EA;/*PointerX, PointerY*/
extern int16 D_B642;/*Selection.obj*/
extern struct tClipInfo D_B644[4];/*TextWindows: [0] full screen, [1] status panel, [3] message scroll*/
extern int16 D_B66A, D_B66C;/*Selection.x, Selection.y*/
extern int16 D_C17C[];/*Link*/
extern int16 D_E628[16];/*InvenSlotObj: object shown in each inventory grid slot*/
extern unsigned char D_E648;/*InvenSize: objects carried, not equipped*/
extern int16 D_E64A;/*InvenOwner: NPC or container whose inventory is shown*/
extern unsigned char D_E64C[8];/*Party_2Handed: party member holds a two-handed weapon*/
extern int16 D_E654;/*ShowNPCInven: NPC whose inventory is shown, 0 for the party*/
extern int16 D_E656;/*NPC_2Handed*/
extern int16 D_E658[8];/*Equipment: object in each slot*/
extern int16 D_E668;/*WeightEquip*/
extern int16 D_E66A;/*WeightInven*/
extern union tHandler D_ED10;/*GrDriver: entry point of the graphic driver (ofs = function number)*/

#define StatusMember StatusMember
#define StatusDisplay StatusDisplay
#define CurClip CurClip
#define ColorRed ColorRed
#define ColorGreen ColorGreen
#define ColorBlue ColorBlue
#define ColorPaper ColorPaper
#define ColorText D_24E6
#define Active Active
#define MapX MapX
#define MapY MapY
#define ObjStatus D_329C
#define DEXTE D_4606
#define Party D_4EDA
#define Level D_4EEC
#define Names D_5104
#define ExpPoints D_51E4
#define NPCComMode D_53E4
#define STREN D_53E8
#define Amount D_54F0
#define NPCStatus D_6CF0
#define HitPoints D_6DF4
#define BaseTile D_6EF4
#define TypeWeight D_763C
#define INTEL D_7644
#define ObjPos D_78E4
#define PartySize D_9CF6
#define ObjShapeType D_9CFA
#define OrigShapeType D_B53E
#define PointerX D_B5E8
#define PointerY D_B5EA
#define SelectionObj D_B642
#define TextWindows D_B644
#define SelectionX D_B66A
#define SelectionY D_B66C
#define Link D_C17C
#define InvenSlotObj D_E628
#define InvenSize D_E648
#define InvenOwner D_E64A
#define Party_2Handed D_E64C
#define ShowNPCInven D_E654
#define NPC_2Handed D_E656
#define Equipment D_E658
#define WeightEquip D_E668
#define WeightInven D_E66A
#define GrDriver D_ED10

#define CON_mouseOffAt CON_mouseOffAt
#define CON_mouseOn CON_mouseOn
#define DrawInvObject DrawInvObject
#define DrawSlotTile DrawSlotTile
#define CON_setClip CON_setClip
#define CON_gotoxy CON_gotoxy
#define CON_printf CON_printf
#define FindInv FindInv
#define NextInv NextInv
#define DeleteObj DeleteObj
#define OBJ_IsInside OBJ_IsInside
#define QuanType QuanType
#define GetObjectString GetObjectString
#define MaxHP MaxHP
#define IsContainerType IsContainerType

#define GR_SET_COLOR(c)              (GrDriver.iii.ofs = 0x06, (*GrDriver.f)(c))
#define GR_FILL_RECT(x0,y0,x1,y1)    (GrDriver.iii.ofs = 0x12, (*GrDriver.f)(x0,y0,x1,y1))
#define GR_PUT_TILE_SCREEN(tile,x,y) (GrDriver.iii.ofs = 0x2d, (*GrDriver.f)(tile,x,y))

#define GetType(i) (ObjShapeType[i] & 0x3ff)
#define GetFrame(i) (ObjShapeType[i] >> 10)
#define GetShape(i) (GetType(i) | (GetFrame(i) << 10))
#define TILE_FRAME(i) (BaseTile[GetType(i)] + GetFrame(i))
#define GetCoordUse(i) (ObjStatus[i] & 0x18)
#define GetQuan(i) (((unsigned char *)(Amount + (i)))[0])
#define GetAssoc(i) (*(int16 *)&(ObjPos[i]))
#define IsNPC(i) ((i) < 0x100 && (i) >= 0)

#define TIL_SLOT_FRAME 0x188
#define TIL_SLOT_EMPTY 0x189

/*08A4*/unsigned char InvenScroll = 0;/*index of the first shown inventory object (multiple of 4)*/

/*combat modes (indexed by NPCComMode - 2)*/
/*08A5*/char *ComModeNames[] = {
	/*0901*/"CMND",
	/*0905*/"",
	/*0906*/"RANGE",
	/*0905*/"",
	/*090C*/"WILD",
	/*0911*/"FLEE",
	/*0916*/"CLOSE"
};

/*display party info: up to 7 members (picture, hit points, name);
  the rows are shifted up by "first"*/
/*C_0E13_000A*/STAT_ShowParty(int16 first) {
	register int16 i, objNum;
	int16 mouseState, tile, objTyp;

	mouseState = CON_mouseOffAt(183, 6, 314, 118);
	GR_SET_COLOR(ColorPaper);
	GR_FILL_RECT(183, 6, 314, 118);
	DrawSkyBar();
	CON_setClip(&TextWindows[1]);
	for(i = 0; i < PartySize && i < 7; i ++) {
		objNum = Party[i];
		/*original (not current) shape, facing south*/
		objTyp = OrigShapeType[objNum] & 0x3ff;
		tile = OBJ_MakeDirFrame(objTyp, 4) + BaseTile[objTyp];
		DrawSlotTile(tile, TIL_SLOT_EMPTY, 184, TIL2SCR(i - first) + 8);
		CON_gotoxy(13, (i - first) << 1);
		/*hit points: green if poisoned, red if low, blue if ObjStatus 0x80 (not the avatar)*/
		if(NPCStatus[objNum] & 8)
			CurClip->fgColor = ColorGreen;
		else if(HitPoints[objNum] < 10)
			CurClip->fgColor = ColorRed;
		else if((ObjStatus[objNum] & 0x80) && objNum != 1)
			CurClip->fgColor = ColorBlue;
		CON_printf(/*091C*/"%3d\n", HitPoints[objNum]);
		CurClip->fgColor = ColorText;
		CON_gotoxy(3, ((i - first) << 1) + 1);
		CON_printf(/*0921*/"%s\n", Names[i]);
	}
	CON_setClip(&TextWindows[3]);
	CON_mouseOn(mouseState);
}

/*character combat mode: (re)print the combat mode of the displayed
  party member (StatusMember) at the bottom right of the panel ("WILD"
  if ObjStatus 0x80 and not the avatar)*/
/*C_0E13_016E*/STAT_refreshComMode() {
	register int16 objNum;
	register struct tClipInfo *savedClip;
	int16 len, mouseState;

	mouseState = CON_mouseOffAt(183, 6, 314, 118);
	savedClip = CurClip;
	CON_setClip(&TextWindows[1]);
	GR_SET_COLOR(ColorPaper);
	GR_FILL_RECT(264, 104, 303, 111);
	objNum = Party[StatusMember];
	if((ObjStatus[objNum] & 0x80) && objNum != 1) {
		len = strlen(ComModeNames[4]);
		CON_gotoxy(10, 12);
		CON_printf(/*0925*/"%s", ComModeNames[4]);
	} else {
		len = strlen(ComModeNames[NPCComMode[objNum] - 2]);
		CON_gotoxy(10, 12);
		CON_printf(/*0925*/"%s", ComModeNames[NPCComMode[objNum] - 2]);
	}
	CON_setClip(savedClip);
	CON_mouseOn(mouseState);
}

/*character status view: portrait, attributes, health, level/experience
  of party member #partyIdx, and the panel buttons*/
/*C_0E13_0254*/STAT_ShowPortrait(register int16 partyIdx) {
	register int16 objNum;
	int16 mouseState;

	mouseState = CON_mouseOffAt(183, 6, 314, 118);
	GR_SET_COLOR(ColorPaper);
	GR_FILL_RECT(183, 6, 314, 118);
	CON_setClip(&TextWindows[1]);
	CON_gotoxy((17 - strlen(Names[partyIdx])) / 2, 0);
	CON_printf(/*0921*/"%s\n", Names[partyIdx]);
	objNum = Party[partyIdx];
	GR_SET_COLOR(ColorBlack);
	DrawPortrait(objNum, 185, 16);
	CON_gotoxy(10, 1); CON_printf(/*0928*/"STR:%d\n", STREN[objNum]);
	CON_gotoxy(10, 2); CON_printf(/*0930*/"DEX:%d\n", DEXTE[objNum]);
	CON_gotoxy(10, 3); CON_printf(/*0938*/"INT:%d\n", INTEL[objNum]);
	CON_gotoxy(10, 4); CON_printf(/*0940*/"HP:");
	if(NPCStatus[objNum] & 8)
		CurClip->fgColor = ColorGreen;
	else if(HitPoints[objNum] < 10)
		CurClip->fgColor = ColorRed;
	CON_printf(/*0944*/"%3d", HitPoints[objNum]);
	CON_gotoxy(10, 5);
	CurClip->fgColor = ColorText;
	CON_printf(/*0948*/"HM:%3d", MaxHP(objNum));
	CON_gotoxy(10, 6); CON_printf(/*094F*/"Lev:%2d", Level[objNum]);
	CON_gotoxy(10, 7); CON_printf(/*0957*/"Exper:");
	CON_gotoxy(10, 8); CON_printf(/*095E*/"  %4d", ExpPoints[objNum]);
	if(StatusMember > 0)
		GR_PUT_TILE_SCREEN(0x183, 184, 102);/*previous member*/
	GR_PUT_TILE_SCREEN(0x180, 200, 102);/*party*/
	GR_PUT_TILE_SCREEN(0x182, 216, 102);/*inventory*/
	if(PartySize - 1 > StatusMember)
		GR_PUT_TILE_SCREEN(0x184, 232, 102);/*next member*/
	CON_setClip(&TextWindows[3]);
	CON_mouseOn(mouseState);
}

/*object types readied in the right hand*/
/*08B3*/int16 OneHandedTypes[] = {
	0x01,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x21,0x25,0x27,0x2c,0x34,0x77,0x89,0xb1,0xd0,
	0xce,0xf0,0x15,0x8a,0xd1,0xf1
};

/*equipment slot an object goes to (by its type): 1 neck, 4 chest,
  8 two-handed weapon, 5 left hand, 2 right hand, or -1 if it can't
  be readied*/
/*C_0E13_04C3*/STAT_GetEquipSlot(int16 objNum) {
	register int16 objTyp, i;

	objTyp = GetType(objNum);
	if(objTyp == 0x54 || objTyp == 0x55 || objTyp == 0x87)
		return 1;
	if(objTyp == 7 || objTyp == 8 || objTyp == 9)
		return 4;
	if(objTyp == 0x1f || objTyp == 0x20 || objTyp == 0x22 || objTyp == 0x28 ||
		objTyp == 0x32 || objTyp == 0x5e || objTyp == 0x76 || objTyp == 0x80 ||
		objTyp == 0x83 || objTyp == 0xaf || objTyp == 0xd4)
		return 8;
	if(objTyp == 2 || objTyp == 0xd || objTyp == 0xe || objTyp == 0xf || objTyp == 0x43)
		return 5;
	for(i = 0; i < 22; i ++)
		if(OneHandedTypes[i] == objTyp)
			return 2;
	return -1;
}

/*types whose TypeWeight is for 10 units*/
/*08DF*/int16 WeightPer10Types[] = {
	0x29,0x36,0x37,0x38,0x47,0x40,0x49,0x4a,0x7b,0x81,0x82,0x86,0xbf,0x45,0x78,0x48,
	0x30
};

/*is objType one of WeightPer10Types (weight counted per 10 units)?*/
/*C_0E13_057F*/IsWeightPer10Type(int16 objType) {
	register int16 i;

	for(i = 0; i < 17; i ++) {
		if(WeightPer10Types[i] == objType)
			return 1;
	}
	return 0;
}

/*weight of an object (not of its content), in tenths of stones*/
/*C_0E13_05A6*/GetWeight(int16 objNum) {
	register int16 qty, quanType;
	int16 objTyp;

	objTyp = GetType(objNum);
	quanType = QuanType(objTyp);
	if(quanType == 4)
		/*quantity on 16 bits*/
		qty = Amount[objNum];
	else if(quanType == 2)
		/*quantity on 8 bits*/
		qty = GetQuan(objNum);
	else
		qty = 1;
	return GetTypeWeight(objTyp, qty);
}

/*weight of qty objects of type objTyp, in tenths of stones*/
/*C_0E13_05FC*/GetTypeWeight(register int16 objTyp, int16 qty) {
	register int16 weight;

	if(IsWeightPer10Type(objTyp))
		weight = (TypeWeight[objTyp] * qty) / 10;
	else
		weight = TypeWeight[objTyp] * qty;
	return weight;
}

/*total weight of the objects contained (at any depth) in objNum:
  they follow it in the Link list*/
/*C_0E13_0637*/Encumbrance(int16 objNum) {
	register int16 item, weight;

	weight = 0;
	for(
		item = Link[objNum];
		item >= 0 && GetCoordUse(item) && OBJ_IsInside(item, objNum);
		item = Link[item]
	) weight += GetWeight(item);
	return weight;
}

/*compute InvenSize, WeightInven and (for an NPC) WeightEquip of
  objNum's inventory, without touching Equipment[]*/
/*C_0E13_0672*/STAT_ComputeWeights(int16 objNum) {
	register int16 item, partyIdx;
	int16 objTyp;

	InvenSize = 0;
	WeightInven = 0;
	if(IsNPC(objNum))
		WeightEquip = 0;
	/*party index: computed but unused*/
	for(partyIdx = 0; partyIdx < 8; partyIdx ++)
		if(Party[partyIdx] == objNum)
			break;
	for(item = FindInv(objNum); item >= 0; item = NextInv()) {
		objTyp = GetType(item);
		if(GetCoordUse(item) == 0x18) {
			WeightEquip += TypeWeight[objTyp];
		} else {
			WeightInven += GetWeight(item) + Encumbrance(item);
			InvenSize ++;
		}
	}
}

/*same as STAT_ComputeWeights, and also put each equipped object of objNum
  in its Equipment[] slot (Equipment[] must have been cleared)*/
/*C_0E13_070D*/STAT_ComputeEquipment(int16 objNum) {
	register int16 item, partyIdx;
	int16 slot, objTyp;

	InvenSize = 0;
	WeightInven = 0;
	if(IsNPC(objNum))
		WeightEquip = 0;
	for(partyIdx = 0; partyIdx < 8; partyIdx ++) {
		if(Party[partyIdx] == objNum)
			break;
	}
	for(item = FindInv(objNum); item >= 0; item = NextInv()) {
		objTyp = GetType(item);
		if(GetCoordUse(item) == 0x18) {
			WeightEquip += TypeWeight[objTyp];
			slot = STAT_GetEquipSlot(item);
			if(slot == 8) {
				/*two-handed: right hand, left hand marked as taken*/
				slot = 2;
				Equipment[5] = 1;
				if(!ShowNPCInven && partyIdx < 7)
					Party_2Handed[partyIdx] = 1;
				else
					NPC_2Handed = 1;
			} else if(slot == 2 && Equipment[2]) {
				slot = 5;
			} else if(slot == 5 && Equipment[5]) {
				slot = 2;
			}
			Equipment[slot] = item;
		} else {
			WeightInven += GetWeight(item) + Encumbrance(item);
			InvenSize ++;
		}
	}
}

/*draw equipment: the 8 slots of Equipment[] (empty slot frame if
  nothing; left hand "taken" if a two-handed weapon is readied),
  shifted by (x, y)*/
/*C_0E13_080F*/STAT_DrawEquipment(int16 x, register int16 y) {
	register int16 objNum;
	int16 slot;

	for(slot = 0; slot < 8; slot ++) {
		if((objNum = Equipment[slot]) != 0) {
			switch(slot) {
				case 0: DrawSlotTile(TILE_FRAME(objNum), TIL_SLOT_EMPTY, x + 208, y + 16); break;
				case 1: DrawSlotTile(TILE_FRAME(objNum), TIL_SLOT_EMPTY, x + 184, y + 24); break;
				case 4: DrawSlotTile(TILE_FRAME(objNum), TIL_SLOT_EMPTY, x + 232, y + 24); break;
				case 2: DrawSlotTile(TILE_FRAME(objNum), TIL_SLOT_EMPTY, x + 184, y + 40); break;
				case 5:
					if((!ShowNPCInven && Party_2Handed[StatusMember]) || (ShowNPCInven && NPC_2Handed))
						GR_PUT_TILE_SCREEN(0x187, x + 232, y + 40);
					else
						DrawSlotTile(TILE_FRAME(objNum), TIL_SLOT_EMPTY, x + 232, y + 40);
				break;
				case 3: DrawSlotTile(TILE_FRAME(objNum), TIL_SLOT_EMPTY, x + 184, y + 56); break;
				case 6: DrawSlotTile(TILE_FRAME(objNum), TIL_SLOT_EMPTY, x + 232, y + 56); break;
				case 7: DrawSlotTile(TILE_FRAME(objNum), TIL_SLOT_EMPTY, x + 208, y + 64); break;
			}
		} else {
			switch(slot) {
				case 0: GR_PUT_TILE_SCREEN(TIL_SLOT_FRAME, x + 208, y + 16); break;
				case 1: GR_PUT_TILE_SCREEN(TIL_SLOT_FRAME, x + 184, y + 24); break;
				case 4: GR_PUT_TILE_SCREEN(TIL_SLOT_FRAME, x + 232, y + 24); break;
				case 2: GR_PUT_TILE_SCREEN(TIL_SLOT_FRAME, x + 184, y + 40); break;
				case 5:
					if((!ShowNPCInven && Party_2Handed[StatusMember]) || (ShowNPCInven && NPC_2Handed))
						GR_PUT_TILE_SCREEN(0x187, x + 232, y + 40);
					else
						GR_PUT_TILE_SCREEN(TIL_SLOT_FRAME, x + 232, y + 40);
				break;
				case 3: GR_PUT_TILE_SCREEN(TIL_SLOT_FRAME, x + 184, y + 56); break;
				case 6: GR_PUT_TILE_SCREEN(TIL_SLOT_FRAME, x + 232, y + 56); break;
				case 7: GR_PUT_TILE_SCREEN(TIL_SLOT_FRAME, x + 208, y + 64); break;
			}
		}
	}
}

/*tenths of stones -> displayed stones: rounded to nearest,
  but anything below 1 stone shows as 1*/
/*C_0E13_0B63*/static WeightToStones(int16 tenths) {
	register int16 rest, stones;

	stones = tenths / 10;
	rest = tenths % 10;
	if(stones)
		return stones + (rest >= 5);
	else
		return stones + (rest != 0);
}

/*draw the equipment and inventory of InvenOwner (16 objects, recorded
  in InvenSlotObj), the owner's picture and the equipped/total weights
  vs capacity*/
/*C_0E13_0BA1*/STAT_DrawInventory() {
	register int16 slot, item;
	int16 mouseState, objTyp, tile, owner;
	struct tClipInfo *savedClip;

	if(StatusDisplay != 0x92)
		return;
	mouseState = CON_mouseOffAt(183, 6, 314, 118);
	savedClip = CurClip;
	CON_setClip(&TextWindows[1]);
	GR_SET_COLOR(ColorPaper);
	GR_FILL_RECT(248, 16, 311, 95);
	STAT_DrawEquipment(0, 0);
	item = FindInv(InvenOwner);
	slot = 0;
	/*the last row became empty: scroll back one row*/
	if(InvenScroll >= 4 && InvenScroll + 12 >= InvenSize)
		InvenScroll -= 4;
	/*skip the scrolled-out objects*/
	while(InvenScroll > slot) {
		if(GetCoordUse(item) != 0x18 || GetCoordUse(item) == 8)
			slot ++;
		item = NextInv();
	}
	/*owner's picture (an NPC facing south, or the container)*/
	objTyp = GetType(InvenOwner);
	if(IsNPC(InvenOwner)) {
		objTyp = OrigShapeType[InvenOwner] & 0x3ff;
		tile = OBJ_MakeDirFrame(objTyp, 4) + BaseTile[objTyp];
		DrawSlotTile(tile, TIL_SLOT_EMPTY, 272, 16);
	} else {
		DrawSlotTile(TILE_FRAME(InvenOwner), TIL_SLOT_EMPTY, 272, 16);
	}
	/*the 4x4 grid*/
	for(slot = 0; item >= 0 && slot < 16; item = NextInv()) {
		if(GetCoordUse(item) != 0x18 || GetCoordUse(item) == 8) {
			DrawInvObject(item, 248 + TIL2SCR(slot & 3), 32 + TIL2SCR(slot >> 2));
			InvenSlotObj[slot++] = item;
		}
	}
	for( ;slot < 16; slot ++) {
		InvenSlotObj[slot] = 0;
		DrawSlotTile(TIL_SLOT_FRAME, TIL_SLOT_EMPTY, 248 + TIL2SCR(slot & 3), 32 + TIL2SCR(slot >> 2));
	}
	/*the NPC carrying it all (InvenOwner may be a container)*/
	owner = InvenOwner;
	if(!IsNPC(owner)) {
		do
			owner = GetAssoc(owner);
		while(!IsNPC(owner));
	}
	/*display equipped weight (capacity: strength)*/
	CON_gotoxy(2, 9);
	CON_printf(/*0964*/"      ");
	CON_gotoxy(2, 9);
	CON_printf(/*096B*/"%d%/%ds", WeightToStones(WeightEquip), STREN[owner]);
	/*display inventory weight (total, capacity: 2 x strength)*/
	CON_gotoxy(2, 10);
	CON_printf(/*0964*/"      ");
	CON_gotoxy(2, 10);
	CON_printf(/*096B*/"%d%/%ds", WeightToStones(WeightInven + WeightEquip), STREN[owner] << 1);
	/* */
	CON_setClip(savedClip);
	CON_mouseOn(mouseState);
}

/*recompute Equipment[] and weights of objNum and redraw the
  inventory panel if it shows objNum or objNum is a container*/
/*C_0E13_0E54*/STAT_RefreshInventory(register int16 objNum) {
	register int16 i;

	for(i = 0; i < 16; i ++)
		InvenSlotObj[i] = 0;
	if(IsNPC(objNum)) {
		for(i = 0; i < 8; i ++)
			Equipment[i] = 0;
	}
	STAT_ComputeEquipment(objNum);
	if((StatusDisplay == 0x92 && Party[StatusMember] == objNum) || IsContainerType(GetType(objNum)))
		STAT_DrawInventory();
}

/*character equipment view: of party member StatusMember, or of NPC
  ShowNPCInven if not 0, with the panel buttons*/
/*C_0E13_0EC8*/STAT_ShowInventory(register int16 partyIdx) {
	register int16 mouseState;

	InvenScroll = 0;
	mouseState = CON_mouseOffAt(183, 6, 314, 118);
	GR_SET_COLOR(ColorPaper);
	GR_FILL_RECT(183, 6, 314, 118);
	CON_setClip(&TextWindows[1]);
	if(!ShowNPCInven) {
		CON_gotoxy((17 - strlen(Names[StatusMember])) / 2, 0);
		CON_printf(/*0921*/"%s\n", Names[StatusMember]);
	}
	DrawPaperDoll(200, 32, Party[StatusMember]);
	CON_gotoxy(0, 9); CON_printf(/*0973*/"E:");
	CON_gotoxy(0, 10); CON_printf(/*0976*/"I:");
	if(!ShowNPCInven) {
		if(StatusMember > 0)
			GR_PUT_TILE_SCREEN(0x183, 184, 102);/*previous member*/
		GR_PUT_TILE_SCREEN(0x180, 200, 102);/*party*/
		GR_PUT_TILE_SCREEN(0x181, 216, 102);/*portrait*/
		if(PartySize - 1 > StatusMember)
			GR_PUT_TILE_SCREEN(0x184, 232, 102);/*next member*/
		/*combat mode button: active unless the avatar or ObjStatus 0x80*/
		if(Party[partyIdx] != 1 && !(ObjStatus[Party[partyIdx]] & 0x80))
			GR_PUT_TILE_SCREEN(0x16d, 248, 102);
		else
			GR_PUT_TILE_SCREEN(0x16c, 248, 102);
		GR_PUT_TILE_SCREEN(0x16e, 264, 102);
		GR_PUT_TILE_SCREEN(0x16f, 280, 102);
		GR_PUT_TILE_SCREEN(0x170, 296, 102);
	}
	if(!ShowNPCInven)
		InvenOwner = Party[StatusMember];
	else
		InvenOwner = ShowNPCInven;
	Party_2Handed[StatusMember] = 0;
	STAT_RefreshInventory(InvenOwner);
	STAT_refreshComMode();
	CON_setClip(&TextWindows[3]);
	CON_mouseOn(mouseState);
}

/*scroll the inventory down one row: does nothing in Savage Empire*/
/*C_0E13_10DC*/STAT_InvenScrollDown(int16 unusedArg) {
}

/*scroll the inventory up one row: does nothing in Savage Empire*/
/*C_0E13_10E3*/STAT_InvenScrollUp(int16 unusedArg) {
}

/*mouse on the inventory grid or the owner's picture?
  sets the selection (obj: object or -1, x: grid slot + 1 / 0 for a
  container owner / map position for an NPC owner); returns 1 if hit*/
/*C_0E13_10EA*/STAT_PickInvenSlot() {
	register int16 slot;

	if(
		PointerX >= 248 && PointerX < 312 &&
		PointerY >= 32 && PointerY < 96
	) {
		slot = SCR2TIL(PointerX - 248) + (SCR2TIL(PointerY - 32) << 2);
		SelectionObj = InvenSlotObj[slot];
		if(SelectionObj == 0)
			SelectionObj = -1;
		SelectionX = slot + 1;
		return 1;
	}
	if(PointerX >= 272 && PointerX < 288 && PointerY >= 16 && PointerY < 32) {
		if(!IsNPC(InvenOwner)) {
			SelectionX = 0;
		} else {
			SelectionX = MapX;
			SelectionY = MapY;
		}
		SelectionObj = InvenOwner;
		return 1;
	}
	SelectionObj = -1;
	return 0;
}

/*mouse on an equipment slot? sets selection x = slot and obj = object
  (-1 if empty or taken by a two-handed weapon); returns 1 if hit*/
/*C_0E13_119B*/STAT_PickEquipSlot() {
	if(PointerX >= 208 && PointerX < 224 && PointerY >= 16 && PointerY < 32) {
		SelectionX = 0;
		SelectionObj = Equipment[0];
	} else if(PointerX >= 184 && PointerX < 200 && PointerY >= 24 && PointerY < 72) {
		/*left column: neck, right hand, right finger*/
		SelectionX = SCR2TIL(PointerY - 24) + 1;
		SelectionObj = Equipment[SelectionX];
	} else if(PointerX >= 232 && PointerX < 248 && PointerY >= 24 && PointerY < 72) {
		/*right column: chest, left hand, left finger*/
		SelectionX = SCR2TIL(PointerY - 24) + 4;
		SelectionObj = Equipment[SelectionX];
	} else if(PointerX >= 208 && PointerX < 224 && PointerY >= 64 && PointerY < 96) {
		SelectionX = 7;
		SelectionObj = Equipment[7];
	} else {
		SelectionObj = -1;
		return 0;
	}
	if(SelectionObj == 0 || SelectionObj == 1)
		SelectionObj = -1;
	return 1;
}

/*slot where objNum is equipped, or -1*/
/*C_0E13_127D*/STAT_FindEquipSlot(int16 objNum) {
	register int16 i;

	for(i = 0; i < 8; i++) {
		if(Equipment[i] == objNum)
			return i;
	}

	return -1;
}

/*slot where an object of type objTyp is equipped, or -1*/
/*C_0E13_12A4*/STAT_FindEquipSlotByType(int16 objTyp) {
	register int16 i;

	for(i = 0; i < 8; i++) {
		if(GetType(Equipment[i]) == objTyp)
			return i;
	}

	return -1;
}

/*"Ready": equip the selected object on the displayed party member
  (modern weapons only for some members)*/
/*C_0E13_12D4*/STAT_Ready() {
	register int16 slot, objTyp;
	int16 owner, strength, npcNum;

	npcNum = Party[StatusMember];
	objTyp = GetShape(SelectionObj);
	strength = STREN[npcNum];
	LastNumberPlural = 0;
	CON_printf(/*0979*/"Ready-");
	if(objTyp == 0xd0 || objTyp == 0xd1)
		CON_printf(/*0980*/"torch\n");
	else
		CON_printf(/*0921*/"%s\n", GetObjectString(SelectionObj));
	if((slot = STAT_GetEquipSlot(SelectionObj)) == -1) {
		CON_printf(/*0987*/"\nCan't be readied!\n");
	} else if((objTyp == 0x89 || objTyp == 0x32 || objTyp == 0x28) &&
		npcNum != 7 && npcNum != 4 && npcNum != 1 && npcNum != 5
	) {
		CON_printf(/*099B*/"\n%s can't use modern weapons!\n", Names[StatusMember]);
	} else {
		if(slot == 8) {
			if(Equipment[2]) {
				slot = 2;
			} else if(Equipment[5]) {
				slot = 5;
			} else {
				slot = 2;
				Equipment[5] = 1;
				Party_2Handed[StatusMember] = 1;
			}
		} else if(slot == 2 && Equipment[2] && Equipment[5] == 0) {
			slot = 5;
		} else if(slot == 5 && Equipment[5] && Equipment[2] == 0) {
			slot = 2;
		} else if(slot == 9) {
			if(Equipment[3])
				slot = 6;
			else
				slot = 3;
		}
		if(Equipment[slot]) {
			CON_printf(/*09BA*/"\nNo place to put!\n");
		} else {
			/*the NPC carrying the object*/
			owner = SelectionObj;
			do
				owner = GetAssoc(owner);
			while(!IsNPC(owner));
			if(QuanType(objTyp)) {
				/*take one from the stack*/
				if(GetQuan(SelectionObj) > 1)
					GetQuan(SelectionObj) -= 1;
				else
					DeleteObj(SelectionObj);
				Equipment[slot] = AddInvObj(objTyp, 0x18, 1, owner);
			} else {
				DeleteObj(SelectionObj);
				Equipment[slot] = AddInvObj(objTyp, 0x18, 0, owner);
			}
			InvenSize --;
			STAT_RefreshInventory(Party[StatusMember]);
		}
	}
	CON_printf(/*09CD*/"\n%s:\n%>", Names[Active]);
}

/*can equipped objNum be removed? returns 1 if it was a lit torch (now
  burnt out and deleted), 0 otherwise (npcNum is ignored)*/
/*C_0E13_14F3*/STAT_CheckUnready(register int16 objNum, int16 npcNum) {
	register int16 objTyp;

	if(GetCoordUse(objNum) != 0x18)
		return 0;
	objTyp = GetShape(objNum);
	if(objTyp == 0xd1) {
		DeleteObj(objNum);
		CON_printf(/*091F*/"\n");
		CON_printf(BurnOutMsg);
		return 1;
	}
	return 0;
}

/*stop the effect of equipped objNum worn by npcNum: none in Savage Empire*/
/*C_0E13_155B*/STAT_StopEquipEffect(int16 objNum, int16 npcNum) {
	return 0;
}

/*"Unready": put the selected equipped object of the displayed party
  member back in the inventory*/
/*C_0E13_1564*/STAT_Unready() {
	register int16 npcNum, result;
	int16 objTyp;

	result = 0;
	if(SelectionObj == 1)
		return;
	CON_printf(/*09D5*/"Unready-%s\n", GetObjectString(SelectionObj));
	objTyp = GetShape(SelectionObj);
	npcNum = Party[StatusMember];
	result = STAT_CheckUnready(SelectionObj, npcNum);
	Equipment[STAT_FindEquipSlot(SelectionObj)] = 0;
	if(result == 0) {
		STAT_StopEquipEffect(SelectionObj, npcNum);
		if(STAT_GetEquipSlot(SelectionObj) == 8) {
			Party_2Handed[StatusMember] = 0;
			Equipment[5] = 0;
		}
		DeleteObj(SelectionObj);
		if(AddInvObj(objTyp, 0x10, 1, npcNum) == -1) {
			AddInvObj(objTyp, 0x18, 1, npcNum);
			CON_printf(/*09E1*/"There is no room left in your pack.\n");
		}
	}
	STAT_RefreshInventory(Party[StatusMember]);
	CON_printf(/*09CD*/"\n%s:\n%>", Names[Active]);
}
