/* flags: -mm -c -O */
/*
 * Worlds of Ultima: The Savage Empire - SAVAGE.EXE
 * segment 071C: the sparkles of the title's last scene (8 frames each, with a sound
 * panned by where they are)
 */

/*a sparkle (12 bytes)*/
struct tSprite {
	int frame;/*+00 -1: free*/
	int x;/*+02*/
	int y;/*+04*/
	int kind;/*+06*/
	void far *sound;/*+08*/
};

extern int (*D_2BC6)();/*the graphics driver: draws a shape*/
extern void far *D_3574;/*the shapes of the sparkles*/
extern struct tSprite D_3578[30];

extern StopSound(void far *, int);/*stops a sound*/
extern void far *PlaySound(void far *snd, int, int, int pan, int, int);/*plays a sound*/

/*200E*/unsigned char D_200E[] = {0x00, 0x80, 0x40, 0x40, 0x3c, 0x00};/*the sound of a sparkle*/

/*C_071C_0006*/void ClearSprites() {
	int i;

	for(i = 0; i < 30; i++)
		D_3578[i].frame = -1;
}

/*draws and moves the first n sparkles; returns how many of them are free*/
/*C_071C_0026*/DrawSprites(int surf, int n) {
	int i;
	int nFree = 0;

	nFree = 0;
	for(i = n - 1; i >= 0; i--) {
		if(D_3578[i].frame == -1) {
			nFree++;
			continue;
		}
		(*D_2BC6)(surf, D_3578[i].x, D_3578[i].y, D_3574, D_3578[i].frame + D_3578[i].kind * 8);
		if(D_3578[i].frame++ == 7) {
			D_3578[i].frame = -1;
			StopSound(D_3578[i].sound, i);
		} else if(D_3578[i].frame == 1) {
			D_3578[i].sound = PlaySound(D_200E, 0, 0x7f, (char)(0x7f - D_3578[i].x * 0x7fL / 0x13f), i, 1);
		}
	}
	return nFree;
}
