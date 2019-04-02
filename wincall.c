/*
 * windows glue
 */

#include "levee.h"
#include "extern.h"

#ifdef OS_WINDOWS

#include <stdio.h>
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <termcap.h>
#include <errno.h>
#include <stdarg.h>
#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

/* **** FILE IO ABSTRACTIONS **** */
FILEDESC
OPEN_OLD(char *name)
{
    int fd = open(name, O_RDONLY);

    if ( fd == -1 )
	return NOWAY;

    _setmode(fd, _O_BINARY);
    return (FILEDESC)fd;
}

FILEDESC
OPEN_NEW(char *name)
{
    int fd = open(name, O_WRONLY|O_CREAT|O_TRUNC, 0666);

    if ( fd == -1 )
	return NOWAY;

    _setmode(fd, _O_BINARY);
    return (FILEDESC)fd;
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


/* i/o handling mess
 */
static struct _input_fd {
    HANDLE fd;			/* Windows file descriptor */
    DWORD mode;			/* original event mode for ^^ */
    int cooked;			/* might want to know what mode we're in */
    int set;
#define SZ_EV	128
    INPUT_RECORD events[SZ_EV];	/* last pile of events gotten */
    int cur_ev, nr_ev;		/* current event read, nr events total */
} console_in = { .set=0 };

static struct _window_fd {
    HANDLE fd;
    DWORD mode;
    int set;
} console_out = { .set=0 };


int
os_write(ptr, size)
char *ptr;
{
    DWORD result;

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
os_rename(char *old, char *new)
{
    return rename(old, new);
}


int
os_unlink(char *file)
{
    return unlink(file);
}


char *
dotfile()
{
    static char *dotname = 0;
    char *disk, *path;
    static char dot[] = "/lv.rc";

    unless ( dotname ) {
	disk = getenv("HOMEDRIVE");
	path = getenv("HOMEPATH");

	if ( disk && path ) {
	    int needed = 1+strlen(disk)+strlen(path)+sizeof dot;

	    dotname = dotname ? realloc(dotname, needed) : malloc(needed);

	    if ( dotname ) {
#if USING_STDIO
		sprintf(dotname, "%s%s%s", disk, path, dot);
#else
		strcpy(dotname, disk);
		strcat(dotname, path);
		strcat(dotname, dot);
#endif

		return dotname;
	    }
	}
    }
    return "lv.rc";
}

int
os_mktemp(char *dest, int size, const char *template)
{
    char *tmpdir = getenv("TEMP");
    int required = 12 + (tmpdir ? strlen(tmpdir) : 0) + strlen(template);

    unless (size > required) {
	errno = E2BIG;
	return 0;
    }

#if USING_MKTEMP
    if ( tmpdir )
	sprintf(dest, "%s\\%s.%d", tmpdir, template, getpid());
    else
	sprintf(dest, "%s.%d", template, getpid());
#else
    dest[0] = 0;

    if ( tmpdir ) {
	strcat(dest, tmpdir);
	strcat(dest, "\\");
    }
    strcat(dest, template);
    numtoa(&dest[strlen(dest)], getpid());
#endif
    return 1;
}


int
os_screensize(x,y)
int *x;
int *y;
{
    CONSOLE_SCREEN_BUFFER_INFO xyzzy;


    if (GetConsoleScreenBufferInfo(console_out.fd, &xyzzy)) {
	(*y) = xyzzy.srWindow.Bottom - xyzzy.srWindow.Top + 1;
	(*x) = xyzzy.srWindow.Right - xyzzy.srWindow.Left + 1;
	logit(("os_screensize -> (%d, %d)", (*x), (*y)));
    }
    else {
	(*y) = 24;
	(*x) = 80;
	logit(("os_screensize: -> DEFAULT (%d,%d)", (*x), (*y)));
    }
    return 1;
}


FILE *
os_cmdopen(char *command, char *workfile, os_pid_t *child)
{
    *child = 0;

    return 0;
}

int
os_cmdclose(FILE *cmd, os_pid_t child)
{
    int status;

    waitpid(child, &status, 0);
    fclose(cmd);

    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
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


/* set up the stdin descriptor for initcon()/fixcon()
 */
int
os_initialize()
{
    static char obuf[4096];
#define NOPE INVALID_HANDLE_VALUE

    fflush(stdout);
    _setmode(fileno(stdout), _O_BINARY);
    setvbuf(stdout, obuf, _IOFBF, sizeof obuf);

    dputs("\033[r");	/* set the scrolling margin to the whole screen */

    TERMNAME = "Windows console";
    Erasechar = '\b';	/* ^H */
    Eraseline = 21;	/* ^U */

    /* disable ! command in visual mode
     */
    movemap['!'] = BAD_COMMAND;

    /* set cursor movement keys to zero for now */
    FkL = CurRT = CurLT = CurUP = CurDN = EOF;

    /* yes we can do all these things */
    canUPSCROLL = CA = canOL = 1;

    /* grab the tty input handle */

    if ( (console_in.fd = GetStdHandle(STD_INPUT_HANDLE)) != NOPE ) {
	GetConsoleMode(console_in.fd, &console_in.mode);
	console_in.set = 1;
    }
    console_in.nr_ev = console_in.cur_ev = 0;
    console_in.cooked = 1;


    /* and the tty output handle */

    if ( (console_out.fd = GetStdHandle(STD_OUTPUT_HANDLE)) != NOPE ) {
	DWORD written;
	GetConsoleMode(console_out.fd, &console_out.mode);
	logit(("output mode=%016x", console_out.mode));
	console_out.set = 1;

	/* and and the tty dimensions */


	unless( WriteConsole(console_out.fd, "?\r", 2, &written, 0) ) {
	    fprintf(stderr, "can't run with stdout redirected\n");
	    exit(1);
	}
    }

    unless ( console_out.set && console_in.set ) {
	/* can't function without input or output, sorry */
	fprintf(stderr, "levee: cannot get tty & window handles\n");
	exit(1);
    }

    return 1;
}


os_restore()
{
    return 1;
}


os_subshell(char *cmdline)
{
    return system(cmdline);
}


/*
 * implement the glob() command (with GLOB_NOSORT always set)
 */


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
	    return p+1;
    return s;
} /* basename */
#endif


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
    char isdotpattern;		/* looking for files starting with . */
    intptr_t find_ctx;		/* findfirst/findnext context */
    struct _finddata_t entry;	/* lastest entry found */
    register st;		/* status from _dos_findxxx */
    int size;			/* size of path buffer (needed?) */

    unless (path)
	return -1;

    size = strlen(path) + 256;

    unless (path_bfr = malloc(size))
	return -1;

    strcpy(path_bfr, path);
    file_part = basename(path_bfr);

    /* set up initial parameters for DosFindFirst()
     */
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

    unless (find_ctx = _findfirst(path, &entry)) {
	/* no matches for this pattern; if permit_nomatch,
	 * add it verbatim to dta, otherwise fail
	 */
	free(path_bfr);

	return permit_nomatch ? glob_addfile(path, count, dta) : -1;
    }

    do {
	logit(("glob_wildcard: file=%s", entry.name));
	if (entry.name[0] == '.' && !isdotpattern)
	    continue;

	strcpy(file_part, entry.name);

	if (st = glob_addfile(path_bfr, count, dta)) {
	    _findclose(find_ctx);
	    free(path_bfr);
	    return -1;
	}
	++count;
	st = _findnext(find_ctx, &entry);
	logit(("glob_wildcard: find_next -> %d", st));
    } while ( st == 0 );
    //} while ( _findnext(find_ctx, &entry) == 0 );
    _findclose(find_ctx);
    free(path_bfr);
    return 0;
} /* glob_wildcard */


/*
 * non-sorting glob with windows wildcarding
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


/*
 * do ~username expansions on a filename
 */
char *
os_tilde(char *path)
{
    char *expanded;
    char *disk,*dir;

    logit(("os_tilde %s", path));

    /* ~ only for now
     */

    unless ( path && (path[0] == '~') && (strcspn(path, "/\\") == 1) )
	return 0;

    unless ( (disk=getenv("HOMEDRIVE")) && (dir=getenv("HOMEPATH")) )
	return 0;

    if (expanded = malloc(strlen(disk)+strlen(dir)+strlen(1+path)+1)) {
#if USING_STDIO
	sprintf(expanded, "%s%s/%s", disk, dir, 2+path);
#else
	strcpy(expanded, disk);
	strcat(expanded, dir);
	strcat(expanded, 2+path);
#endif

	logit(("os_tilde -> %s", expanded));
    }

    return expanded;
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


void
set_input()
{
    if ( console_in.set ) {
	console_in.cooked = 0;
	SetConsoleMode(console_in.fd, ENABLE_EXTENDED_FLAGS);
	SetConsoleMode(console_out.fd, ENABLE_PROCESSED_OUTPUT|
				       ENABLE_VIRTUAL_TERMINAL_PROCESSING|
				       DISABLE_NEWLINE_AUTO_RETURN);
    }
    os_clearscreen();
    curpos.x = curpos.y = 0;
}


void
reset_input()
{
    fflush(stdout);
    if ( console_in.set ) {
	console_in.cooked = 1;
	SetConsoleMode(console_in.fd, console_in.mode);
	SetConsoleMode(console_out.fd, console_out.mode);
    }
}



int
getKey()
{
    fflush(stdout);
    while ( 1 ) {
	if ( console_in.cur_ev >= console_in.nr_ev ) {
	    int rc = ReadConsoleInput(console_in.fd, console_in.events,
					SZ_EV,
					&console_in.nr_ev);

	    if ( !rc )
		return EOF;
	    console_in.cur_ev = 0;
	}

	while ( console_in.cur_ev < console_in.nr_ev ) {
	    int idx = console_in.cur_ev++;

	    if ( console_in.events[idx].EventType == KEY_EVENT) {
		KEY_EVENT_RECORD *key;
		key = &console_in.events[idx].Event.KeyEvent;

		if ( key->bKeyDown ) {
		    logit(("getkey -> %c", key->uChar.AsciiChar));
		    return key->uChar.AsciiChar;
		}
	    }
	}
    }
    return EOF;
}
#endif

