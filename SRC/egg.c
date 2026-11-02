/* flags: -d -mm -p -c -Z -O -y */
/*
 * The Savage Empire (Worlds of Ultima) - GAME.EXE
 * segment 2667: creature generation "eggs" (Ultima VI's egg.c).
 *
 * An egg (object type 0xE6) is an invisible map object whose inventory
 * holds the "embryos": one object per creature (or object) type to make,
 * quantity = maximum number made. The egg's own quantity is the
 * percentage chance to hatch. Eggs hatch when their area is loaded
 * (EGG_hatchArea), once until their "hatched" status bit (0x40) is
 * cleared. EGG_generate makes one creature with random attributes from
 * its class tables (monsdata.c), the other parts of multi-tile creatures
 * and its possessions.
 * Unlike U6 there are no day/night eggs, no alignment in the egg's
 * quality, no combat mode in the embryo's quality, and no loot lists.
 */
#include "se.h"

#undef abs

/*monster tables (monsdata.c, far segment 2F43)*/
extern int16 far MonsterTypes[];/*MonsterTypes*/
extern unsigned char far MonsterStr[];/*MonsterStr*/
extern unsigned char far MonsterDex[];/*MonsterDex*/
extern unsigned char far MonsterInt[];/*MonsterInt*/
extern unsigned char far MonsterHP[];/*MonsterHP*/
extern unsigned char far MonsterAlignment[];/*MonsterAlignment*/
extern char far HearSomethingMsg[];/*HearSomethingMsg: "You hear something to the %s!\n"*/

extern char DirIncrX[];
extern char DirIncrY[];
extern char *DirNames[];
extern char PromptFlag;/*PromptFlag*/
extern unsigned char DirOriginX, DirOriginY;/*DirOriginX, DirOriginY: screen position of the party*/
extern int16 EnemyWarningGiven;/*EnemyWarningGiven*/
extern unsigned char RangeTable[8][8];/*RangeTable*/
extern int16 MapX, MapY, MapZ;/*MapX, MapY, MapZ*/
extern unsigned char far *D_3298;/*MAGIC?*/
extern unsigned char D_329C[];/*ObjStatus*/
extern char D_44FA[];/*MovePts*/
extern unsigned char far *D_4602;/*NPCFlag*/
extern unsigned char D_4606[];/*DEXTE*/
extern unsigned char far *D_4706;/*NPCMode*/
extern unsigned char far *D_4EEC;/*Level*/
extern uint16 D_51E4[];/*ExpPoints*/
extern unsigned char far *D_53E4;/*NPCComMode*/
extern unsigned char D_53E8[];/*STREN*/
extern int16 D_54F0[];/*Amount: quantity (low byte), quality (high byte)*/
extern unsigned char D_6CF0[];/*NPCStatus*/
extern unsigned char D_6DF4[];/*HitPoints*/
extern int16 far *D_6EF4;/*BaseTile*/
extern unsigned char D_6EFC[];/*TalkFlags*/
extern unsigned char D_7644[];/*INTEL*/
extern struct coord D_78E4[];/*ObjPos*/
extern uint16 D_9CFA[];/*ObjShapeType*/
extern unsigned char far *D_B4FA;/*TerrainType*/
extern int16 D_BAFE, D_BB00;/*AreaX, AreaY*/
extern int16 D_EC6E;/*GetMonsterClass: last type looked up*/
extern int16 D_EC70;/*GetMonsterClass: its class*/
extern int16 D_EC72[3];/*parts of a multi-tile creature: log2 of the number of frames*/
extern int16 D_EC78[3];/*parts of a multi-tile creature: object types (-1 ends)*/
extern struct coord D_EC7E;/*position of a new part*/

extern int16 MkDirection(int16, int16);/*MkDirection*/
extern cdecl CON_printf(char far *, ...);/*CON_printf*/
extern unsigned char GetTileAtXYZ(int16, int16, int16);
extern int16 FindInv(int16);/*FindInv*/
extern int16 NextInv(void);/*NextInv*/
extern int16 NextArea(void);/*NextArea*/
extern int16 SearchArea(int16, int16, int16, int16);/*SearchArea*/
extern C_0A5D_0D5B(int16, int16 *, int16 *);/*the parts of a multi-tile type*/
extern int16 C_0A5D_0DEF(int16);/*multi-tile type?*/
extern int16 AddObj(int16, struct coord, int16, int16, int16);/*AddObj*/
extern int16 AddMapObj(int16, int16, int16, int16, int16, int16);/*AddMapObj*/
extern int16 AddInvObj(int16, int16, int16, int16);/*AddInvObj*/
extern int16 AddMonster(int16, int16, int16, int16);/*AddMonster*/
extern DeleteObj(int16);/*DeleteObj*/
extern int16 QuanType(int16);/*QuanType*/
extern int16 C_130F_0002(int16);
extern int16 COMBAT_TryTeleport(int16, int16);/*COMBAT_TryTeleport*/
extern int16 C_1DBD_18FD(int16, int16);
extern C_2701_04A7();
extern int16 C_2B57_001F(int16, int16);/*OSI_rand(lo, hi)*/

#define GetType(i) (D_9CFA[i] & 0x3ff)
#define GetFrame(i) (D_9CFA[i] >> 10)
#define GetQuan(i) (((unsigned char *)(D_54F0 + (i)))[0])
#define GetQual(i) (((unsigned char *)(D_54F0 + (i)))[1])
#define GetX(i) ((*(int16 *)&(D_78E4[i].raw[0]))&0x3ff)
#define GetY(i) (((*(int16 *)&(D_78E4[i].raw[1]))>>2)&0x3ff)
#define GetZ(i) (D_78E4[i].raw[2] >> 4)

/*2360*/int16 ForceHatching = 0;
/*2362*/int16 HatchingDisabled = 0;/*set while a game is loaded*/

/*same as COMBAT_range(), but returns 8 when too far*/
/*C_2667_0009*/EGG_CombatRange(int16 x0, int16 y0, int16 x1, int16 y1) {
	int16 delta_x, delta_y;

	delta_x = abs(x1 - x0);
	delta_y = abs(y1 - y0);
	if(delta_x > 7 || delta_y > 7)
		return 8;
	return RangeTable[delta_x][delta_y];
}

/*monster class of an object type (index in the monsdata.c tables,
  minus 1); -1 if the type is not a creature. The last answer is kept.*/
/*C_2667_004F*/GetMonsterClass(int16 objType) {
	int16 far *p;

	if(objType == D_EC6E)
		return D_EC70;
	D_EC6E = objType;
	D_EC70 = 0;
	for(p = MonsterTypes + 1; *p; p ++) {
		if(*p == objType)
			return D_EC70;
		D_EC70 ++;
	}
	return D_EC70 = -1;
}

/*random value in [val - val/2, val + val/2] (triangular distribution)*/
/*C_2667_00A2*/mkRandom(int16 val) {
	int16 half = val >> 1;
	if(half)
		val = (val - half) + C_2B57_001F(0, half) + C_2B57_001F(0, half);
	return val;
}

/*the possessions of a new creature*/
/*C_2667_00DA*/EGG_GivePossessions(int16 objNum) {
	if(GetType(objNum) == 0x140) {
		AddInvObj(0x21, 0x38, 0, objNum);
		AddInvObj(0x09, 0x38, 0, objNum);
	} else if(GetType(objNum) == 0x13c) {
		if(AddInvObj(0x20, 0x38, 0, objNum) >= 0)
			AddInvObj(0x24, 0x30, 0x1e, objNum);
		else
			AddInvObj(0x19, 0x38, 0, objNum);
	}
	return;
}

/*create a creature of type objType at x,y,z: attributes from its class
  (mkRandom'ized), alignment from the class, the other parts of
  multi-tile creatures and its possessions. returns the new actor
  number (< 0 if none was free)*/
/*C_2667_015D*/EGG_generate(int16 objType, int16 x, int16 y, int16 z) {
	int16 objNum, i, j;
	int16 objClass, partObj;

	objNum = AddMonster(objType, x, y, z);
	if(objNum < 0)
		return objNum;
	if((objClass = GetMonsterClass(objType)) >= 0) {
		D_53E8[objNum] = mkRandom(MonsterStr[objClass + 1]);
		D_4606[objNum] = mkRandom(MonsterDex[objClass + 1]);
		D_7644[objNum] = mkRandom(MonsterInt[objClass + 1]);
		D_6DF4[objNum] = mkRandom(MonsterHP[objClass + 1]);
		D_4EEC[objNum] = 0;
		if(objType == 0x13c && !(D_6EFC[0x35] & 0x80))
			D_6CF0[objNum] = 0x60;
		else
			D_6CF0[objNum] = MonsterAlignment[objClass + 1];
	} else {
		D_53E8[objNum] =
		D_4606[objNum] =
		D_7644[objNum] = 10;
		D_6DF4[objNum] = 30;
		D_4EEC[objNum] = 1;
		D_6CF0[objNum] = 0x20;
	}
	D_3298[objNum] = 0;
	D_44FA[objNum] = D_4606[objNum];
	D_51E4[objNum] = 0;
	D_4706[objNum] =
	D_53E4[objNum] = 8;

	if(objType == 0x120 || C_130F_0002(objNum)) {
		D_9CFA[objNum] = GetType(objNum) + (1 << 10);
	} else if(objType == 0x128) {
		D_9CFA[objNum] = GetType(objNum) + (3 << 10);
	} else if(C_0A5D_0DEF(objType)) {
		/*multi-tile creature*/
		C_0A5D_0D5B(objType, D_EC72, D_EC78);
		D_9CFA[objNum] = GetType(objNum) + (((1 << D_EC72[0]) - 1) << 10);
		for(i = 1; i < 3 && D_EC78[i] != -1; i ++) {
			for(j = 0; j < 4; j ++) {
				if(
					(D_B4FA[D_6EF4[GetType(objNum)] + GetFrame(objNum)] & (0x80 >> j)) &&
					(D_B4FA[D_6EF4[D_EC78[i]] + (1 << D_EC72[i]) - 1] & (0x80 >> ((j + 2) % 4)))
				) {
					D_EC7E.raw[2] = MapZ << 4;
					*(int16 *)&(D_EC7E.raw[0]) = x + DirIncrX[j * 2];
					*(int16 *)&(D_EC7E.raw[1]) |= (y + DirIncrY[j * 2]) << 2;
					partObj = AddObj(0, D_EC7E, D_EC78[i], (1 << D_EC72[i]) - 1, 0);
					GetQual(partObj) = objNum;
					break;
				}
			}
		}
	} else if(objType == 0x124) {
		D_9CFA[objNum] = GetType(objNum);
		GetQual(objNum) = 4;
		for(i = 0; i < 8; i += 2) {
			partObj = EGG_generate(0x11a, x + DirIncrX[i], y + DirIncrY[i], MapZ);
			if(partObj >= 0) {
				D_4602[partObj] = (D_4602[partObj] & 0xf8) | i;
				GetQual(partObj) = objNum;
				D_6CF0[partObj] = (D_6CF0[partObj] & 0x9f) | (D_6CF0[objNum] & 0x60);
				D_53E4[partObj] =
				D_4706[partObj] = 0xd;
			}
		}
	}

	EGG_GivePossessions(objNum);

	return objNum;
}

/*try to hatch an egg (see the module header). The first creature
  appears on the egg, the others are scattered around it (off screen
  unless ForceHatching is set). When hostile creatures appear, a message
  says where something is heard. The egg is then marked hatched (and
  invisible) whatever the result*/
/*C_2667_04F7*/EGG_hatches(int16 egg_objNum) {
	int16 i, quan, objTyp, typParam, numMonsters;
	int16 prevPart, x, y, embryo_objNum, dir;
	int16 hostileOffscreen, isFirstBorn, npcMod, onlyOne, onScreenNotOk;
	int16 rx, ry;
	int16 monster;

	prevPart = 0;/*U6 leftovers, unused*/
	npcMod = 0;
	onlyOne = 0;/*TalkFlags[1] & 8: only one creature of type 0x143*/

	if(HatchingDisabled)
		return;
	onScreenNotOk = !ForceHatching;
	if(D_6EFC[1] & 8)
		onlyOne = 1;
	if(!(D_329C[egg_objNum] & 0x40)) {
		/*Quantity of egg: percent chance of hatching*/
		if(C_2B57_001F(1, 100) <= GetQuan(egg_objNum)) {
			hostileOffscreen = 0;
			isFirstBorn = 1;
			for(embryo_objNum = FindInv(egg_objNum); embryo_objNum >= 0; embryo_objNum = NextInv()) {
				/*two copies of the embryo's type (U6: type and type + mutant flag)*/
				objTyp =
				typParam = GetType(embryo_objNum);
				numMonsters = GetQuan(embryo_objNum);
				if(typParam == 0x7f || typParam == 0xcb)
					numMonsters = C_2B57_001F(10, 20);
				if(typParam == 0x143) {
					if(onlyOne)
						numMonsters = 1;
					else if(MapZ == 1)
						numMonsters = C_2B57_001F(5, 15);
				} else if(!(typParam == 0x12d || typParam == 0x11a || (typParam >= 0x136 && typParam <= 0x146))) {
					numMonsters = 1;
				} else if(GetQuan(egg_objNum) <= 100) {
					numMonsters = C_2B57_001F(1, numMonsters);
				}
				if(typParam == 0x7f || typParam == 0xcb)
					numMonsters = C_2B57_001F(10, 20);
				for(i = 0; i < numMonsters; i ++) {
					if(objTyp < 0x11a) {
						/*not a creature: plain object*/
						if(QuanType(objTyp))
							quan = 1;
						else
							quan = 0;
						if(objTyp == 0xa7) {
							monster = AddMapObj(objTyp, 0x20, quan, GetX(egg_objNum), GetY(egg_objNum), MapZ);
							D_9CFA[monster] = GetType(monster) + (3 << 10);
						} else {
							rx = GetX(egg_objNum) + C_2B57_001F(0, 6) - 3;
							ry = GetY(egg_objNum) + C_2B57_001F(0, 6) - 3;
							if(
								!C_1DBD_18FD(rx, ry) &&
								!(D_B4FA[GetTileAtXYZ(rx, ry, MapZ)] & 2) &&
								!(D_B4FA[GetTileAtXYZ(rx, ry, MapZ)] & 4)
							) monster = AddMapObj(objTyp, 0x20, quan, rx, ry, MapZ);
						}
						if(monster >= 0 && (objTyp == 0x7f || objTyp == 0xcb))
							GetQuan(monster) = 5;
					} else {
						monster = EGG_generate(typParam, GetX(egg_objNum), GetY(egg_objNum), MapZ);
						if(monster >= 0) {
							if(D_6CF0[monster] & 0x20)
								hostileOffscreen = 1;
							if(D_6CF0[monster] & 0x60)
								D_53E4[monster] = D_4706[monster] = 8;
							else
								D_53E4[monster] = D_4706[monster] = 0x8f;
						}
					}
					if(monster >= 0) {
						if(!isFirstBorn) {
							if(!COMBAT_TryTeleport(monster, onScreenNotOk))
								DeleteObj(monster);
						} else {
							isFirstBorn = 0;
						}
					}
				}
			}
			/*"You hear something to the <dir>"*/
			if(!EnemyWarningGiven && hostileOffscreen) {
				x = GetX(egg_objNum) - MapX;
				y = GetY(egg_objNum) - MapY;
				dir = MkDirection((x << 4) + DirOriginX, (y << 4) + DirOriginY);
				C_2701_04A7();
				CON_printf(HearSomethingMsg, DirNames[dir]);
				PromptFlag = 1;
				EnemyWarningGiven = 1;
			}
		}
	}
	D_329C[egg_objNum] |= 0x40;/*hatched*/
	D_329C[egg_objNum] |= 0x02;/*invisible*/
}

/*hatch the eggs of the current level in the loaded 40x40 area: the
  ones out of sight (more than 8 tiles away), or all of them with
  ForceHatching*/
/*C_2667_08E0*/EGG_hatchArea() {
	int16 objNum, x, y, objTyp;

	if(HatchingDisabled)
		return;
	x = D_BAFE;
	y = D_BB00;
	for(objNum = SearchArea(x, y, x + 39, y + 39); objNum >= 0; objNum = NextArea()) {
		x = GetX(objNum);
		y = GetY(objNum);
		objTyp = GetType(objNum);
		if(objTyp == 0xe6 && GetZ(objNum) == MapZ) {
			if(ForceHatching || !CLOSE_ENOUGH(8, x, y, MapX, MapY))
				EGG_hatches(objNum);
		}
	}
}
