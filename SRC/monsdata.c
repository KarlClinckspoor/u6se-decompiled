/* flags: -d -mh -p -c -Z -O -zTFAR_DATA */
/*
	The monster tables (far data segment 2F43, after the code), Ultima VI's
	monsdata.c rewritten for Savage Empire. A "monster class" is the index of
	a creature's object type in MonsterTypes; class 0 is unused (all its
	entries are 0) and the list ends with a 0. All other tables are indexed
	by class (48 entries): base strength, dexterity, intelligence, armor,
	damage and hit points, initial alignment (NPCStatus bits: 0x20 evil,
	0x40 good, 0x60 chaotic), and ability flags (U6's MONSTER_xxx bits,
	monsdata.h there; their meaning in Savage Empire is not checked yet).
	Unlike U6 there are no starting inventory lists.
*/

/*C_2F43_0000*/int MonsterTypes[] = {
/*00*/	0x000,0x136,0x137,0x138,0x139,0x13A,0x13B,0x13C,
/*08*/	0x13E,0x13F,0x140,0x141,0x142,0x143,0x144,0x145,
/*16*/	0x146,0x13D,0x126,0x128,0x11C,0x121,0x129,0x12F,
/*24*/	0x131,0x120,0x11B,0x12C,0x147,0x14A,0x14B,0x14C,
/*32*/	0x14D,0x14E,0x14F,0x123,0x12B,0x12D,0x12E,0x133,
/*40*/	0x134,0x135,0x150,0x151,0x148,0x149,0x11A,0x124,
/*48*/	0x000
};
/*strength*/
/*C_2F43_0062*/unsigned char MonsterStr[] = {
/*00*/	  0, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 10, 20, 25,
/*16*/	 20,  1, 60, 35, 20, 40, 50, 30, 40, 10, 17, 25, 25,  7, 30, 20,
/*32*/	 22, 30, 30,  1,  3, 20, 15, 15,  3,  5,  9,  5,  0, 20, 50,  0
};
/*dexterity*/
/*C_2F43_0092*/unsigned char MonsterDex[] = {
/*00*/	  0, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 45, 20, 15,
/*16*/	 20, 20,  5, 15,  1, 11,  9,  3,  8, 35, 25, 20,  2, 20, 25,  5,
/*32*/	 30,  5, 15, 60,  2, 15,  5,  5,  7, 13, 11, 14,  0, 30, 30,  0
};
/*intelligence*/
/*C_2F43_00C2*/unsigned char MonsterInt[] = {
/*00*/	  0, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,  1, 20, 10,
/*16*/	 20,  1,  1, 10,  1,  2, 60,  1,  2,  2, 12,  2,  1,  2,  7,  3,
/*32*/	  8,  2,  6, 60,  1, 60,  4,  4,  6,  4,  4,  6, 30, 20,  0,  0
};
/*armor*/
/*C_2F43_00F2*/unsigned char MonsterArmor[] = {
/*00*/	  0,  0,  0,  0,  0,  0,  0,  0,  3,  0,  0,  0,  0,  8,  0,  0,
/*16*/	  0,  0,255,  3,  4,  8, 15,  8, 10,  3,  2,  5, 15,  3,  3,  1,
/*32*/	  2,  6,  3,  0,  0,  7, 10,  5,  1,  0,  0,  0, 10,  0,  5,  5
};
/*damage (natural attack)*/
/*C_2F43_0122*/unsigned char MonsterDamage[] = {
/*00*/	  0,  3,  3,  3,  3,  3,  3,  3,  5,  3,  3,  3,  3, 10,  3, 10,
/*16*/	  3,  2,150, 25,  5, 35, 40, 20, 30, 20, 15, 15, 15, 10, 30,  5,
/*32*/	 25, 25, 27,  1,  3, 15, 10, 10,  2,  6,  8,  2,  0,  3, 10,  0
};
/*hit points*/
/*C_2F43_0152*/unsigned char MonsterHP[] = {
/*00*/	  0, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 15,
/*16*/	 10,  3,255, 40, 20, 50, 60, 25, 50, 10, 20, 20, 25, 20, 35, 20,
/*32*/	 25, 20, 40, 30,  3, 30, 25, 25,  5,  8, 10,  7, 20, 10, 10, 10
};
/*initial alignment (NPCStatus)*/
/*C_2F43_0182*/unsigned char MonsterAlignment[] = {
/*00*/	0x00,0x40,0x40,0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,
/*16*/	0x00,0x00,0x60,0x60,0x00,0x60,0x60,0x00,0x00,0x60,0x20,0x60,0x00,0x60,0x60,0x00,
/*32*/	0x60,0x60,0x60,0x00,0x00,0x60,0x00,0x00,0x60,0x60,0x60,0x00,0x20,0x00,0x60,0x60
};
/*ability flags (U6's MONSTER_xxx bits)*/
/*C_2F43_01B2*/int MonsterFlags[] = {
/*00*/	0x1000,0x6000,0x6000,0x6000,0x6000,0x6000,0x6000,0x6000,
/*08*/	0x6000,0x4000,0x6000,0x6000,0x6000,0x6000,0x6000,0x6000,
/*16*/	0x6000,0x2100,0x2024,0x2020,0x2025,0x2025,0x002D,0x2025,
/*24*/	0x2025,0x2105,0x2044,0x020D,0x2025,0x2001,0x2000,0x2020,
/*32*/	0x2000,0x2025,0x2020,0x1429,0x0011,0x002D,0x0A28,0x4A28,
/*40*/	0x2840,0x2840,0x0051,0x2020,0x3000,0x6000,0x1011,0x1011
};

/*14 zero bytes up to msgs.c's segment (2F65:0000), referenced by nothing: an unused table
  or Origin's padding (huge-model data segments are only word aligned)*/
/*C_2F43_0212*/unsigned char MonsterUnused[14] = {0};
