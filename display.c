/*
 * LEVEE, or Captain Video;  A vi clone
 *
 * Copyright (c) 1982-2007 David L Parsons
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, without or
 * without modification, are permitted provided that the above
 * copyright notice and this paragraph are duplicated in all such
 * forms and that any documentation, advertising materials, and
 * other materials related to such distribution and use acknowledge
 * that the software was developed by David L Parsons (orc@pell.portland.or.us).
 * My name may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE.
 */
#include "levee.h"
#include "extern.h"

#include <stdlib.h>
#include <string.h>

#if HAVE_TERMCAP_H
#include <termcap.h>
#endif


#define MAXCOLS 320

/* do a gotoXY -- allowing -1 for same row/column
 */
void
dgotoxy(x,y)
{
    if (y == -1)
	y = curpos.y;
    else
	curpos.y = y;
    if (y >= LINES)
	y = LINES-1;
    if (x == -1)
	x = curpos.x;
    else
	curpos.x = x;
    if (x >= COLS)
	x = COLS-1;

    unless ( os_gotoxy(x,y) ) {
#if 0
	char *gt = tgoto(CM, x, y);
	logit(("dgotoxy(%d,%s) -> <%s>", x, y, gt));
#else
	dputs(tgoto(CM,x,y));
#endif
    }
}


#if USE_TPUTS && !USING_STDIO

/* write a single character to output w/o the assistance
 * of stdio.
 */
static int
tputs_putc(c)
char c;
{
    char s[1];
    s[0] = c;
    return write(1, s, 1) == 1 ? c : EOF;
}

#endif


void
dwrite(s,len)
char *s;
{
    if ( len <= 0 )
	return;

    unless ( os_write(s, len) ) {
	logit(("dwrite <%.*s>(%d)", len, s, len));
#if USING_STDIO
	fwrite(s, len, 1, stdout);
#elif USING_TPUTS
	tputs(s, len, tputs_putc);
#else
	write(1, s, len);
#endif
    }
}


/* write a string to our display
 */
void
dputs(s)
char *s;
{
    if ( s )
	dwrite(s, strlen(s));
}


/* write a character to our display
 */
void
dputc(char c)
{
    char s[1];
    s[0] = c;
    dwrite(s, 1);
}


/* add a blank line to the screen at our current row
 */
void
dopenline()
{
    unless ( os_openline() )
	dputs(OL);
}


/* spit out a newline
 */
void
dnewline()
{
    unless ( os_newline() )
	dputs("\r\n");
}


/* clear the screen
 */
void
dclearscreen()
{
    unless ( os_clearscreen() )
	dputs(CL);
}


/* clear to end of line
 */
void
dclear_to_eol()
{
    unless ( os_clear_to_eol() )
	dputs(CE);
}


/* turn the cursor off or on
 */
void
d_cursor(visible)
{
    unless ( os_cursor(visible) )
	dputs(visible ? CURon : CURoff);
}


/* highlight text
 */
void
d_highlight(yes_or_no)
{
    unless ( os_highlight(yes_or_no) ) {
	if ( SO && SE )
	    dputs( yes_or_no ? SO : SE );
    }
}


/* get the screensize
 */
void
dscreensize(x,y)
int *x;
int *y;
{
    int li, co;

    unless ( os_screensize(x,y) ) {
	li = tgetnum("li");
	co = tgetnum("co");

	if ( (li > 0) && (co > 0) ) {
	    (*x) = co;
	    (*y) = li;
	}
	else {
	    (*x) = 0;
	    (*y) = 0;
	}
    }
}


/* initialize everything
 */
void
dinitialize()
{
    static char tcbuf[4096];
#if USING_STDIO
    static char iob[4096];
#endif
    char *term, *bufp;

#if USING_STDIO
    fflush(stdout);
    setvbuf(stdout, iob, _IOFBF, sizeof iob);
#endif

    if (os_initialize())
	return;

    /* default initialize assumes termcap, so
     * read in the termcap entry for this terminal.
     */

    unless ( term=getenv("TERM") )
	term = "dumb";

    tgetent(tcbuf, term);

    TERMNAME = term;
    bufp = tcbuf+2048;
    CM = tgetstr("cm", &bufp);
    UP = tgetstr("up", &bufp);
    unless ( HO = tgetstr("ho", &bufp) ) {
	char *goto0 = tgoto(CM, 0, 0);

	if (goto0)
	    HO = strdup(goto0);
    }

    logit(("CM=%s", CM));

    CL = tgetstr("cl", &bufp);
    CE = tgetstr("ce", &bufp);
#if 0
    unless ( BELL = tgetstr("vb", &bufp) )
	BELL = "\007";
#else
    BELL = "\007";
#endif
    OL = tgetstr("al", &bufp);
    UpS = tgetstr("sr", &bufp);

    CURon = tgetstr("ve", &bufp);
    CURoff = tgetstr("vi", &bufp);

    SO = tgetstr("so", &bufp);
    SE = tgetstr("se", &bufp);

    /* set cursor movement keys to zero for now */
    FkL = CurRT = CurLT = CurUP = CurDN = EOF;

    canUPSCROLL = (UpS != NULL);
    CA = (CM != NULL);
    canOL = (OL != NULL);
}


/* restore everything back to normal
 */
void
drestore()
{
    os_restore();
}


/* ring the bell
 */
void
Ping()
{
    unless (bell)	/* if the bell is turned off, do nothing */
	return;
    unless ( os_Ping() )
	dputs(BELL);
}


/* convert a number to a string, w/o using sprintf
 */
char *
ntoa (int n)
{
    static char bfr[(7+(8*sizeof(int)))/3] = { 0 };
    int i;
    int neg = n < 0;
    char *q = bfr + sizeof bfr;

    if ( neg )
	n = -n;

    for ( *--q = '0' + (n%10);  ((n /= 10) > 0); )
	*--q = '0' + (n%10);

    if ( neg )
	*--q = '-';
    return q;
}


/* print out a number, w/o using printf
 */
void
printi(num)
int num;
{
    prints(ntoa(num));
}


/* do a newline, updating x & y
 */
void
println()
{
    curpos.x = 0;
    curpos.y = Min(curpos.y+1, LINES-1);
    dnewline();
}


/* format: put a displayable version of c into out
 *    ^<x> for control-<x>
 *    spaces for <tab>
 *    normal for everything else
 */
int
format(out,c)
register char *out;
register unsigned c;
{
    static char hexdig[] = "0123456789ABCDEF";
    register int i;
    int size;

    switch (os_cclass(c)) {
    case CC_TAB:
	    for (i = size = tabsize - (curpos.x % tabsize);i > 0;)
		out[--i] = ' ';
	    return size;
    case CC_CTRL:    /* control character, represented by ^(c^64) */
	    out[0] = '^';
	    out[1] = c^64;
	    return 2;
    case CC_PRINT:    /* printable */
	    out[0] = c;
	    return 1;
    default:
	    out[0] = '\\';
	    out[1] = hexdig[(c>>4)&017];
	    out[2] = hexdig[c&017];
	    return 3;
    }
}


/* print a formatted block of text
 */
void
printbuf(s, len)
char *s;
{
    int size,oxp = curpos.x;
    char buf[MAXCOLS+1];
    register int bi = 0;

    while (len > 0 && curpos.x < COLS) {
	size = format(&buf[bi],*s++);
	bi += size;
	curpos.x += size;
	--len;
    }
    size = Min(bi,COLS-oxp);
    if (size > 0) {
	dwrite(buf, size);
    }
}


/* print a formatted character
 */
void
printch(c)
char c;
{
    register int size;
    char buf[MAXCOLS];

    size = Min(format(buf,c),COLS-curpos.x);
    if (size > 0) {
	dwrite(buf, size);
	curpos.x += size;
    }
}


/* print a formatted string
 */
void
prints(s)
char *s;
{
    printbuf(s, strlen(s));
}


/* print a line of editor content
 */
void
writeline(y,x,start)
int y,x,start;
{
    int endd,oxp;

    endd = fseekeol(start);
    if (start==0 || core[start-1] == EOL)
	dgotoxy(0, y);
    else
	dgotoxy(x, y);

    printbuf(&core[start], endd-start);

    if ( list )
	printch('$');

    if (curpos.x < COLS)
	dclear_to_eol();
}


/* redraw && refresh the screen
 */
void
refresh(y,x,start,endd,rest)
int y,x,start,endd;
bool rest;
{
    int sp;
    extern int screenlines;


    d_cursor(0);

    sp = start;
    while (sp <= endd) {
	writeline(y, x, sp);
	sp = 1+fseekeol(sp);
	y++;
	x = 0;
    }
    screenlines = y;
    if (rest && sp >= bufmax)
	if ( y<LINES-1) {
	    dgotoxy(0, y);
	    while (y<LINES-1) {
		printch('~');
		dclear_to_eol();
		if (++y != LINES-1)
		    dnewline();
	    }
	}

    d_cursor(1);
}


/* redraw everything */

void
redisplay(flag)
bool flag;
{
    if (flag)
	clrprompt();
    refresh(0, 0, ptop, pend, TRUE);
}

void
scrollback(curr)
int curr;
{
    dgotoxy(0,0);		/* move to the top line */
    do {
	ptop = bseekeol(ptop-1);
	unless ( os_scrollback() )
	    dputs(UpS);
	writeline(0, 0, ptop);
    } while (ptop > curr);
    setend();
}

void
scrollforward(curr)
int curr;
{
    do {
	writeline(LINES-1, 0, pend+1);
	dnewline();
	pend = fseekeol(pend+1);
	ptop = fseekeol(ptop)+1;
    } while (pend < curr);
}

/* find if the number of lines between top && bottom is less than dofscroll */

bool
ok_to_scroll(top,bottom)
int top,bottom;
{
    int nl, i;

    nl = dofscroll;
    i = top;
    do
	i += 1+scan(bufmax-i,'=',EOL, &core[i]);
    while (--nl > 0 && i < bottom);
    return(nl>0);
}

void
clrprompt()
{
    dgotoxy(0, LINES-1);
    dclear_to_eol();
}

void
prompt(toot,s)
bool toot;
char *s;
{
    if (toot)
	error();
    clrprompt();
    prints(s);
}
