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
#include <stdlib.h>

int amatch(char*,char*,char*);
int locate(char*,char*);
void patsize(char**);

static int arg;		/* arguments inside of a RE */

int
REmatch(char *pattern, int start, int end)
{
    char *endp = &core[end];

    if (!*pattern)
	return -1;
    arg = 0;
    while (start <= end && !amatch(pattern, &core[start], endp))
	start++;
    return start;
}

int
omatch(char *pattern, char **cp, char *endp)
{
    register int flag;
    extern int ignorecase;

    switch (*pattern) {
      case LEND:
	  return (**cp == EOL);
      case LSTART:
	  return (*cp == core) || (*(*cp-1) == EOL);
      case TOKENB:
	  return (*cp == core) || !isalnum(*(*cp-1));
      case TOKENE:
	  return !isalnum(**cp);
      case LITCHAR:
	  if (ignorecase)
	      flag = (toupper(**cp) == toupper(*(pattern+1)));
	  else
	      flag = (**cp == *(pattern+1));
	  break;
      case ANY:
	  flag = (**cp != EOL);
	  break;
      case CCL:
	  flag = locate(pattern,*cp);
	  break;
      case NCCL:
	  flag = !locate(pattern,*cp);
	  break;
      case ARGSTART:
	  RE_start[arg] = (*cp)-core;
	  return TRUE;
      case ARGEND:
	  RE_size[arg] = ((*cp)-core) - RE_start[arg];
	  ++arg;
	  return TRUE;
      default:
	  return TRUE;
    }

    if (*cp <= endp && flag) {
	(*cp)++;
	return TRUE;
    }
    return FALSE;
}

int
amatch(char *pattern,char *start,char *endp)
{
    int sarg = arg;	/* save old arg match count for errors */

    while (*pattern) {
	if (*pattern == CLOSURE) {
	/*	Find the longest closure possible and work backwards trying
	 *	to match the rest of the pattern.
	 */
	    char *oldstart = start;

	    ++pattern;	/* skip over the closure token */
	    while (start <= endp && omatch(pattern,&start,endp))
		 ;
	/*	start points at the character that failed the search.
	 *	Try to match the rest of the pattern against it, working
	 *	back down the line if failure
	 */
	    patsize(&pattern);
	    while (start >= oldstart)
		if (amatch(pattern,start--,endp))
		    return TRUE;
	    arg = sarg;
	    return FALSE;
	}
	else {
	    if (!omatch(pattern,&start,endp)) {
		arg = sarg;
		return FALSE;
	    }
	    patsize(&pattern);
	}
    }
    lastp = start-core;
    return TRUE;
}

/*  increment pattern by the size of the token being scanned
 */
void
patsize(char **pattern)
{
    register int count;

    switch (**pattern) {
      case LITCHAR:
	*pattern += 2;
	break;
      case CCL:
      case NCCL:
	count = *(++*pattern) & 0xff;
	*pattern += 1+count;
	break;
      default:
	(*pattern)++;
	break;
    }
}

int
locate(char *pattern,register char *linep)
/* locate: find a character in a closure */
{
    register char *p = 1+pattern;
    register int count;

    if ((count = (*p++)&0xff) == 0)
	return FALSE;
    while (count--)
	if (*p++ == *linep)
	    return TRUE;
    return FALSE;
}
char *p;

void
concatch(char c)
/* add a character to the pattern */
{
    if (p < &pattern[MAXPAT-1])
	*p++ = c;
}

char
esc(char **s)
{
    if (**s != ESCAPE || *(1+*s) == 0)
	return **s;
    ++(*s);
    switch (**s) {
      case 't': return TAB;
      case 'n': return EOL;
    }
    return **s;
}

char *
dodash(char *src)
/* parse the innards of a [] */
{
    int k;
    char *start = src;
    char cs[128];

    fillchar(cs,sizeof(cs),FALSE);
    while (*src && *src != CCLEND) {
	if (*src == DASH && src>start && src[1] != CCLEND && src[-1]<src[1]) {
	    for ( k = src[-1]; k <= src[1]; k++)
		cs[k] = TRUE;
	    src++;
	}
	else
	    cs[(unsigned int)esc(&src)] = TRUE;
	src++;
    }
    for (k=0; k < sizeof(cs); k++)
	if (cs[k])
	    concatch((char)k);
    return src;
}

char *
badccl(char *src)
/* a [] was encountered. is it a CCL (match one of the included
 *  characters); or is it a NCCL (match all but the included characters)?
 */
{
    char *jstart;

    if (*src == NEGATE) {
	concatch(NCCL);
	++src;
    }
    else
	concatch(CCL);
    jstart = p;
    concatch(0);		/* this will be the length of the pattern */
    src = dodash(src);
    *jstart = (p-jstart)-1;
    return src;
}

	/* patterns that cannot be closed */
char badclose[] = { LSTART, LEND, CLOSURE, 0 };

char *
makepat(char *string,int delim)
/* make up the pattern string for find	-- ripped from 'Software Tools' */
{
    char *cp = 0, *oldcp;
    char *start = string;
    int inarg = FALSE;

    for(arg=0;arg<9;++arg)
	RE_start[arg] = RE_size[arg] = (-1);
    arg = 0;
    p = pattern;

    while ((*string != delim) && (*string != 0)) {
	oldcp = cp;
	cp = p;

	if ((*string == LSTART) && (string == start))
	    concatch(LSTART);
	else if ((*string == LEND) && (string[1] == delim || string[1] == 0))
	    concatch(LEND);
	else if (!magic)	/* ^ & $ are significant no matter what */
	    goto normal;
	else if (*string == ANY)
	    concatch(ANY);
	else if (*string == CCL)
	    string = badccl(1+string);
	else if ((*string == CLOSURE) && (p > pattern)) {
	    cp = oldcp;
	    if (strchr(badclose, *cp) || p >= &pattern[MAXPAT-1])
		return NULL;
	    moveright(cp,1+cp,(int)(p-cp));
	    *cp = CLOSURE;
	    p++;
	}
	else if (*string == ESCAPE) {
	    if (string[1] == ARGSTART || string[1] == ARGEND) {
		if (string[1] == ARGEND)
		    if (!inarg)
			goto normal;
		if (string[1] == ARGSTART) {
		    if (inarg)
			goto normal;
		    if (++arg > 9)
			return NULL;
		}
		inarg = !inarg;
	    }
	    else if (string[1] != TOKENB && string[1] != TOKENE)
		goto normal;
	    ++string;
	    concatch(*string);
	}
	else {
     normal:concatch(LITCHAR);
	    concatch(esc(&string));
	}
	if (*string)
	    string++;
    }
    if (inarg)
	concatch(ARGEND);
    if (p > pattern) {		/* new pattern was created */
	strncpy(lastpatt,start,(int)(string-start));
	lastpatt[string-start] = 0;
	concatch(0);
	if (p-pattern >= MAXPAT)
	    return NULL;
    }
    return (*string == delim)?(string+1):(string);
}

int
findfwd(char *pattern, int start, int endp)
/* look for a regular expression forward */
{
    int ep;

     while (start < endp) {
	 ep = fseekeol(start);
	 if ((start = REmatch(pattern, start, ep)) <= ep)
	     return start;
     }
     return ERR_NOMATCH;
 }

int
findback(char *pattern, int start, int endp)
/* look for a regular expression backwards */
{
    int ep,i;

    while (start > endp) {
	ep = bseekeol(start);
	if ((i = REmatch(pattern, ep, start)) <= start)
	    return i;
	start = ep-1;
    }
    return ERR_NOMATCH;
}


char *
expr_errstring(int errno)
{
    switch (errno) {
    case ERR_NOMATCH:	return "Pattern not found";
    case ERR_PATTERN:	return "Unreadable pattern";
    case ERR_RANGE:	return "Out of range";
    case ERR_UNDEF:	return "Marker unset";
    case ERR_EXPR:	return "Impossible address";
    default:		return "error?";
    }

}


bool s_wrapped = 0;

int
search(int start, char **bufp)
/* get a token for find & find it in the buffer
 */
{
    int  pos;
    char marker = **bufp;
    bool forwd = (lsearch = marker) == '/';


    unless ( *bufp = makepat(1+(*bufp), marker) )
	return ERR_PATTERN;


    do {
	if (forwd) {
	    pos = findfwd(pattern, start+1, bufmax-1);
	    if ((pos == ERR_NOMATCH) && wrapscan) {
		s_wrapped = 1;
		pos = findfwd(pattern, 0, start-1);
	    }
	}
	else {
	    pos = findback(pattern, start-1, 0);
	    if ((pos == ERR_NOMATCH) && wrapscan) {
		s_wrapped = 1;
		pos = findback(pattern, bufmax-1, start+1);
	    }
	}
	unless ( pos > ERR ) {
	    logit(("search: error %s", expr_errstring(pos)));
	    return pos;
	}
	start = pos;
    } while ( --count > 0);

    return start;
}

static int
address_fragment(int start, char **bufp, int offset)
/* parse part of an address (a /pattern/, ?pattern?, [0-9]*, 0, $) */
{
    int addr;

    count = s_wrapped = 0;

    switch (**bufp) {
    case '/':			/* search forward */
	    if ( offset < 0 )
		return ERR_RANGE;
	    addr = search(start, bufp);
	    break;
    case '?':			/* search backwards */
	    if ( offset > 0 )
		return ERR_RANGE;
	    addr = search(start, bufp);
	    break;

	    addr = search(start, bufp);
	    break;

    case '0':			/* line # or offset */
    case '1': case '2': case '3':
    case '4': case '5': case '6':
    case '7': case '8': case '9':
	    /* fabricate a count */
	    count = strtol(*bufp, bufp, 10);

	    if ( offset ) {
		if ( count )
		    addr = nextline((offset>0), start, count);
	    }
	    else
		addr = to_index(count);
	    break;

    case '$':			/* jump to end of file */
	if ( offset )
	    return ERR_EXPR;
	addr = bufmax-1;
	++(*bufp);
	break;

    case '.' :
	if ( offset )
	    return ERR_EXPR;
	addr = start;
	++(*bufp);
	break;

    default:
	return ERR_UNKNOWN;
    }
    return addr;
}


int
findparse(int start, char **bufp, int offset) /* driver for ?, /, && : lineranges */
{
    int addr;
    char c;

    c = **bufp;

    if ( c == '`' || c == '\'' ) {
	unless ( (addr = lvgetcontext( (*bufp)[1], c == '\'')) > ERR )
	    return ERR_UNDEF;

	(*bufp) += 2;
    }
    else
	addr = address_fragment(start, bufp, offset);

    if ( addr > ERR )
	start = addr;
    else if ( addr != ERR_UNKNOWN )
	return addr;

    c = **bufp;

    logit(("initial fragment: addr=%d, c=%c", addr, c));

    while ( c == '+' || c == '-' ) {
	++(*bufp);
	offset = (c == '+') ? 1 : -1;

	addr = address_fragment(start, bufp, offset);

	if ( addr > ERR )
	    start = addr;
	else if ( addr == ERR_UNKNOWN ) {
	    logit(("after+ ERR_UNKNOWN (next char %d)", **bufp));
	    return nextline( (offset > 0), start, 1);
	}
	else
	    return addr;
	c = **bufp;
    }
    return addr;
}

int
nextline(bool advance, int dest, int count)
{
    int start = dest;
    int ocount= count;

    if (advance) {
	do {
	    dest = fseekeol(dest) + 1;
	    count--;
	} while (count>0 && dest<bufmax);
    }
    else {
	do {
	    dest = bseekeol(dest) - 1;
	    count--;
	} while (count>0 && dest>=0);
	dest = bseekeol(dest);
    }
    logit(("nextline: advance=%d, start=%d, count=%d -> dest=%d",
		 advance, start, ocount, dest));
    return(dest);
}

int
fseekeol(int origin)
{
#if 0
    char *res = memchr(core+origin, EOL, bufmax-origin);

    if ( res )
	return res-core;
    else
	return bufmax;
#endif

    return(origin + scan(bufmax-origin-1,'=',EOL,&core[origin]));
}

int
bseekeol(int origin)
{
    return(origin + scan(-origin,'=',EOL,&core[origin-1]));
}

/* get something from the context table */

int
lvgetcontext(int c, bool begline)
{
    int i;

    if (c == '\'')
	c = '`';
    if (c >= '`' && c <= 'z')
	i = contexts[c-'`'];
    else
	i = -1;
    if (begline && i>=0)
	return(bseekeol(i));
    return(i);
}
