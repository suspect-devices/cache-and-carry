/*
	Copyright 2001, 2002 Georges Menie (www.menie.org)
	stdarg version contributed by Christian Ettinger

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*
	putchar is the only external dependency for this file,
	if you have a working putchar, leave it commented out.
	If not, uncomment the define below and
	replace outbyte(c) by your own function call.

#define putchar(c) outbyte(c)
*/

#include <stdarg.h>

static void doPrintchar(char **str, int c)
{
	extern int putchar(int c);
	
	if (str) {
		**str = c;
		++(*str);
	}
	else (void)putchar(c);
}

#define PAD_RIGHT 1
#define PAD_ZERO 2

static int doPrints(char **out, const char *string, int width, int pad)
{
	register int pc = 0, padchar = ' ';

	if (width > 0) {
		register int len = 0;
		register const char *ptr;
		for (ptr = string; *ptr; ++ptr) ++len;
		if (len >= width) width = 0;
		else width -= len;
		if (pad & PAD_ZERO) padchar = '0';
	}
	if (!(pad & PAD_RIGHT)) {
		for ( ; width > 0; --width) {
			doPrintchar (out, padchar);
			++pc;
		}
	}
	for ( ; *string ; ++string) {
		doPrintchar (out, *string);
		++pc;
	}
	for ( ; width > 0; --width) {
		doPrintchar (out, padchar);
		++pc;
	}

	return pc;
}

/* the following should be enough for 32 bit int */
#define doPrint_BUF_LEN 12

static int doPrinti(char **out, int i, int b, int sg, int width, int pad, int letbase)
{
	char doPrint_buf[doPrint_BUF_LEN];
	register char *s;
	register int t, neg = 0, pc = 0;
	register unsigned int u = i;

	if (i == 0) {
		doPrint_buf[0] = '0';
		doPrint_buf[1] = '\0';
		return doPrints (out, doPrint_buf, width, pad);
	}

	if (sg && b == 10 && i < 0) {
		neg = 1;
		u = -i;
	}

	s = doPrint_buf + doPrint_BUF_LEN-1;
	*s = '\0';

	while (u) {
		t = u % b;
		if( t >= 10 )
			t += letbase - '0' - 10;
		*--s = t + '0';
		u /= b;
	}

	if (neg) {
		if( width && (pad & PAD_ZERO) ) {
			doPrintchar (out, '-');
			++pc;
			--width;
		}
		else {
			*--s = '-';
		}
	}

	return pc + doPrints (out, s, width, pad);
}

static int doPrint( char **out, const char *format, va_list args )
{
	register int width, pad;
	register int pc = 0;
	char scr[2];

	for (; *format != 0; ++format) {
		if (*format == '%') {
			++format;
			width = pad = 0;
			if (*format == '\0') break;
			if (*format == '%') goto out;
			if (*format == '-') {
				++format;
				pad = PAD_RIGHT;
			}
			while (*format == '0') {
				++format;
				pad |= PAD_ZERO;
			}
			for ( ; *format >= '0' && *format <= '9'; ++format) {
				width *= 10;
				width += *format - '0';
			}
			if( *format == 's' ) {
				register char *s = (char *)va_arg( args, int );
				pc += doPrints (out, s?s:"(null)", width, pad);
				continue;
			}
			if( *format == 'd' ) {
				pc += doPrinti (out, va_arg( args, int ), 10, 1, width, pad, 'a');
				continue;
			}
			if( *format == 'x' ) {
				pc += doPrinti (out, va_arg( args, int ), 16, 0, width, pad, 'a');
				continue;
			}
			if( *format == 'X' ) {
				pc += doPrinti (out, va_arg( args, int ), 16, 0, width, pad, 'A');
				continue;
			}
			if( *format == 'u' ) {
				pc += doPrinti (out, va_arg( args, int ), 10, 0, width, pad, 'a');
				continue;
			}
			if( *format == 'c' ) {
				/* char are converted to int then pushed on the stack */
				scr[0] = (char)va_arg( args, int );
				scr[1] = '\0';
				pc += doPrints (out, scr, width, pad);
				continue;
			}
		}
		else {
		out:
			doPrintchar (out, *format);
			++pc;
		}
	}
	if (out) **out = '\0';
	va_end( args );
	return pc;
}

int doPrintf(const char *format, ...)
{
        va_list args;
        
        va_start( args, format );
        return doPrint( 0, format, args );
}

int sdoPrintf(char *out, const char *format, ...)
{
        va_list args;
        
        va_start( args, format );
        return doPrint( &out, format, args );
}


int sndoPrintf( char *buf, unsigned int count, const char *format, ... )
{
        va_list args;
        
        ( void ) count;
        
        va_start( args, format );
        return doPrint( &buf, format, args );
}


#ifdef TEST_doPrintF
int main(void)
{
	char *ptr = "Hello world!";
	char *np = 0;
	int i = 5;
	unsigned int bs = sizeof(int)*8;
	int mi;
	char buf[80];

	mi = (1 << (bs-1)) + 1;
	doPrintf("%s\n", ptr);
	doPrintf("doPrintf test\n");
	doPrintf("%s is null pointer\n", np);
	doPrintf("%d = 5\n", i);
	doPrintf("%d = - max int\n", mi);
	doPrintf("char %c = 'a'\n", 'a');
	doPrintf("hex %x = ff\n", 0xff);
	doPrintf("hex %02x = 00\n", 0);
	doPrintf("signed %d = unsigned %u = hex %x\n", -3, -3, -3);
	doPrintf("%d %s(s)%", 0, "message");
	doPrintf("\n");
	doPrintf("%d %s(s) with %%\n", 0, "message");
	sdoPrintf(buf, "justif: \"%-10s\"\n", "left"); doPrintf("%s", buf);
	sdoPrintf(buf, "justif: \"%10s\"\n", "right"); doPrintf("%s", buf);
	sdoPrintf(buf, " 3: %04d zero padded\n", 3); doPrintf("%s", buf);
	sdoPrintf(buf, " 3: %-4d left justif.\n", 3); doPrintf("%s", buf);
	sdoPrintf(buf, " 3: %4d right justif.\n", 3); doPrintf("%s", buf);
	sdoPrintf(buf, "-3: %04d zero padded\n", -3); doPrintf("%s", buf);
	sdoPrintf(buf, "-3: %-4d left justif.\n", -3); doPrintf("%s", buf);
	sdoPrintf(buf, "-3: %4d right justif.\n", -3); doPrintf("%s", buf);

	return 0;
}

/*
 * if you compile this file with
 *   gcc -Wall $(YOUR_C_OPTIONS) -DTEST_doPrintF -c doPrintf.c
 * you will get a normal warning:
 *   doPrintf.c:214: warning: spurious trailing `%' in format
 * this line is testing an invalid % at the end of the format string.
 *
 * this should display (on 32bit int machine) :
 *
 * Hello world!
 * doPrintf test
 * (null) is null pointer
 * 5 = 5
 * -2147483647 = - max int
 * char a = 'a'
 * hex ff = ff
 * hex 00 = 00
 * signed -3 = unsigned 4294967293 = hex fffffffd
 * 0 message(s)
 * 0 message(s) with %
 * justif: "left      "
 * justif: "     right"
 *  3: 0003 zero padded
 *  3: 3    left justif.
 *  3:    3 right justif.
 * -3: -003 zero padded
 * -3: -3   left justif.
 * -3:   -3 right justif.
 */

#endif

