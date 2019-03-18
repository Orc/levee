/*
 * windows glue
 */

#include "levee.h"
#include "extern.h"

#ifdef OS_WINDOWS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termcap.h>
#include <stdarg.h>
#include <windows.h>
#include <io.h>
#include <fcntl.h>


/* i/o handling mess
 */

static struct _obuf {
    char *buf;
    int size;
    int next;
} obuf;

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

static FILE *log = 0;

void
logit(char *fmt, ...)
{
    int i;
    char buffer[800];
    
    va_list args;

    va_start(args, fmt);
    vsnprintf(buffer, 800, fmt, args);
    va_end(args);

    for ( i=0; (i < 800) && buffer[i]; i++ ) {
	if ( buffer[i] >= ' ' && buffer[i] < 127 )
	    fputc(buffer[i], log);
	else
	    fprintf(log, "<%02x>", buffer[i]);
    }
    if ( i == 800 )
	fprintf(log, "...");
    fputc('\n', log);
}


static int
os_flush()
{
    if ( obuf.next ) {
	logit("os_flush %d byte%s", obuf.next, (obuf.next==1)?"":"s");
	write(fileno(stdout), obuf.buf, obuf.next);
	obuf.next = 0;
	return 1;
    }
    return 0;
}

static int
os_clearscreen()
{
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

    return 0;
}

int
os_dwrite(s, len)
char *s;
{
    logit("os_dwrite(\"%s\",%d)", s, len);
#if 1
    if ( obuf.next + len > obuf.size ) {
	if ( obuf.next )
	    zflush();
	if ( len > obuf.size ) {
	    logit("toolarge write");
	    write(fileno(stdout), s, len);
	    return 1;
	}
    }
    memcpy(obuf.buf + obuf.next, s, len);
    obuf.next += len;
#else
    register rc;
    DWORD written;
    
    rc = WriteFile(window.fd, s, len, &written, 0);

    if ( written != len )
	logit("os_dwrite wanted to write %d, but actually wrote %d", len, written);
    /* we should care about rc, yes? */
#endif
}


int
os_newline()
{
    os_write("\r\n", 2);
    return 1;
}


extern int LINES, COLS;

/*static char spaces[1024];*/

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
os_windowsize(x,y)
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


int
os_gotoxy(y, x)
{
#if 1
    COORD xyzzy;
    
    xyzzy.X = x;
    xyzzy.Y = y;

    SetConsoleCursorPosition(window.fd, xyzzy);
    return 1;
#else
    return 0;
#endif
}


/* set up the stdin descriptor for initcon()/fixcon()
 */
static int
os_initialize()
{
    char *bigbuf = malloc(20480);

#define NOPE INVALID_HANDLE_VALUE

    obuf.buf = bigbuf;
    obuf.size = 20480;
    obuf.next = 0;
    
    if ( log == 0 ) {
	log = fopen("wincall.log", "w+");
	setvbuf(log, (char *)NULL, _IOLBF, 0);
    }
    
    Erasechar = '\b';	/* ^H */
    Eraseline = 21;	/* ^U */

    fflush(stdin); _setmode(fileno(stdin), _O_BINARY);
    fflush(stdout); _setmode(fileno(stdout), _O_BINARY);
    fflush(stderr); _setmode(fileno(stderr), _O_BINARY);
    
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
	/*memset(spaces, ' ', sizeof spaces);*/
    }

    return 0;
}


void
set_input()
{
    if ( tty_stdin.set ) {
	tty_stdin.cooked = 0;
	SetConsoleMode(tty_stdin.fd, ENABLE_EXTENDED_FLAGS);
	SetConsoleMode(window.fd, ENABLE_PROCESSED_OUTPUT|ENABLE_VIRTUAL_TERMINAL_PROCESSING);
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
    zflush();
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
