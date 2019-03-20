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

/* i/o handling mess
 */
static struct _iob {
    char ibuf[4096];
    int  iptr;
} iobuf = { .iptr = 0 };

static struct _input_fd {
    HANDLE fd;			/* Windows file descriptor */
    DWORD mode;			/* original event mode for ^^ */
    int cooked;			/* might want to know what mode we're in */
    int set;
#define SZ_EV	128
    INPUT_RECORD events[SZ_EV];	/* last pile of events gotten */
    int cur_ev, nr_ev;		/* current event read, nr events total */
} tty_stdin = { .set=0 };

static struct _window_fd {
    HANDLE fd;
    DWORD mode;
    int set;
} window = { .set=0 };


void
iwrite(char *buf, int size)
{
#if 0
    write(fileno(stdout), buf, size);
#else
    register rc;
    DWORD written;

    rc = WriteFile(window.fd, buf, size, &written, 0);

    if ( written != size )
	logit("iwrite wanted to write %d, but actually wrote %d", size, written);
    /* we should care about rc, yes? */
#endif
}

void
iflush()
{
    if ( iobuf.iptr > 0 ) {
	iwrite(iobuf.ibuf, iobuf.iptr);
	iobuf.iptr = 0;
    }
}


os_openline()
{
    return 1;
}


os_clearscreen()
{
#if 0
    COORD coordScreen = { 0, 0 };    /* here's where we'll home the
					cursor */ 
    BOOL bSuccess;
    DWORD cCharsWritten;
    CONSOLE_SCREEN_BUFFER_INFO csbi; /* to get buffer info */ 
    DWORD dwConSize;                 /* number of character cells in
					the current buffer */ 

    /* get the number of character cells in the current buffer */ 

    bSuccess = GetConsoleScreenBufferInfo( window.fd, &csbi );
    dwConSize = csbi.dwSize.X * csbi.dwSize.Y;

    /* fill the entire screen with blanks */ 

    bSuccess = FillConsoleOutputCharacter( window.fd, (TCHAR) ' ',
       dwConSize, coordScreen, &cCharsWritten );

    /* get the current text attribute */ 

    bSuccess = GetConsoleScreenBufferInfo( window.fd, &csbi );

    /* now set the buffer's attributes accordingly */ 

    bSuccess = FillConsoleOutputAttribute( window.fd, csbi.wAttributes,
       dwConSize, coordScreen, &cCharsWritten );

    /* put the cursor at (0, 0) */ 

    bSuccess = SetConsoleCursorPosition( window.fd, coordScreen );

#endif
    return 0;
}


int
os_dwrite(ptr, size)
char *ptr;
{
    logit("os_dwrite(\"%.*s\",%d)", size, ptr, size);
    
    if ( iobuf.iptr + size > sizeof iobuf.ibuf )
	iflush();

    if ( size > sizeof iobuf.ibuf )
	iwrite(ptr, size);

    memcpy(iobuf.ibuf + iobuf.iptr, ptr, size);
    iobuf.iptr += size;
    return 1;
}


int
os_highlight(int visible)
{
    return 0;
}

int
os_Ping()
{
    return 0;
}

os_newline()
{
    return 0;
}

int
os_scrollback()
{
    return 0;
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

#if USING_STDIO
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
os_cursor(visible)
{
    return 0;
}

int
os_clear_to_eol()
{
#if 1
    return 0;
#else
    CONSOLE_SCREEN_BUFFER_INFO whereis;
    DWORD written;

    GetConsoleScreenBufferInfo(window.bfd, &whereis);
    WriteConsole(window.fd, spaces, COLS-whereis.dwCursorPosition.X, &written, 0);
    SetConsoleCursorPosition(window.fd, whereis.dwCursorPosition);
    return 1;
#endif
}

static CHAR_INFO clear = { .Char.AsciiChar=' ', .Attributes=0 };

#if 0
static void
scroll(lines)
{
    SMALL_RECT toscroll;
    COORD moveto;

	logit( "scroll(%d)", lines);

	toscroll.Left = toscroll.Top = 0;
	toscroll.Right = COLS-1;
	toscroll.Bottom = LINES-1;

	moveto.X = 0;
	moveto.Y = lines;

	ScrollConsoleScreenBuffer(window.bfd, &toscroll, 0, moveto, &clear);
}
#endif

#if 0
open_line(y)
int *y;
{
    SMALL_RECT toscroll;
    COORD moveto;

    if ( window.set ) {

	logit( "open_line(->%d)", (*y));

	toscroll.Left = 0;
	toscroll.Top = *y;
	toscroll.Right = COLS-1;
	toscroll.Bottom = LINES-1;
	moveto.X = 0;
	moveto.Y = ++(*y);
	ScrollConsoleScreenBuffer(window.bfd, &toscroll, 0, moveto, &clear);
    }
}
#endif


int
os_screensize(x,y)
int *x;
int *y;
{
    if ( window.set ) {
	int co, li;
	CONSOLE_SCREEN_BUFFER_INFO xyzzy;

	if ( !GetConsoleScreenBufferInfo(window.fd, &xyzzy) )
	    return 0;

	co = xyzzy.srWindow.Right - xyzzy.srWindow.Left + 1;
	li = xyzzy.srWindow.Bottom - xyzzy.srWindow.Top + 1;

	logit( "gotten: LINES=%d, COLS=%d", li, co);

	(*x) = co;
	(*y) = li;

	return 1;
    }
    return 0;
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


int
os_gotoxy(x, y)
{
    COORD xyzzy;

    xyzzy.X = x;
    xyzzy.Y = y;

    SetConsoleCursorPosition(window.fd, xyzzy);
#if 0
    return 1;
#else
    return 0;
#endif
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
#define NOPE INVALID_HANDLE_VALUE

#if USING_STDIO
    static char iobuf[4096];
#endif

    Erasechar = '\b';	/* ^H */
    Eraseline = 21;	/* ^U */

#if USING_STDIO
    fflush(stdin); _setmode(fileno(stdin), _O_BINARY);
    fflush(stdout); _setmode(fileno(stdout), _O_BINARY);
    fflush(stderr); _setmode(fileno(stderr), _O_BINARY);

    setvbuf(stdout, iobuf, _IOFBF, sizeof iobuf);
#else
    _setmode(0, _O_BINARY);
    _setmode(1, _O_BINARY);
    _setmode(2, _O_BINARY);
#endif

    /* disable ! command in visual mode
     */
    movemap['!'] = BAD_COMMAND;

    /* get our tty handles or die trying
     */
    if ( !tty_stdin.set ) {

	logit( "getting input fd");

	tty_stdin.cooked = 1;
	tty_stdin.nr_ev = tty_stdin.cur_ev = 0;

	if ( (tty_stdin.fd = GetStdHandle(STD_INPUT_HANDLE)) == NOPE )
	    return EOF;
	GetConsoleMode(tty_stdin.fd, &tty_stdin.mode);

	tty_stdin.set = 1;
    }

    if ( !window.set ) {

	logit( "getting window fds");

	if ( (window.fd = GetStdHandle(STD_OUTPUT_HANDLE)) == NOPE )
	    return EOF;

	GetConsoleMode(window.fd, &window.mode);

	logit("output mode=%016x", window.mode);

	window.set = 1;
    }

    return 0;
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


/* local function to add a single file (broken out so I can implement
 * wildcards using _findfirst/_findnext)
 */
static int
glob_addfile(char *file, int count, glob_t *result)
{
    char **newlist;

    logit("os_glob: adding %s", file);
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

    unless (result->gl_pathv[count-1] = malloc(strlen(file)+2))
	return GLOB_NOSPACE;

    strcpy(result->gl_pathv[count-1], file);
    if ( result->gl_flags & GLOB_MARK )
	strcat(result->gl_pathv[count-1], "/");
    result->gl_pathv[count] = 0;
    result->gl_pathc++;
    result->gl_matchc = 1;
    logit("os_glob: count=%d, pathc=%d,matchc=%d",
	    count, result->gl_pathc, result->gl_matchc);
    return 0;
}


/*
 * non-sorting glob with windows wildcarding
 */
int
os_glob(const char* pattern, int flags, glob_t *result)
{
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

    logit("os_tilde %s", path);

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

	logit("os_tilde -> %s", expanded);
    }

    return expanded;
}


void
set_input()
{
    if ( tty_stdin.set ) {
	tty_stdin.cooked = 0;
	SetConsoleMode(tty_stdin.fd, ENABLE_EXTENDED_FLAGS);
	SetConsoleMode(window.fd, ENABLE_PROCESSED_OUTPUT|
				  ENABLE_VIRTUAL_TERMINAL_PROCESSING|
				  DISABLE_NEWLINE_AUTO_RETURN);
    }
}


void
reset_input()
{
    if ( tty_stdin.set ) {
	tty_stdin.cooked = 1;
	SetConsoleMode(tty_stdin.fd, tty_stdin.mode);
	SetConsoleMode(window.fd, window.mode);
    }
}



int
getKey()
{
#if 0 /*USING_STDIO*/
    logit("getkey: flush stdout");
    fflush(stdout);
#endif
    iflush();
    while ( 1 ) {
	if ( tty_stdin.cur_ev >= tty_stdin.nr_ev ) {
	    int rc = ReadConsoleInput(tty_stdin.fd, tty_stdin.events,
					SZ_EV,
					&tty_stdin.nr_ev);

	    if ( !rc )
		return EOF;
	    tty_stdin.cur_ev = 0;
	}

	while ( tty_stdin.cur_ev < tty_stdin.nr_ev ) {
	    int idx = tty_stdin.cur_ev++;

	    if ( tty_stdin.events[idx].EventType == KEY_EVENT) {
		KEY_EVENT_RECORD *key;
		key = &tty_stdin.events[idx].Event.KeyEvent;

		if ( key->bKeyDown ) {
		    logit("getkey -> %c", key->uChar.AsciiChar);
		    return key->uChar.AsciiChar;
		}
	    }
	}
    }
    return EOF;
}

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

#endif
