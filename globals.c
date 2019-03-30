/*
 * LEVEE, or Captain Video;  A vi clone
 *
 * Copyright (c) 1982-2008 David L Parsons
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
/* global declarations */

#include "levee.h"
#include "extern.h"

#define GLOBALS

char lastchar,		/* Last character read via peekc */
     ch;		/* Global command char */

exec_type mode;			/* editor init state */
int lastexec = 0;		/* last exec command */

int contexts['z'-'`'+1];	/* Labels */

		/* C O N S T A N T S */

bool adjcurr[PARA_BACK+1],
     adjendp[PARA_BACK+1];

		/* A R G U M E N T S */
char *startcmd = NULL;	/* initial command after read */
glob_t args;		/* arguments
			 * argv -> args->gl_pathv
			 * argc -> args->gl_pathc
			 */
int filenm = F_UNSET,	/* current filename */
    altnm = F_UNSET;	/* previous filename */

		/* M A C R O   S T U F F */
struct macrecord mbuffer[MAXMACROS];
struct tmacro mcr[NMACROS];		/* A place for executing macros */

		/* S E A R C H   S T U F F */
char dst[80] = "",			/* last replacement pattern */
     lastpatt[80] = "",			/* last search pattern */
     pattern[MAXPAT] = "";		/* encoded last pattern */

int RE_start[9],			/* start of substitution arguments */
    RE_size [9],			/* size of substitution arguments */
    lastp;				/* end of last pattern */

struct undostack undo;			/* To undo a command */


		/* R A N D O M   S T R I N G S */

char instring[SZ_INSTRING];	/* Latest input */
char gcb[16];			/* Command buffer for mutations of insert */

char *undobuf;
char *undotmp;
char *yankbuf;

FILEDESC uread,		/* reading from the undo stack */
	 uwrite;	/* writing to the undo stack */

			    /* B U F F E R S */
char rcb[256];		/* last modification command */
char *rcp;		/* this points at the end of the redo */
char core[EDITSIZE+1];	/* data space */

struct ybuf yank;	/* last deleted/yanked text */


/* STATIC INITIALIZATIONS: */

/* ttydef stuff */

int LINES, COLS;

bool CA = 0,
     canUPSCROLL = 0,
     canOL = 0;

char FkL, CurRT, CurLT, CurUP, CurDN;

char *TERMNAME,		/* will be set in termcap handling */
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

char Erasechar,				/* our erase character */
     Eraseline;				/* and line-kill character */

char fismod[] = "File is modified",	/* File is modified message */
     fisro[] = "File is readonly";	/* when you can't write the file */

excmd_t excmds[] = {
	{ "print", 1, 0},		/* lines to screen */
	{ "quit", 1, 0},		/* quit editor */
	{ "read", 1, 1},		/* add file to buffer */
	{ "edit", 1, 1},		/* replace buffer with file */
	{ "write", 1, 1},		/* write out file */
	{ "wq", 1, 1},			/* write file and quit */
	{ "next", 1, 1},		/* make new arglist or traverse this one */
	{ "substitute", 1},		/* pattern */
	{ "xit", 1},			/* write changes and quit */
	{ "file", 1, 1},		/* show/set file name */
	{ "set", 1, 0},			/* options */
	{ "rm", 0, 1},			/* a file (never implemented?) */
	{ "previous", 1, 0},		/* back up in arglist */
	{ "delete", 1, 0},		/* lines from buffer */
	{ "=", 1, 0},			/* tell line number */
	{ "yank", 1, 0},		/* lines from buffer */
	{ "put", 1, 0},			/* back yanked lines */
	{ "visual", 1, 0},		/* go to visual mode */
	{ "exec", 1, 0},		/* go to exec mode */
	{ "insert", 1, 0},		/* text below current line */
	{ "open", 1, 0},		/* insert text above current line */
	{ "change", 1, 0},		/* lines */
	{ "undo", 1, 0},		/* last change */
	{ "!", 1, 1},			/* shell escape */
	{ "map", 1, 0},			/* keyboard macro */
	{ "unmap", 1, 0},		/* keyboard macro */
	{ "source", 1, 1},		/* read commands from file */
	{ "version", 1, 0},		/* print version # */
	{ "args", 1, 0},		/* print argument list */
	{ "rewind", 1, 0},		/* rewind argument list */
	{ "tag", 1, 0},			/* jump to a tag */
	{ NULL }
};

char wordset[] = "0123456789$_#ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
#if HARD_EOL
char spaces[] = { TAB,EOL,' ',0 };
#else
char spaces[] = { TAB, '\n', ' ', 0 };
int EOL = 10;
#endif

int shiftwidth = 4,
    dofscroll,
    tabsize    = 8,
    autoindent = YES,
    autocopy   = NO,
    autowrite  = YES,
    wrapscan   = YES,
    overwrite  = YES,
    beautify   = YES,
    list       = NO,
    magic      = YES,
    bell       = YES,
    ignorecase = NO;

#if LOGGING
int is_logging = YES;
#endif

struct variable vars[]={
#if LOGGING
    {"DEBUG BUILD","",   VBOOL,	V_CONST,	(void*)&is_logging },
#endif
    {"terminal",   "",	 VSTR,	V_CONST,	(void*)&TERMNAME   },
    {"shiftwidth", "sw", VINT,	0,		(void*)&shiftwidth },
    {"scroll",	   "",	 VINT,	0,		(void*)&dofscroll  },
    {"tabsize",    "ts", VINT,	V_DISPLAY,	(void*)&tabsize    },
    {"autoindent", "ai", VBOOL,	0,		(void*)&autoindent },
    {"autocopy",   "ac", VBOOL,	0,		(void*)&autocopy   },
    {"backup",     "",   VBOOL, V_ALIAS,	(void*)&autocopy   },
    {"autowrite",  "aw", VBOOL,	0,		(void*)&autowrite  },
    {"wrapscan",   "ws", VBOOL,	0,		(void*)&wrapscan   },
    {"overwrite",  "ow", VBOOL,	0,		(void*)&overwrite  },
    {"beautify",   "be", VBOOL,	0,		(void*)&beautify   },
    {"list",	   "",	 VBOOL,	V_DISPLAY,	(void*)&list       },
    {"magic",	   "",	 VBOOL,	0,		(void*)&magic      },
    {"ignorecase", "ic", VBOOL,	0,		(void*)&ignorecase },
    {"bell",       "",	VBOOL,	0,		(void*)&bell       },
    {NULL}
};

/* For movement routines */
int setstep[2] = {-1,1};

/* Where the last diddling left us */
struct coord curpos={0, 0};

    /* initialize the buffer */
int  bufmax = 0,		/* End of file here */
     lstart = 0, lend = 0,	/* Start & end of current line */
     ptop   = 0, pend = 0,	/* Top & bottom of CRT window */
     curr   = 0,		/* Global cursor pos */
     xp     = 0, yp   = 0,	/* Cursor window position */
     count  = 0;		/* Latest count */

bool modified= NO,		/* File has been modified */
     readonly= NO,		/* is this file readonly? */
     needchar= YES,		/* Peekc flag */
     deranged= NO,		/* Up-arrow || down-arrow left xpos in Oz. */
     redoing = NO,		/* doing a redo? */
     xerox   = NO,		/* making a redo buffer? */
     newfile = YES,		/* Editing a new file? */
     newline = NO,		/* Last insert/delete included a EOL */
     lineonly= NO,		/* Dumb terminal? */
     zotscreen=NO,		/* ask for [more] in execmode */
     redraw  = NO;		/* force new window in editcore */

bool is_viewer = NO;		/* set when the -r flag is passed to levee */

int  indirect= 0;		/* Nonzero when we're executing a script */

int  macro = -1;    		/* Index into MCR */
char lsearch = 0;		/* for N and n'ing... */

/* movement, command codes */

cmdtype movemap[256]={
    /*^@*/ BAD_COMMAND,
    /*^A*/ DEBUG_C,
    /*^B*/ HARDMACRO,
    /*^C*/ BAD_COMMAND,
    /*^D*/ WINDOW_UP,
    /*^E*/ HARDMACRO,
    /*^F*/ HARDMACRO,
    /*^G*/ FILE_C,
    /*^H*/ GO_LEFT,		/* also leftarrow  */
    /*^I*/ REDRAW_C,
    /*^J*/ GO_DOWN,		/* also downarrow  */
    /*^K*/ GO_UP,		/* also uparrow    */
    /*^L*/ GO_RIGHT,		/* also rightarrow */
    /*^M*/ CR_FWD,
    /*^N*/ BAD_COMMAND,
    /*^O*/ BAD_COMMAND,
    /*^P*/ BAD_COMMAND,
    /*^Q*/ BAD_COMMAND,
    /*^R*/ FULL_REDRAW_C,
    /*^S*/ BAD_COMMAND,
    /*^T*/ BAD_COMMAND,		/* in case I put a tabstack in */
    /*^U*/ WINDOW_DOWN,
    /*^V*/ BAD_COMMAND,
    /*^W*/ BAD_COMMAND,
    /*^X*/ BAD_COMMAND,
    /*^Y*/ HARDMACRO,
    /*^Z*/ BAD_COMMAND,
    /*^[*/ BAD_COMMAND,
    /*^\*/ BAD_COMMAND,
    /*^]*/ TAG_C,
    /*^^*/ BAD_COMMAND,
    /*^_*/ BAD_COMMAND,
    /*  */ GO_RIGHT,
    /*! */ EXEC_C,
    /*" */ BAD_COMMAND,
    /*# */ BAD_COMMAND,
    /*$ */ TO_EOL,
    /*% */ MATCHEXPR,
    /*& */ RESUBST_C,
    /*\ */ TO_MARK_LINE,
    /*( */ SENT_BACK,
    /*) */ SENT_FWD,
    /** */ BAD_COMMAND,
    /*+ */ CR_FWD,
    /*, */ BAD_COMMAND,
    /*- */ CR_BACK,
    /*. */ REDO_C,
    /*/ */ PATT_FWD,
    /*0 */ TO_COL,
    /*1 */ BAD_COMMAND,
    /*2 */ BAD_COMMAND,
    /*3 */ BAD_COMMAND,
    /*4 */ BAD_COMMAND,
    /*5 */ BAD_COMMAND,
    /*6 */ BAD_COMMAND,
    /*7 */ BAD_COMMAND,
    /*8 */ BAD_COMMAND,
    /*9 */ BAD_COMMAND,
    /*: */ COLIN_C,
    /*; */ BAD_COMMAND,
    /*< */ ADJUST_C,
    /*= */ BAD_COMMAND,
    /*> */ ADJUST_C,
    /*? */ PATT_BACK,
    /*@ */ BAD_COMMAND,
    /*A */ A_AT_END,
    /*B */ BACK_WD,
    /*C */ HARDMACRO,
    /*D */ HARDMACRO,
    /*E */ BAD_COMMAND,
    /*F */ BACK_CHAR,
    /*G */ GLOBAL_LINE,
    /*H */ PAGE_BEGIN,
    /*I */ I_AT_NONWHITE,
    /*J */ JOIN_C,
    /*K */ BAD_COMMAND,
    /*L */ PAGE_END,
    /*M */ PAGE_MIDDLE,
    /*N */ BSEARCH,
    /*O */ OPENUP_C,
    /*P */ PUT_AFTER,
    /*Q */ EDIT_C,
    /*R */ BIG_REPL_C,
    /*S */ BAD_COMMAND,
    /*T */ BACKTO_CHAR,
    /*U */ BAD_COMMAND,
    /*V */ BAD_COMMAND,
    /*W */ FORWD,
    /*X */ HARDMACRO,
    /*Y */ HARDMACRO,
    /*Z */ ZZ_C,
    /*[ */ BAD_COMMAND,
    /*\ */ BAD_COMMAND,
    /*] */ BAD_COMMAND,
    /*^ */ NOTWHITE,
    /*_ */ BAD_COMMAND,
    /*` */ TO_MARK,
    /*a */ APPEND_C,
    /*b */ BACK_WD,
    /*c */ CHANGE_C,
    /*d */ DELETE_C,
    /*e */ FORWD,
    /*f */ TO_CHAR,
    /*g */ BAD_COMMAND,
    /*h */ GO_LEFT,
    /*i */ INSERT_C,
    /*j */ GO_DOWN,
    /*k */ GO_UP,
    /*l */ GO_RIGHT,
    /*m */ MARKER_C,
    /*n */ FSEARCH,
    /*o */ OPEN_C,
    /*p */ PUT_BEFORE,
    /*q */ BAD_COMMAND,
    /*r */ REPLACE_C,
    /*s */ HARDMACRO,
    /*t */ UPTO_CHAR,
    /*u */ UNDO_C,
    /*v */ BTO_WD,
    /*w */ TO_WD,
    /*x */ HARDMACRO,
    /*y */ YANK_C,
    /*z */ REWINDOW,
    /*{ */ PARA_BACK,
    /*| */ TO_COL,
    /*} */ PARA_FWD,
    /*~ */ TWIDDLE_C,
    /*^?*/ BAD_COMMAND,
    /*80*/ BAD_COMMAND,
    /*81*/ BAD_COMMAND,
    /*82*/ BAD_COMMAND,
    /*83*/ BAD_COMMAND,
    /*84*/ BAD_COMMAND,
    /*85*/ BAD_COMMAND,
    /*x6*/ BAD_COMMAND,
    /*x7*/ BAD_COMMAND,
    /*x8*/ BAD_COMMAND,
    /*x9*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND,
    /*xx*/ BAD_COMMAND
};
