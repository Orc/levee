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
#ifndef GLOBALS_D
#define GLOBALS_D
extern
char lastchar,		/* Last character read via peekc */
     ch;		/* Global command char */
extern
exec_type mode;			/* editor init state */
extern
int lastexec;			/* last exec mode command */

extern
int contexts['z'-'`'+1];	/* Labels */
		/* C O N S T A N T S */
extern
bool adjcurr[PARA_BACK+1],
     adjendp[PARA_BACK+1];
		/* A R G U M E N T S */
extern
char startcmd[];	/* initial command after read */
extern
char **argv;		/* Arguments */
extern
int  argc,		/* # arguments */
     pc;		/* Index into arguments */
		/* M A C R O   S T U F F */
extern
struct macrecord mbuffer[];
extern
struct tmacro mcr[];		/* A place for executing macros */
			/* S E A R C H   S T U F F */
extern
char dst[],			/* last replacement pattern */
     lastpatt[],		/* last search pattern */
     pattern[];
extern
int RE_start[9],		/* start of substitute argument */
    RE_size [9],		/* size of substitute argument */
    lastp;			/* last character matched in search */
extern
struct undostack undo;		/* To undo a command */
		/* R A N D O M   S T R I N G S */
		
extern
char instring[],		/* Latest input */
     filenm[],			/* Filename */
     altnm[],			/* Alternate filename */
     gcb[];			/* Command buffer for mutations of insert */
	
extern
char undobuf[],
     undotmp[],
     yankbuf[];

extern
FILEDESC uread,			/* reading from the undo stack */
	uwrite;			/* writing to the undo stack */
			    /* B U F F E R S */
extern
char rcb[], *rcp,		/* last modification command */
     core[];			/* data space */
		    
extern
struct ybuf yank;		/* last deleted/yanked text */
/* STATIC INITIALIZATIONS: */

extern int LINES, COLS;

extern bool CA,			/* cursor addressable? */
	    canUPSCROLL,	/* can the display upscroll */
	    canOL;		/* can the display open a line? */

extern char FkL,
	    CurRT,
	    CurLT,
	    CurDN,
	    CurUP;

extern char *TERMNAME,
	    *HO,
	    *UP,
	    *CE,
	    *CL,
	    *OL,
	    *BELL,
	    *CM,
	    *UpS,
	    *CURoff,
	    *CURon;

extern
char Erasechar,
     Eraseline;

extern
char codeversion[],		/* Editor version */
     fismod[],			/* File is modified message */
     fisro[];			/* permission denied message */
     
extern
char *excmds[],
     wordset[],
     spaces[];
extern
struct variable vars[];
extern
int autowrite,
    autocopy,
    overwrite,
    beautify,
    autoindent,
    dofscroll,
    shiftwidth,
    tabsize,
    list,
    wrapscan,
    bell,
    magic;
/*extern 
char *suffix;	*/
/* For movement routines */
extern
int setstep[];
/* Where the last diddling left us */
extern
struct coord curpos;
    
    /* initialize the buffer */
extern
int curr,		/* Global cursor pos */
    lstart, lend,	/* Start & end of current line */
    count,		/* Latest count */
    xp, yp,		/* Cursor window position */
    bufmax,		/* End of file here */
    ptop, pend;		/* Top & bottom of CRT window */
extern
bool modified,		/* File has been modified */
     readonly,		/* is this file readonly? */
     needchar,		/* Peekc flag */
     deranged,		/* Up-arrow || down-arrow left xpos in Oz. */
     indirect,		/* Reading from an indirect file?? */
     redoing,		/* doing a redo? */
     xerox,		/* making a redo buffer? */
     newfile,		/* Editing a new file? */
     newline,		/* Last insert/delete included a EOL */
     lineonly,		/* Dumb terminal? */
     zotscreen,		/* do more after command in execmode */
     diddled;		/* force redraw when I enter editcore */
     
extern
int macro;    /* Index into MCR macro execution stack */
extern
char lsearch;
/* movement, command codes */
extern
cmdtype movemap[];
#endif /*GLOBALS_D*/
#ifndef EXTERN_D
#define EXTERN_D
#define wc(ch)	(scan(65,'=',(ch),wordset)<65)

#if HAVE_STRING_H
#include <string.h>
#endif

#if HAVE_MEMSET
#define fillchar(p,l,c)	memset((p),(c),(l))
#elif HAVE_BLKFILL
#define fillchar(p,l,c)	blkfill((p),(c),(l))
#endif
#if HAVE_STRCHR
#define index(s,c)	strchr((s),(c))
#endif

extern findstates PROC findCP();
extern exec_type PROC editcore();

extern char PROC line(), peekc(), readchar();
extern char PROC *findparse(),*makepat();

extern bool PROC lvgetline();
extern bool PROC putfile();
extern bool PROC doyank(), deletion(), putback();
extern bool PROC pushb(),pushi(),pushmem(),uputcmd(), delete_to_undo();
extern bool PROC ok_to_scroll(), move_to_undo();

extern int PROC fseekeol(), bseekeol(), settop();
extern int PROC scan(), findDLE(), setY(), skipws(), nextline(), setX();
extern int PROC insertion(), chop(), fixcore(), lookup(), to_index();
extern int PROC doaddwork(), addfile(), expandargs(), to_line();
extern int PROC findfwd(), findback(), lvgetcontext(), getKey();
extern int PROC cclass();
extern int PROC insertfile();

extern VOID PROC strput(), numtoa(), clrprompt(), setend(), error();
extern VOID PROC insert_to_undo(), resetX(), zerostack(), swap();
extern VOID PROC mvcur(), printch(), prints(), writeline(), refresh();
extern VOID PROC redisplay(), scrollback(), scrollforward(), prompt();
extern VOID PROC setpos(), resetX(), insertmacro(), wr_stat();
extern VOID PROC movearound(), printi(), println(), killargs();
extern VOID PROC exec(), initcon(), fixcon(), version(), setcmd();
extern VOID PROC toedit(), inputf(), fixmarkers(), errmsg();

#ifndef moveleft
extern VOID PROC moveleft();
#endif
#ifndef moveright
extern VOID PROC moveright();
#endif
#ifndef fillchar
extern VOID PROC fillchar();
#endif


extern VOID PROC dwrite(char *, int);
extern VOID PROC dputs(char *);
extern VOID PROC dputc(char);
extern VOID PROC dgotoxy(int,int);
extern VOID PROC dclear_to_eol();
extern VOID PROC dclearscreen();
extern VOID PROC dnewline();
extern VOID PROC dopenline();
extern VOID PROC d_cursor(int);
extern VOID PROC d_highlight(int);
extern VOID PROC dinitialize();
extern VOID PROC dscreensize(int *, int *);
extern VOID PROC drestore();
extern VOID PROC Ping();

extern int PROC os_initialize();
extern int PROC os_restore();
extern int PROC os_cangotoxy();
extern int PROC os_clear_to_eol();
extern int PROC os_clearscreen();
extern int PROC os_cursor(int);
extern int PROC os_highlight(int);
extern int PROC os_dwrite(char *, int);
extern int PROC os_gotoxy(int,int);
extern int PROC os_highlight(int);
extern int PROC os_initialize();
extern int PROC os_newline();
extern int PROC os_openline();
extern int PROC os_screensize(int *, int *);
extern int PROC os_scrollback();
extern int PROC os_Ping();
extern VOID PROC set_input();
extern VOID PROC reset_input();

extern int PROC os_mktemp(char *, const char *);
extern int PROC os_unlink(char *);
extern int PROC os_rename(char *, char *);

extern int PROC Max(int,int);
extern int PROC Min(int,int);
extern char *PROC dotfile();

extern FILE* PROC expandfopen(char *file, char *mode);

extern VOID PROC lowercase(char*);

#if !HAVE_STRDUP
extern char* PROC strdup(char*);
#endif

#endif /*EXTERN_D*/
