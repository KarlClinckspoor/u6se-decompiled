/* flags: -d -mm -p -c -Z -O -y */
/*
 * The Savage Empire (Worlds of Ultima) - GAME.EXE
 * segment 11FF: magic (what is left of Ultima VI's spell.c).
 *
 * Savage Empire has no spellbook: the shaman (Triolo's messages) casts
 * spells when an object is "invoked" with an offering (SPELL_Invoke): the
 * invoked object (type 0x56 Heluzz, 0x58 Aphazz, 0x57 Motazz, in combat
 * only) gives the circle (0x00, 0x10, 0x20) and the offering (type 0x37,
 * 0x36 or 0x38) the spell in it. SPELL_Execute does the spell: light,
 * peer at the map, sense danger, call help, or a spell on the creatures
 * around the party (SPELL_CastOnNearby: heal, charm, protect, curse,
 * make the party "wild").
 * Also holds U6's SPELL_CountCarried, OBJ_FindInContainer and
 * SPELL_ReviveNPC (used by map.c and talkdr.c).
 */
#include "se.h"

#undef abs

/*the graphic driver: D_ED10's offset selects the entry*/
#define GR_SET_COLOR(c)           (D_ED10.iii.ofs = 0x06, (*D_ED10.f)(c))
#define GR_FILL_RECT(x0,y0,x1,y1) (D_ED10.iii.ofs = 0x12, (*D_ED10.f)(x0,y0,x1,y1))

extern char far C_2F65_00E4[];/*NothingMsg*/
extern char far C_2F65_01FE[];/*OutOfRangeMsg*/
extern char far C_2F65_034F[];/*NoEffectMsg*/
extern char far C_2F65_036A[];/*SuccessMsg*/
extern char far C_2F65_03A7[];/*FailedMsg*/

extern char D_0202;/*StatusDirty*/
extern unsigned char D_03A4;/*SelectMode*/
extern int16 D_03C5;/*StatusMember*/
extern int16 D_03CD;/*StatusDisplay*/
extern int16 D_24D6;/*ScreenFade*/
extern int16 D_24E4;/*ColorPaper*/
extern unsigned char D_27F2;/*Active*/
extern int16 D_27F4, D_27F6, D_27F8;/*MapX, MapY, MapZ*/
extern unsigned char D_27FA[2];/*SpellFx: light*/
extern char D_2860;/*InCombat*/
extern unsigned char D_329C[];/*ObjStatus: 0x04 charmed, 0x40 cursed, 0x80 wild (party)*/
extern unsigned char far *D_4602;/*NPCFlag: 0x60 alignment before a charm*/
extern unsigned char far *D_4706;/*NPCMode*/
extern unsigned char D_4EDA[];/*Party*/
extern int16 D_54F0[];/*Amount: quantity (low byte), quality (high byte)*/
extern unsigned char D_6CF0[];/*NPCStatus: 0x01 protected, 0x08 poisoned, 0x10 dead, 0x60 alignment, 0x80 party*/
extern unsigned char D_6DF4[];/*HitPoints*/
extern int16 far *D_6EF4;/*BaseTile*/
extern struct coord D_78E4[];/*ObjPos*/
extern uint16 D_9CFA[];/*ObjShapeType*/
extern int16 far *D_B53E;/*OrigShapeType*/
extern int16 D_B642;/*Selection.obj*/
extern int16 D_B66A, D_B66C;/*Selection.x, Selection.y*/
extern int16 D_C17C[];/*Link*/
extern int16 D_EADE;/*the creature called by "help" (none while 0)*/
extern int16 D_EAE2[0x40];/*SPELL_ListNearbyNPCs's list*/
extern union tHandler D_ED10;/*GrDriver*/

extern C_0359_0CD4();/*DisplayTick*/
extern C_0359_11B6(int16);/*PassTime*/
extern C_0359_1806();/*RefreshStatus*/
extern int16 C_05D0_0004(int16, int16, int16, int16);/*CON_mouseOffAt*/
extern C_05D0_004D(int16);/*CON_mouseOn*/
extern C_05D0_1834(int16);/*DrawSlotCursor*/
extern int16 C_05D0_22C8();/*CON_getch*/
extern C_05D0_2B58(int16);/*CON_putch*/
extern cdecl C_05D0_2DB9(char far *, ...);/*CON_printf*/
extern C_09DA_0327();/*VIS_UpdateView*/
extern int16 C_0A5D_039A();/*NextLoc*/
extern int16 C_0A5D_05C4(int16, int16, int16);/*FindLoc(x, y, z)*/
extern int16 C_0A5D_0DEF(int16);/*multi-tile type?*/
extern C_0A5D_1506(int16);/*DeleteObj*/
extern int16 C_0A5D_1B36(int16);/*OBJ_ReviveFromBody*/
extern int16 C_0A5D_1BF3(int16, int16);/*OBJ_IsInside(objNum, container)*/
extern C_0A5D_1C76(int16, int16, int16, int16);/*MoveObj*/
extern C_0A5D_339C(int16, int16, int16, int16);/*ShowObject(tile, x, y, mode)*/
extern C_0A5D_3577();/*ShowObjects*/
extern char *C_0A5D_3A76(int16);/*GetObjectString*/
extern C_0E13_0BA1();/*STAT_DrawInventory*/
extern C_0E13_0EC8(int16);/*STAT_ShowInventory*/
extern int16 C_0F8D_0290(int16);/*JoinParty*/
extern int16 C_130F_0136(int16);
extern int16 C_130F_0265(int16, int16, int16);/*can objNum move to (x, y)?*/
extern int16 C_1810_0562(int16, int16);/*COMBAT_ResistsSpell?*/
extern int16 C_1810_062A(int16);/*MaxHP*/
extern int16 C_1DBD_092C(int16);/*GetPartyObjOrVehicle*/
extern int16 C_2667_015D(int16, int16, int16, int16);/*EGG_generate*/
extern C_2701_0648(int16);/*MUS_PlaySfx*/
extern C_276B_012F(int16, int16);/*MagicalHalo*/
extern C_276B_1953(int16, int16, int16);/*DrawPortrait*/
extern C_276B_1BAF();
extern C_276B_2008();/*PeerMap*/
extern int16 C_2B57_001F(int16, int16);/*OSI_rand(lo, hi)*/

#define GetType(i) (D_9CFA[i] & 0x3ff)
#define GetFrame(i) (D_9CFA[i] >> 10)
#define TILE_FRAME(i) (D_6EF4[GetType(i)] + GetFrame(i))
#define GetCoordUse(i) (D_329C[i] & 0x18)
#define GetQuan(i) (((unsigned char *)(D_54F0 + (i)))[0])
#define GetQual(i) (((unsigned char *)(D_54F0 + (i)))[1])
#define GetX(i) ((*(int16 *)&(D_78E4[i].raw[0]))&0x3ff)
#define GetY(i) (((*(int16 *)&(D_78E4[i].raw[1]))>>2)&0x3ff)
#define GetZ(i) (D_78E4[i].raw[2] >> 4)

/*0B3C*/int16 Unused0B3C[] = {0x38, 0x36, 0x37};/*the offering types?*/
/*0B42*/char *AlignmentNames[] = {"neutral", "evil", "good", "chaotic"};/*not used*/

/*total quantity of objects of type objTyp and quality objQual carried
  by objNum (walks the Link chain after objNum until an object lying on
  the map is reached, so nested containers are counted too)*/
/*C_11FF_0004*/SPELL_CountCarried(int16 objNum, int16 objTyp, int16 objQual) {
	int16 item, quantity;

	quantity = 0;
	for(item = D_C17C[objNum]; item >= 0 && GetCoordUse(item); item = D_C17C[item]) {
		if(GetType(item) == objTyp && GetQual(item) == objQual)
			quantity += GetQuan(item);
	}
	return quantity;
}

/*returns the first object of type objType (and quality objQual, 0 = any)
  inside objNum (inventory or container), -1 if none*/
/*C_11FF_0056*/OBJ_FindInContainer(int16 objNum, int16 objType, int16 objQual) {
	int16 item;
	for(
		item = D_C17C[objNum];
		item >= 0 && GetCoordUse(item) && C_0A5D_1BF3(item, objNum);
		item = D_C17C[item]
	) {
		if(GetType(item) == objType) {
			if(objQual == 0 || GetQual(item) == objQual)
				return item;
		}
	}
	return -1;
}

/*bring the NPC whose dead body is corpse back to life with full hit
  points, not poisoned, good. A body of quality >= 0x10 (not a real NPC)
  is only deleted. The NPC is placed on the first free tile in growing
  squares around the body. Party members rejoin the party.
  (also used by map.c and talkdr.c)*/
/*C_11FF_00B0*/SPELL_ReviveNPC(int16 corpse) {
	int16 npc, radius;
	int16 placed, step, new_x, new_y;

	placed = 0;
	if(corpse < 0)
		return -1;
	if(GetQual(corpse) >= 0x10)
		return C_0A5D_1506(corpse);
	npc = C_0A5D_1B36(corpse);
	D_6CF0[npc] &= ~0x10;
	D_6DF4[npc] = C_1810_062A(npc);
	D_6CF0[npc] &= ~0x08;
	D_9CFA[npc] = D_B53E[npc];
	D_6CF0[npc] = (D_6CF0[npc] & 0x9f) | 0x40;
	for(radius = 1; radius < 5; radius ++) {
		for(step = 0; step < (radius << 1) + 1; step ++) {
			new_x = GetX(npc) - radius + step;
			new_y = GetY(npc) - radius;
			if(C_130F_0265(npc, new_x, new_y)) {
				C_0A5D_1C76(npc, new_x, new_y, D_27F8);
				placed = 1;
				break;
			}
			new_x = GetX(npc) - radius;
			new_y = GetY(npc) - radius + step;
			if(C_130F_0265(npc, new_x, new_y)) {
				C_0A5D_1C76(npc, new_x, new_y, D_27F8);
				placed = 1;
				break;
			}
			new_x = GetX(npc) + radius - step;
			new_y = GetY(npc) + radius;
			if(C_130F_0265(npc, new_x, new_y)) {
				C_0A5D_1C76(npc, new_x, new_y, D_27F8);
				placed = 1;
				break;
			}
			new_x = GetX(npc) + radius;
			new_y = GetY(npc) + radius - step;
			if(C_130F_0265(npc, new_x, new_y)) {
				C_0A5D_1C76(npc, new_x, new_y, D_27F8);
				placed = 1;
				break;
			}
		}
		if(placed)
			break;
	}
	if(!placed)
		C_0A5D_1C76(npc, GetX(npc), GetY(npc), D_27F8);
	if(D_6CF0[npc] & 0x80)
		C_0F8D_0290(npc);
	return npc;
}

/*"curse": shows tile 0x104 over each creature of the list (ended by -1)*/
/*C_11FF_02BF*/SPELL_ShowCurse(int16 *list) {
	int16 x, y;

	for(; *list > -1; list ++) {
		x = (GetX(*list) - D_27F4 + 5) & 0x3ff;
		y = (GetY(*list) - D_27F6 + 5) & 0x3ff;
		if(x >= 0 && x < 11 && y >= 0 && y < 11)
			C_0A5D_339C(0x104, x, y, 1);
	}
	D_24D6 = 1;
	C_0359_0CD4();
	C_0A5D_3577();
	D_24D6 = 1;
	C_0359_0CD4();
}

/*"charm": the creatures of the list (ended by -1) take the alignment of
  caster (theirs is kept in NPCFlag) and attack (NPCMode 8); tiles 0x102
  then 0x103 are shown over them*/
/*C_11FF_0358*/SPELL_Charm(int16 caster, int16 *list) {
	int16 x, y;
	int16 *p;

	for(p = list; *p > -1; p ++) {
		x = GetX(*p) - D_27F4 + 5;
		y = GetY(*p) - D_27F6 + 5;
		C_0A5D_339C(0x102, x, y, 1);
	}
	D_24D6 = 1;
	C_0359_0CD4();
	C_0A5D_3577();
	D_24D6 = 1;
	C_0359_0CD4();
	for(; *list > -1; list ++) {
		D_4602[*list] = (D_4602[*list] & 0x9f) | (D_6CF0[*list] & 0x60);
		D_329C[*list] |= 4;
		D_6CF0[*list] = (D_6CF0[*list] & 0x9f) | (D_6CF0[caster] & 0x60);
		D_4706[*list] = 8;
		x = (GetX(*list) - D_27F4 + 5) & 0x3ff;
		y = (GetY(*list) - D_27F6 + 5) & 0x3ff;
		C_0A5D_339C(0x103, x, y, 1);
	}
	D_24D6 = 1;
	C_0359_0CD4();
	C_0A5D_3577();
	D_24D6 = 1;
	C_0359_0CD4();
}

/*"heal": the creatures of the list (ended by -1) get 1-10 hit points
  (up to their maximum) and are cured of poison*/
/*C_11FF_04AC*/SPELL_Heal(int16 *list) {
	int16 points, x;
	int16 y;

	for(; *list > -1; list ++) {
		points = C_2B57_001F(1, 10);
		D_6DF4[*list] = (C_1810_062A(*list) > D_6DF4[*list] + points)?D_6DF4[*list] + points:C_1810_062A(*list);
		if(D_6CF0[*list] & 8)
			D_6CF0[*list] &= ~8;
		x = (GetX(*list) - D_27F4 + 5) & 0x3ff;
		y = (GetY(*list) - D_27F6 + 5) & 0x3ff;
		C_0A5D_339C(TILE_FRAME(*list) + 0x800, x, y, 2);
	}
	D_24D6 = 1;
	C_0359_0CD4();
	C_0A5D_3577();
	D_24D6 = 1;
	C_0359_0CD4();
}

/*"help": a creature (type 0x12C, or 0x14B unless npc is of type 0x133)
  appears near the party with npc's alignment; only one at a time*/
/*C_11FF_05AE*/SPELL_Help(int16 npc) {
	int16 objType, dx, dy, alignment;
	int16 objNum, i;

	i = C_2B57_001F(0, 15);
	C_2701_0648(10);
	if(GetType(npc) != 0x133)
		objType = 0x14b;
	else
		objType = 0x12c;
	if(!D_EADE) {
		objNum = C_2667_015D(objType, D_27F4, D_27F6, D_27F8);
		for(i = 0; i < 8; i ++) {
			dx = C_2B57_001F(0, 10);
			dy = C_2B57_001F(0, 10);
			if(C_130F_0265(objNum, D_27F4 + dx - 5, D_27F6 + dy - 5)) {
				C_0A5D_1C76(objNum, D_27F4 + dx - 5, D_27F6 + dy - 5, D_27F8);
				alignment = D_6CF0[npc] & 0x60;
				C_05D0_2DB9(C_2F65_036A);
				C_2701_0648(0x1b);
				D_6CF0[objNum] = (D_6CF0[objNum] & 0x9f) | alignment;
				D_4706[objNum] = 8;
				C_276B_012F(objNum, 1);
				C_09DA_0327();
				D_24D6 = 1;
				C_0359_0CD4();
				D_EADE = objNum;
				return;
			}
		}
		C_0A5D_1506(objNum);
	}
	C_05D0_2DB9(C_2F65_03A7);
	C_2701_0648(0x21);
}

/*the NPCs (not dead) in the 11x9 tiles around the active party member,
  at most 63; the list ends with -1*/
/*C_11FF_06F4*/int16 *SPELL_ListNearbyNPCs() {
	int16 i, j, x, y, leader;
	int16 objNum, count;

	count = 0;
	leader = D_4EDA[D_27F2];
	x = GetX(leader);
	y = GetY(leader);
	for(i = 0; i < 11; i ++) {
		for(j = 0; j < 9; j ++) {
			for(objNum = C_0A5D_05C4(x + i - 5, y + j - 4, D_27F8); count < 0x3f && objNum != -1; objNum = C_0A5D_039A()) {
				if(objNum < 0x100 && objNum >= 0 && !(D_4602[objNum] & 0x10))
					D_EAE2[count ++] = objNum;
			}
		}
	}
	D_EAE2[count] = -1;
	return D_EAE2;
}

/*"Triolo senses danger": in which directions hostile (or 0x9B mode)
  creatures are, up to 25 tiles away*/
/*C_11FF_07BB*/SPELL_SenseDanger() {
	int16 dx, dy;
	int16 dirCount[8] = {0, 0, 0, 0, 0, 0, 0, 0};/*its initialiser is at DS 0B4A*/
	/*0B5A*/static char *DangerDirNames[] = {
		"north", "northeast", "east", "southeast",
		"south", "southwest", "west", "northwest"
	};
	register int16 objNum, numDirs;

	numDirs = 0;
	for(objNum = 0x10; objNum < 0x100; objNum ++) {
		if(GetZ(objNum) == D_27F8 && !(D_6CF0[objNum] & 0x10) && ((D_6CF0[objNum] & 0x20) || D_4706[objNum] == 0x9b)) {
			dx = abs(GetX(objNum) - D_27F4);
			dy = abs(GetY(objNum) - D_27F6);
			if(dx > 25 || dy > 25)
				continue;
			if(dy == 0)
				dy = -1;
			if(dx == 0)
				dx = -1;
			if(abs(dx) < (abs(dy) >> 1)) {
				if(dy > 0)
					dirCount[0] ++;
				else
					dirCount[4] ++;
			} else if(abs(dy) < (abs(dx) >> 1)) {
				if(dx > 0)
					dirCount[2] ++;
				else
					dirCount[6] ++;
			} else if(dx > 0) {
				if(dy > 0)
					dirCount[3] ++;
				else
					dirCount[1] ++;
			} else {
				if(dy > 0)
					dirCount[5] ++;
				else
					dirCount[7] ++;
			}
		}
	}
	for(objNum = 0; objNum < 8; objNum ++)
		if(dirCount[objNum])
			numDirs ++;
	C_05D0_2DB9("\nTriolo senses ");
	if(numDirs == 0) {
		C_05D0_2DB9("no danger.\n");
		C_2701_0648(0x21);
	} else {
		if(numDirs == 8) {
			C_05D0_2DB9("danger all around!\n");
		} else {
			C_05D0_2DB9("danger to the ");
			for(objNum = 0; objNum < 8; objNum ++) {
				if(dirCount[objNum]) {
					C_05D0_2DB9("%s", DangerDirNames[objNum]);
					dirCount[objNum] = 0;
					break;
				}
			}
			for(numDirs --; numDirs > 0; numDirs --) {
				if(numDirs > 1)
					C_05D0_2DB9(", ");
				else
					C_05D0_2DB9(" and ");
				for(objNum = 0; objNum < 8; objNum ++) {
					if(dirCount[objNum]) {
						C_05D0_2DB9("%s", DangerDirNames[objNum]);
						dirCount[objNum] = 0;
						break;
					}
				}
			}
			C_05D0_2DB9(".\n");
		}
		C_2701_0648(0x10);
	}
}

/*a spell on the NPCs around the party: 0x10 heal (party members), 0x11
  charm (hostile ones), 0x12 protect (party members), 0x20 curse
  (hostile ones), 0x22 make the party members "wild"; then says who is
  affected*/
/*C_11FF_0A08*/SPELL_CastOnNearby(int16 spell) {
	int16 caster, i;
	char *msg;
	int16 list[100];
	int16 *p;
	int16 count;

	count = 0;
	caster = D_4EDA[D_03C5];
	C_05D0_2DB9("\n");
	for(p = SPELL_ListNearbyNPCs(); *p >= 0; p ++) {
		if(*p >= 0x100 || *p < 0)
			continue;
		if(C_0A5D_0DEF(GetType(*p)))
			continue;
		switch(spell) {
			case 0x22:
				if((D_6CF0[*p] & 0x80) && !(D_329C[*p] & 0x80)) {
					D_329C[*p] |= 0x80;
					if(*p != 1)
						list[count ++] = *p;
				}
				break;
			case 0x12:
				if((D_6CF0[*p] & 0x80) && !(D_6CF0[*p] & 1))
					D_6CF0[*p] |= 1;
				break;
			case 0x20:
				if((D_6CF0[*p] & 0x20) && !(D_329C[*p] & 0x40) && !C_1810_0562(D_03C5, *p) && C_130F_0136(*p)) {
					list[count ++] = *p;
					D_329C[*p] |= 0x40;
				}
				break;
			case 0x10:
				if((D_6CF0[*p] & 0x80) && !(D_6CF0[*p] & 0x10) && (C_1810_062A(*p) > D_6DF4[*p] || (D_6CF0[*p] & 8)))
					list[count ++] = *p;
				break;
			case 0x11:
				if((D_6CF0[*p] & 0x20) && !(D_329C[*p] & 4) && !C_1810_0562(D_03C5, *p) && C_130F_0136(*p))
					list[count ++] = *p;
				break;
		}
	}
	if(spell != 0x12) {
		list[count] = -1;
		switch(spell) {
			case 0x22:
				C_2701_0648(0xf);
				msg = "wild!";
				C_0E13_0EC8(D_03C5);
				break;
			case 0x20:
				msg = "cursed!";
				C_2701_0648(0xc);
				SPELL_ShowCurse(list);
				break;
			case 0x10:
				msg = "healed.";
				C_2701_0648(0xd);
				SPELL_Heal(list);
				break;
			case 0x11:
				msg = "charmed.";
				C_2701_0648(0xb);
				SPELL_Charm(D_4EDA[D_03C5], list);
				break;
		}
		if(count == 0) {
			C_2701_0648(0x21);
			C_05D0_2DB9(C_2F65_034F);
		} else {
			C_05D0_2DB9("\n");
			for(i = 0; i < count - 2; i ++)
				C_05D0_2DB9("%s, ", C_0A5D_3A76(list[i]));
			if(count > 1) {
				C_05D0_2DB9("%s ", C_0A5D_3A76(list[count - 2]));
				C_05D0_2DB9("and ");
				C_05D0_2DB9("%s are ", C_0A5D_3A76(list[count - 1]));
			} else {
				C_05D0_2DB9("%s ", C_0A5D_3A76(list[0]));
				C_05D0_2DB9("is ");
			}
			C_05D0_2DB9("%s\n", msg);
		}
	} else {
		C_2701_0648(0xe);
		C_05D0_2DB9("\nAphazz protects the party.\n");
	}
}

/*does a spell (circle * 0x10 + offering): 0 peer at the map, 1 light,
  2 sense danger, 0x21 help, others SPELL_CastOnNearby*/
/*C_11FF_0D96*/SPELL_Execute(int16 spell) {
	int16 mouseState;
	int16 savedDisplay;

	savedDisplay = D_03CD;
	C_05D0_2DB9("\n");
	C_276B_012F(D_4EDA[D_03C5], 2);
	switch(spell) {
		case 1:
			D_27FA[0] = 200;
			C_2701_0648(0x10);
			C_05D0_2DB9("\nTriolo creates light.\n");
			C_0359_11B6(0);
			break;
		case 0:
			GR_SET_COLOR(D_24E4);
			D_03CD = 0x9e;
			mouseState = C_05D0_0004(183, 6, 314, 118);
			GR_FILL_RECT(183, 6, 314, 118);
			C_276B_1BAF();
			C_276B_1953(1000, 210, 7);
			C_0359_0CD4();
			C_2701_0648(0x10);
			C_05D0_2DB9("\nTriolo leaves his body and looks down at the party!\n");
			C_276B_2008();
			D_03CD = savedDisplay;
			D_0202 = 1;
			C_0359_1806();
			C_05D0_004D(mouseState);
			break;
		case 0x21:
			if(D_27F8) {
				C_05D0_2DB9("\nNo help available.\n");
				C_2701_0648(0x21);
			} else {
				SPELL_Help(D_4EDA[D_03C5]);
			}
			break;
		case 0x10: case 0x11: case 0x12: case 0x20: case 0x22:
			SPELL_CastOnNearby(spell);
			break;
		case 2:
			SPELL_SenseDanger();
			break;
	}
	C_09DA_0327();
	C_0359_0CD4();
}

/*"Invoke": objNum (0x56, 0x58, or 0x57 in combat) gives the circle;
  the offering then selected (0x37, 0x36, 0x38, next to the party or
  carried) the spell. One offering is used up*/
/*C_11FF_0F07*/SPELL_Invoke(int16 objNum) {
	int16 who, offerType, objType, x, y, key;
	unsigned char circle, quan;
	int16 done, offering;

	done = 0;
	objType = GetType(objNum);
	C_05D0_2DB9("\nInvoke: ");
	switch(objType) {
		case 0x56:
			C_05D0_2DB9("Heluzz!\n");
			circle = 0;
			break;
		case 0x58:
			C_05D0_2DB9("Aphazz!\n");
			circle = 0x10;
			break;
		case 0x57:
			if(D_2860 == 1) {
				C_05D0_2DB9("Motazz!\n");
				circle = 0x20;
				break;
			}
			C_2701_0648(0x21);
			C_05D0_2DB9("Motazz aids only those ");
			C_05D0_2DB9("who are in mortal combat.\n");
			return;
	}
	who = C_1DBD_092C(D_27F2);
	C_05D0_2DB9("offering-- ");
	if(D_03A4) {
		C_05D0_1834(0);
		D_03A4 = 1;
	}
	key = C_05D0_22C8();
	if(key == 0x8e && D_B642 != -1) {
		C_05D0_2DB9(C_0A5D_3A76(D_B642));
		C_05D0_2B58('\n');
		offerType = GetType(D_B642);
		offering = D_B642;
		x = GetX(who);
		y = GetY(who);
		if(GetCoordUse(D_B642) == 0 && (abs(D_B66A - x) > 1 || abs(D_B66C - y) > 1)) {
			C_05D0_2DB9(C_2F65_01FE);
		} else switch(offerType) {
			case 0x37:
				SPELL_Execute(circle);
				done = 1;
				break;
			case 0x36:
				SPELL_Execute(circle | 1);
				done = 1;
				break;
			case 0x38:
				SPELL_Execute(circle | 2);
				done = 1;
				break;
			default:
				C_05D0_2DB9("\nOffering refused.\n");
				break;
		}
	} else {
		C_05D0_2DB9(C_2F65_00E4);
	}
	if(done) {
		quan = GetQuan(offering);
		if(quan > 1)
			GetQuan(offering) = quan - 1;
		else
			C_0A5D_1506(offering);
		if(D_03CD == 0x92)
			C_0E13_0BA1();
	}
}
