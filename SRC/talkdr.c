/* flags: -d -mm -p -c -Z -O -y */
/*
 * The Savage Empire (Worlds of Ultima) - GAME.EXE
 * Conversation engine, from Ultima VI's talkdr.c.
 *
 * Interpreter for the NPC conversation scripts, loaded into TalkBuf (D_6EF8)
 * by LoadConversation (LoadConversation). A script is a byte stream: bytes < 0x80
 * are text that is printed as is, bytes >= 0x80 are opcodes (OP_xxx below).
 * Script layout:
 *   OP_SCRIPT_MARK <npc num> <npc name> OP_DESC <description text...>
 *   OP_MAIN <statements...> then a loop of
 *   OP_ASKTOP / OP_GETxxx (input) followed by keyword blocks
 *   OP_KEY "kw1,kw2,..." OP_RES <answer statements> OP_ENDRES ...
 * Expressions are stored in postfix order and end with OP_END_OF_FACTOR (or
 * OP_LET_VALUE); parse_factor() evaluates them on a small local stack of
 * longs. Numeric constants are prefixed by OP_BYTE / OP_WORD / OP_ADDRESS.
 * Variables: VarInt[] (#0..#9, #A..#Z) and VarStr[] ($0..$9, $A..$Z); a letter
 * variable 'X' lives at index 'X' - 0x37. Flags: 16 per NPC, bits 0-7 in
 * TalkFlags[npc] (D_6EFC), bits 8-15 in D_3298[npc] (OP_SET/OP_CLR/OP_TST).
 * TalkNpcs[0] is the NPC being talked to (the OP_NPC pseudo-number),
 * TalkNpcs[1] the party member talking. TalkDriver() is the entry point.
 *
 * Savage Empire's changes: no horses, no karma, no rest, no item selection;
 * the party holds at most 5 members besides the three core characters (NPCs
 * 2, 5 and 0x0b); flags 8-15; OP_EVENT (0xd1) runs scripted events
 * (TALK_Event); the conversation uses the large text window and the
 * portrait panel.
 */
#include "se.h"

/*comparison operators (push 1/0)*/
#define OP_SUP            0x81
#define OP_SUPE           0x82
#define OP_INF            0x83
#define OP_INFE           0x84
#define OP_DIF            0x85
#define OP_EQU            0x86

/*arithmetic/logical operators*/
#define OP_ADD            0x90
#define OP_SUB            0x91
#define OP_MUL            0x92
#define OP_DIV            0x93
#define OP_OR             0x94
#define OP_AND            0x95

#define OP_CANCARRY       0x9a
#define OP_WEIGHT         0x9b
#define OP_GETHORSE       0x9c
#define OP_HORSED         0x9d
#define OP_REST           0x9e
#define OP_OWNS           0x9f

#define OP_RND            0xa0
#define OP_IF             0xa1
#define OP_ENDIF          0xa2
#define OP_ELSE           0xa3
#define OP_SET            0xa4
#define OP_CLR            0xa5
#define OP_LET            0xa6
#define OP_END_OF_FACTOR  0xa7
#define OP_LET_VALUE      0xa8
#define OP_TST            0xab

#define OP_GOTO           0xb0
/*after an OP_ADDRESS operand: "no index expression follows"*/
#define OP_NOINDEX        0xb1
#define OP_VARINT         0xb2
#define OP_VARSTR         0xb3
/*read 16 bit value from an array: (array address, index)*/
#define OP_DATA           0xb4
#define OP_PRINTSTR       0xb5
#define OP_LEAVE          0xb6
#define OP_STRSEARCH      0xb7
#define OP_ENDSEARCH      0xb8
#define OP_GIVEOBJ        0xb9
#define OP_TAKEOBJ        0xba
#define OP_TEST_OBJ       0xbb
/*stubs: pop 2 operands, push constant 100*/
#define OP_STUB_BC        0xbc
#define OP_STUB_BD        0xbd
#define OP_SHOW_INVENTORY 0xbe
#define OP_SHOW_CONVERSE  0xbf
#define OP_OWNER          0xc1
#define OP_OBJTYPE        0xc2
#define OP_VALSEARCH      0xc3
#define OP_ISINPARTY      0xc6
#define OP_WHOSGOT        0xc7
#define OP_MOVEOBJ        0xc8
#define OP_TRANSFEROBJ    0xc9
#define OP_JOIN           0xca
#define OP_WAIT           0xcb
#define OP_LEAVEPARTY     0xcc
#define OP_SETMODE        0xcd

#define OP_DELAY          0xd0
/*run a scripted event (TALK_Event)*/
#define OP_EVENT          0xd1

/*constant prefixes*/
#define OP_ADDRESS        0xd2
#define OP_BYTE           0xd3
#define OP_WORD           0xd4
/*a parameter to OP_PRINTSTR: print nothing*/
#define OP_PRINT_NONE     0xd5
#define OP_RESURRECT      0xd6
#define OP_ISONSCREEN     0xd7
/*$Y := name of npc*/
#define OP_SETNAME_Y      0xd8
#define OP_HEAL           0xd9
#define OP_WOUNDED        0xda
#define OP_CURE           0xdb
#define OP_POISONED       0xdc
/*get party member npc num: (party index, any/onscreen only)*/
#define OP_PARTYMEMBER    0xdd

/*$Y := name of npc, or "you" for the player*/
#define OP_SETNAME_Y_YOU  0xdf
#define OP_ADDEXP         0xe0
#define OP_ADDLVL         0xe1
#define OP_ADDSTR         0xe2
#define OP_ADDINT         0xe3
#define OP_ADDDEX         0xe4

/*pseudo npc number: "the npc being talked to"*/
#define OP_NPC            0xeb

#define OP_ENDRES         0xee
#define OP_KEY            0xef
/*opcodes >= this stop parse_statement (section/input commands)*/
#define OP_FIRST_SECTION  0xf0
#define OP_DESC           0xf1
#define OP_MAIN           0xf2
#define OP_RES            0xf6
#define OP_ASKTOP         0xf7
#define OP_GET            0xf8
#define OP_GETSTR         0xf9
#define OP_GETCHR         0xfa
#define OP_GETINT         0xfb
#define OP_GETDIGIT       0xfc
/*start of script (followed by npc num) / end of conversation*/
#define OP_SCRIPT_MARK    0xff

/*text window (Ultima VI's)*/
struct tClipInfo/*size:9*/ {
	unsigned char originX, originY, cols, rows;
	unsigned char curX, curY;
	unsigned char fgColor, bgColor;
	unsigned char attrFlags;
};

/*other modules*/
extern int16 MkDirection(int16, int16);/*MkDirection(x, y): direction of the screen point from the party, or -1*/
extern int16 CON_getch();/*CON_getch*/
extern CON_gets(char *, int16);/*CON_gets(buffer, max length)*/
extern CON_setClip(struct tClipInfo *);/*CON_setClip*/
extern CON_gotoxy(int16, int16);/*CON_gotoxy*/
extern CON_putch(int16);/*CON_putch*/
extern cdecl CON_printf(char far *, ...);/*CON_printf*/
extern DrawMapWindow(int16);/*redraws the map window (U6: DrawMapWindow?)*/
extern DisplayTick();/*DisplayTick: one animation frame*/
extern VIS_UpdateView();/*VIS_UpdateView*/
extern int16 C_0A5D_0613(int16);/*is the NPC able to talk (onscreen)?*/
extern int16 OBJ_FindTypeAt(int16, int16, int16);/*OBJ_FindTypeAt(type, x, y)?*/
extern int16 FindInv(int16);/*FindInv: first object of an inventory*/
extern int16 NextInv();/*NextInv*/
extern int16 FindInvType(int16, int16, int16);/*FindInvType(npc, type, quality)*/
extern int16 AddMapObj(int16, int16, int16, int16, int16, int16);/*AddMapObj*/
extern DeleteObj(int16);/*DeleteObj*/
extern MoveObj(int16, int16, int16, int16);/*MoveObj(obj, x, y, z)*/
extern InsertObj(int16, int16, int16);/*InsertObj(obj, owner, how)*/
extern int16 GiveObj(int16, int16, int16);/*GiveObj(npc, type, quality/quantity): -1 on failure*/
extern TakeObj(int16, int16, int16);/*TakeObj(npc, type, quality/quantity)*/
extern int16 TestObj(int16, int16);/*TestObj(npc, type)*/
extern OBJ_DeleteTmpBlocks();/*OBJ_DeleteTmpBlocks*/
extern int16 IsWeightPer10Type(int16);/*IsWeightPer10Type*/
extern STAT_ComputeEquipment(int16);/*STAT_ComputeWeights*/
extern int16 OBJ_FindInContainer(int16, int16, int16);/*OBJ_FindInContainer(npc, type, quality)*/
extern int16 SPELL_ReviveNPC(int16);/*SPELL_ReviveNPC(dead body): the NPC, or < 0*/
extern int16 CanMoveTo(int16, int16, int16);/*(npc, x, y): may a creature appear there?*/
extern UpdateMoveFrame(int16, int16);/*UpdateMoveFrame(npc, direction)*/
extern int16 MaxHP(int16);/*MaxHP*/
extern SetTypeUnconscious(int16);/*an NPC falls asleep (shape)*/
extern SetOkToGet(int16);/*SetOkToGet*/
extern int16 ParseNumberInput(char *, int16);/*ParseNumberInput*/
extern ShowPortrait(int16);/*ShowPortraitAndInventory*/
extern int16 EGG_generate(int16, int16, int16, int16);/*(type, x, y, z): creates a creature*/
extern LoadConversation(int16, unsigned char far *);/*LoadConversation(npc, buffer)*/
extern int16 HasConversation(int16);/*has the npc a conversation script?*/
extern ClearMapWindow(int16);
extern int16 C_2B57_001F(int16, int16);/*OSI_rand(lo, hi)*/
extern cdecl MEM_shutdown(int16);
extern cdecl ShutdownSound(void);
extern char far NotInSoloMsg[];/*"Not in solo mode!\n" (far messages)*/
extern char far NoResponseMsg[];/*"\nNo response.\n" (far messages)*/

extern unsigned char StatusDirty;/*StatusDirty*/
extern unsigned char IsArmageddon;/*conversations are impossible (U6: IsArmageddon)*/
extern char AnimateAtPrompt;/*AnimateAtPrompt*/
extern int16 EndPagingAtPrompt;
extern int16 StatusMember;/*StatusMember*/
extern int16 StatusDisplay;/*StatusDisplay*/
extern int16 PromptCh;/*PromptCh*/
extern struct tClipInfo *CurClip;/*CurClip*/
extern unsigned char PartyListTop;/*PartyListTop*/
extern int16 ScreenFade;
extern int16 D_24E6;/*ColorText*/
extern int16 D_24F0;/*color of the player's input*/
extern unsigned char Active;/*Active*/
extern int16 MapX, MapY, MapZ;/*MapX, MapY, MapZ*/
extern unsigned char DrunkCounter;
extern unsigned char InCombat;
extern char SoloMember;/*SoloMember*/

extern unsigned char far *D_3298;/*NPC flags: 0x80 ?, bits 0-7: talk flags 8-15*/
extern unsigned char D_329C[];/*ObjStatus*/
extern unsigned char D_4606[];/*DEXTE*/
extern unsigned char far *D_4706;/*NPCMode*/
extern unsigned char D_4EDA[];/*Party*/
extern unsigned char far *D_4EEC;/*Level*/
extern char D_5104[][14];/*Names: of the party members*/
extern uint16 D_51E4[];/*ExpPoints*/
extern unsigned char far *D_53E4;/*NPCComMode*/
extern unsigned char D_53E8[];/*STREN*/
extern unsigned char far *D_54E8;/*ScratchBuf*/
extern unsigned char D_6CF0[];/*NPCStatus*/
extern unsigned char D_6DF4[];/*HitPoints*/
extern unsigned char far *D_6EF8;/*TalkBuf*/
extern unsigned char D_6EFC[];/*TalkFlags*/
extern unsigned char far *D_763C;/*TypeWeight*/
extern unsigned char D_7644[];/*INTEL*/
extern struct coord D_78E4[];/*ObjPos*/
extern unsigned char D_9CF6;/*PartySize*/
extern uint16 D_9CFA[];/*ObjShapeType*/
extern int16 D_B5E8, D_B5EA;/*PointerX, PointerY*/
extern int16 D_B5FA[10+26];/*VarInt*/
extern struct tClipInfo D_B644[4];/*TextWindows*/
extern char *D_B66E[10+26];/*VarStr*/
extern int16 D_B6B6;
extern unsigned char D_BD10[11][11];/*Obj_11x11: objects shown on each square of the map window*/
extern int16 D_C08A[11][11];/*Tile_11x11*/
extern int16 D_E668;/*WeightEquip*/
extern int16 D_E66A;/*WeightInven*/
extern unsigned char D_E66C[];/*TalkKeyword*/
extern int32 D_E678;/*TalkDescPC*/
extern int16 D_E67E;/*TalkBase*/
extern int16 D_E682[2];/*TalkNpcs*/
extern char D_E686[];/*TalkInput*/
extern unsigned char D_E6B8[];/*TalkNpcName*/
extern unsigned char D_E6EA;/*TalkAnswered*/
extern int16 D_E6EC;/*Talk_PC*/
extern int32 D_E6EE;/*TalkSavedPC*/
extern union tHandler D_ED10;/*GrDriver*/

#define GR_SHUTDOWN()                (D_ED10.iii.ofs = 0x03, (*D_ED10.f)())
#define GR_PUT_TILE_SCREEN(tile,x,y) (D_ED10.iii.ofs = 0x2d, (*D_ED10.f)(tile,x,y))

#define Active Active
#define PartySize D_9CF6
#define Party D_4EDA
#define Names D_5104
#define NPCStatus D_6CF0
#define NPCMode D_4706
#define TalkBuf D_6EF8
#define Talk_PC D_E6EC
#define TalkBase D_E67E
#define TalkSavedPC D_E6EE
#define TalkNpcs D_E682
#define TalkFlags D_6EFC
#define VarInt D_B5FA
#define VarStr D_B66E
#define TalkInput D_E686
#define TalkKeyword D_E66C
#define TalkNpcName D_E6B8
#define TalkAnswered D_E6EA
#define CurClip CurClip
#define StatusMember StatusMember
#define StatusDisplay StatusDisplay
#define PromptCh PromptCh

#define IN_VEHICLE (Active == PartySize)
#define IsParalyzed(i)   (NPCStatus[i] & 0x02)
#define IsAsleep(i)      (NPCStatus[i] & 0x04)
#define IsPoisoned(i)    (NPCStatus[i] & 0x08)
#define IsDead(i)        (NPCStatus[i] & 0x10)
#define IsPlrControl(i)  (NPCStatus[i] & 0x80)
#define GetAlignment(i)  (NPCStatus[i] & 0x60)
#define SetAlignment(i, v) (NPCStatus[i] = (NPCStatus[i] & ~0x60) | (v))
#define NEUTRAL 0
#define EVIL 0x20
#define GOOD 0x40
#define CHAOTIC 0x60
#define GetType(i) (D_9CFA[i] & 0x3ff)
#define GetX(i) ((*(int16 *)&(D_78E4[i].raw[0]))&0x3ff)
#define GetY(i) (((*(int16 *)&(D_78E4[i].raw[1]))>>2)&0x3ff)
#define GetAssoc(i) (*(uint16 *)&(D_78E4[i]))
#define QualQuan(quality, quantity) ((quality) << 8 | (quantity))
#define INVEN 0x10

/*this macro creates an "offset" into the current conversation file
(TalkBase is always 0)*/
#define TALK_OFFSET(i) (int16)(i) - TalkBase

#define PARSE_U8 (TalkBuf[Talk_PC ++])
#define READ_U16 (*(uint16 far *)(TalkBuf + Talk_PC))
#define READ_S16 (*(int16 far *)(TalkBuf + Talk_PC))
#define READ_U32 (*(uint32 far *)(TalkBuf + Talk_PC))

/*====---- ----====*/
/*0A36*/unsigned char Unused_0A36 = 0;
/*0A37*/unsigned char Unused_0A37 = 0;
/*set by OP_LEAVE: end the conversation*/
/*0A38*/static unsigned char mustLeave = 0;
/*0A39*/unsigned char IsInConversation = 0;
/*====---- ----====*/

/*keyword match: returns 1 if the first <len> chars of <pattern> match
(case insensitive, '?' is a wildcard) the start of any space separated
word of <input>*/
/*C_0F8D_0004*/static str_i_compare(unsigned char *pattern, unsigned char *input, int16 len) {
	unsigned char *word;
	int16 i;
	unsigned char buf[100];

	strcpy(buf, input);
	for(word = strtok(buf, " "); word; word = strtok(0, " ")) {
		for(i = 0; i < len; i ++) {
			if(tolower(pattern[i]) != tolower(word[i]) && pattern[i] != '?')
				break;
		}
		if(i == len)
			return 1;
	}
	return 0;
}

/*translate the OP_NPC pseudo-number into the current npc's number*/
/*C_0F8D_007E*/static unsigned char mk_npcnum(int16 npc) {
	if(npc == OP_NPC)
		return TalkNpcs[0];
	return npc;
}

/*get NPC name: loads the npc's script into ScratchBuf and copies the
name (which follows OP_SCRIPT_MARK and the npc num, and ends with the first
opcode byte) into TalkNpcName, after "a " for the NPCs >= 0x40;
returns TalkNpcName*/
/*C_0F8D_0094*/static unsigned char *TALK_GetNpcName(int16 objNum) {
	int16 i, j;

	i = 0;
	j = 2;
	if(objNum >= 0x40) {
		TalkNpcName[0] = 'a';
		TalkNpcName[1] = ' ';
		i = 2;
	}
	if(HasConversation(objNum)) {
		LoadConversation(objNum, D_54E8);
		for(j = 2; i < 50; i ++, j ++)
			TalkNpcName[i] = D_54E8[j];
	} else {
		TalkNpcName[0] = 0;
	}
	i = 0;
	while((TalkNpcName[i] & 0x80) == 0 && TalkNpcName[i++]);
	if(i == 0)
		CON_printf("\nUndefined.\n");
	TalkNpcName[i] = 0;

	return TalkNpcName;
}

/*name of a character: party members use the party name table (Names[]),
other npcs the name from their conversation script*/
/*C_0F8D_011C*/char *TALK_GetCharName(register int16 objNum) {
	register int16 i;

	if(IsPlrControl(objNum)) {
		for(i = 0; i < 8; i ++)
			if(Party[i] == objNum)
				break;
		return Names[i];
	}
	TALK_GetNpcName(objNum);

	return (char *)TalkNpcName;
}

/*is object onscreen? (within the visible 9x9 part of the 11x11 view
and not on a hidden/dark tile); party members are always "onscreen"
while in a vehicle*/
/*C_0F8D_0159*/IsNpcOnScreen(register int16 objNum) {
	int16 x;
	register int16 y;
	int16 tile;

	if(IN_VEHICLE && IsPlrControl(objNum))
		return 1;
	if(
		GetX(objNum) > MapX - 5 && GetX(objNum) < MapX + 5 &&
		GetY(objNum) > MapY - 5 && GetY(objNum) < MapY + 5
	) {
		x = GetX(objNum) - (MapX - 5);
		y = GetY(objNum) - (MapY - 5);
		/*objects displayed there, or an object tile*/
		if(D_BD10[y][x] || D_C08A[y][x] >= 0x200)
			return 1;
		tile = D_C08A[y][x];
		return (tile != 0xff && tile != 0x1bc);
	}
	return 0;
}

/*add npc to the party: at most 5 members besides the core characters
(npcs 2, 5 and 0x0b)
returns 0:ok (or already in the party), 1:party is in a vehicle, 2:party is full*/
/*C_0F8D_0290*/JoinParty(int16 objNum) {
	unsigned char *name;
	int16 nameLen;
	register int16 i;
	register int16 freeSlots;

	freeSlots = 5;
	for(i = 0; i < PartySize; i ++) {
		if(Party[i] != 2 && Party[i] != 0xb && Party[i] != 5)
			freeSlots --;
		if(Party[i] == objNum)
			return 0;
	}
	if(freeSlots < 1 && objNum != 2 && objNum != 0xb && objNum != 5)
		return 2;
	if(IN_VEHICLE)
		return 1;
	PartySize ++;
	Party[PartySize - 1] = objNum;
	name = TALK_GetNpcName(objNum);
	nameLen = strlen(name);
	if(nameLen >= 14)
		strncpy(Names[PartySize - 1], name, 13);
	else
		strcpy(Names[PartySize - 1], name);
	NPCStatus[objNum] |= 0x80;
	D_3298[objNum] &= 0x7f;
	if(GetAlignment(objNum) == NEUTRAL)
		SetAlignment(objNum, GOOD);
	NPCMode[objNum] = 1;
	D_53E4[objNum] = 8;
	return 0;
}

/*remove npc from the party; he drops his whole inventory at his feet
returns 0:ok, 1:party is in a vehicle, 2:not in party*/
/*C_0F8D_03B4*/static LeaveParty(register int16 objNum) {
	int16 slot, invObj;
	register int16 i;

	if(IN_VEHICLE)
		return 1;
	for(slot = 0; slot < PartySize; slot ++)
		if(Party[slot] == objNum)
			break;
	if(PartySize == slot)
		return 2;
	for(i = slot; i < PartySize; i ++) {
		Party[i] = Party[i + 1];
		strcpy(Names[i], Names[i + 1]);
	}
	PartySize --;
	NPCStatus[objNum] &= ~0x80;
	if(GetAlignment(objNum) == GOOD)
		SetAlignment(objNum, NEUTRAL);
	if(D_3298[objNum] & 0x80) {
		NPCMode[objNum] = 0x92;
	} else {
		NPCMode[objNum] = 0x8f;
		for(invObj = FindInv(objNum); invObj >= 0; invObj = FindInv(objNum)) {
			MoveObj(invObj, GetX(objNum), GetY(objNum), MapZ);
			SetOkToGet(invObj);
		}
		MoveObj(objNum, GetX(objNum), GetY(objNum), MapZ);
	}
	return 0;
}

/*returns the first onscreen party member carrying an object of type
<objType> (and quality <objQual>), or -1*/
/*C_0F8D_04FB*/FindPartyMemberWithObj(int16 objType, int16 objQual) {
	int16 i;

	for(i = 0; i < PartySize; i ++) {
		if(IsNpcOnScreen(Party[i]) && OBJ_FindInContainer(Party[i], objType, objQual) != -1)
			return Party[i];
	}
	return -1;
}

/*evaluate a postfix expression starting at Talk_PC, up to (and
including) OP_END_OF_FACTOR/OP_LET_VALUE; returns its value.
Constants are pushed; an operator pops its operands from the top of
lstack and leaves its result in the lowest operand's slot.*/
/*C_0F8D_0543*/static parse_factor() {
	register int16 sidx;/*stack index*/
	int32 lstack[10];/*local "stack"*/
	int32 arrayAddr;
	int16 i, j, opcode, cmpLen;
	unsigned char cparam0, cparam1, varIdx;
	int16 isOperator, isString;
	register int16 iparam0;
	int16 iparam1, iparam2;
	unsigned char scratch[80];
	int16 inputStr;

	sidx = 0;
	i = 0;
	j = 0;
	isString = 0;
	/*reset local stack*/
	for(sidx = 0; sidx < 10; sidx ++)
		lstack[sidx] = 0L;
	sidx = 0;
	opcode = PARSE_U8;
	while(opcode != OP_END_OF_FACTOR && opcode != OP_LET_VALUE) {
		isOperator = 0;
		switch(opcode) {
			case OP_ADDRESS: lstack[sidx] = READ_U32; Talk_PC += 4; break;
			case OP_BYTE: lstack[sidx] = PARSE_U8; break;
			case OP_WORD: lstack[sidx] = READ_U16; Talk_PC += 2; break;
			default:
				/*a raw byte: either an operator or a small constant*/
				isOperator = 1;
				lstack[sidx] = opcode;
		}
		if(isOperator) {
			switch(lstack[sidx]) {
				case OP_RND:/*random number in [min, max] (8 bits)*/
					cparam0 = lstack[sidx - 2];
					cparam1 = lstack[sidx - 1];
					lstack[sidx - 2] = C_2B57_001F(cparam0, cparam1);
					sidx -= 2;
				break;
				case OP_TEST_OBJ:/*(npc, object type)*/
					iparam0 = mk_npcnum(lstack[sidx - 2]);
					iparam1 = lstack[sidx - 1];
					lstack[sidx - 2] = TestObj(iparam0, iparam1);
					sidx -= 2;
				break;
				case OP_OWNS:/*(npc, object type, quality)*/
					iparam0 = mk_npcnum(lstack[sidx - 3]);
					iparam1 = lstack[sidx - 2];
					iparam2 = lstack[sidx - 1];
					if(OBJ_FindInContainer(iparam0, iparam1, iparam2) != -1)
						lstack[sidx - 3] = 1;
					else
						lstack[sidx - 3] = 0;
					sidx -= 3;
				break;
				case OP_WEIGHT:/*(object type, quantity): weight in 1/10 units*/
					iparam0 = lstack[sidx - 1];/*quantity*/
					iparam1 = lstack[sidx - 2];/*object type*/
					iparam2 = D_763C[iparam1];
					if(IsWeightPer10Type(iparam1))
						lstack[sidx - 2] = iparam0 * iparam2;
					else
						lstack[sidx - 2] = iparam0 * iparam2 * 10;
					sidx -= 2;
				break;
				case OP_JOIN:/*(npc): see JoinParty for the result*/
					iparam0 = mk_npcnum(lstack[sidx - 1]);
					lstack[sidx - 1] = JoinParty(iparam0);
					sidx --;
				break;
				case OP_CANCARRY:/*(npc): remaining capacity (STR * 20 stones)*/
					iparam0 = mk_npcnum(lstack[sidx - 1]);
					STAT_ComputeEquipment(iparam0);
					iparam1 = D_53E8[iparam0] * 200;
					iparam2 = (D_E668 + D_E66A) * 10;
					if(iparam1 > iparam2)
						lstack[sidx - 1] = iparam1 - iparam2;
					else
						lstack[sidx - 1] = 0;
					sidx --;
				break;
				case OP_LEAVEPARTY:/*(npc): see LeaveParty for the result*/
					iparam0 = mk_npcnum(lstack[sidx - 1]);
					lstack[sidx - 1] = LeaveParty(iparam0);
					sidx --;
				break;
				case OP_TST:/*(npc, flag bit 0-15)*/
					iparam0 = mk_npcnum(lstack[sidx - 2]);
					iparam1 = lstack[sidx - 1];
					if(iparam1 <= 7)
						lstack[sidx - 2] = (TalkFlags[iparam0] >> iparam1) & 1;
					else if(iparam1 <= 15)
						lstack[sidx - 2] = (D_3298[iparam0] >> (iparam1 - 8)) & 1;
					sidx -= 2;
				break;
				case OP_OWNER:/*(object): its owner (assoc field)*/
					lstack[sidx - 1] = (int16)GetAssoc(lstack[sidx - 1]);
					/*patches the next operand of the script in place*/
					TalkBuf[Talk_PC + 1] = mk_npcnum(TalkBuf[Talk_PC + 1]);
					sidx --;
				break;
				case OP_WOUNDED:/*(npc)*/
					iparam0 = mk_npcnum(lstack[sidx - 1]);
					if(MaxHP(iparam0) > D_6DF4[iparam0])
						lstack[sidx - 1] = 1;
					else
						lstack[sidx - 1] = 0;
					sidx --;
				break;
				case OP_POISONED:/*(npc)*/
					iparam0 = mk_npcnum(lstack[sidx - 1]);
					lstack[sidx - 1] = IsPoisoned(iparam0);
					sidx --;
				break;
				case OP_ADDEXP:/*(npc, amount): new value, max 9999*/
					iparam0 = mk_npcnum(lstack[sidx - 2]);
					iparam1 = lstack[sidx - 1];
					D_51E4[iparam0] += iparam1;
					if(D_51E4[iparam0] > 9999)
						D_51E4[iparam0] = 9999;
					lstack[sidx - 2] = D_51E4[iparam0];
					sidx -= 2;
				break;
				case OP_ADDLVL:/*(npc, amount): new level*/
					iparam0 = mk_npcnum(lstack[sidx - 2]);
					iparam1 = lstack[sidx - 1];
					D_4EEC[iparam0] += iparam1;
					lstack[sidx - 2] = D_4EEC[iparam0];
					sidx -= 2;
				break;
				case OP_ADDSTR:/*(npc, amount): new value, max 30*/
					iparam0 = mk_npcnum(lstack[sidx - 2]);
					iparam1 = lstack[sidx - 1];
					D_53E8[iparam0] += iparam1;
					if(D_53E8[iparam0] > 30)
						D_53E8[iparam0] = 30;
					lstack[sidx - 2] = D_53E8[iparam0];
					sidx -= 2;
				break;
				case OP_ADDINT:/*(npc, amount): new value, max 30*/
					iparam0 = mk_npcnum(lstack[sidx - 2]);
					iparam1 = lstack[sidx - 1];
					D_7644[iparam0] += iparam1;
					if(D_7644[iparam0] > 30)
						D_7644[iparam0] = 30;
					lstack[sidx - 2] = D_7644[iparam0];
					sidx -= 2;
				break;
				case OP_ADDDEX:/*(npc, amount): new value, max 30*/
					iparam0 = mk_npcnum(lstack[sidx - 2]);
					iparam1 = lstack[sidx - 1];
					D_4606[iparam0] += iparam1;
					if(D_4606[iparam0] > 30)
						D_4606[iparam0] = 30;
					lstack[sidx - 2] = D_4606[iparam0];
					sidx -= 2;
				break;
				case OP_PARTYMEMBER:/*get party member npc num (0 if none)*/
					iparam0 = lstack[sidx - 2];/*party member index*/
					iparam1 = lstack[sidx - 1];/*if 0, member must be onScreen*/
					if(iparam0 >= PartySize) {
						lstack[sidx - 2] = 0;
					} else if(iparam1) {
						lstack[sidx - 2] = Party[iparam0];
					} else {
						/*party's onscreen members list*/
						j = 0;
						for(i = 0; i < PartySize; i ++) {
							if(IsNpcOnScreen(Party[i]))
								scratch[j++] = i;
						}
						for(cparam0 = j; cparam0 < PartySize; cparam0 ++)
							scratch[cparam0] = 0xff;
						if(scratch[iparam0] == 0xff)
							lstack[sidx - 2] = 0;
						else
							lstack[sidx - 2] = Party[scratch[iparam0]];
					}
					sidx -= 2;
				break;
				case OP_ISONSCREEN:/*(npc)*/
					iparam0 = mk_npcnum(lstack[sidx - 1]);
					lstack[sidx - 1] = IsNpcOnScreen(iparam0);
					sidx --;
				break;
				case OP_WHOSGOT:/*(object type, quality): onscreen party member carrying it, or 0x8001*/
					iparam0 = lstack[sidx - 2];
					iparam1 = lstack[sidx - 1];
					i = FindPartyMemberWithObj(iparam0, iparam1);
					if(i == -1)
						lstack[sidx - 2] = 0x8001;
					else
						lstack[sidx - 2] = i;
					sidx -= 2;
				break;
				case OP_ISINPARTY:/*(npc): in party, onscreen and able to act*/
					iparam0 = mk_npcnum(lstack[sidx - 1]);
					if(
						IsPlrControl(iparam0) &&
						IsNpcOnScreen(iparam0) &&
						!IsDead(iparam0) &&
						!IsAsleep(iparam0) &&
						!IsParalyzed(iparam0)
					)
						lstack[sidx - 1] = 1;
					else
						lstack[sidx - 1] = 0;
					sidx --;
				break;
				case OP_HORSED:/*(npc): no horses, always 0*/
					iparam0 = mk_npcnum(lstack[sidx - 1]);
					lstack[sidx - 1] = 0;
					sidx --;
				break;
				case OP_OBJTYPE:/*(object)*/
					lstack[sidx - 1] = GetType(lstack[sidx - 1]);
					sidx --;
				break;
				case OP_STUB_BC:/*constant 100?*/
					lstack[sidx - 2] = 100;
					sidx -= 2;
				break;
				case OP_STUB_BD:/*constant 100?*/
					lstack[sidx - 2] = 100;
					sidx -= 2;
				break;
				case OP_DATA:/*read a 16bit value from array*/
					arrayAddr = lstack[sidx - 2];/*array address*/
					varIdx = lstack[sidx - 1];/*array index*/
					TalkSavedPC = Talk_PC;
					Talk_PC = (int16)arrayAddr + (varIdx << 1) - TalkBase;
					lstack[sidx - 2] = READ_S16;
					Talk_PC = TalkSavedPC;
					sidx -= 2;
				break;
				case OP_VARINT:/*# var*/
					varIdx = lstack[sidx - 1];
					lstack[sidx - 1] = (uint16)VarInt[varIdx];
					sidx --;
				break;
				case OP_VARSTR:/*$ string: pushes a pointer*/
					varIdx = lstack[sidx - 1];
					lstack[sidx - 1] = (int16)VarStr[varIdx];
					/*from now on, OP_EQU compares strings*/
					isString = 1;
					sidx --;
				break;
				case OP_ADD:
					lstack[sidx - 2] = lstack[sidx - 1] + lstack[sidx - 2];
					sidx -= 2;
				break;
				case OP_SUB:
					lstack[sidx - 2] = lstack[sidx - 2] - lstack[sidx - 1];
					sidx -= 2;
				break;
				case OP_MUL:
					lstack[sidx - 2] = lstack[sidx - 1] * lstack[sidx - 2];
					sidx -= 2;
				break;
				case OP_DIV:
					if(lstack[sidx - 1])
						lstack[sidx - 2] = (uint32)lstack[sidx - 2] / (uint32)lstack[sidx - 1];
					else
						CON_printf("\nDivide error from talkdr.\n");
					sidx -= 2;
				break;
				case OP_AND:
					if(lstack[sidx - 1] && lstack[sidx - 2])
						lstack[sidx - 2] = 1;
					else
						lstack[sidx - 2] = 0;
					sidx -= 2;
				break;
				case OP_OR:
					if(lstack[sidx - 1] || lstack[sidx - 2])
						lstack[sidx - 2] = 1;
					else
						lstack[sidx - 2] = 0;
					sidx -= 2;
				break;
				case OP_EQU:
					if(isString) {
						if(stricmp((char *)lstack[sidx - 1], (char *)lstack[sidx - 2]) == 0)
							lstack[sidx - 2] = 1;
						else
							lstack[sidx - 2] = 0;
					} else {
						if(lstack[sidx - 1] == lstack[sidx - 2])
							lstack[sidx - 2] = 1;
						else
							lstack[sidx - 2] = 0;
					}
					sidx -= 2;
				break;
				/*relational operators compare unsigned*/
				case OP_SUP:
					if((uint32)lstack[sidx - 2] > (uint32)lstack[sidx - 1])
						lstack[sidx - 2] = 1;
					else
						lstack[sidx - 2] = 0;
					sidx -= 2;
				break;
				case OP_SUPE:
					if((uint32)lstack[sidx - 2] >= (uint32)lstack[sidx - 1])
						lstack[sidx - 2] = 1;
					else
						lstack[sidx - 2] = 0;
					sidx -= 2;
				break;
				case OP_INF:
					if((uint32)lstack[sidx - 2] < (uint32)lstack[sidx - 1])
						lstack[sidx - 2] = 1;
					else
						lstack[sidx - 2] = 0;
					sidx -= 2;
				break;
				case OP_INFE:
					if((uint32)lstack[sidx - 2] <= (uint32)lstack[sidx - 1])
						lstack[sidx - 2] = 1;
					else
						lstack[sidx - 2] = 0;
					sidx -= 2;
				break;
				case OP_DIF:
					if(lstack[sidx - 1] != lstack[sidx - 2])
						lstack[sidx - 2] = 1;
					else
						lstack[sidx - 2] = 0;
					sidx -= 2;
				break;
				case OP_STRSEARCH:
					/*(array of strings, $var): index of the first entry whose
					first 4 chars match a word of $var (see str_i_compare),
					or the number of entries if none*/
					varIdx = lstack[sidx - 1];/*string variable*/
					arrayAddr = lstack[sidx - 2];/*array variable*/
					inputStr = (int16)VarStr[varIdx];
					TalkSavedPC = Talk_PC;
					Talk_PC = TALK_OFFSET(arrayAddr);
					for(i = 0; (opcode = PARSE_U8) != OP_ENDSEARCH; ) {
						Talk_PC --;
						i ++;
						for(j = 0; opcode = PARSE_U8; j ++)
							scratch[j] = opcode;
						scratch[j] = 0;
						if(strlen(scratch) >= 4)
							cmpLen = 4;
						else
							cmpLen = strlen(scratch);
						if(str_i_compare(scratch, (unsigned char *)inputStr, cmpLen))
							break;
					}
					if(opcode == OP_ENDSEARCH)
						lstack[sidx - 2] = i;
					else
						lstack[sidx - 2] = i - 1;
					Talk_PC = TalkSavedPC;
					sidx -= 2;
				break;
				case OP_VALSEARCH:
					/*(array of 16bit values, #var): index of the entry equal
					to #var, or 0x8001 if none*/
					varIdx = lstack[sidx - 1];/*integer variable*/
					arrayAddr = lstack[sidx - 2];/*array variable*/
					iparam1 = VarInt[varIdx];
					TalkSavedPC = Talk_PC;
					Talk_PC = TALK_OFFSET(arrayAddr);
					for(i = 0; (opcode = TalkBuf[Talk_PC]) != OP_ENDSEARCH; Talk_PC += 2) {
						i ++;
						iparam0 = READ_S16;
						if(iparam0 == iparam1)
							break;
					}
					if(opcode == OP_ENDSEARCH)
						lstack[sidx - 2] = 0x8001;
					else
						lstack[sidx - 2] = i - 1;
					Talk_PC = TalkSavedPC;
					sidx -= 2;
				break;
			}
		}
		sidx ++;
		opcode = PARSE_U8;
	}
	if(sidx > 1)
		CON_printf("Error occured.\n");
	return lstack[0];
}

/*resolve an OP_ADDRESS operand (Talk_PC points just after OP_ADDRESS):
reads the U32 address; if OP_NOINDEX follows it is a plain address,
otherwise an index expression follows and selects an element of the
array (isStringArray: skip <index> NUL-terminated strings; else skip
<index> 16bit words).
Leaves Talk_PC on the data and returns the PC where the caller must
resume afterwards*/
/*C_0F8D_14FB*/static uint32 TALK_ResolveAddress(int16 isStringArray) {
	register int16 index;
	uint32 resumePC, address;

	address = READ_U32;
	Talk_PC += 4;
	if(PARSE_U8 != OP_NOINDEX) {
		Talk_PC --;
		index = parse_factor();
		resumePC = Talk_PC;
		Talk_PC = TALK_OFFSET(address);
		if(isStringArray) {
			while(index > 0)
				if(PARSE_U8 == 0)
					index --;
		} else {
			Talk_PC += index << 1;
			resumePC -= 1;
		}
	} else {
		resumePC = Talk_PC;
		Talk_PC = TALK_OFFSET(address);
	}
	return resumePC;
}

/*OP_EVENT: scripted events of the story*/
/*C_0F8D_15A2*/static TALK_Event(int16 event) {
	int16 r0, r1;
	int16 count;
	int16 objNum;
	int16 x, y;
	register int16 k;
	register int16 npc;

	switch(event) {
		case 0xaf:/*npc 0xb3 calms down; npc 0x19 is moved*/
			MoveObj(0x19, 0x3e5, 0x153, 0);
			NPCMode[0xb3] = 0;
			SetAlignment(0xb3, NEUTRAL);
		break;
		case 0x96:
			objNum = AddMapObj(0xc94, 0, 0, 0x215, 0x1c5, 0);
			D_329C[objNum] &= 0xdf;
		break;
		case 0x95:
			DrunkCounter ++;
		break;
		case 0xde:
			DeleteObj(OBJ_FindTypeAt(0xa1, 0x92, 0x89));
		break;
		case 2: case 5:/*hostile creatures (type 0x143) appear around the party*/
			if(!(TalkFlags[0x28] & 0x80)) {
				count = C_2B57_001F(5, 10);
				for(k = 0; k < count; k ++) {
					r0 = C_2B57_001F(0, 1);
					r1 = C_2B57_001F(0, 1);
					if(C_2B57_001F(0, 1)) {
						x = MapX + C_2B57_001F(0, 10) - 5;
						y = MapY + (C_2B57_001F(0, 1) << 3) - 4;
					} else {
						x = MapX + (C_2B57_001F(0, 1) << 3) - 4;
						y = MapY + C_2B57_001F(0, 10) - 5;
					}
					if(CanMoveTo(Party[Active], x, y)) {
						npc = EGG_generate(0x143, x, y, MapZ);
						k ++;
						SetAlignment(npc, EVIL);
						NPCMode[npc] = 6;
					}
				}
			}
		break;
		case 1: case 3: case 4:/*hostile creatures (type 0x140) appear around the party*/
			count = C_2B57_001F(2, 10);
			for(k = 0; k < count; k ++) {
				r0 = C_2B57_001F(0, 1);
				r1 = C_2B57_001F(0, 1);
				if(C_2B57_001F(0, 1)) {
					x = MapX + C_2B57_001F(0, 10) - 5;
					y = MapY + (C_2B57_001F(0, 1) << 3) - 4;
				} else {
					x = MapX + (C_2B57_001F(0, 1) << 3) - 4;
					y = MapY + C_2B57_001F(0, 10) - 5;
				}
				if(CanMoveTo(Party[Active], x, y)) {
					npc = EGG_generate(0x140, x, y, MapZ);
					k ++;
					SetAlignment(npc, CHAOTIC);
					NPCMode[npc] = 6;
				}
			}
		break;
		case 10:/*end of the game: back to DOS*/
			OBJ_DeleteTmpBlocks();
			GR_SHUTDOWN();
			ShutdownSound();
			MEM_shutdown(8);
			exit(0);
		break;
	}
}

/*execute one opcode (already read) of a statement;
objNum is the npc being talked to.
returns 0:ok, 1:syntax error, -1:unknown opcode*/
/*C_0F8D_18A3*/static execute_op(unsigned char opcode, int16 objNum) {
	register int16 tmp;
	uint32 gotoAddr;
	int16 ch, varIdx, varType;
	int16 *pVarInt;
	register int16 arg0;
	int16 arg1, arg2, arg3, count, revived;
	int16 far *pArrayElem;
	char *pStr;

	switch(opcode) {
		case OP_GETHORSE:
		case OP_REST:
		case OP_ENDIF:
		case OP_END_OF_FACTOR:
		case OP_SHOW_INVENTORY:
		case 0xcf:
		break;
		case OP_LEAVE:
			mustLeave = 1;
		break;
		case OP_PRINTSTR:
			ch = PARSE_U8;
			if(ch == OP_ADDRESS) {/*constant string[address]*/
				TalkSavedPC = TALK_ResolveAddress(1);
				while(ch = PARSE_U8)
					CON_putch(ch);
				Talk_PC = TalkSavedPC;
			} else if(ch != OP_PRINT_NONE) {/*VarStr: <x> <index> OP_VARSTR <x>*/
				varIdx = PARSE_U8;
				varType = PARSE_U8;
				if(varType != OP_VARSTR)
					return 1;
				CON_printf(VarStr[varIdx]);
				PARSE_U8;
			}
		break;
		case OP_LET:
			/*destination: OP_ADDRESS (array element) or <index> OP_VARINT/OP_VARSTR*/
			varIdx = PARSE_U8;
			if(varIdx == OP_ADDRESS) {
				TalkSavedPC = TALK_ResolveAddress(0);
				pArrayElem = (int16 far *)(TalkBuf + Talk_PC);
				Talk_PC = TalkSavedPC;
				varType = 0;
			} else {
				varType = PARSE_U8;
			}
			if(PARSE_U8 != OP_LET_VALUE)
				return 1;
			if(varType == 0) {
				*pArrayElem = parse_factor();
			} else if(varType == OP_VARINT) {
				pVarInt = VarInt + varIdx;
				*pVarInt = parse_factor();
			} else {/*String affectation*/
				pStr = VarStr[varIdx];
				if((ch = PARSE_U8) == OP_ADDRESS) {
					/*from a constant string*/
					TalkSavedPC = TALK_ResolveAddress(1);
					for(tmp = 0; ch = PARSE_U8; tmp ++)
						pStr[tmp] = ch;
					pStr[tmp] = 0;
					Talk_PC = TalkSavedPC;
				} else {
					/*from another string variable*/
					varIdx = ch;
					varType = PARSE_U8;
					if(varType != OP_VARSTR)
						return 1;
					strcpy(pStr, VarStr[varIdx]);
				}
			}
		break;
		case OP_IF:
			tmp = parse_factor();
			if(tmp == -1)
				return 1;
			if(tmp == 0) {
				/*false: skip to the matching OP_ELSE/OP_ENDIF
				(no nesting; skips the operands that may contain those bytes)*/
				ch = PARSE_U8;
				while(ch != OP_ELSE && ch != OP_ENDIF) {
					if(ch == OP_GOTO)
						Talk_PC +=4;
					else if(ch == OP_BYTE)
						Talk_PC ++;
					else if(ch == OP_WORD)
						Talk_PC += 2;
					ch = PARSE_U8;
				}
			}
		break;
		case OP_ELSE:
			/*end of the "true" part: skip to OP_ENDIF*/
			ch = PARSE_U8;
			while(ch != OP_ENDIF) {
				if(ch == OP_GOTO)
					Talk_PC +=4;
				else if(ch == OP_BYTE)
					Talk_PC ++;
				else if(ch == OP_WORD)
					Talk_PC += 2;
				ch = PARSE_U8;
			}
		break;
		case OP_GOTO:
			gotoAddr = READ_U32;
			Talk_PC = TALK_OFFSET(gotoAddr);
		break;
		case OP_GIVEOBJ: case OP_TAKEOBJ:
			/*(npc, object type, quality, quantity); OP_GIVEOBJ sets #T to 1 if it worked*/
			arg0 = mk_npcnum(parse_factor());
			arg1 = parse_factor();
			arg2 = parse_factor();
			arg3 = parse_factor();
			if(opcode == OP_TAKEOBJ) {
				TakeObj(arg0, arg1, QualQuan(arg2, arg3));
			} else {
				if(GiveObj(arg0, arg1, QualQuan(arg2, arg3)) == -1)
					VarInt['T' - 0x37] = 0;
				else
					VarInt['T' - 0x37] = 1;
			}
		break;
		case OP_WAIT:
			PromptCh = 1;
			CON_getch();
			PromptCh = 5;
		break;
		case OP_SETMODE:/*(npc, AI mode)*/
			arg0 = mk_npcnum(parse_factor());
			arg1 = parse_factor();
			NPCMode[arg0] = arg1;
			if(arg1 == 0x91) {
				NPCStatus[arg0] |= 0x04;
				SetTypeUnconscious(arg0);
			}
		break;
		case OP_MOVEOBJ:/*(object, new owner npc): put in npc's inventory*/
			arg0 = mk_npcnum(parse_factor());
			arg1 = parse_factor();
			InsertObj(arg0, arg1, INVEN);
		break;
		case OP_TRANSFEROBJ:/*(object type, quality, from npc, to npc)*/
			arg0 = parse_factor();
			arg1 = parse_factor();
			arg2 = mk_npcnum(parse_factor());
			arg3 = mk_npcnum(parse_factor());
			InsertObj(FindInvType(arg2, arg0, arg1), arg3, INVEN);
		break;
		case OP_EVENT:
			arg0 = parse_factor();
			TALK_Event(arg0);
		break;
		case OP_DELAY:/*(n): n * 20 animation frames*/
			arg0 = parse_factor();
			for(count = 0; count < arg0 * 20; count ++)
				DisplayTick();
		break;
		case OP_SET:/*(npc, flag bit 0-15)*/
			arg0 = mk_npcnum(parse_factor());
			arg1 = parse_factor();
			if(arg1 <= 7)
				TalkFlags[arg0] |= 1 << arg1;
			else if(arg1 <= 15)
				D_3298[arg0] |= 1 << (arg1 - 8);
		break;
		case OP_CLR:/*(npc, flag bit 0-15)*/
			arg0 = mk_npcnum(parse_factor());
			arg1 = parse_factor();
			if(arg1 <= 7)
				TalkFlags[arg0] &= ~(1 << arg1);
			else if(arg1 <= 15)
				D_3298[arg0] &= ~(1 << (arg1 - 8));
		break;
		case OP_RESURRECT:/*(npc): raise the dead body in npc's inventory; #R := the npc, or -1*/
			arg0 = mk_npcnum(parse_factor());
			for(arg1 = FindInv(arg0); arg1 >= 0; arg1 = NextInv())
				if(GetType(arg1) == 0xf9)
					break;
			revived = SPELL_ReviveNPC(arg1);
			if(revived >= 0) {
				VarInt['R' - 0x37] = revived;
				VarStr['Y' - 0x37] = TALK_GetCharName(revived);
			} else {
				VarInt['R' - 0x37] = -1;
			}
		break;
		case OP_HEAL:/*(npc): full hit points*/
			arg0 = mk_npcnum(parse_factor());
			D_6DF4[arg0] = MaxHP(arg0);
		break;
		case OP_CURE:/*(npc)*/
			arg0 = mk_npcnum(parse_factor());
			NPCStatus[arg0] &= ~0x08;
		break;
		case OP_SETNAME_Y:/*set var $Y to the npc's name*/
			arg0 = mk_npcnum(parse_factor());
			VarStr['Y' - 0x37] = TALK_GetCharName(arg0);
		break;
		case OP_SETNAME_Y_YOU:/*set var $Y to the npc's name ("you" for the player)*/
			arg0 = mk_npcnum(parse_factor());
			if(arg0 == 1)
				VarStr['Y' - 0x37] = "you";
			else
				VarStr['Y' - 0x37] = TALK_GetCharName(arg0);
		break;
		case OP_SHOW_CONVERSE:
			arg0 = mk_npcnum(parse_factor());
			ShowPortrait(arg0);/*display character's portrait*/
		break;
		default:
			CON_printf("Unknown command.\n");
			return -1;
	}
	return 0;
}

static TALK_MatchKeywords(int16 objNum);

/*execute statements (printing text bytes) until an input/section
opcode, 0, OP_ENDRES or OP_LEAVE; Talk_PC is left on the stop opcode*/
/*C_0F8D_1EEC*/static parse_statement(int16 objNum) {
	unsigned char opcode;

	opcode = PARSE_U8;
	while(
		opcode != OP_ASKTOP &&
		opcode < OP_FIRST_SECTION && opcode != 0 &&
		opcode != OP_GETSTR && opcode != OP_GET &&
		opcode != OP_ENDRES
	) {
		if(opcode < 0x80) {
			if(opcode >= ' ' || opcode == '\n') {
				CON_putch(opcode);
			} else {
				CON_printf("\nTry to print out a control char\n");
				break;
			}
		} else if(opcode == OP_KEY) {
			/*only the first matching KEY block of an input answers*/
			if(!TalkAnswered) {
				TALK_MatchKeywords(objNum);
			} else {
				while(opcode != OP_ENDRES)
					opcode = PARSE_U8;
			}
		} else {
			execute_op(opcode, objNum);
		}
		if(mustLeave)
			break;
		opcode = PARSE_U8;
	}
	Talk_PC --;
}

/*KEY/RES: Talk_PC is just after OP_KEY. Compares each comma separated
keyword (TalkKeyword) with the player's input (TalkInput; "*" matches anything);
on a match, runs the answer after OP_RES (and sets TalkAnswered), otherwise
skips to the next OP_KEY/OP_ENDRES/OP_SCRIPT_MARK*/
/*C_0F8D_1FA8*/static TALK_MatchKeywords(int16 objNum) {
	register int16 len;
	register int16 op;
	int16 moreKeys;

	moreKeys = 1;
	if(TalkInput[0] == 0)
		strcpy(TalkInput, "bye");
	while(moreKeys) {
		moreKeys = 0;
		for(len = 0; (TalkKeyword[len] = PARSE_U8) != ',' && TalkKeyword[len] != OP_RES; len ++);
		if(TalkKeyword[len] == ',')
			moreKeys = 1;
		else
			Talk_PC --;
		TalkKeyword[len] = 0;
		if(str_i_compare(TalkKeyword, TalkInput, strlen(TalkKeyword)) || TalkKeyword[0] == '*') {
			while(PARSE_U8 != OP_RES);
			CON_printf("\n");
			TalkAnswered = 1;
			parse_statement(objNum);
			return;
		}
	}
	/*no match: skip the answer*/
	do {
		op = PARSE_U8;
		if(op == OP_GOTO)
			Talk_PC += 4;
		else if(op == OP_BYTE)
			Talk_PC += 1;
		else if(op == OP_WORD)
			Talk_PC += 2;
	} while(op != OP_KEY && op != OP_ENDRES && op != OP_SCRIPT_MARK);
	if(op == OP_KEY || op == OP_SCRIPT_MARK)
		Talk_PC --;
}

/*conversation entry point: <speaker> (a party member) talks to <npcObj>.
Checks whether the npc can answer, loads its script, opens the
conversation window, prints the description, runs the main section, then
loops reading the player's input and running the matching keyword answers
until OP_LEAVE/OP_SCRIPT_MARK*/
/*C_0F8D_20C8*/TalkDriver(int16 speaker, int16 npcObj) {
	register int16 idx;
	register int16 number;
	int16 opcode, dir, savedStatus, savedMember, objTyp;
	int16 savedB6B6;
	struct tClipInfo *savedClip;

	TalkNpcs[1] = speaker;
	TalkNpcs[0] = npcObj;
	if(IsPlrControl(npcObj) && !C_0A5D_0613(npcObj)) {
		CON_printf(NoResponseMsg);
		return;
	}
	if(SoloMember != -1 && SoloMember != 0 && npcObj != 0x19) {
		CON_printf(NotInSoloMsg);
		return;
	}
	objTyp = GetType(npcObj);
	if(
		IsDead(TalkNpcs[0]) || IsAsleep(TalkNpcs[0]) ||
		NPCMode[TalkNpcs[0]] == 0x9b ||
		NPCMode[TalkNpcs[0]] == 0x13 ||
		NPCMode[TalkNpcs[0]] == 7
	) {
		CON_printf(NoResponseMsg);
		return;
	}
	if(IsArmageddon) {
		CON_printf(NoResponseMsg);
		return;
	}
	if(TalkNpcs[1] == TalkNpcs[0]) {
		CON_printf("\nTalking to yourself?\n");
		return;
	}

	/*face the npc*/
	dir = MkDirection(D_B5E8, D_B5EA);
	if(dir != -1) {
		UpdateMoveFrame(Party[Active], dir);
		VIS_UpdateView();
	}

	if(TalkNpcs[0] == 0 || !HasConversation(TalkNpcs[0])) {
		CON_printf(NoResponseMsg);
		return;
	}
	IsInConversation = 1;
	AnimateAtPrompt = 0;
	ClearMapWindow(1);
	LoadConversation(TalkNpcs[0], TalkBuf);
	Talk_PC = 0;
	/*skip OP_SCRIPT_MARK ncpnum*/
	idx = 2;
	/*get the npc name: $N*/
	while((opcode = TalkBuf[idx++]) != OP_DESC)
		VarStr['N' - 0x37][idx - 3] = opcode;
	VarStr['N' - 0x37][idx - 3] = 0;
	/* */
	savedStatus = StatusDisplay;
	savedMember = StatusMember;
	ShowPortrait(npcObj);/*display character's portrait*/
	StatusDisplay = 0x9e;
	savedB6B6 = D_B6B6;
	savedClip = CurClip;
	/*the conversation window*/
	CON_setClip(&D_B644[2]);
	CurClip->attrFlags |= 0x20;
	CurClip->attrFlags |= 0x10;
	CON_gotoxy(0, 0);
	D_B6B6 = CurClip->rows;
	GR_PUT_TILE_SCREEN(0x175, 0x40, 0);
	GR_PUT_TILE_SCREEN(0x176, 0x50, 0);
	CON_printf("You see ");
	while(PARSE_U8 != OP_DESC);
	/*remember the description for "look"*/
	D_E678 = Talk_PC;
	parse_statement(TalkNpcs[0]);
	CON_printf("\n");
	while(PARSE_U8 != OP_MAIN);

	opcode = PARSE_U8;
	if(opcode != OP_ASKTOP) {
		Talk_PC --;
		parse_statement(TalkNpcs[0]);
	} else {
		Talk_PC --;
	}

	/*main loop: get some input, then run the keyword blocks*/
	while(!mustLeave) {
		TalkSavedPC = Talk_PC;
		opcode = PARSE_U8;
		if(opcode == OP_ASKTOP) {
			CurClip->fgColor = D_24F0;
			CON_printf("\nYou say:");
			CON_gets(TalkInput, 50);
			CON_printf("\n");
			TalkAnswered = 0;
			CurClip->fgColor = D_24E6;
		} else if(opcode == OP_GETSTR) {/*<index> OP_VARSTR*/
			CurClip->fgColor = D_24F0;
			CON_gets(TalkInput, 50);
			idx = PARSE_U8;
			if(PARSE_U8 != OP_VARSTR)
				break;
			strcpy(VarStr[idx], TalkInput);
			CON_printf("\n\n");
			CurClip->fgColor = D_24E6;
			TalkAnswered = 0;
		} else if(opcode == OP_GETINT) {/*<index> OP_VARINT*/
			CurClip->fgColor = D_24F0;
			CON_gets(TalkInput, 50);
			idx = PARSE_U8;
			if(PARSE_U8 != OP_VARINT)
				break;
			number = ParseNumberInput(TalkInput, 0);
			VarInt[idx] = (number == -1 || number == -2)?0:number;
			CON_printf("\n\n");
			CurClip->fgColor = D_24E6;
			TalkAnswered = 0;
		} else if(opcode == OP_GETDIGIT) {/*<index> OP_VARINT*/
			CurClip->fgColor = D_24F0;
			do {
				TalkInput[0] = CON_getch();
				TalkInput[1] = 0;
				number = ParseNumberInput(TalkInput, 0);
				if(TalkInput[0] == '\r' || TalkInput[0] == 27)
					number = 0;
			} while(number == -1 || number == -2);
			idx = PARSE_U8;
			if(PARSE_U8 != OP_VARINT)
				break;
			VarInt[idx] = number;
			CON_printf("%d\n\n", number);
			CurClip->fgColor = D_24E6;
			TalkAnswered = 0;
		} else if(opcode == OP_GETCHR) {
			CurClip->fgColor = D_24F0;
			TalkInput[0] = CON_getch();
			TalkInput[1] = 0;
			CON_printf("%s\n\n", TalkInput);
			CurClip->fgColor = D_24E6;
			TalkAnswered = 0;
		} else if(opcode == OP_WAIT) {
			CON_getch();
		} else if(opcode == OP_GET) {/*OP_GET c0 .. cn OP_KEY -- GET [...]*/
			CurClip->fgColor = D_24F0;
			/*wait for one of the allowed keys*/
			for(idx = 0; (opcode = PARSE_U8) != OP_KEY; idx ++)
				TalkKeyword[idx] = opcode;
			Talk_PC --;
			TalkKeyword[idx] = 0;
			do {
				opcode = CON_getch();
				for(idx = 0; TalkKeyword[idx] && toupper(TalkKeyword[idx]) != opcode; idx++);
			} while(!TalkKeyword[idx]);
			if(opcode >= ' ')
				CON_printf("%c", opcode);
			TalkInput[0] = opcode;
			TalkInput[1] = 0;
			CON_printf("\n");
			CurClip->fgColor = D_24E6;
			TalkAnswered = 0;
		} else if(opcode == OP_KEY) {
			CON_printf("\nCommand for input key expected.");
			break;
		} else if(opcode == OP_SCRIPT_MARK) {
			break;
		} else if(opcode == OP_ENDRES) {
			/**/
		} else {
			CON_printf("\nLogical error occured.");
			break;
		}
		/*$Z is the last player's input*/
		VarStr['Z' - 0x37] = TalkInput;

		if(stricmp(TalkInput, "look") == 0) {
			/*replay the description, then resume*/
			CON_printf("You see ");
			Talk_PC = D_E678;
			parse_statement(TalkNpcs[0]);
			Talk_PC = TalkSavedPC;
		} else {
			parse_statement(TalkNpcs[0]);
		}
	}
	mustLeave = 0;
	IsInConversation = 0;
	CON_printf("\n\nPress any key...");
	PromptCh = 1;
	CON_getch();
	PromptCh = 5;
	/*back to the normal display*/
	CON_setClip(savedClip);
	EndPagingAtPrompt = 1;
	D_B6B6 = savedB6B6;
	AnimateAtPrompt = 1;
	if(InCombat) {
		GR_PUT_TILE_SCREEN(0x177, 0x40, 0);
		GR_PUT_TILE_SCREEN(0x178, 0x50, 0);
	} else {
		GR_PUT_TILE_SCREEN(0x173, 0x40, 0);
		GR_PUT_TILE_SCREEN(0x174, 0x50, 0);
	}
	VIS_UpdateView();
	ScreenFade = 1;
	DrawMapWindow(0);
	CON_printf("\n");
	/*restore the status display (the displayed member may have left)*/
	if(PartySize == StatusMember) {
		StatusMember = 0;
		PartyListTop = 0;
		StatusDisplay = 0x91;
	} else {
		StatusDisplay = savedStatus;
		StatusMember = savedMember;
	}
	StatusDirty ++;
}
