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
#include <sys/ioctl.h>
#include <unistd.h>

#if HAVE_TERMCAP_H
#include <termcap.h>
#endif


#define MAXCOLS 320

/* do a gotoXY -- allowing -1 for same row/column
 */
void 
dgotoxy(y,x)
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
    
    unless ( os_gotoxy(y,x) )
	dputs(tgoto(CM,x,y));
}


#if USE_TERMCAP
#  if USING_STDIO

#define tputs_putc	putchar

#  else

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

#  endif
#endif


void 
dwrite(s,len)
char *s;
{
    if ( len == 0 )
	return;

    unless ( os_dwrite(s, len) ) {
	logit("dwrite <%.*s>(%d)", len, s, len);
#if 0 /*USING_TERMCAP*/
	tputs(s, len, tputs_putc);
#elif USING_STDIO
	fwrite(s, len, 1, stdout);
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
dputc(c)
char c;
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
	else
	    dputc( yes_or_no ? '[' : ']' );
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
    register st;
    
    if (os_initialize())
	return;

    /* default initialize assumes termcap, so
     * read in the termcap entry for this terminal.
     */
    
    unless ( term=getenv("TERM") )
	term = "dumb";
    
    st = tgetent(tcbuf, term);

    TERMNAME = term;
    bufp = tcbuf+2048;
    CM = tgetstr("cm", &bufp);
    UP = tgetstr("up", &bufp);
    unless ( HO = tgetstr("ho", &bufp) ) {
	char *goto0 = tgoto(CM, 0, 0);

	if (goto0)
	    HO = strdup(goto0);
    }

    CL = tgetstr("cl", &bufp);
    CE = tgetstr("ce", &bufp);
    unless ( BELL = tgetstr("vb", &bufp) )
	BELL = "\007";
    OL = tgetstr("al", &bufp);
    UpS = tgetstr("sr", &bufp);
	
    CURon = tgetstr("ve", &bufp);
    CURoff = tgetstr("vi", &bufp);

    SO = tgetstr("so", &bufp);
    SE = tgetstr("se", &bufp);

    dofscroll = LINES/2;

    /* set cursor movement keys to zero for now */
    FkL = CurRT = CurLT = CurUP = CurDN = EOF;

    canUPSCROLL = (UpS != NULL);
    CA = (CM != NULL);
    canOL = (OL != NULL);

    logit("canUPSCROLL = %d", canUPSCROLL);
    logit("CA = %d", CA);
    logit("canOL = %d", canOL);

#if USING_STDIO
    fflush(stdout);
    setvbuf(stdout, iob, _IOFBF, sizeof iob);
#endif
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
    unless ( os_Ping() )
	dputs(BELL);
}


/* convert a number to a string, w/o using sprintf
 */
void 
numtoa(str,num)
char *str;
int num;
{
    int i = 10;			/* I sure hope that str is 10 bytes long... */
    bool neg = (num < 0);

    if (neg)
	num = -num;

    str[--i] = 0;
    do{
	str[--i] = (num%10)+'0';
	num /= 10;
    }while (num > 0);
    if (neg)
	str[--i] = '-';
    moveleft(&str[i], str, 10-i);
}


/* print out a number, w/o using printf
 */
void 
printi(num)
int num;
{
    char nb[10];
    register int size;
    
    numtoa(nb,num);
    size = Min(strlen(nb),COLS-curpos.x);
    if (size > 0) {
	nb[size] = 0;
	dwrite(nb, size);
	curpos.x += size;
    }
}


/* do a newline, updating x & y
 */
void 
println()
{
    dnewline();
    curpos.x = 0;
    curpos.y = Min(curpos.y+1, LINES-1);
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

    if (c >= ' ' && c < '') {
    	out[0] = c;
    	return 1;
    }
    else if (c == '\t' && !list) {
	register int i;
	int size;

	for (i = size = tabsize - (curpos.x % tabsize);i > 0;)
	    out[--i] = ' ';
	return size;
    }
    else if (c < 128) {
    	out[0] = '^';
    	out[1] = c^64;
    	return 2;
    }
    else {
#if OS_DOS
	out[0] = c;
	return 1;
#else
	out[0] = '\\';
	out[1] = hexdig[(c>>4)&017];
	out[2] = hexdig[c&017];
	return 3;
#endif
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
    int size,oxp = curpos.x;
    char buf[MAXCOLS+1];
    register int bi = 0;

    while (*s && curpos.x < COLS) {
    	size = format(&buf[bi],*s++);
    	bi += size;
    	curpos.x += size;
    }
    size = Min(bi,COLS-oxp);
    if (size > 0) {
	dwrite(buf, size);
    }
}


/* print a line of editor content
 */
void 
writeline(y,x,start)
int y,x,start;
{
    int endd,oxp;
    register int size;
    char buf[MAXCOLS+1];
    register int bi = 0;
    
    endd = fseekeol(start);
    if (start==0 || core[start-1] == EOL)
	dgotoxy(y, 0);
    else
	dgotoxy(y, x);
    oxp = curpos.x;

    while (start < endd && curpos.x < COLS) {
    	size = format(&buf[bi],core[start++]);
    	bi += size;
    	curpos.x += size;
    }
    if (list) {
    	buf[bi++] = '$';
    	curpos.x++;
    }
    size = Min(bi,COLS-oxp);
    dwrite(buf, size);
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
    

    d_cursor(0);
	
    sp = start;
    while (sp <= endd) {
	writeline(y, x, sp);
	sp = 1+fseekeol(sp);
	y++;
	x = 0;
    }
    if (rest && sp >= bufmax)
	while (y<LINES-1) { /* fill screen with ~ */
	    dgotoxy(y, 0);
	    printch('~'); dclear_to_eol();
	    y++;
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
    dgotoxy(LINES-1,0);
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
