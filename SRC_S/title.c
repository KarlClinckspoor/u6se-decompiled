/* flags: -mm -c -O -y */
/*
 * Worlds of Ultima: The Savage Empire - SAVAGE.EXE
 * segment 0636: the title (TITLE.LZC): the letters of the title change and fly apart,
 * the sun sets behind the mountains, sparkles
 */

/*a drawing surface (16 bytes; the fields this module uses)*/
struct tSurface {
	int w00;
	int w02;
	int w04;
	int x0;/*+06*/
	int w08;
	int y0;/*+0A*/
	int w0C;
	int w0E;
};

/*a sparkle of the last scene (12 bytes; see segment 071C)*/
struct tSprite {
	int frame;/*+00 -1: free*/
	int x;/*+02*/
	int y;/*+04*/
	int kind;/*+06*/
	void far *sound;/*+08*/
};

/*how a letter of the title moves*/
struct tLetter {
	int x;
	int y;
	int dx;
	int dy;
};

extern unsigned char far C_07D9_0021;/*nonzero when a key was pressed (the event module's code segment)*/
extern int D_20CA;/*the sound card? (1, 2: sounds are played; 3)*/
extern int D_20CE;/*how far the music has got?*/
extern int (*D_2B6A)();/*the graphics driver: draws a shape, reports its rectangle*/
extern void (pascal *D_2B9E)();/*the sound driver: plays a sound*/
extern int (*D_2BBA)();/*the graphics driver: shows a surface*/
extern int (*D_2BC6)();/*the graphics driver: draws a shape*/
extern int (*D_2BDC)();/*the graphics driver: draws a shape, scaled*/
extern int (*D_3516)();
extern int D_353E;/*nonzero: music?*/
extern void far *D_3540[13];/*the shapes of TITLE.LZC*/
extern void far *D_3574;
extern struct tSprite D_3578[30];

extern void far *C_034C_03DB(char *name, int n);/*item n of an LZC file, loaded*/
extern C_0737_002E();
extern pascal C_0896_0004(struct tSurface *, int, int);
extern pascal C_08CA_0006(struct tSurface *, struct tSurface *);
extern C_08E4_08A7(void far *, int);/*stops a sound*/
extern C_08E4_1478(int);
extern C_08E4_19FF(void far *);
extern C_08E4_08F0();
extern C_071C_0006();/*frees all the sparkles*/
extern C_071C_0026(struct tSurface *, int n);/*draws the first n sparkles; returns how many are free*/
extern C_07D7_0008();/*waits for the vertical retrace*/
extern C_0BC5_0126(void far *);/*frees*/
extern int C_0C97_00BE();
extern int pascal C_0D23_0027(int lo, int hi);/*random number from lo to hi*/
extern int kbhit(void);

/*1E3E*/char D_1E3E[] = "opoqopoqopoqopoqqrstrq";/*frames of the sun: p, q play a sound*/
/*1E55*/char *D_1E55[10] = {/*frames of each letter of the title*/
	"abcdefghijkaakkkkaaaalllllllmmll",
	"aaaaaaaabcddeeddccffgghhgghhiiih",
	"aaaaaaaaaabbccbbaaccccccccccaccc",
	"aaaabbbcccddeeddccffggaaggffcaaa",
	"aabbbaacccddddddccddeeeeeeddccca",
	"aaaaaabbbaaabbaaaabbbbaabbbbaccc",
	"aabbcdbbbeaaaaaaeeaaaaaaaaaabaaa",
	"abbbaacccdeeaaeeddaaaaaaaaaadacc",
	"aaaaaaaaabbbccbbbbccddddddccaaaa",
	"abccdefgghiijjiihhkkllmmllkkhnnn"
};
/*1E69*/struct tLetter D_1E69[10] = {
	{0x3a, 0x5e, -1, 1}, {0xba, 0x5e, 0, 1}, {0x116, 0x5e, 1, 1},
	{0x3a, 0x66, -3, 2}, {0xba, 0x66, 0, 2}, {0x116, 0x66, 3, 2},
	{0x3a, 0x6e, -5, 3}, {0xba, 0x6e, 0, 3}, {0x116, 0x6e, 5, 3},
	{0x9e, 0x4a, 0, 4}
};

/*C_0636_0003*/void Show(struct tSurface *surf, struct tSurface *screen) {
	C_07D7_0008();
	(*D_2BBA)(surf, screen);
}

/*C_0636_0019*/void DrawBackground(struct tSurface *surf, int y) {
	(*D_2BC6)(surf, 0, 0, D_3540[0], 0);
	(*D_2BC6)(surf, 0xa0, y, D_3540[12], 0);
}

/*C_0636_0051*/void C_0636_0051(struct tSurface *surf, struct tSurface *screen) {
	int i;

	DrawBackground(surf, 0x18);
	for(i = 0; i < 10; i++)
		(*D_2BC6)(surf, D_1E69[i].x, D_1E69[i].y, D_3540[i + 1], 0);
	C_08CA_0006(surf, screen);
}

/*the letters of the title change*/
/*C_0636_00AC*/void C_0636_00AC(struct tSurface *surf, struct tSurface *screen, int speed) {
	int from;
	int count;
	int t;
	int i;
	int f;

	from = 0;
	count = 0;
	t = 1;
	do {
		for(f = from; f < 0x20; f++) {
			if(--t < 1) {
				t = speed;
				DrawBackground(surf, 0x18);
				for(i = 0; i < 10; i++)
					(*D_2BC6)(surf, D_1E69[i].x, D_1E69[i].y, D_3540[i + 1], D_1E55[i][f] - 'a');
				Show(surf, screen);
			}
			if(kbhit())
				C_07D9_0021 = 0xff;
			if(D_20CE >= 1)
				return;
			if(C_07D9_0021)
				return;
			count++;
		}
		from = C_0D23_0027(0, 0xd) + 9;
		for(f = 0x1f; f >= from; f--) {
			if(--t < 1) {
				t = speed;
				DrawBackground(surf, 0x18);
				for(i = 0; i < 10; i++)
					(*D_2BC6)(surf, D_1E69[i].x, D_1E69[i].y, D_3540[i + 1], D_1E55[i][f] - 'a');
				Show(surf, screen);
			}
			if(kbhit())
				C_07D9_0021 = 0xff;
			if(D_20CE >= 1)
				return;
			if(C_07D9_0021)
				return;
			count++;
		}
	} while((D_353E && D_20CA != 3) || count <= 30);
}

/*the sun sets*/
/*C_0636_0255*/void C_0636_0255(struct tSurface *surf, struct tSurface *screen, int speed) {
	int i;
	int t;
	int f;

	t = 1;
	for(f = 0; f < 0x14; f++) {
		if(D_1E3E[f] == 'p') {
			if(D_20CA == 2) {
				t = 1;
				(*D_2B9E)(0x12, 0x30, 0x60);
			} else if(D_20CA == 1) {
				t = 1;
				(*D_2B9E)(0xa, 0x25, 0x60);
			}
		} else if(D_1E3E[f] == 'q') {
			if(D_20CA == 2) {
				(*D_2B9E)(0x12, 0x30, 0);
				t = 1;
			} else if(D_20CA == 1) {
				(*D_2B9E)(0xa, 0x25, 0);
				t = 1;
			}
		}
		if(--t < 1) {
			t = speed;
			DrawBackground(surf, 0x18);
			for(i = 0; i < 9; i++)
				(*D_2BC6)(surf, D_1E69[i].x, D_1E69[i].y, D_3540[i + 1], D_1E55[i][0x1f] - 'a');
			(*D_2BC6)(surf, D_1E69[9].x, D_1E69[9].y, D_3540[10], D_1E3E[f] - 'a');
			Show(surf, screen);
		}
		if(kbhit())
			C_07D9_0021 = 0xff;
		if(C_07D9_0021)
			return;
	}
	DrawBackground(surf, 0x18);
	(*D_2BC6)(surf, D_1E69[0].x, D_1E69[0].y, D_3540[1], D_1E55[0][0x1f] - 'a');
	(*D_2BC6)(surf, D_1E69[1].x, D_1E69[1].y, D_3540[2], D_1E55[1][0x1f] - 'a');
	(*D_2BC6)(surf, D_1E69[2].x, D_1E69[2].y, D_3540[3], D_1E55[2][0x1f] - 'a');
	(*D_2BC6)(surf, D_1E69[3].x, D_1E69[3].y, D_3540[4], D_1E55[3][0x1f] - 'a');
	(*D_2BC6)(surf, D_1E69[4].x, D_1E69[4].y, D_3540[5], D_1E55[4][0x1f] - 'a');
	(*D_2BC6)(surf, D_1E69[5].x, D_1E69[5].y, D_3540[6], D_1E55[5][0x1f] - 'a');
	(*D_2BC6)(surf, D_1E69[6].x, D_1E69[6].y, D_3540[7], D_1E55[6][0x1f] - 'a');
	(*D_2BC6)(surf, D_1E69[7].x, D_1E69[7].y, D_3540[8], D_1E55[7][0x1f] - 'a');
	(*D_2BC6)(surf, D_1E69[8].x, D_1E69[8].y, D_3540[9], D_1E55[8][0x1f] - 'a');
	(*D_2BC6)(surf, D_1E69[9].x, D_1E69[9].y, D_3540[10], D_1E3E[20] - 'a');
	if(D_353E && (D_20CA == 2 || D_20CA == 1)) {
		do {
			if(kbhit())
				C_07D9_0021 = 0xff;
		} while(D_20CE < 2 && C_07D9_0021 == 0);
		Show(surf, screen);
	}
}

/*draws the mountains in front of the sun at x, y*/
/*C_0636_0564*/void C_0636_0564(struct tSurface *surf, int x, int y) {
	int r[4];

	(*D_2B6A)(surf, 0xa2, x, D_3540[0], 2, 0, y, 0, r);
	(*D_2BDC)(surf, r[0], x, D_3540[0], 1, 0, y, 0);
	(*D_2BDC)(surf, 0xa2, x, D_3540[0], 2, 0, y, 0);
	(*D_2BDC)(surf, r[2], x, D_3540[0], 3, 0, y, 0);
}

/*the letters fly apart*/
/*C_0636_05FA*/void C_0636_05FA(struct tSurface *surf, struct tSurface *screen, int speed) {
	int x;
	int y;
	int t;
	int d;
	int i;

	t = 1;
	for(d = 1; d < 0x78; d = d + d / 4 + 1) {
		if(--t < 1) {
			t = speed;
			DrawBackground(surf, 0x18);
			for(i = 0; i < 9; i++) {
				x = D_1E69[i].x + D_1E69[i].dx * d;
				y = D_1E69[i].y + D_1E69[i].dy * d;
				(*D_2BDC)(surf, x, y, D_3540[i + 1], D_1E55[i][0x1f] - 'a', 0, d * 4 * D_1E69[i].dy + 0x100, 0);
			}
			x = D_1E69[9].x + d * D_1E69[9].dx;
			y = D_1E69[9].y + d * D_1E69[9].dy;
			(*D_2BDC)(surf, x, y, D_3540[10], D_1E3E[21] - 'a', 0, d * 4 * D_1E69[9].dy + 0x100, 0);
			Show(surf, screen);
		}
		if(kbhit())
			C_07D9_0021 = 0xff;
		if(C_07D9_0021)
			break;
	}
}

/*the mountains rise and the sun sets; returns where the mountains are*/
/*C_0636_0764*/C_0636_0764(struct tSurface *surf, struct tSurface *screen, int speed) {
	int unused;
	int y;
	int sunY;
	int n;
	int c1;
	int c2;
	int moved;
	int behind;
	int r[4];
	int t;
	int x;

	unused = 0;
	behind = 0;
	t = 1;
	x = 0x3b;
	for(n = 5000, c1 = 0, c2 = 0; n >= 1000; n--, c1--, c2--) {
		moved = 0;
		if(c1 == 0) {
			moved = 1;
			sunY = 120000L / n;
			c1 = 200;
		}
		if(c2 == 0) {
			moved = 1;
			y = 256000L / n;
			x = n > 3000 ? x - 2 : x + 2;
			c2 = 100;
		}
		if(moved) {
			if(--t < 1) {
				t = speed;
				(*D_2BC6)(surf, 0, 0, D_3540[0], 0);
				(*D_2B6A)(surf, 0xa2, x, D_3540[0], 2, 0, y, 0, r);
				if(r[3] < sunY)
					behind = 1;
				if(!behind) {
					C_0636_0564(surf, x, y);
					(*D_2BC6)(surf, 0xa0, sunY, D_3540[12], 0);
				} else {
					(*D_2BC6)(surf, 0xa0, sunY, D_3540[12], 0);
					C_0636_0564(surf, x, y);
				}
				Show(surf, screen);
			}
		}
		if(kbhit())
			C_07D9_0021 = 0xff;
		if(C_07D9_0021)
			return;
	}
	(*D_2BC6)(surf, 0, 0, D_3540[0], 0);
	C_0636_0564(surf, x, y);
	Show(surf, screen);
	return x;
}

/*sparkles over the mountains*/
/*C_0636_0920*/void C_0636_0920(struct tSurface *surf, struct tSurface *screen, int x) {
	int j;
	int nFree;
	int done;
	int add;
	int count;
	int i;

	done = 0;
	count = 0;
	C_08E4_08F0();
	C_071C_0006();
	do {
		if(!done) {
			/*DIFFERS: the original does not merge the end of "add = C_0D23_0027(0, 5) == 0"
			  (0636:0977-0993) with the same code of the last case; this merges it (9 bytes
			  shorter)*/
			if(D_353E && (D_20CA == 2 || D_20CA == 1)) {
				if(D_20CE >= 4)
					add = C_0D23_0027(0, 0) == 0;
				else
					add = C_0D23_0027(0, 5) == 0;
			} else
				add = C_0D23_0027(0, 0) == 0;
		}
		if(add && !done) {
			for(i = 0; i < 5; i++) {
				if(D_3578[i].frame == -1) {
					D_3578[i].frame = 0;
					D_3578[i].x = C_0D23_0027(0, 0x13f);
					D_3578[i].y = C_0D23_0027(0, 0x7f);
					D_3578[i].kind = C_0D23_0027(0, 2);
					break;
				}
			}
		}
		(*D_2BC6)(surf, 0, 0, D_3540[0], 0);
		nFree = C_071C_0026(surf, 5);
		C_0636_0564(surf, x, 0x100);
		Show(surf, screen);
		count++;
		if(done == 1 && nFree == 5)
			goto all;
		/*DIFFERS: the original keeps both "done = 1" (0636:0A9A, 0AA7); this merges them
		  (2 bytes shorter)*/
		if(D_353E && (D_20CA == 2 || D_20CA == 1)) {
			if(D_20CE >= 5)
				done = 1;
		} else if(count > 10)
			done = 1;
		if(kbhit())
			C_07D9_0021 = 0xff;
	} while(C_07D9_0021 == 0);
	return;
all:
	for(i = 0; i < 0x1d; i++) {
		D_3578[i].frame = 0;
		D_3578[i].x = C_0D23_0027(0, 0x13f);
		D_3578[i].y = C_0D23_0027(0, 0x7f);
		D_3578[i].kind = C_0D23_0027(0, 2);
	}
	nFree = 0;
	j = 0;
	while(nFree != 0x1e) {
		if(++j == 4) {
			D_3578[29].frame = 0;
			D_3578[29].x = C_0D23_0027(0, 0x13f);
			D_3578[29].y = C_0D23_0027(0, 0x7f);
			D_3578[29].kind = C_0D23_0027(0, 2);
		}
		(*D_2BC6)(surf, 0, 0, D_3540[0], 0);
		nFree = C_071C_0026(surf, 0x1e);
		C_0636_0564(surf, x, 0x100);
		Show(surf, screen);
		if(kbhit())
			C_07D9_0021 = 0xff;
		if(C_07D9_0021)
			return;
	}
	for(i = 0x1d; i >= 0; i--) {
		if(D_3578[i].frame != -1)
			C_08E4_08A7(D_3578[i].sound, i);
	}
}

/*the number of frames between two pictures, from the speed of the computer*/
/*C_0636_0C29*/C_0636_0C29() {
	return 2 - C_0C97_00BE() / 8 < 1 ? 1 : 2 - C_0C97_00BE() / 8;
}

/*C_0636_0C5A*/void far *LoadShape(int n) {
	return C_034C_03DB("TITLE.LZC", n);
}

/*plays the title on surf, shown on screen; mode: the video mode (3: shapes 7-9 are not
  loaded); music: the song to play during it (or 0)*/
/*C_0636_0C6D*/void PlayTitle(struct tSurface *surf, struct tSurface *screen, int mode, void far *music) {
	int speed;
	struct tSurface save;
	int i;

	speed = C_0636_0C29();
	save = *screen;
	screen->x0 = 0x18;
	screen->y0 = 0x97;
	for(i = 0; i < 12; i++) {
		if(mode != 3)
			D_3540[i] = LoadShape(i);
		else if(i < 7 || i > 9)
			D_3540[i] = LoadShape(i);
	}
	D_3574 = D_3540[11];
	D_3540[12] = LoadShape(12);
	C_07D9_0021 = 0;
	if(music)
		C_08E4_19FF(music);
	C_0636_0051(surf, screen);
	if(C_07D9_0021 == 0xff)
		goto end;
	C_0636_00AC(surf, screen, speed);
	if(C_07D9_0021 == 0xff)
		goto end;
	C_0636_0255(surf, screen, speed);
	if(C_07D9_0021 == 0xff)
		goto end;
	C_0636_05FA(surf, screen, speed);
	if(C_07D9_0021 != 0xff) {
		i = C_0636_0764(surf, screen, speed);
		if(D_353E && (D_20CA == 2 || D_20CA == 1)) {
			while(D_20CE < 3) {
				if(kbhit())
					C_07D9_0021 = 0xff;
				if(C_07D9_0021 == 0xff)
					break;
			}
		}
		if(C_07D9_0021 != 0xff)
			C_0636_0920(surf, screen, i);
	}
end:
	C_07D9_0021 = 0;
	if(music)
		C_08E4_1478(0);
	C_0896_0004(&save, 0, 1);
	(*D_3516)(&save, 0);
	C_0737_002E();
	for(i = 0; i < 13; i++)
		C_0BC5_0126(D_3540[i]);
	if(music)
		C_0BC5_0126(music);
	*screen = save;
}
