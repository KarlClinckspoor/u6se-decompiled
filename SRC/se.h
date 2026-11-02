/*
 * The Savage Empire (Worlds of Ultima) - GAME.EXE
 * Types and macros shared by the game's modules: only the ones that do not
 * depend on where a variable is. Each module declares the variables and
 * functions it uses itself (extern), by address name (D_XXXX, C_SSSS_OOOO)
 * or by the name sesym.py gives them. Taken from Ultima VI's u6.h/portable.h.
 */
#ifndef __SE_H__
#define __SE_H__

#include <dos.h>
#include <stdlib.h>
#include <alloc.h>
#include <process.h>
#include <ctype.h>
#include <string.h>

typedef int int16;
typedef unsigned uint16;
typedef long int32;
typedef unsigned long uint32;

/*a far pointer seen as offset:segment, as a data pointer or as a function*/
union tHandler {
	struct {
		uint16 ofs, seg;
	} iii;
	void far *p;
	void far *(*f)();
};

/*an object's packed position: x (10 bits), y (10 bits), z (4 bits)*/
struct coord {
	unsigned char raw[3];
};

#define SCR2TIL(i) ((i) >> 4)
#define TIL2SCR(i) ((i) << 4)

#define DELTA(a,b) (abs((a)-(b)))
#define CLOSE_ENOUGH(d,x1,y1,x2,y2) (DELTA(x1,x2) <= (d) && DELTA(y1,y2) <= (d))
#define CLOSE_ENOUGH_S(d,x1,y1,x2,y2) (DELTA(x1,x2) < (d) && DELTA(y1,y2) < (d))
#define DELTA0(a,b) (__abs__((a)-(b)))
#define CLOSE_ENOUGH0(d,x1,y1,x2,y2) (DELTA0(x1,x2) <= (d) && DELTA0(y1,y2) <= (d))
#define CLOSE_ENOUGH0_S(d,x1,y1,x2,y2) (DELTA0(x1,x2) < (d) && DELTA0(y1,y2) < (d))

/*Turbo C's variable arguments: the address of "..." (each argument a word); written so
  that Turbo C++ 1.00 steps the pointer first, as Origin's code does*/
#define VA_LIST void **
#define VA_START(ap, last) ap = ...
#define VA_INT(ap) (((int16 *)++(ap))[-1])
#define VA_CHAR(ap) (((char *)++(ap))[-2])
#define VA_STR(ap) (((char **)++(ap))[-1])

#endif
