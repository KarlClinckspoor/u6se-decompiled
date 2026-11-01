/* flags: -mm -c -O -G -Z -d */
/*
 * Worlds of Ultima: The Savage Empire, SAVAGE.EXE (and GAME.EXE, frame 2B5C)
 * segment 08E4: music and sound effects through the STRAX.DRV sound driver.
 * Plays standard MIDI files (MThd/MTrk, format 0-2, up to 32 tracks) from a timer
 * interrupt, with Origin's 0xFE loop events, and sound effects ("voices": sequences of
 * 8-byte steps) on MIDI channels 2-9, borrowed from the music and given back.
 */
#include <dos.h>

#undef MK_FP/*Turbo C 2.0's*/
#define MK_FP(seg, ofs) ((void far *)(((unsigned long)(seg) << 16) | (unsigned)(ofs)))

/*a step of a sound effect (8 bytes)*/
struct tSfxStep {
	unsigned char flags;/*_00*//*1: go on with the next step, 2: glide to target, 4: hold, 8: repeat*/
	unsigned char program;/*_01*//*1+: MIDI program + 1; 0: the end*/
	unsigned char note;/*_02*/
	unsigned char velocity;/*_03*/
	int duration;/*_04*//*in timer ticks*/
	unsigned char target;/*_06*//*the note a glide goes to*/
	unsigned char pad;/*_07*/
};

/*a sound effect being played (14 bytes)*/
struct tVoice {
	unsigned char flags;/*_00*//*1: stereo (two channels), 2: keep the channel at the end*/
	unsigned char chan1;/*_01*//*MIDI channel (left); 0: the voice is free*/
	unsigned char chan2;/*_02*//*right channel (stereo)*/
	unsigned char note;/*_03*/
	unsigned char volume;/*_04*/
	unsigned char pan;/*_05*/
	int id;/*_06*/
	int counter;/*_08*//*ticks left of the step*/
	struct tSfxStep far *step;/*_0A*/
};

/*what the driver's init entry returns*/
struct tDrvInfo {
	unsigned char unk00;/*_00*/
	unsigned char ok;/*_01*//*0: the timbre bank is not kept*/
	char unk02[8];/*_02*//*given to the interrupt hook with the driver's timer routine*/
	void far *cb1;/*_0A*//*callbacks given to the driver*/
	void far *cb2;/*_0E*/
};

typedef void (far pascal *tDrv3)(int, int, int);
typedef long (far pascal *tDrv3L)(int, int, int);/*see SetSoundParam*/
typedef void (far pascal *tDrv2)(int, int);
typedef void (far pascal *tDrv1)(void far *);
typedef void (far pascal *tDrv0)(void);
typedef struct tDrvInfo far *(far pascal *tDrvInit)(void);

/*other modules*/
extern void far * pascal C_0C58_0305(char far *, int, long, int, long);/*(name, index, 0L, flags, 0L): load entry index of a file into memory*/
extern C_0BC5_0126(void far *);/*free memory loaded by C_0C58_0305*/
extern void far *C_0AD2_0024(void far *);/*maps (expanded memory) data, returns its address*/
extern C_0AD2_00A7(void);/*saves the expanded memory mapping (0: failed)*/
extern C_0AD2_00DD(void);/*restores it*/
extern C_0CED_000E(int, void far *, void *, void far *);/*(vector, routine, save, chain): hook an interrupt*/
extern C_0CED_00A5(int);/*sets the timer rate (Hz)*/
extern C_0CED_0187(void);/*unhooks the interrupts*/
extern C_0D11_0102();/*a callback given to the driver*/
extern int D_23D4;/*expanded memory in use*/

/*uninitialised data*/
extern tDrv2 D_2814;/*driver: program change (channel, program)*/
extern void far *D_2B2A;/*driver entry +14h*/
extern tDrvInit D_2B6E;/*driver: init*/
extern tDrv3 D_2B9E;/*driver: note (channel, note, velocity; 0: off)*/
extern tDrv1 D_2BAE;/*driver: load timbre bank*/
extern unsigned far *D_2BC2;/*STRAX.DRV in memory: entry offsets at +2..+14h*/
extern tDrv0 D_2BD2;/*driver: shutdown*/
extern void far *D_2BE0;/*driver: timer routine*/
extern void far *D_34E4;/*driver entry +12h*/
extern struct tDrvInfo far *D_34E8;/*what the driver's init returned*/
extern tDrv3 D_34F6;/*driver: controller (channel, controller, value)*/
extern tDrv3 D_34FA;/*driver: pitch bend (channel, lsb, msb)*/
extern int D_3EEA[32];/*the track has ended*/
extern unsigned char far *D_3F2A[32];/*where each track is*/
extern char D_3FAA[8];/*hook of the driver's timer routine*/
extern void interrupt (far *D_3FB2)();/*the timer interrupt's previous routine*/
extern void far *D_3FB6;/*the timbre bank in memory*/
extern unsigned char D_3FBA[32];/*running status of each track*/
extern int D_3FDA;/*MThd: ticks per quarter note*/
extern char D_3FDC[8];/*hook of the timer routine*/
extern int D_3FE4;/*MThd: number of tracks*/
extern long D_3FE6[32];/*ticks until each track's next event*/
extern int D_4066;/*MThd: format*/

/*08E4's initialised data*/
/*20CA*/int soundCard = 0;/*0: none, 1: Roland, 2: AdLib, 3: the third driver (timer at 7680 Hz); STRAX.DRV entry soundCard - 1*/
/*20CC*/int sfxReady = 0;/*the sound effects may be updated (0 while they are changed)*/
/*20CE*/int musicMarker = 0;/*the last 0xFE type 3 event's value*/
/*20D0*/int musicCondition = 0;/*value the 0xFE type 4 and 5 events test*/
/*20D2*/int D_20D2 = 1;/*1 when a song starts, 0 when the next song is set (not read here)*/
/*20D4*/int musicNotesOn = 1;/*music notes are played*/
/*20D6*/int musicVolume = 0x100;/*music volume (0x100: full)*/
/*20D8*/int musicVolumeSet = 0x100;/*music volume set (fades go back to it)*/
/*20DA*/int fadeStep = 0;/*fade step*/
/*20DC*/int fadeLevel = 0;/*fade level*/
/*20DE*/int fadeNoStop = 0;/*the fade does not stop the music*/
/*20E0*/int D_20E0 = 0;/*the next song is set: 0xFE loops of type 0 are skipped, type 1 ends the song*/
/*20E2*/void far *nextSong = 0;/*the song to play when the music stops*/
/*20E6*/void far *curSong = 0;/*the song playing*/
/*20EA*/struct tVoice voices[32] = {0};/*the sound effects*/
/*22AA*/unsigned char chanProgram[11] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};/*program of each channel*/
/*22B5*/unsigned char chanVolume[11] = {0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f};/*the music's volume of each channel*/
/*22C0*/unsigned char chanPan[11] = {0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40};/*the music's pan of each channel*/
/*22CB*/unsigned char chanMusicProgram[11] = {0};/*the music's program of each channel*/
/*22D6*/unsigned char chanList[13] = {0, 1, 3, 4, 5, 6, 7, 8, 9, 11, 10, 2, 12};/*channels 2-9 in two lists: free (from 11), in use (from 12), oldest first*/
/*22E3*/signed char chanPriority[13] = {0};/*priority of the sound effect on each channel*/
/*22F0*/unsigned char chanUser[13] = {0};/*channel: 0 free, 1 music, 2 sound effect*/
/*22FD*/unsigned long tickTime = 10000;/*time of a MIDI tick*/
/*2301*/long midiTime = 0;/*MIDI time to play (0x10000 a timer tick)*/
/*2305*/unsigned char drumChan[77] = {/*AdLib: the channel (10-26, see InitSound) of each percussion note*/
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 10, 10, 18, 11, 0, 12, 13, 17, 13, 16, 13, 14, 13, 13, 15, 13, 19, 0, 0,
	0, 0, 21, 0, 0, 0, 26, 26, 25, 20, 20, 0, 0, 21, 21, 22, 23, 0, 0, 24, 0, 20, 0
};
/*2352*/unsigned char drumNote[77] = {/*AdLib: the note played for each percussion note*/
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 48, 48, 48, 48, 0, 48, 42, 71, 42, 71, 47, 71, 47, 52, 79, 52, 77, 0, 0,
	0, 0, 71, 0, 0, 0, 72, 79, 79, 64, 58, 0, 0, 89, 84, 48, 72, 0, 0, 36, 0, 96, 0
};
/*239F*/int musicStopped = 1;/*the music is stopped*/
/*23A1*/int musicFlags = 0;/*music flags: 1 looped or ended, 2 started, 4 keep the next song*/

void interrupt SoundTimer(void);
PlaySong(void far *);

/*puts channel a in a list before channel b*/
/*C_08E4_000F*/void ChanInsert(register int a, register int b)
{
	register int i;

	for (i = 0; i < 13; i++) {
		if (chanList[i] == b) {
			chanList[i] = a;
			chanList[a] = b;
			break;
		}
	}
}

/*a sound effect of priority prio takes channel ch: moves it to the end of the list in use*/
/*C_08E4_0040*/void ChanTake(int ch, int prio)
{
	register int i;

	if (ch >= 2 && ch <= 9) {
		for (i = 0; i < 13; i++) {
			if (chanList[i] == ch) {
				chanList[i] = chanList[ch];
				chanList[ch] = ch;
				ChanInsert(ch, 12);
				chanPriority[ch] = prio;
				break;
			}
		}
		if (sfxReady) {
			sfxReady = 0;
			for (i = 0; i < 32; i++) {
				if (voices[i].chan1 == ch)
					voices[i].chan1 = 0;
			}
			sfxReady = 1;
		}
	}
}

/*frees channel ch: notes off, back to the end of the free list*/
/*C_08E4_00D4*/void ChanFree(int ch)
{
	register int i;

	if (ch >= 2 && ch <= 9) {
		disable();
		(*D_34F6)(ch, 0x7b, 0);
		enable();
		chanUser[ch] = 0;
		for (i = 0; i < 13; i++) {
			if (chanList[i] == ch) {
				chanList[i] = chanList[ch];
				chanList[ch] = ch;
				ChanInsert(ch, 11);
				break;
			}
		}
	}
}

/*finds a channel for a sound effect of priority prio: a free one, or the oldest of no
  higher priority (0: none)*/
/*C_08E4_0134*/ChanFind(int prio)
{
	register int i;
	register int ch;

	ch = 0;
	if (chanList[11] != 11) {
		ch = chanList[11];
	} else {
		for (i = chanList[12]; i != 12; i = chanList[i]) {
			if (chanPriority[i] <= prio) {
				ch = i;
				break;
			}
		}
	}
	ChanTake(ch, prio);
	return ch;
}

/*starts the current step of sound effect n (or ends the effect)*/
/*C_08E4_017C*/void SfxStartStep(int n)
{
	struct tSfxStep far *step;
	int program;
	int note;
	int velocity;
	int duration;
	int vol1;
	int vol2;
	int pan1;
	register struct tVoice *v;
	register int pan2;

	if (soundCard) {
		v = &voices[n];
		step = v->step;
		if ((program = step->program) != 0 && v->chan1 != 0) {
			note = step->note;
			velocity = step->velocity;
			duration = step->duration;
			if (v->flags & 1) {
				pan1 = 0;
				pan2 = 0x7f;
				vol1 = ((v->pan < 0x20 ? 0x7f : 0x9f - v->pan) * v->volume) >> 7;
				vol2 = ((v->pan > 0x5f ? 0x7f : v->pan + 0x20) * v->volume) >> 7;
			} else {
				vol1 = vol2 = v->volume;
				pan1 = pan2 = v->pan;
			}
			disable();
			if (chanUser[v->chan1] != 2) {
				(*D_34F6)(v->chan1, 0x7b, 0);
				chanUser[v->chan1] = 2;
			}
			(*D_2814)(v->chan1, program - 1);
			chanProgram[v->chan1] = program - 1;
			(*D_34F6)(v->chan1, 7, vol1);
			(*D_34F6)(v->chan1, 10, pan1);
			(*D_2B9E)(v->chan1, note, velocity);
			if ((v->flags & 1) && v->chan2 != 0) {
				if (chanUser[v->chan2] != 2) {
					(*D_34F6)(v->chan2, 0x7b, 0);
					chanUser[v->chan2] = 2;
				}
				(*D_2814)(v->chan2, program - 1);
				chanProgram[v->chan2] = program - 1;
				(*D_34F6)(v->chan2, 7, vol2);
				(*D_34F6)(v->chan2, 10, pan2);
				(*D_2B9E)(v->chan2, note, velocity);
			}
			enable();
			v->note = note;
			v->counter = duration;
		} else {
			ChanFree(v->chan1);
			if (v->flags & 1)
				ChanFree(v->chan2);
			if (!(v->flags & 2))
				v->chan1 = 0;
		}
	}
}

/*a timer tick of the sound effects: ends steps, glides, starts the next steps*/
/*C_08E4_038A*/void SfxTick()
{
	int flags;
	int changed;
	register struct tVoice *v;
	register int n;

	if (soundCard && sfxReady) {
		for (n = 0; n < 32; n++) {
			changed = 0;
			if ((v = &voices[n])->step == 0 || v->chan1 == 0)
				continue;
			if ((flags = v->step->flags) & 4)
				continue;
			if (--v->counter)
				continue;
			(*D_2B9E)(v->chan1, v->note, 0);
			if (v->flags & 1)
				(*D_2B9E)(v->chan2, v->note, 0);
			if (flags & 2) {
				if (v->step->target > v->note) {
					v->note++;
					changed = 1;
				} else if (v->step->target < v->note) {
					v->note--;
					changed = 1;
				} else {
					v->note = v->step->note;
				}
			}
			if (flags & 8)
				changed = 1;
			if (changed) {
				(*D_2B9E)(v->chan1, v->note, v->step->velocity);
				v->counter = v->step->duration;
			} else if (flags & 1) {
				v->step++;
				SfxStartStep(n);
			} else {
				ChanFree(v->chan1);
				if (v->flags & 1)
					ChanFree(v->chan2);
				if (!(v->flags & 2))
					v->chan1 = 0;
			}
		}
	}
}

/*plays a sound effect (replacing the one of the same id); returns it (0: no channel)*/
/*C_08E4_04DA*/struct tVoice far *PlaySound(struct tSfxStep far *steps, int flags, int volume, int pan, int id, int prio)
{
	struct tVoice far *v;
	int ch1 = 0;
	int ch2 = 0;
	register int i;
	register int slot = -1;

	if (soundCard) {
		for (i = 0; i < 32; i++) {
			if (voices[i].chan1 != 0 && voices[i].id == id) {
				ch1 = voices[i].chan1;
				ch2 = voices[i].chan2;
				disable();
				(*D_2B9E)(ch1, voices[i].note, 0);
				if (voices[i].flags & 1)
					(*D_2B9E)(ch2, voices[i].note, 0);
				enable();
				voices[i].chan1 = 0;
				break;
			}
			if (voices[i].chan1 == 0)
				slot = i;
		}
		if (ch1 == 0 && slot >= 0) {
			i = slot;
			if ((ch1 = ChanFind(prio)) != 0 && (flags & 1))
				ch2 = ChanFind(prio);
		}
		if (ch1 != 0 && (ch2 != 0 || !(flags & 1))) {
			v = &voices[i];
			v->flags = flags;
			v->chan1 = ch1;
			v->chan2 = ch2;
			v->volume = volume;
			v->pan = pan;
			v->id = id;
			v->step = steps;
			SfxStartStep(i);
			return v;
		}
	}
	return 0;
}

/*changes the volume (ctl 7) or pan (ctl 10) of sound effect v (if its id is id, or id 0).
  The last call of the stereo branch must not be merged by the compiler with the
  else branch's: the only way found is a call returning long (DX:AX), hence the cast*/
/*C_08E4_065D*/void SetSoundParam(struct tVoice far *v, register int ctl, int val, int id)
{
	int vol1;
	register int vol2;

	if (!soundCard)
		return;
	if (v == 0)
		return;
	if (id != 0 && v->id != id)
		return;
	if (v->chan1 == 0)
		return;
	if (ctl == 7)
		v->volume = val;
	if (ctl == 10)
		v->pan = val;
	if ((v->flags & 1) && ctl == 10) {
		vol1 = ((v->pan < 0x20 ? 0x7f : 0x9f - v->pan) * v->volume) >> 7;
		vol2 = ((v->pan > 0x5f ? 0x7f : v->pan + 0x20) * v->volume) >> 7;
		disable();
		if (chanUser[v->chan1] == 2)
			(*D_34F6)(v->chan1, 7, vol1);
		if (chanUser[v->chan2] == 2)
			(*(tDrv3L)D_34F6)(v->chan2, 7, vol2);/*see above*/
		enable();
	} else {
		disable();
		if (chanUser[v->chan1] == 2)
			(*D_34F6)(v->chan1, ctl, val);
		if ((v->flags & 1) && chanUser[v->chan2] == 2)
			(*D_34F6)(v->chan2, ctl, val);
		enable();
	}
}

/*gives channel ch back to the music*/
/*C_08E4_07CB*/void ChanToMusic(register int ch)
{
	if (soundCard && chanUser[ch] == 2) {
		ChanFree(ch);
		disable();
		(*D_34F6)(ch, 0x7b, 0);
		(*D_2814)(ch, chanMusicProgram[ch]);
		chanProgram[ch] = chanMusicProgram[ch];
		(*D_34F6)(ch, 7, (chanVolume[ch] * musicVolume) >> 8);
		(*D_34F6)(ch, 10, chanPan[ch]);
		chanUser[ch] = 1;
		enable();
	}
}

/*stops sound effect v (if its id is id, or id 0)*/
/*C_08E4_0839*/void SfxStop(struct tVoice far *v, int id)
{
	if (soundCard && sfxReady) {
		sfxReady = 0;
		if (v != 0 && (id == 0 || v->id == id) && v->chan1 != 0) {
			ChanToMusic(v->chan1);
			if (v->flags & 1)
				ChanToMusic(v->chan2);
			v->chan1 = 0;
		}
		sfxReady = 1;
	}
}

/*stops sound effect v, or (v 0) all of them, if their id is id (or id 0)*/
/*C_08E4_08A7*/void StopSound(struct tVoice far *v, register int id)
{
	register int i;

	if (soundCard) {
		if (v == 0) {
			for (i = 0; i < 32; i++)
				SfxStop(&voices[i], id);
		} else {
			SfxStop(v, id);
		}
	}
}

/*stops all the sound effects*/
/*C_08E4_08F0*/void StopAllSounds()
{
	register int i;

	if (soundCard && sfxReady) {
		sfxReady = 0;
		for (i = 0; i < 32; i++)
			voices[i].chan1 = 0;
		for (i = 2; i < 10; i++)
			ChanToMusic(i);
		sfxReady = 1;
	}
}

/*reads a MIDI variable-length number from track t*/
/*C_08E4_093B*/long MidiVarLen(register int t)
{
	long n;

	n = 0;
	while (*D_3F2A[t] & 0x80)
		n = (n << 7) + (*D_3F2A[t]++ & 0x7f);
	n = (n << 7) + *D_3F2A[t]++;
	return n;
}

/*p begins with the 4 characters of tag*/
/*C_08E4_09D8*/IsChunk(char far *p, register char *tag)
{
	register int i;

	for (i = 0; i < 4; i++) {
		if (*p++ != *tag++)
			break;
	}
	return i == 4;
}

/*the length of the chunk at p (only 16 bits of it)*/
/*C_08E4_0A0A*/ChunkLength(unsigned char far *p)
{
	long n;

	n = (p[4] << 24) + (p[5] << 16) + (p[6] << 8) + p[7];
	return n;
}

/*a meta event of track t: sets the tempo*/
/*C_08E4_0A4B*/void MidiMetaEvent(int t, int type, register int len)
{
	long tempo;
	register int i;

	switch (type) {
	case 0x51:
		tempo = 0;
		for (i = 0; i < len; i++) {
			tempo <<= 8;
			tempo += D_3F2A[t][i];
		}
		tickTime = tempo / D_3FDA;
		tickTime *= 0x1317;
		tickTime /= 0x4db;
		break;
	}
	D_3F2A[t] += len;
}

/*adds a delta time to track t*/
/*C_08E4_0AF6*/void TrackAddDelta(int t, long delta)
{
	D_3FE6[t] += delta;
}

/*the track whose event comes next; advances the time to it*/
/*C_08E4_0B10*/NextTrack()
{
	long min;
	register int i;
	register int t;

	t = -1;
	min = 0x7fffffffL;
	for (i = 0; i < D_3FE4; i++) {
		if (D_3EEA[i] == 0 && D_3FE6[i] < min) {
			t = i;
			min = D_3FE6[i];
		}
	}
	if (min != 0)
		midiTime += min * tickTime;
	for (i = 0; i < D_3FE4; i++)
		D_3FE6[i] -= min;
	return t;
}

/*plays the MIDI events due in this timer tick; 1: the song has ended*/
/*C_08E4_0BC0*/PlayMidiEvents()
{
	int i;
	int done;
	int d1;
	int d2;
	int type;
	int len;
	int ofs;
	long delta;
	register int ch;
	register int t;

	done = 0;
	while (!done) {
		t = NextTrack();
		if (midiTime > 0xffffL)
			break;
		d1 = *D_3F2A[t]++;
		if (d1 >= 0x80 && d1 < 0xf0) {
			D_3FBA[t] = d1;
			d1 = *D_3F2A[t]++;
		}
		if (d1 < 0xf0) {
			ch = (D_3FBA[t] & 0xf) + 1;
			if (chanUser[ch] == 0 || chanPriority[ch] < 0) {
				(*D_34F6)(ch, 0x7b, 0);
				if (chanProgram[ch] != chanMusicProgram[ch]) {
					if (ch != 10)
						(*D_2814)(ch, chanMusicProgram[ch]);
					chanProgram[ch] = chanMusicProgram[ch];
				}
				(*D_34F6)(ch, 7, (chanVolume[ch] * musicVolume) >> 8);
				(*D_34F6)(ch, 10, chanPan[ch]);
				chanUser[ch] = 1;
			}
			if (chanUser[ch] == 1)
				ChanTake(ch, 0);
			switch (D_3FBA[t] & 0xf0) {
			case 0x80:
				d2 = *D_3F2A[t]++;
				if (musicNotesOn && chanUser[ch] == 1) {
					if (ch == 10 && soundCard == 2)
						(*D_2B9E)(drumChan[d1], drumNote[d1], 0);
					else
						(*D_2B9E)(ch, d1, 0);
				}
				break;
			case 0x90:
				d2 = *D_3F2A[t]++;
				if (musicNotesOn && chanUser[ch] == 1) {
					if (ch == 10 && soundCard == 2)
						(*D_2B9E)(drumChan[d1], drumNote[d1], d2);
					else
						(*D_2B9E)(ch, d1, d2);
				}
				break;
			case 0xa0:
				d2 = *D_3F2A[t]++;
				break;
			case 0xb0:
				d2 = *D_3F2A[t]++;
				if (musicNotesOn && chanUser[ch] == 1) {
					if (d1 == 7)
						(*D_34F6)(ch, d1, (d2 * musicVolume) >> 8);
					else
						(*D_34F6)(ch, d1, d2);
				}
				if (d1 == 7)
					chanVolume[ch] = d2;
				if (d1 == 10)
					chanPan[ch] = d2;
				break;
			case 0xc0:
				if (musicNotesOn && chanUser[ch] == 1) {
					(*D_2814)(ch, d1);
					chanProgram[ch] = d1;
				}
				chanMusicProgram[ch] = d1;
				break;
			case 0xe0:
				d2 = *D_3F2A[t]++;
				if (musicNotesOn && chanUser[ch] == 1)
					(*D_34FA)(ch, d1, d2);
				break;
			}
		} else {
			switch (d1) {
			case 0xf0:
			case 0xf7:
				len = MidiVarLen(t);
				D_3F2A[t] += len;
				break;
			case 0xfe:/*Origin's: type, length, data; types 0, 2, 4, 5 jump back by the
				  first data word (0: always, 2: when the next song is set, 4/5: when data[2]
				  differs from/equals musicCondition), 1 ends the song, 3 sets musicMarker*/
				ch = (D_3FBA[t] & 0xf) + 1;
				type = *D_3F2A[t]++;
				len = *D_3F2A[t]++;
				if (type == 0) {
					if (D_20E0) {
						D_3F2A[t] += len;
					} else {
						musicFlags |= 1;
						(*D_34F6)(ch, 0x7b, 0);
						ofs = D_3F2A[t][0] + (D_3F2A[t][1] << 8);
						D_3F2A[t] -= ofs;
					}
				} else if (type == 1) {
					if (D_20E0) {
						musicFlags |= 1;
						for (i = 2; i <= 10; i++) {
							if (musicNotesOn && chanUser[i] == 1)
								(*D_34F6)(i, 0x7b, 0);
						}
						return 1;
					}
				} else if (type == 2) {
					if (D_20E0) {
						musicFlags |= 1;
						(*D_34F6)(ch, 0x7b, 0);
						ofs = D_3F2A[t][0] + (D_3F2A[t][1] << 8);
						D_3F2A[t] -= ofs;
					} else {
						D_3F2A[t] += len;
					}
				} else if (type == 3) {
					musicMarker = D_3F2A[t][0];
					D_3F2A[t] += len;
				} else if (type == 4) {
					if (D_3F2A[t][2] != musicCondition) {
						musicFlags |= 1;
						(*D_34F6)(ch, 0x7b, 0);
						ofs = D_3F2A[t][0] + (D_3F2A[t][1] << 8);
						D_3F2A[t] -= ofs;
					} else {
						D_3F2A[t] += len;
					}
				} else if (type == 5) {
					if (D_3F2A[t][2] == musicCondition) {
						musicFlags |= 1;
						(*D_34F6)(ch, 0x7b, 0);
						ofs = D_3F2A[t][0] + (D_3F2A[t][1] << 8);
						D_3F2A[t] -= ofs;
					} else {
						D_3F2A[t] += len;
					}
				} else {
					D_3F2A[t] += len;
				}
				break;
			case 0xff:
				type = *D_3F2A[t]++;
				len = MidiVarLen(t);
				MidiMetaEvent(t, type, len);
				if (type == 0x2f) {
					D_3EEA[t] = 1;
					for (i = 0; i < D_3FE4; i++) {
						if (D_3EEA[i] == 0)
							break;
					}
					if (i == D_3FE4)
						done = 1;
				}
				break;
			}
		}
		if (!done) {
			delta = MidiVarLen(t);
			TrackAddDelta(t, delta);
		}
	}
	return done;
}

/*stops the music: notes off*/
/*C_08E4_123F*/void MusicNotesOff()
{
	register int ch;

	musicStopped = 1;
	curSong = 0;
	musicFlags |= 1;
	for (ch = 1; ch <= 16; ch++) {
		if (chanUser[ch] == 1)
			(*D_34F6)(ch, 0x7b, 0);
	}
}

/*sets the volume of the music's channels*/
/*C_08E4_127C*/void SetChanVolumes(int vol)
{
	register int ch;

	musicVolume = vol;
	for (ch = 2; ch <= 10; ch++) {
		if (chanUser[ch] == 1)
			(*D_34F6)(ch, 7, (chanVolume[ch] * musicVolume) >> 8);
	}
}

/*the timer interrupt: fades, plays the music, starts the next song, the sound effects*/
/*C_08E4_12B3*/void interrupt SoundTimer()
{
	void far *song;

	if (soundCard) {
		if (fadeStep && !musicStopped) {
			fadeLevel += fadeStep;
			if (fadeLevel < 0)
				fadeStep = 0;
			else
				SetChanVolumes(((fadeLevel >> 8) * musicVolumeSet) >> 7);
			if (fadeStep == 0) {
				if (!fadeNoStop)
					MusicNotesOff();
				SetChanVolumes(musicVolumeSet);
			}
		}
		if (!musicStopped) {
			if (D_23D4 == 0 || C_0AD2_00A7() != 0) {
				C_0AD2_0024(curSong);
				musicStopped = PlayMidiEvents();
				C_0AD2_00DD();
			}
			if (musicStopped) {
				musicFlags |= 1;
				curSong = 0;
			}
			midiTime -= 0x10000L;
		} else if (nextSong) {
			song = curSong;
			PlaySong(nextSong);
			if (musicFlags & 4) {
				nextSong = song;
				musicFlags &= ~4;
			}
		}
		SfxTick();
	}
	(*D_3FB2)();
}

/*rewinds the song's tracks (to play only track only; -1: all)*/
/*C_08E4_13D4*/void RewindTracks(register int only)
{
	register int i;

	for (i = 2; i <= 10; i++)
		(*D_34F6)(i, 0x79, 0);
	for (i = 0; i < D_3FE4; i++) {
		D_3FE6[i] = 0;
		if (only == -1 || only == i)
			D_3EEA[i] = 0;
		else
			D_3EEA[i] = 1;
		D_3F2A[i] += 8;
		TrackAddDelta(i, MidiVarLen(i));
	}
	musicStopped = D_20E0 = musicMarker = 0;
	D_20D2 = 1;
	musicFlags &= ~1;
	musicFlags |= 2;
	nextSong = 0;
}

/*fades the music out in time ticks (0: stops it now)*/
/*C_08E4_1478*/void FadeOutMusic(int time)
{
	musicFlags &= ~2;
	if (soundCard) {
		if (!fadeStep)
			fadeLevel = 0x7fff;
		nextSong = 0;
		if (time) {
			fadeStep = -(0x8000 / time);
			fadeNoStop = 0;
		} else {
			fadeStep = 0;
			MusicNotesOff();
			SetChanVolumes(musicVolumeSet);
		}
	}
}

/*loads the sound card's driver from STRAX.DRV and its timbre bank from file name*/
/*C_08E4_14D2*/LoadSoundDriver(char far *name)
{
	register unsigned seg;
	register int card;

	if (soundCard) {
		card = soundCard - 1;
		D_2BC2 = C_0C58_0305("strax.drv", card, 0L, 0x12, 0L);
		D_3FB6 = C_0C58_0305(name, card, 0L, 0x10, 0L);
		if (D_3FB6 == 0 && soundCard != 1) {
			C_0BC5_0126(D_2BC2);
			D_2BC2 = 0;
			soundCard = 0;
		}
		if (D_2BC2) {
			seg = FP_SEG(D_2BC2);
			D_2B6E = (tDrvInit)MK_FP(seg, D_2BC2[1]);
			D_2BAE = (tDrv1)MK_FP(seg, D_2BC2[2]);
			D_2BD2 = (tDrv0)MK_FP(seg, D_2BC2[3]);
			D_2BE0 = MK_FP(seg, D_2BC2[4]);
			D_2B9E = (tDrv3)MK_FP(seg, D_2BC2[5]);
			D_34F6 = (tDrv3)MK_FP(seg, D_2BC2[6]);
			D_34FA = (tDrv3)MK_FP(seg, D_2BC2[7]);
			D_2814 = (tDrv2)MK_FP(seg, D_2BC2[8]);
			D_34E4 = MK_FP(seg, D_2BC2[9]);
			D_2B2A = MK_FP(seg, D_2BC2[10]);
			D_34E8 = (*D_2B6E)();
			D_34E8->cb1 = C_0CED_000E;
			D_34E8->cb2 = C_0D11_0102;
			(*D_2BAE)(D_3FB6);
			if (!D_34E8->ok)
				C_0BC5_0126(D_3FB6);
			return 1;
		}
	}
	return 0;
}

/*starts the music and sound: loads the driver and the timbres, hooks the timer*/
/*C_08E4_164A*/void InitSound(char far *name)
{
	if (!soundCard)
		return;
	LoadSoundDriver(name);
	if (soundCard == 2) {
		(*D_2814)(10, 0x80);
		(*D_2814)(11, 0x72);
		(*D_2814)(12, 0x83);
		(*D_2814)(13, 0x71);
		(*D_2814)(14, 0x86);
		(*D_2814)(15, 0x87);
		(*D_2814)(16, 0x85);
		(*D_2814)(17, 0x84);
		(*D_2814)(18, 0x81);
		(*D_2814)(19, 0x88);
		(*D_2814)(20, 0x8d);
		(*D_2814)(21, 0x8f);
		(*D_2814)(22, 0x90);
		(*D_2814)(23, 0x91);
		(*D_2814)(24, 0x93);
		(*D_2814)(25, 0x8c);
		(*D_2814)(26, 0x8b);
	}
	C_0CED_00A5(soundCard == 3 ? 0x1e00 : 0x3c);
	musicStopped = 1;
	curSong = 0;
	nextSong = 0;
	musicFlags = 0;
	disable();
	C_0CED_000E(8, SoundTimer, D_3FDC, (void far *)&D_3FB2);
	C_0CED_000E(8, D_2BE0, D_3FAA, D_34E8->unk02);
	enable();
	sfxReady = 1;
}

/*stops the music and sound, shuts the driver down, unhooks the timer*/
/*C_08E4_17BE*/void ShutdownSound()
{
	register int i;

	if (soundCard) {
		sfxReady = 0;
		musicStopped = 1;
		curSong = 0;
		nextSong = 0;
		musicFlags = 0;
		for (i = 0; i < 32; i++)
			voices[i].chan1 = 0;
		for (i = 2; i < 10; i++)
			ChanToMusic(i);
		for (i = 1; i <= 16; i++)
			(*D_34F6)(i, 0x7b, 0);
		(*D_2BD2)();
	}
	C_0CED_0187();
}

/*starts the MIDI file song (0: not a MIDI file)*/
/*C_08E4_1840*/StartSong(void far *song)
{
	int ok;
	unsigned char far *p;
	register int ofs;
	register int t;

	ok = 1;
	musicStopped = 1;
	curSong = song;
	if (D_23D4 == 0 || C_0AD2_00A7() != 0)
		p = C_0AD2_0024(song);
	else
		p = 0;
	if (p == 0) {
		ok = 0;
	} else {
		for (ofs = 0; !IsChunk(p + ofs, "MThd"); ofs += ChunkLength(p + ofs) + 8) {
			if (IsChunk(p + ofs, "MTrk")) {
				ok = 0;
				break;
			}
		}
		if (ChunkLength(p + ofs) != 6)
			ok = 0;
		if (ok) {
			D_4066 = (p[ofs + 8] << 8) + p[ofs + 9];
			D_3FE4 = (p[ofs + 10] << 8) + p[ofs + 11];
			D_3FDA = (p[ofs + 12] << 8) + p[ofs + 13];
			if (D_4066 > 2 || D_4066 < 0)
				ok = 0;
			if (D_3FE4 > 32 || D_3FE4 < 0)
				ok = 0;
		}
		if (ok) {
			for (t = 0; t < D_3FE4; t++) {
				while (!IsChunk(&p[ofs + 14], "MTrk"))
					ofs += ChunkLength(&p[ofs + 14]) + 8;
				D_3F2A[t] = &p[ofs + 14];
				ofs += ChunkLength(&p[ofs + 14]) + 8;
			}
			RewindTracks(-1);
		}
	}
	C_0AD2_00DD();
	return ok;
}

/*plays song now (0: no sound card or not a MIDI file)*/
/*C_08E4_19FF*/PlaySong(void far *song)
{
	if (!soundCard)
		return 0;
	FadeOutMusic(0);
	fadeStep = 0;
	return StartSong(song);
}

/*fades the music out in time ticks, then plays song (flags: musicFlags)*/
/*C_08E4_1A2B*/void FadeToSong(void far *song, int time, int flags)
{
	musicFlags = flags;
	if (soundCard) {
		if (!fadeStep)
			fadeLevel = 0x7fff;
		if (time) {
			fadeStep = -(0x8000 / time);
			nextSong = song;
			fadeNoStop = 0;
			D_20D2 = 0;
		} else {
			fadeStep = 0;
			nextSong = 0;
			StartSong(song);
		}
	}
}

/*song is played when the music reaches its end*/
/*C_08E4_1A97*/void SetNextSong(void far *song, int flags)
{
	nextSong = song;
	D_20E0 = 1;
	D_20D2 = 0;
	musicFlags = flags;
}

/*waits until the music sets one of the flags mask, and clears them*/
/*C_08E4_1ABB*/void WaitMusicFlags(int mask)
{
	if (soundCard) {
		while (!(musicFlags & mask))
			;
		musicFlags &= ~mask;
	}
}

/*sets the music volume (0x100: full)*/
/*C_08E4_1AD6*/void SetMusicVolume(register int vol)
{
	register int ch;

	if (soundCard) {
		musicVolumeSet = vol;
		if (!fadeStep)
			musicVolume = vol;
		for (ch = 2; ch <= 10; ch++) {
			if (chanUser[ch] == 1)
				(*D_34F6)(ch, 7, (chanVolume[ch] * vol) >> 8);
		}
	}
}

/*turns the music's notes on or off*/
/*C_08E4_1B20*/void SetMusicOn(int on)
{
	register int ch;

	if (!soundCard)
		return;
	if ((on != 0) != musicNotesOn) {
		if ((musicNotesOn = on) != 0) {
			for (ch = 2; ch <= 10; ch++) {
				if (chanUser[ch] == 1) {
					if (ch != 10)
						(*D_2814)(ch, chanMusicProgram[ch]);
					chanProgram[ch] = chanMusicProgram[ch];
					(*D_34F6)(ch, 7, (chanVolume[ch] * musicVolume) >> 8);
					(*D_34F6)(ch, 10, chanPan[ch]);
				}
			}
		} else {
			for (ch = 2; ch <= 10; ch++) {
				if (chanUser[ch] == 1)
					(*D_34F6)(ch, 0x7b, 0);
			}
		}
	}
}
