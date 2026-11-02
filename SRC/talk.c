/*
 * The Savage Empire (Worlds of Ultima) - GAME.EXE
 * Conversation entry points, from Ultima VI's talk.c.
 *
 * TALK_talkTo checks that the target is a character (object 0-255) and
 * prints its name; TALK_initTalk sets the conversation script variables
 * ($N $P strings, #D #H #M #N #O #T #W #X #Y #Z integers, and clears
 * $0-$9/#0-#9) before running the script interpreter (TalkDriver).
 * Variables are indexed 0-9 for digits, then 'A' - 0x37 = 10 ... 'Z' = 35.
 */
#include "se.h"

/*text window (Ultima VI's)*/
struct tClipInfo/*size:9*/ {
	unsigned char originX, originY, cols, rows;
	unsigned char curX, curY;
	unsigned char fgColor, bgColor;
	unsigned char attrFlags;
};

extern int16 C_05D0_0004(int16, int16, int16, int16);/*console: saves something for the screen area (0,0)-(319,199), returns a handle for C_05D0_004D*/
extern C_05D0_004D(int16);/*console: restores what C_05D0_0004 saved*/
extern cdecl C_05D0_2DB9(char far *, ...);/*CON_printf*/
extern C_0F8D_20C8(int16 talker, int16 objNum);/*TalkDriver: runs the conversation script of objNum*/
extern char *C_0F8D_011C(int16 objNum);/*TALK_GetCharName*/
extern char *C_0A5D_3A76(int16 objNum);/*GetObjectString*/

extern unsigned char D_27EB;/*Time_H*/
extern unsigned char D_27EC;/*Date_D*/
extern unsigned char D_27ED;/*Date_M*/
extern int16 D_27F4, D_27F6, D_27F8;/*MapX, MapY, MapZ*/
extern unsigned char far *D_4706;/*NPCMode*/
extern char D_5104[];/*Names[0]?: the player's name*/
extern unsigned char D_6EFC[];/*TalkFlags: bit 0, already met*/
extern unsigned char D_9CF6;/*PartySize*/
extern uint16 D_9CFA[];/*ObjShapeType*/
extern int16 D_B5FA[10+26];/*VarInt*/
extern char *D_B66E[10+26];/*VarStr*/
extern char D_E6F2[10][100];/*TalkStrBuf: storage of the string variables $0-$9*/

#define GetType(i) (D_9CFA[i] & 0x3ff)

/*start a conversation between the player and NPC objNum*/
/*C_0F79_000E*/TALK_initTalk(int16 objNum) {
	int16 handle;
	int16 i;

	/*set strings $*/
	D_B66E['N' - 0x37] = "Winona Ryder is a really hot babe";/*[n]ame of NPC (a buffer)*/
	D_B66E['P' - 0x37] = D_5104;/*name of [p]layer*/
	/*set variables #*/
	D_B5FA['D' - 0x37] = D_27EC;/*[d]ay*/
	D_B5FA['H' - 0x37] = D_27EB;/*[h]our*/
	D_B5FA['M' - 0x37] = D_27ED;/*[m]onth*/
	if(objNum >= 0x40)
		D_B5FA['N' - 0x37] = ((objNum - 0x40) & 7) + 1;
	else
		D_B5FA['N' - 0x37] = 0;
	D_B5FA['O' - 0x37] = D_9CF6 - 1;/*number of [o]ther party members*/
	D_B5FA['W' - 0x37] = D_4706[objNum];/*NPC's [w]ork type*/
	D_B5FA['T' - 0x37] = 0;
	D_B5FA['X' - 0x37] = D_27F4;
	D_B5FA['Y' - 0x37] = D_27F6;
	D_B5FA['Z' - 0x37] = D_27F8;
	/*-- reset $0~$9 --*/
	for(i = 0; i < 10; i ++)
		D_B66E[i] = D_E6F2[i];
	/*-- reset #0~#9 --*/
	for(i = 0; i < 10; i ++)
		D_B5FA[i] = 0;
	/*--*/
	handle = C_05D0_0004(0, 0, 319, 199);
	C_0F8D_20C8(1, objNum);
	C_05D0_004D(handle);
}

/*"talk" command: party member #partyId (unused) talks to objNum;
  showName: print the target's name first*/
/*C_0F79_00E0*/TALK_talkTo(int16 partyId, int16 objNum, int16 showName) {
	int16 objTyp;

	objTyp = GetType(objNum);
	if(objNum >= 0 && objNum < 0x100) {
		if(showName)
			C_05D0_2DB9("%s\n", (D_6EFC[objNum] & 1)?
				/*met already*/C_0F8D_011C(objNum):
				/*first time*/C_0A5D_3A76(objNum)
			);
		TALK_initTalk(objNum);
	} else {
		C_05D0_2DB9("nothing!\n");
	}
}
