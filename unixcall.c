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
#include "extern.h"

#ifdef OS_UNIX

#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/ioctl.h>
#if HAVE_PWD_H
#include <pwd.h>
#if HAVE_UUID_UUID_H
#include <uuid/uuid.h>
#endif
#endif
#include <errno.h>


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
os_mktemp(char *dest, int size, const char *template)
{
    static char Xes[] = ".XXXXXX";
    static char tmp[] = "/tmp/";
    
    /* assert |dest| > |/tmp/|+|template|+|XXXXXX| */
    unless (size > sizeof(tmp) + sizeof(Xes) + strlen(template)) {
	errno = E2BIG;
	return 0;
    }

#if USING_STDIO
    sprintf(dest, "%s%s%s", tmp, template, Xes);
    strcpy(dest, template);

    return mktemp(dest) != 0;
#else
    strcpy(dest, tmp);
    strcat(dest, template);
    numtoa(&dest[strlen(dest)], getpid());

    return 1;
#endif
}


/*
 * implement the glob() command (with GLOB_NOSORT always set)
 */
int
os_glob(const char* pattern, int flags, glob_t *result)
{
#if USING_GLOB
    return glob(pattern, flags|GLOB_NOSORT, 0, result);
#else
    /* whoops.  No wildcards here.
    */
    char **newlist;
    int count = 1 + (result->gl_flags & GLOB_DOOFFS ? result->gl_offs : 0);

#if LOGGING
    if ( result->gl_flags & GLOB_APPEND )
	logit("os_glob: add %s to arglist", pattern);
    else
	logit("os_glob: create new arglist, initialized with %s", pattern);
#endif

    if ( result->gl_pathc == 0 )
	result->gl_flags = flags;

    if ( result->gl_flags & GLOB_APPEND )
	count += result->gl_pathc;

    unless ( flags & GLOB_NOMAGIC ) {
	if ( strcspn(pattern, "?*") < strlen(pattern) ) {
	    result->gl_flags |= GLOB_MAGCHAR;
	    unless (result->gl_flags & GLOB_NOCHECK)
		return GLOB_NOMATCH;
	}
	else
	    result->gl_flags &= ~GLOB_MAGCHAR;
    }


    if ( count >= result->gl_pathalloc ) {
	result->gl_pathalloc += 50;
	logit("os_glob: expanding gl_pathv (old pathv=%p, count=%d, pathc=%d)",
		    result->gl_pathv, count, result->gl_pathc);
	if ( result->gl_pathv )
	    newlist = realloc(result->gl_pathv,
			      result->gl_pathalloc * sizeof result->gl_pathv[0]);
	else
	    newlist = calloc(result->gl_pathalloc, sizeof result->gl_pathv[0]);

	unless (newlist)
	    return GLOB_NOSPACE;

	result->gl_pathv = newlist;
    }

    unless (result->gl_pathv[count-1] = malloc(strlen(pattern)+2))
	return GLOB_NOSPACE;

    strcpy(result->gl_pathv[count-1], pattern);
    if ( result->gl_flags & GLOB_MARK )
	strcat(result->gl_pathv[count-1], "/");
    result->gl_pathv[count] = 0;
    result->gl_pathc++;
    result->gl_matchc = 1;
    return 0;
#endif
}

/*
 * clean up a glob_t after use.
 */
void
os_globfree(glob_t *collection)
{
#if USING_GLOB
    globfree(collection);
#else
    int x, start;

    start = collection->gl_flags & GLOB_DOOFFS ? collection->gl_offs : 0;

    for (x=0; x < collection->gl_pathc; x++)
	if ( collection->gl_pathv[start+x] )
	    free(collection->gl_pathv[start+x]);
    if ( collection->gl_pathc )
	free(collection->gl_pathv);
    memset(collection, 0, sizeof(collection[0]));
#endif
}


/*
 * do ~username expansions on a filename
 */
char *
os_tilde(char *path)
{
#if HAVE_PWD_H
    char *name, *slash, *expanded;
    struct passwd *user;

    logit("os_expand %s", path);
    unless ( path && (*path == '~') )
	return 0;

    /* ourself or someone else? */
    unless ( slash = strchr(path, '/') )
	return 0;

    unless ( name = malloc(slash-path) )
	return 0;

    strncpy(name, 1+path, (slash-path)-1);
    name[(slash-path)-1] = 0;

    if ( *name )
	user = getpwnam(name);
    else
	user = getpwuid(getuid());

    free(name);

    unless (user)
	return 0;

    if (expanded = malloc(strlen(user->pw_dir)+1+strlen(slash)+1)) {
	strcpy(expanded, user->pw_dir);
	strcat(expanded, slash);

	logit("os_expand -> %s", expanded);
    }

    return expanded;
#else
    return 0;
#endif
}


/*
 * implement the :! command
 */
int
os_subshell(char *commandline)
{
    return system(commandline);
}


/*
 * plumbing for the ! command: fork off a child process to process
 * a workfile, return a FILE* that will hold the output.
 */
FILE *
os_cmdopen(char *cmdline, char *workfile, os_pid_t *child)
{
    os_pid_t job;
    int io[2];


    if ( pipe(io) < 0 )
	return NULL;

    if ( (job = fork()) < 0 ) {
	close(io[0]);
	close(io[1]);
	return NULL;
    }

    if ( job < 0 )
	return NULL;
    else if ( job == 0 ) {
	/* child */
	int ifd = open(workfile, O_RDONLY);

	close(io[0]);
	if ( (ifd < 0) || (dup2(ifd, 0) < 0) ) {
	    close(io[1]);
	    exit(1);
	}
	dup2(io[1], 1);
	dup2(1,2);
	
	execl("/bin/sh", "sh", "-c", cmdline, 0L);
	close(io[1]);
	exit(1);
    }
    else {
	/* parent */
	close(io[1]);
	*child = job;
	return fdopen(io[0], "r");
    }
}


/*
 * plumbing for the ! command: wait for the child to clean up, the
 * return exit status.
 */
int
os_cmdclose(FILE *input, os_pid_t child)
{
    int status;

    waitpid(child, &status, 0);
    fclose(input);

    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}


int
os_cclass(unsigned int c)
{
    if (c == '\t' && !list)
	return CC_TAB;
    if ( c < 32 || c == 127 )
	return CC_CTRL;
    if (c & 0x80)
	return CC_OTHER;
    return CC_PRINT;
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
	    logit("getKey -> %c", c[0]);
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
