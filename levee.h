/*
 * LEVEE, or Captain Video;  A vi clone
 *
 * Copyright (c) 1980-2008 David L Parsons
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
 *
 *			  Levee v3.C
 *   C version for Unix/Atari ST/MS-DOS/OS-2/FlexOs/iRMX/etc
 *            Pascal version for UCSD Pascal 4.X
 *
 *	written fall 82' - now (approx) by David L. Parsons.
 *
 *		  many patches, suggestions,
 *		and impractical design goals by:
 *			Jim Bolland,
 *			John Plocher,
 *			John Tainter
 */
#ifndef LEVEE_D

#define LEVEE_D

#include "config.h"
#include <stdio.h>


#if LOGGING
extern void logger(char *, ...);
#define logit(x) logger x
#else
#define logit(x) 1
#endif

#ifndef TRUE
#define	TRUE	(1)	/* Nobody defines TRUE & FALSE, so I will do */
#define	FALSE	(0)	/* it myself */
#endif

#define unless(x)	if (!(x))
#define if(x)		if ((x))
#define while(x)	while ((x))

extern char *expand(char *f);

extern FILE *expandfopen(char *f, char *mode);


/* abstract out file i/o (separately from display i/o, hahaha)
 */
typedef void *FILEDESC;
#define NOWAY ((FILEDESC)-1)

extern char NULL_FILE[];
extern FILEDESC OPEN_OLD(char *);
extern FILEDESC OPEN_NEW(char *);
extern int CLOSE_FILE(FILEDESC);
extern long SEEK_POSITION(FILEDESC, long, int);
extern int READ_TEXT(FILEDESC, void *, int);
extern int WRITE_TEXT(FILEDESC, void *, int);

#define bool int

extern int LINES, COLS;

#define	YES	1
#define	NO	0

#define UPARROW	11
#define DNARROW	10
#define LTARROW	Erasechar
#define RTARROW	12

/* nospecific stuff */
#define MAGICNUMBER  42
#define hell_freezes_over  FALSE
#define BUGS	7	/* sometime when you least expect it.. */

#if HARD_EOL
#define EOL	10	/* End Of Line */
#else
extern int EOL;
#endif

#define DW	23	/* Delete Word */

#define DLE	16	/* Space compression lead-in */
#define ESC	27	/* Escape */

#define TAB	9
    
	/* variable types */
#define VBOOL	0
#define VINT	1
#define VSTR	2

#define ERR	(-1)
#define ERR_NOMATCH (ERR-1)	/* pattern didn't match */
#define ERR_PATTERN (ERR-2)	/* can't compile pattern */
#define ERR_RANGE   (ERR-3)	/* fell off end/beginning of buffer */
#define ERR_UNDEF   (ERR-4)	/* undefined marker */
#define ERR_EXPR    (ERR-5)	/* illegal address expression */
#define ERR_UNKNOWN (ERR-6)	/* unknown addressing symbol */

		/* Undostack commands */
#define U_ADDC	'A'
#define U_MOVEC	'M'
#define U_DELC	'D'

		/* magic things for find */
#define MAXPAT	((int)300)

		/* exec mode commands */
#define EX_PR		0
#define EX_QUIT		1
#define EX_READ		2
#define EX_EDIT		3
#define EX_WRITE	4
#define EX_WQ		5
#define EX_NEXT 	6
#define EX_SUBS 	7
#define EX_XIT		8
#define EX_FILE 	9
#define EX_SET		10
#define EX_RM		11
#define EX_PREV 	12
#define EX_DELETE	13
#define EX_LINE		14
#define EX_YANK		15
#define EX_PUT		16
#define EX_VI		17
#define EX_EX		18
#define EX_INSERT	19
#define EX_OPEN		20
#define EX_CHANGE	21
#define EX_UNDO		22
#define EX_ESCAPE	23
#define EX_MAP		24
#define EX_UNMAP	25
#define EX_SOURCE	26
#define EX_VERSION	27
#define EX_ARGS		28
#define	EX_REWIND	29
#define EX_TAG		30
#define EX_POPTAG	31


		/* ex commands that aren't in the excmd[] table */
#define	EX_CR		100
#define EX_COMMENT	101
#define EX_UNKNOWN	102

		/* movement return states */
#define LEGALMOVE	0
#define BADMOVE		1
#define ESCAPED		2
#define findstates char

		/* command codes */
#define BAD_COMMAND	0
		/*visual movement*/
#define GO_RIGHT	1
#define GO_LEFT		2
#define GO_UP		3
#define GO_DOWN		4
#define FORWD		5
#define TO_WD		6
#define BACK_WD		7
#define BTO_WD		8
#define NOTWHITE	9
#define TO_COL		10
#define TO_EOL		11
#define MATCHEXPR	12
#define TO_CHAR		13
#define UPTO_CHAR	14
#define BACK_CHAR	15
#define BACKTO_CHAR	16
#define SENT_FWD	17
#define SENT_BACK	18
#define PAGE_BEGIN	19
#define PAGE_END	20
#define PAGE_MIDDLE	21
#define CR_FWD		22
#define CR_BACK		23
#define PATT_FWD	24
#define PATT_BACK	25
#define FSEARCH		26
#define BSEARCH		27
#define GLOBAL_LINE	28
#define TO_MARK		29
#define TO_MARK_LINE	30
#define PARA_FWD	31
#define PARA_BACK	32
		/*modifications*/
#define DELETE_C	39
#define EXEC_C		40
#define ADJUST_C	41
#define CHANGE_C	42
#define YANK_C		43
#define INSERT_C	44
#define APPEND_C	45
#define I_AT_NONWHITE	46
#define A_AT_END	47
#define OPEN_C		48
#define OPENUP_C	49
#define REPLACE_C	50
#define TWIDDLE_C	51
#define RESUBST_C	52
#define JOIN_C		53
#define UNDO_C		54
#define BIG_REPL_C	55
#define PUT_BEFORE	56
#define PUT_AFTER	57
		/*everything else*/
#define HARDMACRO	70
#define REWINDOW	71
#define ZZ_C		72
#define DEBUG_C		73
#define FILE_C		74
#define WINDOW_UP	75
#define WINDOW_DOWN	76
#define REDRAW_C	77
#define MARKER_C	78
#define REDO_C		79
#define EDIT_C		80
#define COLON_C		81
#define TAG_C		82
#define FULL_REDRAW_C	83
#define POP_TAG		84
		    /*macros*/
#define SOFTMACRO	100
#define INSMACRO	101
#define cmdtype		int

		/* exec mode states */
#define E_VISUAL 0
#define E_INIT	 1
#define E_EDIT	 2
#define exec_type	int

		/* various sizes */
#define INSSIZE	 ((int)80)		/* Insert string size */
#define FSIZE	 ((int)39)		/* File string size */
#ifndef EDITSIZE
# define EDITSIZE ((int)32760)		/* Edit buffer size */
#endif

#define SBUFSIZE  ((int)4096)		/* Incore yank buffer size */
#define MAXMACROS 32			/* Maximum # of macros */
#define NMACROS	  9			/* Nexting level for macros */

#define PAGESIZE ((int)1024)		/* Bytes per block */

struct coord {		/* Screen Coordinate */
    int x,y;
};

struct ybuf {		/* Yank Buffer */
    int size;				/* Bytes yanked */
    bool lines,				/* Yanked whole lines? */
	has_eol;			/* Yanked a EOL? */
    char stuff[SBUFSIZE];		/* The stuff */
};

struct undostack {	/* Undo Stack Descriptor */
    int blockp,				/* block address of core block */
	ptr;				/* offset within coreblock */
    int coreblock[PAGESIZE];		/* core block */
};

union optionrec {	/* Black Magic Option Structure */
    int valu;				/* if integer, the value */
    char *strp;				/* or if string, a pointer to it */
};

struct macrecord {	/* Macro Descriptor */
    char token;				/* Character mapped */
    cmdtype oldmap;			/* Old value in movemap */
    char *m_text;			/* Replacement text */
};
		    
struct tmacro {		/* For running a macro */
    char *mtext,	/* Pointer to macro text */
	 *ip;		/* Pointer into macro text */
    int  m_iter;	/* Number of times to execute */
};

#define V_CONST   0x01	/* this option cannot be modified */
#define V_DISPLAY 0x02	/* this option affects the display */
#define V_ALIAS   0x04	/* this option is an alias for another */

struct variable {	/* Settable Variable Record */
    char *v_name;		/* full name */
    char *v_abbr;		/* abbreviated name */
    int v_tipe;			/* what kind of variable */
    int v_flags;		/* special attributes... */
    union optionrec *u;		/* pointer to it */
};

typedef struct {
    char *name;
    int active;	/* disabled if a feature is not available on a given platform */
    int expand;	/* expand % & #'s on the commandline */
} excmd_t;


/* tag matching
 */
typedef struct _tag {
    char *filename;
    char *pattern;
} Tag;

extern int find_tag(char *, int, Tag *);
extern int gototag(int, char*);

#define DIFFERENTFILE 0
#define SAMEFILE 1

/* tag backlink stack
 */
typedef struct _camefrom {
    int fileno;
    int cursor;
} Camefrom;

extern void push_tag(int, int);
extern Camefrom *pop_tag();


/* data structures for wildcard matching
 */
#if USING_GLOB
#include <glob.h>
#else

typedef struct _glob_t {	/* match the fields in the macos manpage */
    size_t gl_pathc;
    size_t gl_pathalloc;	/* extra field; allocation slop */
    int gl_matchc;
    size_t gl_offs;
    int gl_flags;
    char **gl_pathv;
} glob_t;

/* glob_t flags, from the macos manpage
 */
#define GLOB_APPEND	0x001	/* add to the existing pile of files */
#define GLOB_DOOFFS	0x002	/* put gl_offset empty slots in gl_pathv */
#define GLOB_ERR	0x004	/* stop when an error happens */
#define GLOB_MARK	0x008	/* add a trailing / to matched filenames */
#define GLOB_NOCHECK	0x010	/* treat unmatched patterns as filenames */
#define GLOB_NOESCAPE	0x020	/* don't allow wildcard escapes */
#define GLOB_NOSORT	0x040	/* don't sort the matched file list */

#define GLOB_MAGCHAR	0x100	/* set by glob if a wildcard char was found */
#define GLOB_NOMAGIC	0x200	/* don't expand wildcards */
#define GLOB_TILDE	0x400	/* expand ~user & ~ */

/* glob() error returns
 */
#define GLOB_NOSPACE	1
#define GLOB_ABORTED	2
#define GLOB_NOMATCH	3

#endif

#endif /*LEVEE_D*/
