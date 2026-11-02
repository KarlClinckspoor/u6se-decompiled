/*
	Game state block saved in the savegame: everything from IsOnQuest to
	GameStateEnd (excluded) is read and written as one block, so the order and
	sizes of these variables must not change. A data-only module, linked
	between gfx and kbd. Ultima VI's gamestat.c with Savage Empire's initial
	values; the names are U6's, and some of these variables (moonstones, moons,
	karma, keg, Gargish) may mean something else in Savage Empire or be unused.
*/
#include "se.h"

/*27E8*/unsigned char IsOnQuest = 0;
/*27E9*/unsigned char NextSleep = 0;
/*27EA*/unsigned char Time_M = 0;/*minutes*/
/*27EB*/unsigned char Time_H = 8;/*hour of day*/
/*27EC*/unsigned char Date_D = 1;/*day of month*/
/*27ED*/unsigned char Date_M = 1;/*month*/
/*27EE*/uint16 Date_Y = 0;/*year*/
/*27F0*/unsigned char KARMA = 0x4B;
/*27F1*/char WindDir = 0;
/*27F2*/unsigned char Active = 0;
/*27F3*/char AmbientLight = 7;/*ambient (sun) light level 0..7, see PassTime*/
/*27F4*/int16 MapX = 0x133;
/*27F6*/int16 MapY = 0x160;
/*27F8*/int16 MapZ = 0;
/*27FA*/unsigned char SpellFx[16] = {0};
/*280A*/int16 VanishedShape = 0;/*shape/type of the last "Vanish"ed object, for "Reappear"*/
/*280C*/unsigned char HourlyUnused = 0;/*reset every hour, never read*/
/*280D*/unsigned char CaughtFish = 0;
/*280E*/unsigned char DrunkCounter = 0;
/*280F*/unsigned char MustRingBell = 0;
/*2810*/int16 SoundFlag = 1;
/*2812*/int16 MoonstonePos[8][3] = {/*U6's moonstones x, y, z (the same values in Savage Empire, maybe unused)*/
	0x3A7,0x106,0,
	0x1F7,0x166,0,
	0x09F,0x3AE,0,
	0x127,0x026,0,
	0x33F,0x0A6,0,
	0x147,0x336,0,
	0x017,0x016,1,
	0x397,0x3A6,0
};
/*2842*/int16 KegFuse = 0;/*lit powder keg: minutes before explosion*/
/*2844*/int16 KegObject = 0;/*lit powder keg object*/
/*2846*/int16 SpeakGargish = 0;/*the Avatar speaks Gargish (conversation variable 'L')*/
/*2848*/unsigned char PathObject[8] = {0};
/*2850*/unsigned char PathCounter[8] = {0};
/*2858*/unsigned char PathTries[8] = {0};
/*2860*/char InCombat = 1;
/*2861*/char SoloMember = -1;/*solo mode: party index of the solo member, -1 = party mode*/
/*2862*/char SavedNPCMode = 0;/*NPCMode of the commanded member before it got AI_COMMAND*/
/*2863*/unsigned char DefaultCommand = 0xFF;
/*2864*/unsigned char TrammelPhase = 0;/*moon 1 (Trammel?) phase 0..7*/
/*2865*/unsigned char TrammelPos = 0;/*moon 1 position in the sky 0..23 (visible if <= 14)*/
/*2866*/unsigned char FeluccaPhase = 0;/*moon 2 (Felucca?) phase 0..7*/
/*2867*/unsigned char FeluccaPos = 0;/*moon 2 position in the sky 0..23 (visible if <= 14)*/
/*2868*/unsigned char AvatarFemale = 0;/*avatar's sex (!= 0: female, "milady")*/
/*2869*/unsigned char AvatarPortrait = 0;/*avatar's portrait + 1 (0: no character created yet)*/
/*286A*/int16 GameStateEnd = 0;/*end marker*/
