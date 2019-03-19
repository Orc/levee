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
/*
 * Gemdos (Atari ST) bindings for levee (Alcyon/Sozobon C)
 */
#include "levee.h"
#include "externs.h"

#if OS_ATARI
#include <stdio.h>
#include <atari\osbind.h>

long _STKSIZ = 4096;
long _BLKSIZ = 4096;

extern long gemdos();


FILEDESC
OPEN_OLD(char *file)
{
    int fd = gemdos(0x3d,file,/*open mode*/0);

    return (fd < 0) ? NOWAY : (FILEDESC)fd;
}

    
FILEDESC
OPEN_NEW(char *file)
{
    int fd = gemdos(0x3c,n,/*permissions*/0);

    return (fd < 0) ? NOWAY : (FILEDESC)fd;
}


int
CLOSE_FILE(FILEDESC f)
{
    return gemdos(0x3e,(int)f);
}


long
SEEK_POSITION(FILEDESC f, long offset, int mode)
{
    return gemdos(0x42,offset,(int)f, mode);
}


int
READ_TEXT(FILEDESC f, void *buffer, int size)
{
    return gemdos(0x3f,(int)f,(long)(size),buffer);
}
    

int
WRITE_TEXT(FILEDESC f, void *buffer,int size)
{
    return gemdos(0x40,(int)f,(long)(size),buffer);
}


os_dwrite(s,len)
char *s;
{
    return 0;
}


os_initialize()
{
    TERMNAME = "Atari ST";
    HO    = "\033H",
    UP    = "\033A",
    CE    = "\033K",
    CL    = "\033E",
    OL    = "\033L",
    BELL  = "\007",
    CM    = "\033Y??",
    UpS   = "\033I",
    CURoff= "\033f",
    CURon = "\033e";
    SO    = "\033p";
    SE    = "\033q";

    canUPSCROLL = TRUE;
    canOL = TRUE;
    CA = TRUE;

    Erasechar = '\b';	/* ^H */
    Eraseline = 21;	/* ^U */
    
    return 1;
}

os_cursor(visible)
{
    if ( visible ) {
	/* turn the cursor on */
	asm(" clr.w  -(sp)     ");
	asm(" move.w #1,-(sp)  ");
	asm(" move.w #21,-(sp) ");
	asm(" trap   #14       ");
	asm(" addq.l #6,sp     ");
    }
    else {
	/* turn the cursor off */
	asm(" clr.l  -(sp)     ");
	asm(" move.w #21,-(sp) ");
	asm(" trap   #14       ");
	asm(" addq.l #6,sp     ");
    }
}


os_mktemp(dest, size template)
char *dest;
char *template;
{
    char *p;
    static char Xes[] = ".XXXXXX";
    int required = sizeof(Xes) + strlen(template) + 1;

    if (p=getenv("_TMP")) {

	unless ( size > strlen(p) + required + 1 ) {
	    errno = E2BIG;
	    return 0;
	}

	strcpy(dest, p);

	lastchar = dest[strlen(dest)-1];

	if ( lastchar != '\\' && lastchar != ':' )
	    strcat(dest, "\\");
    }
    else {
	unless (size > required) {
	    errno = E2BIG;
	    return 0;
	}
	s[0] = 0;    
    }

#if USING_STDIO
    sprintf(dest+strlen(dest), "%s%s", template, Xes);

    return mktemp(dest) != 0;
#else
    strcat(s, template);
    numtoa(&s[strlen(s)], getpid());

    return 1;
#endif
}


#if ATARI_SOUND
static char sound[] = {
	0xA8,0x01,0xA9,0x01,0xAA,0x01,0x00,
	0xF8,0x10,0x10,0x10,0x00,0x20,0x03
};

#define SADDR	0xFF8800L

typedef char srdef[4];

static srdef *SOUND = (srdef *)SADDR;
    
/* Make the Atari ST bell sound by stuffing a chord into the sound chip
 */
static void
ping()
{
    register i;

    for (i=0; i<sizeof(sound); i++) {
	(*SOUND)[0] = i;
	(*SOUND)[2] = sound[i];
    }
}
#endif


int
os_Beep()
{
#if ATARI_SOUND
    Supexec(ping);

    return 1;
#else
    return 0;
#endif
}


int
cclass(c)
unsigned int c;
{
    if (c == '\t' && !list)
	return CC_TAB;

    if (c == 127 || c < 32 )
	return CC_CTRL;
    
    return CC_PRINT;
}


unsigned
getKey()
/* get input from the keyboard. All odd keys (function keys, et al) that
 * do not produce a character have their scancode orred with $80 and returned.
 */
{
    unsigned c;
    long key;

    c = (key = Crawcin()) & 0xff;

    unless (c)
	c = (((unsigned)(key>>16))|0x80) & 0xff;
    return c;
} /* getKey */


/*
 * basename() returns the filename part of a pathname
 */
char *
basename(s)
register char *s;
{
    register char *p = s;
    
    for (p = s+strlen(s); --p > s; )
	if ( *p == '\\' || *p == ':' )
	    return p+1;
    return s;
} /* basename */
#endif

#endif /*OS_ATARI*/
