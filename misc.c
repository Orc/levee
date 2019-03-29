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
#include <ctype.h>
#include <errno.h>

bool
lvgetline(str, size)
char *str;
{
    int len;
    char flag;

    flag = line(str, 0, Min(size, COLS-curpos.x), &len);
    str[len] = 0;
    if (lineonly)
	dnewline();
#if 0
    else
	dclear_to_eol();
#endif
    return (flag == EOL);
} /* lvgetline */


char
readchar()
{
    ch = peekc();		/* get the peeked character */
    needchar = TRUE;		/* force a read on next readchar/peekc */
    if (xerox) {		/* save this character for redo */
	if (rcp >= &rcb[256-1])	/* oops, buffer overflow */
	    error();
	else			/* concat it at the end of rcb^ */
	    *rcp++ = ch;
    }
    return ch;
} /* readchar */


/* look at next input character without actually using it */
char
peekc()
{
    if (needchar) {				/* if buffer is empty, */
	if (macro >= 0) {			/* if a macro */
	    lastchar = *mcr[macro].ip;
	    mcr[macro].ip++;
	    if (*mcr[macro].ip == 0) {
		if (--mcr[macro].m_iter > 0)
		    mcr[macro].ip = mcr[macro].mtext;
		else
		    --macro;
	    }
	}
	else				/* else get one from the keyboard */
	    lastchar = getKey();
        needchar = FALSE;
    }
    return lastchar;
} /* peekc */


/* find the amount of leading whitespace between start && limit.
   endd is the last bit of whitespace found.
*/
int
findDLE(start, endd, limit, dle)
int start, *endd, limit, dle;
{
    while ((core[start] == '\t' || core[start] == ' ') && start < limit) {
	if (core[start] == '\t')
	    dle = tabsize * (1+(dle/tabsize));
	else
	    dle++;
	start++;
    }
    *endd = start;
    return dle;
} /* findDLE */


int
skipws(loc)
int loc;
{
    while ((core[loc] == '\t' || core[loc] == ' ') && loc <= bufmax)
	loc++;
    return(loc);
} /* skipws */


int
setX(cp)
int cp;
{
    int top, xp;

    top = bseekeol(cp);
    xp = 0;
    while (top < cp) {
	switch (os_cclass(core[top])) {
	    case CC_TAB:
		    xp = tabsize*(1+(xp/tabsize));
		    break;
	    case CC_CTRL:
		    xp += 2;
		    break;
	    case CC_PRINT:
		    xp++;
		    break;
	    default:
		    xp += 3;
		    break;
	}
	top++;
    }
    return(xp);
} /* setX */


int
setY(cp)
int cp;
{
    int yp, ix;

    ix = ptop;
    yp = -1;
    cp = Min(cp,bufmax-1);
    do {
	yp++;
	ix = 1+fseekeol(ix);
    } while (ix <= cp);
    return(yp);
} /* setY */


int
to_line(cp)
int cp;
{
    int tdx,line;
    tdx = 0;
    line = 0;
    while (tdx <= cp) {
	tdx = 1+fseekeol(tdx);
	line++;
    }
    return(line);
} /* to_line */


int
to_index(line)
int line;
{
    int cp = 0;
    while (cp < bufmax && line > 1) {
	cp = 1+fseekeol(cp);
	line--;
    }
    return(cp);
} /* to_index */


void
swap(a,b)
int *a,*b;
{
    int c;

    c = *a;
    *a = *b;
    *b = c;
} /* swap */


void
error()
{
    indirect = 0;
    macro = -1;
    if (xerox)
	rcb[0] = 0;
    xerox = FALSE;

    Ping();
} /* error */


/* the dirty work to start up a macro */
void
insertmacro(cmdstr, count)
char *cmdstr;
int count;
{
    if (macro >= NMACROS)
	error();
    else if (*cmdstr != 0) {
	macro++;
	mcr[macro].mtext = cmdstr;	/* point at the text */
	mcr[macro].ip = cmdstr;		/* starting index */
	mcr[macro].m_iter = count;	/* # times to do the macro */
    }
} /* insertmacro */


int
lookup(c)
char c;
{
    int ix = MAXMACROS;

    while (--ix >= 0 && mbuffer[ix].token != c)
	;
    return ix;
} /* lookup */


void
fixmarkers(base,offset)
int base,offset;
{
    unsigned char c;

    for (c = 0;c<'z'-'`';c++)
	if (contexts[c] > base) {
	    if (contexts[c]+offset < base || contexts[c]+offset >= bufmax)
		contexts[c] = -1;
	    else
		contexts[c] += offset;
	}
} /* fixmarkers */


void
wr_stat()
{
    clrprompt();
    if ( filenm != F_UNSET ) {
	printch('"');
	prints(args.gl_pathv[filenm]);
	prints("\" ");
	if (newfile)
	    prints("<New file> ");
	printch(' ');
	if (readonly)
	    prints("<readonly> ");
	else if (modified)
	    prints("<Modified> ");

	if (bufmax > 0) {
	    prints(" line ");
	    printi(to_line(curr));
	    prints(" -");
	    printi((int)((long)(curr*100L)/(long)bufmax));
	    prints("%-");
	}
	else
	    prints("-empty-");
    }
    else
	prints("No file");
} /* wr_stat */


static int  tabptr,
	    tabstack[20],
	    ixp;

void
back_up(c)
char c;
{
    int original_xp = ixp, count;
    
    switch (os_cclass(c)) {
	case CC_TAB:
	    ixp = tabstack[--tabptr];
	    break;
	case CC_CTRL:
	    ixp -= 2;
	    break;
	case CC_PRINT:
	    ixp--;
	    break;
	default:
	    ixp -= 3;
	    break;
    }
    
    count = original_xp - ixp;
    
    dgotoxy(ixp, -1);
    
    if ( count > 0 ) {
	do {
	    dwrite("        ", count%8);
	    count -= 8;
	} while ( count > 0 );
	dgotoxy(ixp, -1);
    }
} /* back_up */


/*
 *  put input into buf[] || instring[].
 *  return states are:
 *	0 : backed over beginning
 *    ESC : ended with an ESC
 *    EOL : ended with an '\r'
 */
char
line(s, start, endd, size)
char *s;
int start, endd, *size;
{
    int col0,
	ip;
    unsigned char c;

    col0 = ixp = curpos.x;
    ip = start;
    tabptr = 0;
    while (1) {
	c = readchar();
	if (movemap[c] == INSMACRO)	/* map!ped macro */
	    insertmacro(mbuffer[lookup(c)].m_text, 1);
	else if (c == DW) {
	    while (!wc(s[ip-1]) && ip > start)
		back_up(s[--ip]);
	    while (wc(s[ip-1]) && ip > start)
		back_up(s[--ip]);
	}
	else if (c == Eraseline) {
	    ip = start;
	    tabptr = 0;
	    dgotoxy(ixp=col0, -1);
	}
	else if (c == Erasechar) {
	    if (ip>start)
		back_up(s[--ip]);
	    else {
		*size = 0;
		return(0);
	    }
	}
	else if (c=='\r' || c==ESC) {
	    *size = (ip-start);
            return (c==ESC) ? ESC : EOL;
	}
	else if ((!beautify) || c == TAB || c == ''
			     || (c >= ' ' && c <= '~')
			     || (c & 0x80) ) {
	    if (ip < endd) {
		if (c == '')
		    c = readchar();
		switch (os_cclass(c)) {
		    case CC_TAB:
			tabstack[tabptr++] = ixp;
			ixp = tabsize*(1+(ixp/tabsize));
			break;
		    case CC_CTRL:
			ixp += 2;
			break;
		    case CC_PRINT:
			ixp++;
			break;
		    default:
			ixp += 3;
			break;
		}
		s[ip++] = c;
		printch(c);
	    }
	    else
		error();
	}
        else
	    error();
    }
} /* line */


/* move to core[loc] */
void
setpos(loc)
int loc;
{
    lstart = bseekeol(loc);
    lend = fseekeol(loc);
    xp = setX(loc);
    curr = loc;
} /* setpos */


void
resetX()
{
    if (deranged) {
	xp = setX(curr);
	dgotoxy(xp, -1);
	deranged = FALSE;
    }
} /* resetX */


/* set end of window */
int
setend()
{
    int bottom, count;
    int lines = 0;

    bottom = ptop;
    count = LINES-1;
    while (bottom < bufmax && count > 0) {
	bottom = 1+fseekeol(bottom);
	count--;
	lines++;
    }
    pend = bottom-1;		/* last char before eol || eof */
    return lines;
} /* setend */


/*  set top of window
 *  return the number of lines actually between curr && ptop.
 */
int
settop(lines)
int lines;
{
    int top, yp;

    top = curr;
    yp = -1;
    do {
	yp++;
	top = bseekeol(top) - 1;
	lines--;
    } while (top >= 0 && lines > 0);
    ptop = top+1;			/* tah-dah */
    setend();
    return(yp);
} /* settop */



int
Max(a,b)
{
    return (a>b) ? a : b;
}



int
Min(a,b)
{
    return (a<b) ? a : b;
}


/*
 * return a tempfile name in malloc()ed memory
 */
char *
lvtempfile(char *template)
{
    int length=40;
    char *file = malloc(length);
    char *p;

    while (file) {
       if ( os_mktemp(file, length, template) )
           return file;

       unless (errno == E2BIG) {
           free(file);
           return 0;
       }

       length *= 2;
       if (p = realloc(file, length))
           file = p;
       else {
           free(file);
           return 0;
       }
    }
    return 0;
}


#if !HAVE_STRDUP
char *
strdup(s)
char *s;
{
    char *p;

    if (p=malloc(strlen(s)+1))
	strcpy(p, s);
    return p;
}
#endif
