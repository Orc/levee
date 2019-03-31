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
#include "grep.h"

#include <ctype.h>

/* modification commands that can be accessed by either editcore || execmode */

/* put stuff into the yank buffer */

bool
doyank(low, high)
int low, high;
{
    FILEDESC f;
    register int sz;
    
    yank.size = high - low;
    moveleft(&core[low], yank.stuff, Min(yank.size, SBUFSIZE));
    if (yank.size > SBUFSIZE) {
	if ((f=OPEN_NEW(yankbuf)) != NOWAY ) {
	    low += SBUFSIZE;
	    sz = WRITE_TEXT(f, core+low, high-low);
	    CLOSE_FILE(f);
	    if (sz == high-low)
		return TRUE;
	}
	yank.size = -1;
	return FALSE;
    }
    return TRUE;
}

bool
deletion(low, high)
int low,high;
{
    if (doyank(low, high))		/* fill yank buffer */
	return delete_to_undo(&undo, low, high-low);
    return FALSE;
}

/* move stuff from the yank buffer into core */

bool
putback(start, newend)
int start, *newend;
{
    int siz, st;
    FILEDESC f;
    
    if (yank.size+bufmax < EDITSIZE && yank.size > 0) {
	*newend = start + yank.size;
	if (start < bufmax)
	    moveright(&core[start], &core[start+yank.size], bufmax-start);
	moveleft(yank.stuff, &core[start], Min(SBUFSIZE, yank.size));
	if (yank.size > SBUFSIZE) {
	    siz = yank.size - SBUFSIZE;
	    if ( (f=OPEN_OLD(yankbuf)) != NOWAY ) {
		st = READ_TEXT(f, &core[start+SBUFSIZE], siz);
		CLOSE_FILE(f);
		if (st == siz)
		    goto succeed;
	    }
	    moveleft(&core[start+yank.size], &core[start], bufmax-start);
	    *newend = -1;
	    return FALSE;
	}
    succeed:
	insert_to_undo(&undo, start, yank.size);
	return TRUE;
    }
    return FALSE;
}

#define DSIZE 1024

int
makedest(str,start,ssize,size)
/* makedest: make the replacement string for an regular expression */
char *str;
int start, ssize, size;
{
    char *fr = dst;
    char *to = str;
    int as, asize, c;

    while (*fr && size >= 0) {
	if (*fr == AMPERSAND) {	/* & copies in the pattern that we matched */
	    if ((size -= ssize) < 0)
		return -1;
	    moveleft(&core[start],to,ssize);
	    to += ssize;
	    fr++;
	}
	else if (*fr == ESCAPE) {	/* \1 .. \9 do arguments */
	    c = fr[1];
	    fr += 2;
	    if (c >= '1' && c <= '9') {
		if ((as = RE_start[c-'1']) < 0)
		    continue;
		asize = RE_size [c-'1'];
		if ((size -= asize) < 0)
		    return -1;
		moveleft(&core[as],to,asize);
		to += asize;
	    }
	    else
		*to++ = c;
	}
	else {
	    *to++ = *fr++;
	    --size;
	}
    }
    return to-str;
}

int
chop(start,endd,visual,query)
int start,*endd;
bool visual, *query;
{
    int i,retval;
    char c;
/*>>>>
    bool ok;
  <<<<*/
    char dest[DSIZE];
    register int len, dlen;
    
    retval = -1;
    /*dlen = strlen(dst);*/
restart:
    count = 1;
    if ( (i = findfwd(pattern, start, *endd)) > ERR ) {
	if (*query) {
	    /*>>>> don't delete -- keep for future use
	    if (visual) {
		dgotoxy(setX(i), yp);puts("?");
	    }
	    else {
	    <<<<*/
		println();
		writeline(-1,-1,bseekeol(i));
		println();
		dgotoxy(setX(i), -1);
		prints("^?");
	    /*>>>>
	    }
	    <<<<*/
	    do
		c = tolower(readchar());
	    while (c!='y'&&c!='n'&&c!='q'&&c!='a');
	    if (c == 'n') {
		start = i+1;
		goto restart;
	    }
	    else if (c == 'q')
		return retval;
	    else if (c == 'a')
		*query = FALSE;
	}
	len = lastp-i;
	dlen = makedest(dest, i, len, DSIZE);
	if (dlen >= 0 && bufmax-(int)(len+dlen) < EDITSIZE
		      && delete_to_undo(&undo, i, len)) {
	    modified = TRUE;
	    if (dlen > 0) {
		moveright(&core[i], &core[i+dlen], bufmax-i);
		insert_to_undo(&undo,i,dlen);
		moveleft(dest,&core[i],dlen);
	    }
	    *endd += (dlen-len);
	    retval = i+dlen;
	}
    }
    return retval;
}
