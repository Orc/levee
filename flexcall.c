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
 * flexos interface for levee (Metaware C)
 */
#include "levee.h"
#include "externs.h"

#if OS_FLEXOS
#include <stdio.h>
#include <flexos.h>
#include <ctype.h>

static int oldkmode;
static int oldsmode;


/* **** FILE I/O ABSTRACTION **** */

FILEDESC
OPEN_OLD(name)
char *name;
{
    int fd = s_open(name,0x08);

    return ( fd < 0 ) ? NOWAY : (FILEDESC)fd;
}

FILEDESC
OPEN_NEW(name)
char *name
{
    int fd = s_create(0,0,name,0,0/*mode*/,0)

    return ( fd < 0 ) ? NOWAY : (FILEDESC)fd;
}

int
CLOSE_FILE(FILEDESC f)
{
    return s_close(0,(FILEDESC)f);	/* Full close on handle */
}

long
SEEK_POSITION(FILEDESC f, long offset, int mode)
{
    return  s_seek((mode&03)<<9, (long)f, offset);
}

int
READ_TEXT(FILEDESC f, void* buf, int length)
{
    return s_read(0x0100,(long)(FILEDESC),p,(long)(length),0L);
}

int
WRITE_TEXT(FILEDESC f, void *buf, int length)
{
    return s_write(0x0101,(long)(f),buf,(long)(length),0L)
}


int
os_unlink(char *n)
{
    return s_delete(0, n);
}


int
os_rename(char *a,char *b)
{
    return s_rename(0, a, b);
}


int
os_mktemp(char *dest, int size, char *template)
{
    static char Xes[] = ".XXXXXX";
    
    unless ( size > strlen(dest) + sizeof(Xes) ) {
	errno = E2BIG;
	return 0;
    }
	
#if USING_STDIO
    sprintf(dest, "%s%s", template, Xes);

    return mktemp(dest) != 0;
#else
    strcpy(dest, template);
    numtoa(&dest[strlen(dest)], getpid());

    return 1;
#endif
}

/* **** OS-SPECIFIC DISPLAY I/O **** */

os_dwrite(s, len)
char *s;
{
    s_write(0x01, 1L, s, len, 0L);
    return 1;
}


set_input()
{
    CONSOLE tty;

    s_get(T_CON, 1L, &tty, SSIZE(tty));
    oldkmode = tty.con_kmode;
    oldsmode = tty.con_smode;
    tty.con_kmode = 0x0667;
    tty.con_smode = 0;
    s_set(T_CON, 1L, &tty, SSIZE(tty));
}


reset_input()
{
    CONSOLE tty;

    s_get(T_CON, 1L, &tty, SSIZE(tty));
    tty.con_kmode = oldkmode;
    tty.con_smode = oldsmode;
    s_set(T_CON, 1L, &tty, SSIZE(tty));
}


os_initialize()
{
    TERMNAME = "Flexos console";
    HO    = "\033H";
    UP    = "\033A";
    CE    = "\033K";
    CL    = "\033E";
    OL    = "\033L";
    BELL  = "\007";
    CM    = "\033Y??";
    CURoff= "\033f";
    CURon = "\033e";
    SO    = "\033p";
    SE    = "\033q";
    

    CA = 1;
    canUPSCROLL=0;
    canOL=1;

    Erasechar = '\b';	/* ^H */
    Eraseline = 21;	/* ^U */

    return 1;
}


os_d_restore()
{
    return 1;
}

os_cclass(c)
unsigned int c;
{
    if (c == '\t' && !list)
	return CC_TAB;
    if (c == '' || c < ' ')
	return CC_CTRL;
    if (c & 0x80)
	return CC_OTHER;
    return CC_PRINT;
}


char *
dotfile()
{
    return ":LVRC:";
}


getpid()
{
    PROCESS myself;

    s_get(T_PDEF, 0L, &myself, SSIZE(myself));

    return myself.ps_pid;
}


getKey()
{
    char c;

    s_read(0x0101, 0L, &c, 1L, 0L);
    return c;
}


#if !HAVE_BASENAME
char *
basename(s)
char *s;
{
    char *p;

    for ( p=s+strlen(s); p-- > s; ) {
	if (*p == '/' || *p == '\\' || *p == ':' )
	    return 1+p;

    return s;
}
#endif

#endif

