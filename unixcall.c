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
 * Unix interface for levee
 */
#include "levee.h"

#ifdef OS_UNIX

#include "extern.h"
#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/ioctl.h>


/* **** FILE IO ABSTRACTIONS **** */
FILEDESC
OPEN_OLD(char *name)
{
    int fd = open(name, O_RDONLY);

    return (fd == -1) ? NOWAY : (FILEDESC)fd;
}

FILEDESC
OPEN_NEW(char *name)
{
    int fd = open(name, O_WRONLY|O_CREAT|O_TRUNC, 0666);

    return (fd == -1) ? NOWAY : (FILEDESC)fd;
}

int
CLOSE_FILE(FILEDESC f)
{
    return close( (int)f );
}

long
SEEK_POSITION(FILEDESC f, long offset, int mode)
{
    return lseek((int)f, offset, mode);
}

int
READ_TEXT(FILEDESC f, void *buf, int size)
{
    return read((int)f, buf, size);
}

int
WRITE_TEXT(FILEDESC f, void *buf, int size)
{
    return write((int)f, buf, size);
}


/* *** UNIX-SPECIFIC CONSOLE I/O *** */
int
os_write(s,len)
char *s;
{
    return 0;
}


int
os_gotoxy(y,x)
{
    return 0;
}


int
os_dwrite(s,len)
char *s;
{
    return 0;
}


int
os_newline()
{
    return 0;
}


int
os_clearscreen()
{
    return 0;
}


int
os_clear_to_eol()
{
    return 0;
}


int
os_cursor(int visible)
{
    return 0;
}

int
os_scrollback()
{
    return 0;
}

int
os_scrolldown()
{
    return 0;
}

int
os_openline()
{
    return 0;
}

int
os_highlight(int yes_or_no)
{
    return 0;
}

int
os_Ping()
{
    return 0;
}

/* get the screensize, if we can
 */
int
os_screensize(x,y)
int *x;
int *y;
{
#if defined(TIOCGSIZE)
	struct ttysize tty;
	if (ioctl(0, TIOCGSIZE, &tty) == 0) {
	    if (tty.ts_lines) (*y)=tty.ts_lines;
	    if (tty.ts_cols)  (*x)=tty.ts_cols;

	    logit("os_screensize: col=%d,row=%d", tty.ts_cols, tty.ts_lines);
	    return 1;
	}
#elif defined(TIOCGWINSZ)
	struct winsize tty;
	if (ioctl(0, TIOCGWINSZ, &tty) == 0) {
	    if (tty.ws_row) (*y)=tty.ws_row;
	    if (tty.ws_col) (*x)=tty.ws_col;

	    logit("os_screensize: col=%d,row=%d", tty.ws_col, tty.ws_row);
	    return 1;
	}
#endif
    return 0;
}

#if !HAVE_TCGETATTR
#define tcgetattr(fd,t)	ioctl(fd, TCGETS, t)
#define tcsetattr(fd,n,t) ioctl(fd, n, t)
#define TCSANOW	TCSETAF
#endif


static struct termios old;


int
os_initialize()
{
    tcgetattr(0, &old);	/* get editing keys */

    Erasechar = old.c_cc[VERASE];
    Eraseline = old.c_cc[VKILL];

    return 0;
}


int
os_restore()
{
    return 1;
}


int
os_rename(char *from, char *to)
{
    return rename(from, to);
}


int
os_unlink(char *file)
{
    return unlink(file);
}


int
os_mktemp(char *dest, const char *template)
{
    /* assert |dest| > |/tmp/|+|template|+|XXXXXX| */
#if USING_STDIO
    sprintf(dest, "/tmp/%sXXXXXX", template);
    strcpy(dest, template);

    return mktemp(dest) != 0;
#else
    strcpy(dest, "/tmp");
    strcat(dest, template);
    numtoa(&dest[strlen(dest)], getpid());
    
    return 1;
#endif
}


/* put the terminal into raw mode
 */
void
set_input()
{
    struct termios new = old;

    new.c_iflag &= ~(IXON|IXOFF|IXANY|ICRNL|INLCR);
    new.c_lflag &= ~(ICANON|ISIG|ECHO);
    new.c_oflag = 0;

    tcsetattr(0, TCSANOW, &new);
}


/* reset the terminal to what is was before
 */
void
reset_input()
{
     tcsetattr(0, TCSANOW, &old);
}


/* what does our dotfile look like
 */
char *
dotfile()
{
    /* should expand username */

    return "~/.lvrc";
}


/* get a single keypress from the console
 */
int
getKey()
{
    unsigned char c[1];
    fd_set input;


#if USING_STDIO
    logit("getKey: flush stdout");
    fflush(stdout);
#endif
    /* we're using Unix select() to wait for input, so lets hope that
     * all the Unices out there support select().  If your Unix doesn't,
     * you can make this work by replacing this select loop with:
     *
     *       while (read(0,c,1) != 1)
     *           ;
     *       return c[1];
     *
     * ... and watch your load-average peg.
     */
    while (1) {
	FD_ZERO(&input);
	FD_SET(0, &input);

	select(1, &input, 0, 0, 0);
	if( read(0,c,1) == 1) {
#if 0
	    if (c[0] >= ' ' && c[0] < 127 )
		fprintf(stderr, "getKey -> %c\n", c[0]);
	    else
		fprintf(stderr, "getKey -> %02x\n", (unsigned char)c[0]);
#endif
	    return c[0];
	}
    }
}

#if !HAVE_BASENAME
char *
basename(s)
char *s;
{
    char *p = strrchr(s, '/');

    return p ? 1+p : s;
}
#endif

#endif
