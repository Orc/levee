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
	    *CURon,
	    *SO,
	    *SE;

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

extern findstates findCP();
extern exec_type editcore();

extern char line(), peekc(), readchar();
extern char *findparse(),*makepat();

extern bool lvgetline();
extern bool putfile();
extern bool doyank(), deletion(), putback();
extern bool pushb(),pushi(),pushmem(),uputcmd(), delete_to_undo();
extern bool ok_to_scroll(), move_to_undo();

extern int fseekeol(), bseekeol(), settop();
extern int scan(), findDLE(), setY(), skipws(), nextline(), setX();
extern int insertion(), chop(), fixcore(), lookup(), to_index();
extern int doaddwork(), addfile(), expandargs(), to_line();
extern int findfwd(), findback(), lvgetcontext(), getKey();
extern int cclass();
extern int insertfile();

extern void strput(), numtoa(), clrprompt(), setend(), error();
extern void insert_to_undo(), resetX(), zerostack(), swap();
extern void mvcur(), printch(), prints(), writeline(), refresh();
extern void redisplay(), scrollback(), scrollforward(), prompt();
extern void setpos(), resetX(), insertmacro(), wr_stat();
extern void movearound(), printi(), println(), killargs();
extern void exec(), initcon(), fixcon(), version(), setcmd();
extern void toedit(), inputf(), fixmarkers(), errmsg();

#ifndef moveleft
extern void moveleft();
#endif
#ifndef moveright
extern void moveright();
#endif
#ifndef fillchar
extern void fillchar();
#endif


extern void dwrite(char *, int);
extern void dputs(char *);
extern void dputc(char);
extern void dgotoxy(int,int);
extern void dclear_to_eol();
extern void dclearscreen();
extern void dnewline();
extern void dopenline();
extern void d_cursor(int);
extern void d_highlight(int);
extern void dinitialize();
extern void dscreensize(int *, int *);
extern void drestore();
extern void Ping();

extern int os_initialize();
extern int os_restore();
extern int os_cangotoxy();
extern int os_clear_to_eol();
extern int os_clearscreen();
extern int os_cursor(int);
extern int os_highlight(int);
extern int os_dwrite(char *, int);
extern int os_gotoxy(int,int);
extern int os_initialize();
extern int os_newline();
extern int os_openline();
extern int os_screensize(int *, int *);
extern int os_scrollback();
extern int os_Ping();
extern void set_input();
extern void reset_input();

extern int os_mktemp(char *, const char *);
extern int os_unlink(char *);
extern int os_rename(char *, char *);

extern int Max(int,int);
extern int Min(int,int);
extern char * dotfile();

extern FILE* expandfopen(char *file, char *mode);

extern void lowercase(char*);

#if !HAVE_STRDUP
extern char* strdup(char*);
#endif

#if !HAVE_BASENAME
extern char* basename(char*);
#endif

extern int os_glob(const char *, int, int(*) (const char *, int), glob_t *);
extern void os_globfree(glob_t *);

#endif /*EXTERN_D*/
