/* flags: -mm -c -O -G */
/*
 * Worlds of Ultima: The Savage Empire - SAVAGE.EXE
 * the OSI library: formatted output to the text window
 * (U.EXE's TXTPRINT, newer: far strings, pascal output routines, %J)
 */
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

/*va_arg on a register ap (Turbo C++'s takes the address of ap)*/
#undef va_arg
#define va_arg(ap, type) (*(type *)(((char *)(ap) += sizeof(type)) - sizeof(type)))

struct tSurface;
/*a text window (see OSI.INC)*/
struct tText {
	struct tSurface *surface;/*+00*/
	int curX, curY;/*+02 the cursor*/
	unsigned char far *font;/*+06 height (word), ..., width of each character at +4*/
	char fgColor;/*+0A*/
	char bgColor;/*+0B*/
	char *buffer;/*+0C string buffer of the formatted output*/
	char *bufPtr;/*+0E where its next character goes*/
	char justify;/*+10*/
};

extern struct tText *pText;/*pText: the current text window*/
extern void pascal TXT_puts(char far *s);/*TXT_puts: print a string, word-wrapped and justified*/
extern void pascal TXT_putc(int c);/*TXT_putc: print a character*/
extern void pascal TXT_bufc(int c);/*TXT_bufc: add a character to the window's string buffer*/

/*output a string, a character at a time*/
/*C_078E_0000*/static outstr(void (pascal *out)(int), char far *s) {
	register int c;

	while((c = *s++) != 0)
		(*out)(c);
}

/*the formatter: %d %u %D (long) %U (unsigned long) %x (upper case hex) %c %s, and
  %X %Y: move the cursor, %F %B: foreground, background color, %J: justification,
  %P: print the window's string buffer*/
/*C_078E_001D*/static format(void (pascal *out)(int), char far *fmt, register va_list ap) {
	char buf[64];
	int c;
	register int c2;

	while((c = *fmt++) != 0) {
		switch(c) {
			case '%':
				c2 = *fmt++;
				switch(c2) {
					case 'd': outstr(out, itoa(va_arg(ap, int), buf, 10)); break;
					case 'u': outstr(out, ultoa(va_arg(ap, unsigned), buf, 10)); break;
					case 'D': outstr(out, ltoa(va_arg(ap, long), buf, 10)); break;
					case 'U': outstr(out, ultoa(va_arg(ap, unsigned long), buf, 10)); break;
					case 'x': outstr(out, strupr(ultoa(va_arg(ap, unsigned), buf, 16))); break;
					case 'c': (*out)(va_arg(ap, int)); break;
					case 's': outstr(out, va_arg(ap, char *)); break;
					case 'X': pText->curX = va_arg(ap, int); break;
					case 'Y': pText->curY = va_arg(ap, int); break;
					case 'B': pText->bgColor = va_arg(ap, int); break;
					case 'F': pText->fgColor = va_arg(ap, int); break;
					case 'P': TXT_puts(pText->buffer); break;
					case 'J': pText->justify = va_arg(ap, int); break;
					default: (*out)(c2); break;
				}
				break;
			default:
				(*out)(c);
				break;
		}
	}
}

/*print to the text window*/
/*C_078E_0230*/TXT_printf(char far *fmt, ...) {
	register va_list ap;

	va_start(ap, fmt);
	format(TXT_putc, fmt, ap);
}

/*format into the window's string buffer*/
/*C_078E_0252*/TXT_sprintf(char far *fmt, ...) {
	register va_list ap;

	va_start(ap, fmt);
	pText->bufPtr = pText->buffer;
	format(TXT_bufc, fmt, ap);
}

/*and add to it*/
/*C_078E_027E*/TXT_sappend(char far *fmt, ...) {
	register va_list ap;

	va_start(ap, fmt);
	format(TXT_bufc, fmt, ap);
}
