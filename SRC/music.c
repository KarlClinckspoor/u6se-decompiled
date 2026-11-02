/*
 * The Savage Empire (Worlds of Ultima) - GAME.EXE
 * Music/sound module, from Ultima VI's music.c (rewritten).
 *
 * Savage Empire plays its music and sound effects through its own library
 * (../SRC_S/music.c, frame 2B5C: MIDI songs and 8-byte step sound effects on
 * a Roland or AdLib card through STRAX.DRV). This module is the game's side:
 * MUS_InitDriver() loads the 18 songs of music.lzc into (expanded) memory
 * (MUS_Songs) and starts the sound driver; MUS_PlaySong() / MUS_PlayAreaSong()
 * choose the song; a few functions play a jingle, wait (up to 100 short
 * delays) until the music reaches its marker, then queue the area's song;
 * MUS_PlaySfx() plays one of the 34 sound effects of MUS_Sfx.
 * MusicAdapter: 0 none, 1 a music card (soundCard: 1 Roland, 2 AdLib, 3 the
 * third driver). musicMarker - 1 is the song being played (the songs set it).
 * Songs (by use): 0 and 1 overland, 2 underground, 3 MapZ 3, 4 dawn,
 * 5 combat queue, 6 and 7 jingles, 8 after the jingles, 9/10 by CombatOutcome's sign,
 * 11 and 12 the two battle jingles, 11 also when the party is knocked out.
 */
#include "se.h"

/*text window (Ultima VI's)*/
struct tClipInfo/*size:9*/ {
	unsigned char originX, originY, cols, rows;
	unsigned char curX, curY;
	unsigned char fgColor, bgColor;
	unsigned char attrFlags;/*1 wrap, 2 center, 4 capitalize, 8 runes, 0x10 paginate, 0x20 scroll*/
};

/*a step of a sound effect (8 bytes, the library's)*/
struct tSfxStep {
	unsigned char flags;/*_00*//*1: go on with the next step, 2: glide to target, 4: hold, 8: repeat*/
	unsigned char program;/*_01*//*1+: MIDI program + 1; 0: the end*/
	unsigned char note;/*_02*/
	unsigned char velocity;/*_03*/
	int16 duration;/*_04*//*in timer ticks*/
	unsigned char target;/*_06*//*the note a glide goes to*/
	unsigned char pad;/*_07*/
};

/*OSI routines (asm)*/
extern int16 C_2B57_001F(int16, int16);/*OSI_rand(lo, hi)*/
extern cdecl MOUSE_SERVE();/*MOUSE_SERVE*/

/*other game modules*/
extern int16 IsZFile;/*IsZFile*/
extern int16 MusicAdapter;/*MusicAdapter*/
extern unsigned char MouseOn;/*MouseOn*/
extern char StatusDirty;
extern int16 IdleAnimTick;
extern int16 MapAnimEnabled;/*MapAnimEnabled*/
extern char AnimateAtPrompt;/*AnimateAtPrompt*/
extern int16 EndPagingAtPrompt;
extern struct tClipInfo *CurClip;/*CurClip*/
extern int16 CombatOutcome;
extern unsigned char Time_H;/*Time_H*/
extern unsigned char Active;/*Active*/
extern int16 MapZ;/*MapZ*/
extern int16 SoundFlag;/*sound on*/
extern char InCombat;/*InCombat*/
extern char far *D_3EB6;/*the .lzc loader's work area*/
extern PaletteAnimation();/*PaletteAnimation*/
extern DisplayTick();/*DisplayTick*/
extern RefreshStatus();
extern CON_putch0(int16);/*CON_putch0*/
extern cdecl CON_printf(char far *, ...);/*CON_printf*/
extern C_08CB_0C73();
extern ResetMapView();
extern DISK_confirm(int16);/*DISK_confirm*/
extern char far UnconsciousMsg[];/*UnconsciousMsg*/
extern char far AwakenAtIntanyaMsg[];/*AwakenAtIntanyaMsg*/

/*Savage Empire's library (cdecl)*/
extern int16 cdecl soundCard;
extern int16 cdecl musicMarker;
extern int16 cdecl emsInUse;
extern int16 cdecl osiError;
extern void far * cdecl PlaySound(struct tSfxStep far *steps, int16 flags, int16 volume, int16 pan, int16 id, int16 prio);
extern void cdecl StopAllSounds(void);
extern void cdecl FadeOutMusic(int16 time);
extern void cdecl InitSound(char far *name);
extern int16 cdecl PlaySong(void far *song);
extern void cdecl FadeToSong(void far *song, int16 time, int16 flags);
extern void cdecl SetNextSong(void far *song, int16 flags);
extern void cdecl TimerDelay(int32 t);/*wait (2383 per millisecond)*/
extern void far * cdecl MEM_alloc(int32 size, int16 flags);
extern int32 LIB_entrySize(char far *name, int16 index);
extern void far *LZC_load(char far *name, int16 index, void far *out, int16 memflags, char far *work);

/*uninitialised data*/
extern void far *D_ECBC[18];/*MUS_Songs: the songs of music.lzc*/

/*2364*/int16 MUS_MaxAmbient = 1;/*Ultima VI's (max ambient sounds per tick); not used*/

/*the sound effects (MUS_PlaySfx): one step each*/
/*2366*/struct tSfxStep MUS_Sfx[34] = {
	{0, 0x04, 0x3C, 0x40, 0x3C, 0, 0},
	{0, 0x05, 0x30, 0x40, 0x3C, 0, 0},
	{0, 0x06, 0x3C, 0x40, 0x0A, 0, 0},
	{0, 0x07, 0x3C, 0x40, 0x0A, 0, 0},
	{0, 0x08, 0x3C, 0x40, 0x0A, 0, 0},
	{0, 0x09, 0x3C, 0x40, 0x0A, 0, 0},
	{0, 0x0A, 0x3C, 0x40, 0x3C, 0, 0},
	{0, 0x0B, 0x3C, 0x40, 0x0A, 0, 0},
	{0, 0x0C, 0x3C, 0x40, 0x0A, 0, 0},
	{0, 0x0D, 0x3C, 0x40, 0x1E, 0, 0},
	{0, 0x0E, 0x3C, 0x40, 0x3C, 0, 0},
	{0, 0x0F, 0x45, 0x40, 0x3C, 0, 0},
	{0, 0x10, 0x3C, 0x40, 0x3C, 0, 0},
	{0, 0x11, 0x3C, 0x40, 0x3C, 0, 0},
	{0, 0x12, 0x3C, 0x40, 0x3C, 0, 0},
	{0, 0x13, 0x3C, 0x40, 0x3C, 0, 0},
	{0, 0x14, 0x3C, 0x40, 0x3C, 0, 0},
	{0, 0x15, 0x3C, 0x40, 0x3C, 0, 0},
	{0, 0x16, 0x24, 0x40, 0x3C, 0, 0},
	{0, 0x17, 0x3C, 0x40, 0x3C, 0, 0},
	{0, 0x18, 0x3C, 0x40, 0x3C, 0, 0},
	{0, 0x29, 0x3C, 0x40, 0x3C, 0, 0},
	{0, 0x2A, 0x3C, 0x40, 0x3C, 0, 0},
	{0, 0x2B, 0x3C, 0x40, 0x3C, 0, 0},
	{0, 0x2C, 0x3C, 0x40, 0x3C, 0, 0},
	{0, 0x2D, 0x3C, 0x40, 0x3C, 0, 0},
	{2, 0x2E, 0x30, 0x40, 0x5A, 0, 0},
	{0, 0x2F, 0x3C, 0x40, 0x3C, 0, 0},
	{0, 0x30, 0x3C, 0x40, 0x3C, 0, 0},
	{0, 0x3C, 0x4D, 0x40, 0x5A, 0, 0},
	{0, 0x3D, 0x3C, 0x40, 0x3C, 0, 0},
	{0, 0x3E, 0x35, 0x40, 0x3C, 0, 0},
	{0, 0x3F, 0x3C, 0x40, 0x3C, 0, 0},
	{0, 0x40, 0x3C, 0x40, 0x3C, 0, 0}
};

extern int16 MUS_LoadSongs();

/*init the sound: with a music card, loads the songs (needs expanded memory),
then starts the sound driver with the timbres of savage.tim*/
/*C_2701_000A*/MUS_InitDriver() {
	if(!SoundFlag) {
		soundCard = 0;
	} else {
		if(soundCard == 2 || soundCard == 1) {
			DISK_confirm(7);
			if(emsInUse == 0 || MUS_LoadSongs() == 0) {
				MusicAdapter = 0;
				CON_printf("No EMS found; no music possible.\n");
			}
		}
		DISK_confirm(5);
		if(soundCard == 1) {
			CON_printf("Initializing Roland...");
			CON_putch0(-1);
		}
		InitSound("savage.tim");
		if(soundCard == 1)
			CON_printf("done!\n");
	}
}

/*loads the 18 songs of music.lzc into MUS_Songs (expanded memory);
after a failure the next ones are not loaded. Returns 0 on failure*/
/*C_2701_009B*/int16 MUS_LoadSongs() {
	int32 size;
	int16 i;
	int16 failed = 0;

	DISK_confirm(7);
	for(i = 0; i < 18; i++) {
		if(failed)
			D_ECBC[i] = 0;
		size = LIB_entrySize("music.lzc", i);
		if(osiError || size == -1L) {
			failed = 1;
			D_ECBC[i] = 0;
		}
		if((D_ECBC[i] = MEM_alloc(size, 4)) == 0) {
			failed = 1;
			D_ECBC[i] = 0;
		}
		LZC_load("music.lzc", i, D_ECBC[i], 0, D_3EB6);
		if(osiError) {
			failed = 1;
			D_ECBC[i] = 0;
		}
	}
	return !failed;
}

/*plays song <song> (fading the current one out), or fades the music out
(-1)*/
/*C_2701_0190*/MUS_PlaySong(register int16 song) {
	if(MusicAdapter) {
		if(song == -1)
			FadeOutMusic(60);
		else if(musicMarker - 1 == -1)
			PlaySong(D_ECBC[song]);
		else
			FadeToSong(D_ECBC[song], 60, 0);
	}
}

/*plays the song of the area: 3 on level 3, 2 underground, 8 in combat,
else 0 or 1 at random*/
/*C_2701_01F1*/MUS_PlayAreaSong() {
	int16 song;

	if(MusicAdapter) {
		if(MapZ == 3)
			song = 3;
		else if(MapZ > 0)
			song = 2;
		else if(InCombat)
			song = 8;
		else if(C_2B57_001F(0, 1) == 0)
			song = 0;
		else
			song = 1;
		MUS_PlaySong(song);
	}
}

/*on the surface between 5 and 11 o'clock: plays the dawn song (4), waits for
it to reach its marker, then queues song 1*/
/*C_2701_023F*/C_2701_023F() {
	int16 i = 0;

	if(!MusicAdapter)
		return;
	if(Time_H <= 11 && Time_H >= 5 && MapZ == 0) {
		FadeToSong(D_ECBC[4], 60, 0);
		while(musicMarker - 1 != 4 && ++i < 100) {
			IdleAnimTick = 1;
			if(AnimateAtPrompt) {
				DisplayTick();
				PaletteAnimation();
			}
			TimerDelay(150000L);
			if(MouseOn)
				MOUSE_SERVE();
		}
		SetNextSong(D_ECBC[1], 0);
	}
}

/*plays jingle 7, waits for it, then queues song 8*/
/*C_2701_02D6*/C_2701_02D6() {
	int16 i = 0;

	if(MusicAdapter) {
		FadeToSong(D_ECBC[7], 15, 0);
		while(musicMarker - 1 != 7 && ++i < 100) {
			IdleAnimTick = 1;
			if(AnimateAtPrompt) {
				DisplayTick();
				PaletteAnimation();
			}
			TimerDelay(150000L);
			if(MouseOn)
				MOUSE_SERVE();
		}
		SetNextSong(D_ECBC[8], 0);
	}
}

/*end of a battle(?): <how> < 0 plays jingle 11, > 0 jingle 12, waits for it
and queues the area song (3 on level 3, 2 underground, else 0 or 1);
<how> 0 fades to the area song*/
/*C_2701_0355*/C_2701_0355(int16 how) {
	int16 jingle;
	int16 song;
	int16 i = 0;

	if(!MusicAdapter)
		return;
	if(MapZ == 3)
		song = 3;
	else if(MapZ > 0)
		song = 2;
	else if(C_2B57_001F(0, 1) == 0)
		song = 0;
	else
		song = 1;
	if(how < 0)
		jingle = 11;
	else if(how > 0)
		jingle = 12;
	else
		jingle = -1;
	if(jingle >= 0) {
		FadeToSong(D_ECBC[jingle], 30, 0);
		while(musicMarker - 1 != jingle && ++i < 100) {
			IdleAnimTick = 1;
			if(AnimateAtPrompt) {
				DisplayTick();
				PaletteAnimation();
			}
			TimerDelay(150000L);
			if(MouseOn)
				MOUSE_SERVE();
		}
		SetNextSong(D_ECBC[song], 0);
	} else {
		FadeToSong(D_ECBC[song], 120, 0);
	}
}

/*doubles CombatOutcome; while song 8 plays, queues song 9 or 10 by its sign*/
/*C_2701_045E*/C_2701_045E() {
	CombatOutcome *= 2;
	if(MusicAdapter && musicMarker - 1 == 8) {
		if(CombatOutcome < 0)
			SetNextSong(D_ECBC[9], 4);
		else
			SetNextSong(D_ECBC[10], 4);
	}
}

/*out of combat, while one of the songs 0-4 plays: queues song 5*/
/*C_2701_04A7*/C_2701_04A7() {
	if(MusicAdapter && InCombat != 1 && musicMarker - 1 <= 4)
		SetNextSong(D_ECBC[5], 4);
}

/*plays jingle 6, waits for it, then queues song 8*/
/*C_2701_04D7*/C_2701_04D7() {
	int16 i = 0;

	if(MusicAdapter) {
		FadeToSong(D_ECBC[6], 15, 0);
		while(musicMarker - 1 != 6 && ++i < 100) {
			IdleAnimTick = 1;
			if(AnimateAtPrompt) {
				DisplayTick();
				PaletteAnimation();
			}
			TimerDelay(150000L);
			if(MouseOn)
				MOUSE_SERVE();
		}
		SetNextSong(D_ECBC[8], 0);
	}
}

/*the party is knocked out: "Overcome by your wounds...", song 11, then the
party awakens in Intanya's hut (C_08CB_0C73)*/
/*C_2701_0556*/C_2701_0556() {
	int16 song;
	int16 i = 0;

	if(Active == 0x10)
		return;
	if(MusicAdapter)
		FadeOutMusic(90);
	CurClip->attrFlags |= 0x10;
	CON_printf(UnconsciousMsg);
	MapAnimEnabled = 1;
	StatusDirty = 1;
	RefreshStatus();
	ResetMapView();
	AnimateAtPrompt = 0;
	if(MusicAdapter) {
		PlaySong(D_ECBC[11]);
		if(MapZ == 3)
			song = 3;
		else if(MapZ > 0)
			song = 2;
		else
			song = 1;
		while(musicMarker - 1 != 11 && ++i < 100) {
			TimerDelay(150000L);
			if(MouseOn)
				MOUSE_SERVE();
		}
		SetNextSong(D_ECBC[song], 0);
	}
	CON_printf(AwakenAtIntanyaMsg);
	C_08CB_0C73();
	EndPagingAtPrompt = 1;
	AnimateAtPrompt = 1;
}

/*plays sound effect <sfx> (0-33) when the sound is on*/
/*C_2701_0648*/MUS_PlaySfx(int16 sfx) {
	if(SoundFlag && sfx < 34)
		PlaySound(&(MUS_Sfx[sfx]), 0, 0x7f, 0x3f, 0, 2);
}

/*stops the sound effects; with the sound off, the music too*/
/*C_2701_0684*/C_2701_0684() {
	if(soundCard) {
		StopAllSounds();
		if(!SoundFlag)
			FadeOutMusic(0);
	}
}
