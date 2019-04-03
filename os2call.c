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
 * os2 (and bound) interface for levee (Borland c++)
 */
#include "levee.h"
#include "extern.h"

#if OS_OS2

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>

#define INCL_DOSFILEMGR
#include <os2.h>
#include <conio.h>


FILEDESC
OPEN_OLD(char *name)
{
    int fd = open(name, O_RDONLY|O_BINARY);

    return (fd == -1) ? NOWAY : (FILEDESC)fd;
}

FILEDESC
OPEN_NEW(char *name)
{
    int fd = open(name, O_WRONLY|O_CREAT|O_TRUNC|O_BINARY, 0666);

    return (fd == -1) ? NOWAY : (FILEDESC)fd;
}


CLOSE_FILE(FILEDESC f)
{
    return close((int)f);
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

int
os_mktemp(char *dest, int size, const char *template)
{
    char *p;
    char lastchar;
    int required;

static char Xes[] = ".XXXXXX";

    required = sizeof(Xes) + strlen(template) + 1;

    if (p=getenv("TMP")) {

	unless ( size > strlen(p) + required ) {
	    errno = E2BIG;
	    return 0;
	}

	strcpy(dest, p);

	lastchar = dest[strlen(dest)-1];

	if ( lastchar != '/' && lastchar != '\\' && lastchar != ':' )
	    strcat(dest, "/");
    }
    else {
	unless ( size > required ) {
	    errno = E2BIG;
	    return 0;
	}
	dest[0] = 0;
    }
#if USING_STDIO
    sprintf(dest+strlen(dest), "%s%s", template, Xes);

    return mktemp(dest) != 0;
#else
    strcat(s, template);
    numtoa(dest+strlen(dest), getpid());

    return 1;
#endif
}

/* *** termcap stubs if we build w/o termcap (display.c assumes
 * *** the existence of a termcap library and I don't want to
 * *** put an os ifdef into that code.  plus this will let me
 * *** do a version of win32 levee that supports terminals other
 * *** than ansi console emulation if I can find a way to get
 * *** a cleartext telnet session into a windows box
 */
#if !USING_TERMCAP
char *tgoto(char *cap, int x, int y)
{
    return 0;
}

int tgetnum(char *cap)
{
    return 0;
}

char *tgetstr(char *cap, char **bfr)
{
    return 0;
}

int tgetent(char *buf, char *term)
{
    return 0;
}
#endif

int
os_initialize()
{

    setmode(fileno(stdout), O_BINARY);

    Erasechar = '\177';
    Eraseline = '\025';

    TERMNAME = "OS/2 ANSI";
    canUPSCROLL = 0;
    CA = canOL = 1;
    
    /* disable ! command in visual mode
     */
    movemap['!'] = BAD_COMMAND;


    return 1;
}

int
os_screensize(int *cols, int *lines)
{
    struct text_info tty;
    int count;
    char c;
    char *term = getenv("TERM");
    char *li, *co;
    static char inquiry[] = "\033[s\033[400;400H\033[6n\033[u\r";


    /* if term is NOT set, we are most likely on the console */

    if ( term == 0 ) {
	memset(&tty, 0, sizeof tty);
	gettextinfo(&tty);
    
	if ( tty.screenwidth > 0 && tty.screenheight > 0 ) {
	    *cols = tty.screenwidth;
	    *lines = tty.screenheight;
	    logit(("tty screensize = %d %d", *lines, *cols));
	    return 1;
	}
    }

    /* otherwise we're hopefully on an ansi telnet session.  Check
     * $TERM to make sure.  if it's prefixed with ansi, vt100, or
     * xterm, then we can do an ansi inquiry to find out what it
     * is.
     */
#define ISEQ(s,y)	(strncmp(s,y,strlen(y)) == 0)

    if ( ISEQ(term, "ansi") || ISEQ(term, "vt100") || ISEQ(term, "xterm") ) {
	int x, y;

	dwrite(inquiry, sizeof inquiry);

	/* OS/2 eats incoming escapes after a console inquiry? */
	if ( /*getKey() == '\033' &&*/ getKey() == '[' ) {
	    x=0;
	    while ( (c = getKey()) >= '0' && c <= '9' )
		x = (x*10) + (c-'0');
	    logit(("inquiry: x = %d", x));
	    if ( c == ';' ) {
		y = 0;
		while ( (c = getKey()) >= '0' && c <= '9' )
		    y = (y*10) + (c-'0');
		logit(("inquiry: y = %d", y));
		if ( (c == 'R') && x && y ) {
		    *lines = x;
		    *cols = y;
		    logit(("inquiry screensize = %d %d", *lines, *cols));
		    return 1;
		}
	    }
	}
    }

    
    li = getenv("LINES");
    co = getenv("COLS");

    *lines = li ? atoi(li) : 25;
    *cols = co ? atoi(co) : 80;

    logit(("!tty screensize = %d %d", *lines, *cols));
    return 1;
}

/* get a key, mapping certain control sequences
 */

int
getKey()
{
    register c;

#if USING_STDIO
    fflush(stdout);
#endif
    c = getch();

#if 0
    if (c == 0 || c == 0xe0)
	switch (c=getch()) {
	case 'K': return LTARROW;
	case 'M': return RTARROW;
	case 'H': return UPARROW;
	case 'P': return DNARROW;
	case 'I': return 'U'-'@';	/* page-up */
	case 'Q': return 'D'-'@';	/* page-down */
	default : return 0;
	}
#endif

    return c & 0x7f;
}


/* don't allow interruptions to happen
 */

void
set_input()
{
    //signal(SIGINT, SIG_IGN);
} /* nointr */


/* have ^C do what it usually does
 */

void
reset_input()
{
    //signal(SIGINT, SIG_DFL);
} /* allowintr */


#if !HAVE_BASENAME
/*
 * basename() returns the filename part of a pathname
 */
char *
basename(s)
register char *s;
{
    register char *p;

    for (p = s+strlen(s); --p > s; )
	if (*p == '/' || *p == '\\' || *p == ':')
	    return p;
    return s;
} /* basename */

#endif


static void
lowercase(s)
char *s;
{
    while (*s) {
	if (isupper(*s))
	    *s += 32;
	s++;
    }
}


/*
 * glob() expands a wildcard, via calls to _dos_findfirst/_next()
 * and pathname retention.
 */

/* local function to add a single file (broken out so I can implement
 * wildcards using _findfirst/_findnext)
 */
static int
glob_addfile(char *file, int count, glob_t *result)
{
    char **newlist;

    logit(("os_glob: adding %s", file));
    if ( count >= result->gl_pathalloc ) {
	result->gl_pathalloc += 50;
	logit(("os_glob: expanding gl_pathv (old pathv=%p, count=%d, pathc=%d)",
		    result->gl_pathv, count, result->gl_pathc));
	if ( result->gl_pathv )
	    newlist = realloc(result->gl_pathv,
			      result->gl_pathalloc * sizeof result->gl_pathv[0]);
	else
	    newlist = calloc(result->gl_pathalloc, sizeof result->gl_pathv[0]);

	unless (newlist)
	    return GLOB_NOSPACE;

	result->gl_pathv = newlist;
    }

    unless (result->gl_pathv[count-1] = malloc(strlen(file)+2))
	return GLOB_NOSPACE;

    strcpy(result->gl_pathv[count-1], file);
    if ( result->gl_flags & GLOB_MARK )
	strcat(result->gl_pathv[count-1], "/");
    result->gl_pathv[count] = 0;
    result->gl_pathc++;
    result->gl_matchc = 1;
    lowercase(result->gl_pathv[count-1]);
    logit(("os_glob: count=%d, pathc=%d,matchc=%d",
	    count, result->gl_pathc, result->gl_matchc));
    return 0;
}


/* local function to wildcard expand a filename
 */
static int
glob_wildcard(path, permit_nomatch, count, dta)
char *path;
int permit_nomatch;
int count;
glob_t *dta;
{
    char *path_bfr;		/* full pathname to return */
    char *file_part;		/* points at file - for filling */

    FILEFINDBUF3 finfo[50];	/* OS/2 dta */
    int dir;			/* directory handle */
    register result;		/* status from DosFindxxx */

    ULONG wanted;

    char isdotpattern;		/* looking for files starting with . */
    int idx;			/* for looping through finfo */

    logit(("glob_wildcard \"%s\", %d, %d, dta", path, permit_nomatch, count));
    unless (path)
	return -1;

    unless (path_bfr = malloc(strlen(path) + 256))
	return -1;

    strcpy(path_bfr, path);
    file_part = basename(path_bfr);

    /* set up initial parameters for DosFindFirst()
     */
    dir = HDIR_CREATE;

    if (isdotpattern = (*file_part == '.')) {
	/* _dos_findfirst() magically expands . and .. into their
	 * directory names.  Admittedly, there are cases where
	 * this can be useful, but this is not one of them. So,
	 * if we find that we're matching . and .., we just
	 * special-case ourselves into oblivion to get around
	 * this particular bit of DOS silliness.
	 */
	if (file_part[1] == 0 || (file_part[1] == '.' && file_part[2] == 0))
	    return glob_addfile(path, count, dta);

    }

#define OS_FINDFLAGS	FILE_READONLY|FILE_HIDDEN|FILE_SYSTEM|FILE_ARCHIVED

    wanted = sizeof finfo / sizeof finfo[0];
    result = DosFindFirst(path, &dir, OS_FINDFLAGS,
			finfo, wanted, &wanted, FIL_STANDARD);

    logit(("DosFindFirst -> %d (wanted -> %d)", result, wanted));
    if ( result != 0 ) {
	free(path_bfr);
	DosFindClose(dir);
	return permit_nomatch ? glob_addfile(path,count,dta) : -1;
    }

    do {
	for (idx = 0; idx < wanted; idx++ ) {
	    logit(("glob_wildcard: file=%s", finfo[idx].achName));
	    if (finfo[idx].achName[0] == '.' && !isdotpattern)
		continue;

	    strcpy(file_part, finfo[idx].achName);

	    if (result = glob_addfile(path_bfr, count, dta)) {
		free(path_bfr);
		DosFindClose(dir);
		return -1;
	    }
	    ++count;
	}

	wanted = sizeof finfo / sizeof finfo[0];
	result = DosFindNext(dir, finfo, wanted, &wanted);

    } while ( result == 0 );
    DosFindClose(dir);
    free(path_bfr);
    return 0;
} /* glob_wildcard */


/*
 * non-sorting glob with os2 wildcarding
 */
int
os_glob(const char* pattern, int flags, glob_t *result)
{
    int count = 1 + (result->gl_flags & GLOB_DOOFFS ? result->gl_offs : 0);

#if LOGGING
    if ( result->gl_flags & GLOB_APPEND )
	logit(("os_glob: add %s to arglist", pattern));
    else
	logit(("os_glob: create new arglist, initialized with %s", pattern));
#endif

    if ( result->gl_pathc == 0 )
	result->gl_flags = flags;

    if ( result->gl_flags & GLOB_APPEND )
	count += result->gl_pathc;

    unless ( flags & GLOB_NOMAGIC ) {
	if ( strcspn(pattern, "?*") < strlen(pattern) ) {
	    int stat;
	    result->gl_flags |= GLOB_MAGCHAR;

	    stat = glob_wildcard(pattern,
				 result->gl_flags & GLOB_NOCHECK,
				 count,
				 result);

	    return stat ? GLOB_NOMATCH : 0;
	}
	else
	    result->gl_flags &= ~GLOB_MAGCHAR;
    }

    return glob_addfile((char*)pattern, count, result);
}


/*
 * clean up a glob_t after use.
 */
void
os_globfree(glob_t *collection)
{
    int x, start;

    start = collection->gl_flags & GLOB_DOOFFS ? collection->gl_offs : 0;

    for (x=0; x < collection->gl_pathc; x++)
	if ( collection->gl_pathv[start+x] )
	    free(collection->gl_pathv[start+x]);
    if ( collection->gl_pathc )
	free(collection->gl_pathv);
    memset(collection, 0, sizeof(collection[0]));
}


char *
dotfile()
{
    static char *dot = 0;
    static char lvrc[] = "/levee.rc";
    char *etc;

    if ( dot )
	return dot;

    unless (etc = getenv("ETC"))
	etc="/etc";

    unless (dot = malloc(strlen(etc) + sizeof lvrc + 1))
	return lvrc;

    strcpy(dot, etc);
    strcat(dot, lvrc);

    return dot;
}


char *
os_tilde(char *name)
{
    /* OS/2 is a single-user system with no home directory (sigh) */
    return 0;
}


/*
 * make a backup file name
 */
char *
os_backupname(char *file)
{
    char *p, *base, *ext;
    int size;
    int filelen;
    static char bkp_extension[] = ".bkp";

    base = basename(file);
    ext = strrchr(base, '.');
    filelen = strlen(file);

    /* backup buffer length is 1 + |file| + |bkp_extension| - |ext|
     */
    size = 1 + strlen(file) + sizeof bkp_extension  - (ext ? strlen(ext) : 0);

    if ( p = calloc(1, size) ) {
	strcpy(p, file);
	strcpy(&p[filelen - (ext ? strlen(ext) : 0)], bkp_extension);
    }
    return p;
}


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

int
os_unlink(char *file)
{
    return unlink(file);
}

FILE *
os_cmdopen(char *command, char *input, os_pid_t *child)
{
    return 0;
}

int
os_cmdclose(FILE *f, os_pid_t child)
{
    return 0;
}


int
os_rename(char *src, char *dest)
{
    return rename(src, dest);
}


int
os_subshell(char *commandline)
{
    return system(commandline);
}


int
os_write(ptr, size)
char *ptr;
{
    logit(("os_write(\"%.*s\",%d)", size, ptr, size));
    fwrite(ptr, size, 1, stdout);
    return 1;
}


os_openline()
{
    dputs("\033[1L");
    return 1;
}


os_clearscreen()
{
    dputs("\033[3J\033[2J");
    return 1;
}


int
os_cursor(visible)
{
    dputs(visible ? "\033[?25h" : "\033[?25l");
    return 1;
}


int
os_highlight(int visible)
{
    //dputs(visible ? "\033[27m" : "\033[7m");
    return 1;
}


int
os_Ping()
{
    dputs("\007");
    return 1;
}


os_newline()
{
#if 0
    dputs("\033[1T");
#else
    dputs("\r\n");
#endif
    return 1;
}


int
os_scrollback()
{
#if 0
    dputs("\033[1S");
#endif
    return 1;
}


int
os_clear_to_eol()
{
    dputs("\033[0K");
    return 1;
}


int
os_gotoxy(x, y)
{
    char gt[40];

    sprintf(gt, "\033[%d;%dH", y+1, x+1);
    dputs(gt);
    return 1;
}

int
os_restore()
{
    fflush(stdout);
    return 1;
}

#endif
