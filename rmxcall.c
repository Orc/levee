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
 * iRMX interface for levee (Intel C)
 */

#include "levee.h"
#include "extern.h"

#if OS_RMX

#include <:inc:stdio.h>
#include <:inc:udi.h>

extern alien token rq$c$create$command$connection(),
		   rq$c$delete$command$connection(),
		   rq$c$send$command(),
		   rq$get$task$tokens();	/* for unique files */
extern alien rq$s$write$move();


FILESPEC
OPEN_OLD(char *file)
{
    int fd = open(file, /*open mode*/0);

    return (fd == -1) ? NOWAY : (FILESPEC)fd;
}

FILESPEC
OPEN_NEW(char *file)
{
    int fd = creat(file,/*permissions*/0);

    return (fd == -1) ? NOWAY : (FILESPEC)fd;
}

int
CLOSE_FILE(FILESPEC f)
{
    return close((int)f);
}

long
SEEK_POSITION(FILESPEC f, long offset, int size)
{
    return lseek((int)f, offset, mode);
}

int
READ_TEXT(FILESPEC f, char *buffer, int size)
{
    return read((int)f,buffer, size);
}

int
WRITE_TEXT(FILESPEC f, char *buffer, int size)
{
    return write((int)f,buffer,size);
}


os_write(s,len)
char *s;
{
    int dummy;

    rq$s$write$move(fileno(stdout), s, len, &dummy);
    return 1;
}


os_mktemp(file,size,template)
char *file;
char *template;
{
    token dummy;
    static char Xes[] = ":WORK:";

    unless (size > sizeof(Xes) + strlen(template) + 20) {
	errno = E2BIG;
	return 0;
    }

#if USING_STDIO
    sprintf(s, "%s%s%d", Xes, template, rq$get$task$tokens(0,&dummy));
#else
    strcpy(s, Xes);
    strcat(s, template);
    strcat(s, ntoa(rq$get$task$tokens(0,&dummy)));
#endif

    return 1;
}


os_initialize()
{
    Erasechar = 127;	/* ^? */
    Eraseline = 21;	/* ^U */

    return 1;
}


os_restore()
{
    return 1;
}


os_set_input()
{
    unsigned dummy;

    dq$special(1,&fileno(stdin),&dummy);

    /* turn off control character assignments */
    strput("\033]T:C15=0,C18=0,C20=0,C21=0,C23=0\033\\");

    return 1;
}


os_reset_input()
{
    strputs("\033]T:C15=3,C18=13,C20=5,C21=6,C23=4\033\\\n");
    dq$special(2,&fileno(stdin),&curr);

    return 1;
}

char
getKey()
/* getKey: read a character from stdin */
{
    char c,sw;
    unsigned dummy;

    read(0,&c,1);

    if (c == FkL) {	/* (single character) function key lead-in */
	dq$special(3,&fileno(stdin),&dummy);	/* grab a raw-mode character */
	if (read(0,&sw,1) == 1)
	    if (sw == CurLT)
		c = LTARROW;
	    else if (sw == CurRT)
		c = RTARROW;
	    else if (sw == CurUP)
		c = UPARROW;
	    else if (sw == CurDN)
		c = DNARROW;
	    else
		c = sw | 0x80;
	dq$special(1,&fileno(stdin),&dummy);	/* back into transparent mode */
    }
#if 0
    else if (c == 0x7f)	/* good old dos kludge... */
	return erase;
#endif
    return c;
}

int
os_subshell(s)
/* system: do a shell escape */
char *s;
{
    char *string();
    unsigned cp, error, status, dummy;

    cp = rq$c$create$command$connection(fileno(stdin),fileno(stdout),0,&error);
    if (!error) {
	rq$c$send$command(cp,string(s),&status,&error);
	rq$c$delete$command$connection(cp,&dummy);
    }

    return error?(error|0x8000):(status&0x7fff);
}


int
os_cclass(c)
unsigned int c;
{
    if (c == '\t' && !list)
	return CC_TAB;
    if (c == 127 || c < ' ')
	return CC_CTRL;
    if (c & 0x80)
	return CC_OTHER;
    return CC_PRINT;
}

#endif
