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

#include "levee.h"
#include "extern.h"

#include <stdlib.h>

static void
copyright()
{
     dputs(" copyright (c) 1983-2019 by David Parsons");
}


void
initialize(argc, argv)
int argc;
char **argv;
/* initialize: set up everything I can in levee */
{
    int i;
    int xmode = E_INIT, xquit;
    char *p;
    Tag tag;
    int opt;

    dinitialize();
    set_input();
    dscreensize(&COLS, &LINES);
    dofscroll = LINES/2;

    /* print the version# if we can */
    if ( lineonly = !CA ) {
	version();
	copyright();
	dnewline();
	prints("(line mode)");
    }
    else if ( argc <= 1 ) {
	dgotoxy(0,LINES-1);
	version();
	copyright();
    }


    /* initialize macro table */
    for (i = 0;i < MAXMACROS;i++)
	mbuffer[i].token = 0;
    core[0] = EOL;

    yank.size = ERR;		/* no yanks yet */

    undo.blockp = undo.ptr = 0;

    fillchar(adjcurr, sizeof(adjcurr), 0);
    fillchar(adjendp, sizeof(adjendp), 0);

    adjcurr[BTO_WD]	=	/* more practical to just leave dynamic */
    adjcurr[SENT_BACK]	=
    adjendp[BTO_WD]	=
    adjendp[FORWD]	=
    adjendp[MATCHEXPR]	=
    adjendp[PATT_BACK]	=
    adjendp[TO_CHAR]	=
    adjendp[UPTO_CHAR]	=
    adjendp[PAGE_BEGIN]	=
    adjendp[PAGE_MIDDLE]=
    adjendp[PAGE_END]	= TRUE;

    fillchar(contexts, sizeof(contexts), -1);

    undobuf = lvtempfile("$un");
    yankbuf = lvtempfile("$ya");
    undotmp = lvtempfile("$tm");

    if ( p = getenv("LVRC") ) {
	strncpy(instring, p, SZ_INSTRING-1);
	instring[SZ_INSTRING-1] = 0;
	setarg(instring);
	setcmd();
    }
    else if ( p = dotfile() )
	do_file(p, &xmode, &xquit);

    memset(&args, sizeof args, 0);

#if SOFT_EOL
#  define OPTIONS "pt:"
#else
#  define OPTIONS "t:"
#endif

    while ( (opt=getopt(argc, argv, OPTIONS)) != EOF ) {
	switch (opt) {
#if SOFT_EOL
	    case 'p': /* UCSD-style EOL */
		EOL = '\r';
		break;
#endif
	    case 't':	/* edit file containing a tag */
		if ( find_tag(optarg, strlen(optarg), &tag) ) {
		    startcmd = tag.pattern;
		    if (!os_glob(tag.filename, GLOB_NOMAGIC|GLOB_APPEND, &args)) {
			int wasmagic = magic;
			magic = 0;
			doinput(0);
			magic = wasmagic;
		    }
		}
		else {
		    dgotoxy(0,LINES-1);
		    errmsg("Can't find tag <");
		    prints(optarg);
		    printf(">");
		}
		return;
	}
    }

    argc -= optind;
    argv += optind;

    if (argc > 0 && **argv == '+') {
	char *p = *argv;
	startcmd = p[1] ? &p[1] : "$";
	++argv, --argc;
    }

#if GLOB_REQUIRED
    /* Windows-style OS where command-line wildcards are NOT expanded by
     * the shell before levee is executed
     */
#   define CMDLINE_FLAGS GLOB_APPEND|GLOB_NOCHECK
#else
    /* Unix-style OS where command-line wildcards are expanded by the shell
     * before levee is executed
 */
#   define CMDLINE_FLAGS GLOB_APPEND|GLOB_NOMAGIC
#endif
    while (argc-- > 0)
	os_glob(*argv++, CMDLINE_FLAGS, &args);

    if (args.gl_pathc > 0) {
	filenm = 0;
	if (args.gl_pathc > 1)
	    toedit(args.gl_pathc);
	inputf(args.gl_pathv[filenm], TRUE);
    }
}

bool
execmode(emode)
exec_type emode;
{
    bool more,			/* used [more] at end of line */
	 noquit;		/* quit flag for :q, :xit, :wq */
    exec_type mode;

    zotscreen = redraw = FALSE;
    noquit = TRUE;

    if (lineonly)
	println();

    mode=emode;
    do {
	prompt(FALSE,":");
	if (lvgetline(instring, sizeof instring))
	    exec(instring, &mode, &noquit);
	indirect = FALSE;
	if (mode == E_VISUAL && zotscreen && noquit) {	/*ask for more*/
	    prints(" [more]");
	    if ((ch=peekc()) == 13 || ch == ' ' || ch == ':')
		readchar();
	    more = (ch != ' ' && ch != 13);
	}
	else
	    more = (mode == E_EDIT);
	if (mode != E_VISUAL && curpos.x > 0)
	    println();
    } while (more && noquit);
    if (zotscreen)
	clrprompt();
    return noquit;
}

int
main(argc,argv)
int argc;
char **argv;
{
    initialize(argc, argv);

    redraw = TRUE;	/* force screen redraw when we enter editcore() */
    if (lineonly)
	while (execmode(E_EDIT))
	    prints("(no visual mode)");
    else
	while (execmode(editcore()))
            /* do nada */;

    os_unlink(undobuf);
    os_unlink(yankbuf);

    if ( curpos.x )
	println();

    reset_input();
    os_restore();
    exit(0);
}
