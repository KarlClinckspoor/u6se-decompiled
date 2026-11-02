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

extern int16 CON_mouseOffAt(int16, int16, int16, int16);/*console: saves something for the screen area (0,0)-(319,199), returns a handle for CON_mouseOn*/
extern CON_mouseOn(int16);/*console: restores what CON_mouseOffAt saved*/
extern cdecl CON_printf(char far *, ...);/*CON_printf*/
extern TalkDriver(int16 talker, int16 objNum);/*TalkDriver: runs the conversation script of objNum*/
extern char *TALK_GetCharName(int16 objNum);/*TALK_GetCharName*/
extern char *GetObjectString(int16 objNum);/*GetObjectString*/

extern unsigned char Time_H;/*Time_H*/
extern unsigned char Date_D;/*Date_D*/
extern unsigned char Date_M;/*Date_M*/
extern int16 MapX, MapY, MapZ;/*MapX, MapY, MapZ*/
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
	D_B5FA['D' - 0x37] = Date_D;/*[d]ay*/
	D_B5FA['H' - 0x37] = Time_H;/*[h]our*/
	D_B5FA['M' - 0x37] = Date_M;/*[m]onth*/
	if(objNum >= 0x40)
		D_B5FA['N' - 0x37] = ((objNum - 0x40) & 7) + 1;
	else
		D_B5FA['N' - 0x37] = 0;
	D_B5FA['O' - 0x37] = D_9CF6 - 1;/*number of [o]ther party members*/
	D_B5FA['W' - 0x37] = D_4706[objNum];/*NPC's [w]ork type*/
	D_B5FA['T' - 0x37] = 0;
	D_B5FA['X' - 0x37] = MapX;
	D_B5FA['Y' - 0x37] = MapY;
	D_B5FA['Z' - 0x37] = MapZ;
	/*-- reset $0~$9 --*/
	for(i = 0; i < 10; i ++)
		D_B66E[i] = D_E6F2[i];
	/*-- reset #0~#9 --*/
	for(i = 0; i < 10; i ++)
		D_B5FA[i] = 0;
	/*--*/
	handle = CON_mouseOffAt(0, 0, 319, 199);
	TalkDriver(1, objNum);
	CON_mouseOn(handle);
}

/*"talk" command: party member #partyId (unused) talks to objNum;
  showName: print the target's name first*/
/*C_0F79_00E0*/TALK_talkTo(int16 partyId, int16 objNum, int16 showName) {
	int16 objTyp;

	objTyp = GetType(objNum);
	if(objNum >= 0 && objNum < 0x100) {
		if(showName)
			CON_printf("%s\n", (D_6EFC[objNum] & 1)?
				/*met already*/TALK_GetCharName(objNum):
				/*first time*/GetObjectString(objNum)
			);
		TALK_initTalk(objNum);
	} else {
		CON_printf("nothing!\n");
	}
}
