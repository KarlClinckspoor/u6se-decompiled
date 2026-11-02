/* flags: -d -mm -p -c -Z -O -y */
/*
 * The Savage Empire (Worlds of Ultima) - GAME.EXE
 * segment 09DA: what part of the area is visible (Ultima VI's visible.c).
 *
 * VIS_UpdateView builds the 11x11 ground tiles shown around the party
 * (D_C08A): it marks in the area flags (D_3EBA) what the objects of the
 * area do to sight and light, then runs a breadth-first flood fill
 * (VIS_FloodFill) from the party's square and from every visible light
 * source, accumulating light in D_6FFC. Squares never reached are drawn
 * as tile 0xff (hidden), reached but unlit ones as tile 0x1bc (darkness),
 * and wall tiles seen from one side only are replaced by a variant
 * showing only their visible faces.
 *
 * Area flag bits (per area square):
 *   0x03 light level of the brightest object on the square
 *   0x04 a "background" object is there (does not block sight)
 *   0x08 a wall object is there
 *   0x10 a window object is there
 *   0x20 an opaque object is there
 *   0x40 already queued by the current flood fill
 *   0x80 visible from the party
 */
#include "se.h"

#define AREA_W 40
#define AREA_H 40

/*other modules*/
extern char AmbientLight;/*AmbientLight: sun light level 0..7*/
extern int16 MapX;/*MapX*/
extern int16 MapY;/*MapY*/
extern unsigned char D_3EBA[AREA_H][AREA_W];/*AreaFlags*/
extern unsigned char D_470A[AREA_H][AREA_W];/*AreaTiles: the ground tiles of the area*/
extern unsigned char far *D_6DF0;/*TileFlag: 0x03 light, 0x04 opaque, 0x08 window, 0x40 double height, 0x80 double width*/
extern int16 far *D_6EF4;/*BaseTile: first tile of each object type*/
extern unsigned char D_6EFC[];/*TalkFlags (per NPC); D_6EFC[1] & 4: a story flag (the trap door to the underground city has fallen?)*/
extern unsigned char D_6FFC[AREA_H][AREA_W];/*AreaLight*/
extern unsigned char far *D_7640;/*TileFlag2: 0x20 background*/
extern struct coord D_78E4[];/*ObjPos*/
extern uint16 D_9CFA[];/*ObjShapeType*/
extern unsigned char far *D_B4FA;/*TerrainType: 0x0f kind (0x04 wall), 0x10 W, 0x20 S, 0x40 E, 0x80 N*/
extern int16 D_BB02;/*AreaOffset*/
extern unsigned char D_BB04[256];/*flood fill ring-buffer queue, x*/
extern unsigned char D_BC04[256];/*flood fill ring-buffer queue, y*/
extern int16 D_C08A[11][11];/*Tile_11x11: the tiles of the map window*/

extern SearchArea(int16, int16, int16, int16);/*SearchArea(x1, y1, x2, y2): first object in the rectangle*/
extern NextArea(void);/*NextArea: next object of SearchArea*/
extern ShowObjects(void);/*ShowObjects: draw the map window*/

#define AreaOffset D_BB02
#define AreaFlags D_3EBA
#define AreaTiles D_470A
#define AreaLight D_6FFC
#define TileFlag D_6DF0
#define TileFlag2 D_7640
#define TerrainType D_B4FA
#define GetType(i) (D_9CFA[i] & 0x3ff)
#define GetFrame(i) (D_9CFA[i] >> 10)
#define GetX(i) ((*(int16 *)&(D_78E4[i].raw[0]))&0x3ff)
#define GetY(i) (((*(int16 *)&(D_78E4[i].raw[1]))>>2)&0x3ff)
#define TILE_FRAME(i) (D_6EF4[GetType(i)] + GetFrame(i))

/*069C*/int16 ViewX = 12;/*the 11x11 view's corner in the area*/
/*069E*/int16 ViewY = 12;
/*06A0*/int16 XRayView = 0;/*never used (U6: X-ray vision, all tiles shown and lit)*/

/*rounded euclidean distance: DistanceTable[|dx|][|dy|] ~ sqrt(dx*dx + dy*dy)*/
/*06A2*/static char DistanceTable[][8] = {
	{0,1,2,3,4,5,6,7},
	{1,1,2,3,4,5,6,7},
	{2,2,3,4,5,6,6,7},
	{3,3,4,4,5,6,7,8},
	{4,4,5,5,6,7,7,8},
	{5,5,6,6,7,7,8,9},
	{6,6,6,7,7,8,8,9},
	{7,7,7,8,8,9,9,10}
};

/*flood fill queue (ring buffer of 256 in D_BB04/D_BC04)*/
/*06E2*/static unsigned char visited_w = 0;
/*06E3*/static unsigned char visited_r = 0;

/*cumulative steps that walk around the 8 neighbours of a square,
  starting from the center: N, NE, E, SE, S, SW, W, NW*/
/*06E4*/static char NeighbourStepX[] = {      0, 1,      0,      0, -1, -1,       0,       0};
/*06EC*/static char NeighbourStepY[] = {-     1, 0,      1,      1,  0,  0, -     1, -     1};
/*06F4*/static char NeighbourStepOfs[] = {-AREA_W, 1, AREA_W, AREA_W, -1, -1, -AREA_W, -AREA_W};

/*wall shapes: tile offset from the wall group's base, indexed by the
  wall's connections restricted to the visible sides
  (bit 8 = N, 4 = E, 2 = S, 1 = W)*/
/*06FC*/static char WallShapeOfs[] = {
	-1,/*----*/
	-1,/*---W*/
	-1,/*--S-*/
	 5,/*--SW*/
	-1,/*-E--*/
	 1,/*-E-W*/
	 0,/*-ES-*/
	 0,/*-ESW*/
	-1,/*N---*/
	 2,/*N--W*/
	 3,/*N-S-*/
	 3,/*N-SW*/
	 4,/*NE--*/
	 1,/*NE-W*/
	 0,/*NES-*/
	-1 /*NESW*/
};

/*can sight/light go through the square AreaOffset (x, y are its area
  coordinates)? opaque objects block, windows are only see-through
  from an orthogonally adjacent square of the party, opaque ground
  tiles block unless a background object is on them*/
static /*C_09DA_000E*/VIS_IsSeeThrough(int16 x, int16 y) {
	int16 flag, tile;

	tile = AreaTiles[0][AreaOffset];
	flag = AreaFlags[0][AreaOffset];
	if(flag & 0x20)
		return 0;
	if((TileFlag[tile] & 0x08) || (flag & 0x10)) {
		return
			(ViewX + 5 == x && abs(ViewY + 5 - y) < 2) ||
			(ViewY + 5 == y && abs(ViewX + 5 - x) < 2)
		;
	}
	if(flag & 0x04)
		return 1;
	if(TileFlag[tile] & 0x04)
		return 0;
	return 1;
}

/*uncalled?*/
/*is (x, y) in the 11x11 view square around the party?*/
/*C_09DA_00A2*/VIS_IsInViewSquare(int16 x, int16 y) {
	return CLOSE_ENOUGH(5, x, y, MapX, MapY);
}

/*queue area square (x, y) for the flood fill and mark it 0x40
  (the queue silently drops squares when full)*/
static /*C_09DA_00D2*/VIS_Enqueue(int16 x, int16 y) {
	if(x >= 0 && x < AREA_W && y >= 0 && y < AREA_H) {
		D_BB04[visited_w] = x;
		D_BC04[visited_w] = y;
		if(++visited_w == visited_r)
			visited_w --;
		else
			AreaFlags[y][x] |= 0x40;
	}
}

/*flood fill from area square (x, y): every square reached gets
  light = clamp(4 - distance + lightBase, 0..4) added to AreaLight;
  the fill spreads from see-through squares to their neighbours.
  mode 1: party's view (marks squares visible 0x80, limited to the
          11x11 view plus a 1-square border),
  mode 2: "peer" (marks visible, whole area),
  mode 0: light source (spreads up to distance lightBase + 3).
  If (x, y) itself blocks sight (e.g. a torch on a wall), the fill
  starts from its east and south neighbours instead.*/
/*C_09DA_0144*/VIS_FloodFill(int16 x, int16 y, int16 lightBase, int16 mode) {
	int16 curX, curY;
	int16 dir, light;

	/*AreaOffset = y * AREA_W + x*/
	AreaOffset = y << 3;
	AreaOffset += AreaOffset << 2;
	AreaOffset += x;
	if(VIS_IsSeeThrough(x, y)) {
		VIS_Enqueue(x, y);
	} else {
		AreaOffset ++;
		if(VIS_IsSeeThrough(x + 1, y))
			VIS_Enqueue(x + 1, y);
		AreaOffset += (AREA_W-1);
		if(VIS_IsSeeThrough(x, y + 1))
			VIS_Enqueue(x, y + 1);
	}
	while(visited_w != visited_r) {
		curX = D_BB04[visited_r];
		curY = D_BC04[visited_r];
		visited_r ++;
		AreaOffset = curY << 3;
		AreaOffset += AreaOffset << 2;
		AreaOffset += curX;

		light = 4 - DistanceTable[abs(curX - x)][abs(curY - y)] + lightBase;
		if(light < 0)
			light = 0;
		else if(light > 4)
			light = 4;
		AreaLight[0][AreaOffset] += light;

		if(mode)
			AreaFlags[0][AreaOffset] |= 0x80;
		if(VIS_IsSeeThrough(curX, curY)) {
			for(dir = 0; dir < 8; dir ++) {
				curX += NeighbourStepX[dir];
				curY += NeighbourStepY[dir];
				AreaOffset += NeighbourStepOfs[dir];
				if(!(AreaFlags[0][AreaOffset] & 0x40)) {
					if(mode == 1) {
						if(
							ViewX - 1 <= curX && ViewX + 11 >= curX &&
							ViewY - 1 <= curY && ViewY + 11 >= curY
						) VIS_Enqueue(curX, curY);
					} else if(mode == 2) {/*for peer*/
						VIS_Enqueue(curX, curY);
					} else if(DistanceTable[abs(curX - x)][abs(curY - y)] <= lightBase + 3) {
						VIS_Enqueue(curX, curY);
					}
				}
			}
		}
	}
}

/*compute the visible 11x11 tiles (D_C08A) and the light of the
  area around the party, then redraw the map window*/
/*C_09DA_0327*/VIS_UpdateView() {
	unsigned char *pTiles,*pLight;
	int16 *pView;
	int16 cellX,x,y;
	int16 tile,terrain,wallBase,visibleSides,wallSides,nbTile,nbTerrain,objNum;
	int16 light,radius;
	register unsigned char *pFlags;
	register int16 cellY;

	/*clear flags and light of the central 24x24 squares*/
	pFlags = &(AreaFlags[8][8]);
	pLight = &(AreaLight[8][8]);
	for(cellX = 8; cellX < 32; cellX ++) {
		for(cellY = 8; cellY < 32; cellY ++) {
			*pFlags++ =
			*pLight++ = 0;
		}
		pFlags += 16;
		pLight += 16;
	}
	/*cellX, cellY = area origin*/
	cellX = (MapX - 0x10) & 0xfff8;
	cellY = (MapY - 0x10) & 0xfff8;
	/*flag what the objects of the central squares do to sight and light*/
	for(
		objNum = SearchArea(cellX + 8, cellY + 8, cellX + 31, cellY + 31);
		objNum >= 0;
		objNum = NextArea()
	) {
		x = GetX(objNum) - cellX;
		y = GetY(objNum) - cellY;
		pFlags = &(AreaFlags[y][x]);
		tile = TILE_FRAME(objNum);

		if(TileFlag[tile] & 0x08)
			*pFlags |= 0x10;
		else if(TileFlag[tile] & 0x04)
			*pFlags |= 0x20;
		else if(TileFlag2[tile] & 0x20)
			*pFlags |= 0x04;

		/*double-size objects also cover the square above/left*/
		if(TileFlag[tile - 1] & 0x04) {
			if((TileFlag[tile] & 0x40) && y)
				*(pFlags - AREA_W) |= 0x20;
			if((TileFlag[tile] & 0x80) && x)
				*(pFlags - 1) |= 0x20;
		}

		if(TerrainType[tile] & 0x04) {
			*pFlags |= 0x08;
			if((TileFlag[tile] & 0x40) && y)
				*(pFlags - AREA_W) |= 0x08;
			if((TileFlag[tile] & 0x80) && x)
				*(pFlags - 1) |= 0x08;
		}

		if(light = TileFlag[tile] & 0x03) {
			if(GetType(objNum) == 0x0b && (D_6EFC[1] & 0x04))
				light = 0;
			else if((*pFlags & 0x03) < light)
				*pFlags = (*pFlags & ~0x03) | light;
		}
	}

	/*what the party sees, lit by the ambient light*/
	VIS_FloodFill(ViewX + 5, ViewY + 5, AmbientLight, 1);

	/*every visible light source adds its own light*/
	if(AmbientLight < 7) {
		pFlags = &(AreaFlags[8][8]);
		pTiles = &(AreaTiles[8][8]);
		for(cellY = 8; cellY < 32; cellY ++) {
			for(cellX = 8; cellX < 32; cellX ++) {
				if((*pFlags & 0x03) && (*pFlags & 0x80)) {
					/*a light on a wall only counts if the square south of it is visible*/
					if(
						(TerrainType[*pTiles] & 0x04) &&
						(!(*(pFlags + AREA_W) & 0x80) || !(*(pFlags + AREA_W) & 0x80))/*twice, as in U6*/
					) continue;
					/*forget the "queued" marks around it*/
					radius = ((*pFlags & 0x03) << 1) - 1;
					for(x = cellX - radius; x <= cellX + radius; x ++) {
						for(y = cellY - radius; y <= cellY + radius; y ++)
							AreaFlags[y][x] &= ~0x40;
					}
					VIS_FloodFill(cellX, cellY, radius - 4, 0);
				}
				pFlags ++;
				pTiles ++;
			}
			pFlags += 16;
			pTiles += 16;
		}
	}

	/*build the 11x11 view*/
	pView = &(D_C08A[0][0]);
	pLight = &(AreaLight[ViewY][ViewX]);
	pFlags = &(AreaFlags[ViewY][ViewX]);
	pTiles = &(AreaTiles[ViewY][ViewX]);
	for(cellY = 0; cellY < 11; cellY ++) {
		for(cellX = 0; cellX < 11; cellX ++) {
			x = cellX + ViewX;
			y = cellY + ViewY;
			if(!(*pFlags & 0x80)) {
				tile = 0xff;/*hidden*/
			} else if(*pLight == 0) {
				tile = 0x1bc;/*darkness*/
			} else {
				tile = *pTiles;
				/*a plain wall tile (wall + impassable) not seen from all
				  4 sides is replaced by the wall shape showing only the
				  sides that are visible*/
				terrain = TerrainType[tile] & 0x0f;
				if(terrain == 6) {
					wallBase = (tile <= 0xa0)?0x120:0x110;
					/*which neighbours are visible (8 N, 4 E, 2 S, 1 W)*/
					visibleSides = 0;
					if(*(pFlags - AREA_W) & 0x80)
						visibleSides |= 8;
					if(*(pFlags + 1) & 0x80)
						visibleSides |= 4;
					if(*(pFlags + AREA_W) & 0x80)
						visibleSides |= 2;
					if(*(pFlags - 1) & 0x80)
						visibleSides |= 1;
					if(visibleSides != 0xf) {
						/*the wall's connections: upper nibble of TerrainType*/
						wallSides = TerrainType[tile] >> 4;
						/*with bit 4, the N connection is dropped unless the
						  north square has a wall object or is a wall tile
						  connected south*/
						if(wallSides & 4) {
							nbTile = *(pTiles - AREA_W);
							nbTerrain = TerrainType[nbTile] & 0x0f;
							if(
								!(*(pFlags - AREA_W) & 0x08) &&
								(!(nbTerrain & 0x04) || !(TerrainType[nbTile] & 0x20))
							) wallSides &= ~8;
						}
						/*with bit 2, the same for the W connection with the
						  west square, connected east*/
						if(wallSides & 2) {
							nbTile = *(pTiles - 1);
							nbTerrain = TerrainType[nbTile] & 0x0f;
							if(
								!(*(pFlags - 1) & 0x08) &&
								(!(nbTerrain & 0x04) || !(TerrainType[nbTile] & 0x40))
							) wallSides &= ~1;
						}
						if(wallSides == (4|2|1) || wallSides == (8|2|1) || wallSides > (8|4) || wallSides == visibleSides) {
							wallSides &= visibleSides;
							tile = wallBase + WallShapeOfs[wallSides];
						}
					}
				}
			}
			*pView++ = tile;
			pFlags ++;
			pTiles ++;
			pLight ++;
		}
		pFlags += 29;/*AREA_W - 11*/
		pTiles += 29;
		pLight += 29;
	}
	ShowObjects();
}
