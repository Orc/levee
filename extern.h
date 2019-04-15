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

#include "config.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

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
char *startcmd;		/* initial command after read */
extern glob_t args;	/* Arguments
			 * argv -> args->gl_pathv
			 * argc -> args->gl_pathc
			 */
extern
int filenm,		/* current filename */
    altnm;		/* previous filename */


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
		
#define SZ_INSTRING 200
extern
char instring[SZ_INSTRING],	/* Latest input */
     gcb[];			/* Command buffer for mutations of insert */

#define F_UNSET -1
	
extern
char *undobuf,
     *undotmp,
     *yankbuf;

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
	    *DownS,
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
 excmd_t excmds[];
extern
char wordset[],
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
     redoing,		/* doing a redo? */
     xerox,		/* making a redo buffer? */
     newfile,		/* Editing a new file? */
     newline,		/* Last insert/delete included a EOL */
     lineonly,		/* Dumb terminal? */
     zotscreen,		/* do more after command in execmode */
     redraw;		/* force redraw when I enter editcore */

extern
int  is_viewer,		/* set when the -r flag is passed to levee */
     indirect;		/* Nonzero when reading from an indirect file?? */
     
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

extern findstates findCP(int curp, int *newpos, cmdtype cmd);
extern exec_type editcore(void);
extern void maybe_refresh_screen(int redraw),
	    doinput(int fileptr);
extern char *findbounds(char *ip);
extern int oktoedit(int writeold),
	   findarg(register char *name),
	   addarg(register char *name);

extern char line(char *s, int start, int endd, int *size),
	    peekc(void),
	    readchar(void);
extern char *expr_errstring(int errno),
	    *makepat(char *string, int delim);
extern int findparse(int start, char **bufp, int offset);

extern bool lvgetline(char *str, int size);
extern bool putfile(FILE*,int,int);
extern bool doyank(int low, int high),
	    deletion(int low, int high),
	    putback(int start, int *newend);
extern bool pushmem(struct undostack *u, int start, int size),
	    uputcmd(struct undostack *u, int size, int start, int cmd),
	    delete_to_undo(struct undostack *u, int start, int lump);
extern bool ok_to_scroll(int top, int bottom),
	    move_to_undo(struct undostack *u, int start, int lump);

extern int fseekeol(int origin),
	   bseekeol(int origin),
	   settop(int lines),
	   scan(int length, int tst, int ch, register char *src),
	   findDLE(int start, int *endd, int limit, int dle),
	   setY(int cp),
	   skipws(int loc),
	   nextline(bool advance, int dest, int count),
	   setX(int cp),
	   insertion(int count, int openflag, int *dp, int *yp, bool visual),
	   chop(int start, int *endd, bool visual, bool *query),
	   fixcore(int *topp),
	   lookup(int c),
	   to_index(int line),
	   addfile(FILE*,int,int,int*),
	   to_line(int cp),
	   findfwd(char *pattern, int start, int endp),
	   findback(char *pattern, int start, int endp),
	   lvgetcontext(int c, bool begline),
	   getKey(void),
	   insertfile(FILE*,int,int,int*),
	   do_file(char *fname, exec_type *mode),
	   setend(void);

extern void clrprompt(void),
	    error(void),
	    insert_to_undo(struct undostack *u, int start, int size),
	    resetX(void),
	    zerostack(struct undostack *u),
	    swap(int *a, int *b),
	    printch(int c),
	    prints(char *s),
	    writeline(int y, int x, int start),
	    refresh(int y, int x, int start, int endd, bool rest),
	    redisplay(bool flag),
	    scrollback(int curr),
	    scrollforward(int curr),
	    prompt(bool toot, char *s),
	    setpos(int loc),
	    resetX(void),
	    insertmacro(char *cmdstr, int count),
	    wr_stat(void),
	    movearound(cmdtype cmd),
	    printi(int num),
	    println(void),
	    version(void),
	    setcmd(void),
	    toedit(int count),
	    doinput(int fileptr),
	    inputf(register char *fname, bool newbuf),
	    fixmarkers(int base, int offset),
	    errmsg(char *msg),
	    setarg(char *s);
extern int exec(char *cmd, exec_type *mode);
extern char *ntoa(int num),
	    *class(int c);

#ifndef moveleft
extern void moveleft(register char *src, register char *dest, register int length);
#endif
#ifndef moveright
extern void moveright(register char *src, register char *dest, register int length);
#endif
#ifndef fillchar
extern void fillchar(char*,int,char);
#endif


extern void dwrite(char *, int);
extern void dputs(char *);
extern void dputc(char);
extern void dgotoxy(int,int);
extern void dclear_to_eol(void);
extern void dclearscreen(void);
extern void dnewline(void);
extern void dopenline(void);
extern void d_cursor(int);
extern void d_highlight(int);
extern void dinitialize(void);
extern void dscreensize(int *, int *);
extern void drestore(void);
extern void Ping(void);

extern int os_initialize(void);
extern int os_restore(void);
extern int os_clear_to_eol(void);
extern int os_clearscreen(void);
extern int os_cursor(int);
extern int os_highlight(int);
extern int os_write(char *, int);
extern int os_gotoxy(int,int);
extern int os_initialize(void);
extern int os_openline(void);
extern int os_screensize(int *, int *);
extern int os_scrollback(void);
extern int os_newline(void);
extern int os_Ping(void);


extern void set_input(void);
extern void reset_input(void);
extern char *dotfile(void);

extern int os_mktemp(char *, int, const char *);
extern int os_unlink(char *);
extern int os_rename(char *, char *);
extern int os_glob(const char *, int, glob_t *);
extern void os_globfree(glob_t *);
extern char *os_tilde(char *);
extern char *os_backupname(char *);
extern int os_subshell(char *);
extern FILE* os_cmdopen(char *, char *, os_pid_t *);
extern int os_cmdclose(FILE*, os_pid_t);

extern int os_cclass(unsigned int c);
#define CC_CTRL  0
#define CC_PRINT 1
#define CC_TAB   2
#define CC_OTHER 3

extern int Max(int,int);
extern int Min(int,int);
extern char *lvtempfile(char*);

extern FILE* expandfopen(char *file, char *mode);

extern void lowercase(char*);

#if !HAVE_STRDUP
extern char* strdup(char*);
#endif

#if !HAVE_BASENAME
extern char* basename(char*);
#endif

#endif /*EXTERN_D*/
