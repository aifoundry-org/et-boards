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
 * Modified: 11/3/2021 6:26:28 PM
 *  Author: Mac_Marshall
 */

/*
    putchar is the only external dependency for this file,
    if you have a working putchar, leave it commented out.
    If not, uncomment the define below and
    replace outbyte(c) by your own function call.

*/
#include <compiler.h>
#include <stdarg.h>
#include <stdbool.h>

#include "utils.h"


static void printchar(char **str, void (*afterNlHook)(char), int c);
static int prints(char **out, void (*afterNlHook)(char), const char *string, int width, int pad, int precision);
static int printi(char **out, void (*afterNlHook)(char), int64_t i, int b, int sg, int width, int pad, int letbase);

int printf(const char *format, ...)
{
    va_list args;
    va_start( args, format );
    return printArgs( NULL, NULL, format, args );
}

int vprintf(const char *format, va_list args)
{
    return printArgs( NULL, NULL, format, args );
}

int sprintf(char *out, const char *format, ...)
{
    va_list args;
    va_start( args, format );
    return printArgs( &out, NULL, format, args );
}

#if 0
int snprintf( char *buf, unsigned int count, const char *format, ... )
{
    va_list args;

    ( void ) count;

    va_start( args, format );
    return printArgs( &buf, NULL, format, args );
}
#endif


#define PAD_RIGHT 1
#define PAD_ZERO 2
int printArgs( char **out, void (*afterNlHook)(char), const char *format, va_list args )
{
    register int width=0, pad=0, precision=0, lcount=0;
    register int pc = 0;
    char scr[2];

    for (; *format != 0; ++format) {
        if (*format == '%') {
            ++format;
            lcount = width = pad = precision = 0;
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
            if (*format == '*') {
                width = va_arg( args, int);
                format++;
                } else {
                for ( ; *format >= '0' && *format <= '9'; ++format) {
                    width *= 10;
                    width += *format - '0';
                }
            }
            if (*format == '.') {
                format++;
                if (*format == '*') {
                    precision = va_arg( args, int);
                    format++;
                    } else {
                    for ( ; *format >= '0' && *format <= '9'; ++format) {
                        precision *= 10;
                        precision += *format - '0';
                    }
                }
            }
            if( *format == 's' ) {
                register char *s = (char *)va_arg( args, int );
                pad &=~PAD_ZERO;
                pc += prints (out, afterNlHook, s?s:"(null)", width, pad, precision);
                continue;
            }
            while( *format == 'l' ) { // interpret %ld as %d since both work on long
                format++;
                ++lcount;
            }
            if (*format == 'p') {
                pc += printi (out, afterNlHook, (int) va_arg( args, void * ), 16, 0, sizeof(void *)*2, PAD_ZERO, 'A');
                continue;
            }
            if( *format == 'd' ) {
                pc += lcount<2 ? printi (out, afterNlHook, (int32_t)va_arg( args, int32_t ), 10, 1, width, pad, 'a')
                : printi (out, afterNlHook, va_arg( args, int64_t ), 10, 1, width, pad, 'a');
                continue;
            }
            if( *format == 'x' ) {
                pc += lcount<2 ? printi (out, afterNlHook, (uint32_t)va_arg( args, int32_t ), 16, 0, width, pad, 'a')
                : printi (out, afterNlHook, va_arg( args, int64_t ), 16, 0, width, pad, 'a');
                continue;
            }
            if( *format == 'X' ) {
                pc += lcount<2 ? printi (out, afterNlHook, (uint32_t)va_arg( args, int32_t ), 16, 0, width, pad, 'A')
                : printi (out, afterNlHook, va_arg( args, int64_t ), 16, 0, width, pad, 'A');
                continue;
            }
            if( *format == 'u' ) {
                pc += lcount<2 ? printi (out, afterNlHook, (uint32_t)va_arg( args, int32_t ), 10, 0, width, pad, 'a')
                : printi (out, afterNlHook, va_arg( args, int64_t ), 10, 0, width, pad, 'a');
                continue;
            }
            if( *format == 'c' ) {
                /* char are converted to int then pushed on the stack */
                scr[0] = (char)va_arg( args, int );
                scr[1] = '\0';
                pc += prints (out, afterNlHook, scr, width, pad, 0);
                continue;
            }
        }
        else {
            out:
            printchar (out, afterNlHook, *format);
            ++pc;
        }
    }
    if (out) **out = '\0';
    if( afterNlHook )
        (*afterNlHook)(0); // end of line
    va_end( args );
    return pc;
}

static int prints(char **out, void (*afterNlHook)(char), const char *string, int width, int pad, int precision)
{
    register int pc = 0, padchar = ' ';
    register int len = 0;

    if (pad & PAD_ZERO) padchar = '0';
    if (precision == 0) {
        for (const char *ptr = string; *ptr; ++ptr) ++len;
        } else {
        len = precision;
    }
    if (width > 0) {
        if (len >= width) width = 0; else width -= len;
    }
    if (!(pad & PAD_RIGHT)) {
        for ( ; width > 0; --width) {
            printchar (out, afterNlHook, padchar);
            ++pc;
        }
    }
    for ( ; (*string != '\0') && (len > 0); ++string, len--) {
        printchar (out, afterNlHook, *string);
        ++pc;
    }
    for ( ; width > 0; --width) {
        printchar (out, afterNlHook, padchar);
        ++pc;
    }

    return pc;
}


/* the following should be enough for 64 bit int */
#define PRINT_BUF_LEN 24

static int printi(char **out, void (*afterNlHook)(char), int64_t i, int b, int sg, int width, int pad, int letbase)
{
    char print_buf[PRINT_BUF_LEN];
    register char *s;
    register int neg = 0, pc = 0;
    register uint64_t t, u = (uint64_t)i;

    if (i == 0) {
        print_buf[0] = '0';
        print_buf[1] = '\0';
        return prints (out, afterNlHook, print_buf, width, pad, 0);
    }

    if (sg && b == 10 && i < 0) {
        neg = 1;
        u = (uint64_t)-i;
    }

    s = print_buf + PRINT_BUF_LEN-1;
    *s = '\0';

    while (u) {
        t = u % b;
        if( t >= 10 )
        t += letbase - '0' - 10;
        *--s = (char)(t + '0');
        u /= b;
    }

    if (neg) {
        if( width && (pad & PAD_ZERO) ) {
            printchar (out, afterNlHook, '-');
            ++pc;
            --width;
        }
        else {
            *--s = '-';
        }
    }

    return pc + prints (out, afterNlHook, s, width, pad, 0);
}


static void printchar(char **str, void (*afterNlHook)(char), int c)
{
    if (str) {
        *(*str)++ = (char)c; //    wrong    *(++(*str)) = (char)c;
    }
    else
    {
        putchar(c);
        if( afterNlHook )
            (*afterNlHook)(c); // not end of line
    }
}




#ifdef TEST_PRINTF
int main(void)
{
    char *ptr = "Hello world!";
    char test[] = {'T', 'E', 'S', 'T'};
    char *np = 0;
    int i = 5;
    unsigned int bs = sizeof(int)*8;
    int mi;
    char buf[80];

    mi = (1 << (bs-1)) + 1;
    printf("%s\n", ptr);
    printf("printf test\n");
    printf("%s is null pointer\n", np);
    printf("%d = 5\n", i);
    printf("%d = - max int\n", mi);
    printf("char %c = 'a'\n", 'a');
    printf("hex %x = ff\n", 0xff);
    printf("hex %02x = 00\n", 0);
    printf("hex %08x = 01234567\n", 0x1234567);
    printf("hex %0*x = 01234567 width as arg\n", 8, 0x1234567);
    printf("signed %d = unsigned %u = hex %x\n", -3, -3, -3);
    printf("%d %s(s)%", 0, "message");
    printf("\n");
    printf("%d %s(s) with %%\n", 0, "message");
    printf("%.4s\r\n", test);
    printf("%.*s width as arg\r\n", 4, test);
    printf("0x%p\r\n", &test);
    sprintf(buf, "justif: \"%-10s\"\n", "left"); printf("%s", buf);
    sprintf(buf, "justif: \"%10s\"\n", "right"); printf("%s", buf);
    sprintf(buf, " 3: %04d zero padded\n", 3); printf("%s", buf);
    sprintf(buf, " 3: %-4d left justif.\n", 3); printf("%s", buf);
    sprintf(buf, " 3: %4d right justif.\n", 3); printf("%s", buf);
    sprintf(buf, "-3: %04d zero padded\n", -3); printf("%s", buf);
    sprintf(buf, "-3: %-4d left justif.\n", -3); printf("%s", buf);
    sprintf(buf, "-3: %4d right justif.\n", -3); printf("%s", buf);

    return 0;
}


/*
 * if you compile this file with
 *   gcc -Wall $(YOUR_C_OPTIONS) -DTEST_PRINTF -c printf.c
 * you will get a normal warning:
 *   printf.c:214: warning: spurious trailing `%' in format
 * this line is testing an invalid % at the end of the format string.
 *
 * this should display (on 32bit int machine) :
 *
 * Hello world!
 * printf test
 * (null) is null pointer
 * 5 = 5
 * -2147483647 = - max int
 * char a = 'a'
 * hex ff = ff
 * hex 00 = 00
 * signed -3 = unsigned 4294967293 = hex fffffffd
 * 0 message(s)
 * 0 message(s) with %
 * TEST
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


/* To keep linker happy. */
//int    write( int i, char* c, int n)
//{
//    (void)i;
//    (void)n;
//    (void)c;
//    return 0;
//}

