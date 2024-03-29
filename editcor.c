/*
 * LEVEE, or Captain Video;  A vi clone
 *
 * Copyright (c) 1982-2018 David L Parsons
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
#include <ctype.h>

/* do some undoable modification */

/* These variables make docommand nonrecursive */

bool	ok;
int	newend,		/* end position after change */
	disp,		/* start redisplay here */
	newc,		/* new cursor position for wierd cmds */
	endY;		/* final yp for endp */

extern int affirm;	/* override file is modified checks (exec.c) */

/* called when we enter editcore or do something that invalidates the screen
 */
void
maybe_refresh_screen(int redraw)
{
    logit(("maybe_refresh_screen(%d) curr = %d, ptop = %d, pend = %d", redraw, curr, ptop, pend));

    if ( curr < ptop || curr > pend )
	redraw = TRUE;

    if (redraw) {
	setpos(skipws(curr));		/* set cursor x position.. */
	yp = settop(LINES / 2);		/* Y position */
    }
    if (redraw || zotscreen)		/* redisplay? */
	redisplay(FALSE);
    dgotoxy(xp, yp);			/* and move the cursor */
}


/* move a line of text right || left */

void
adjuster(bool sleft,int endd,int sw)
{
    bool noerror;
    int np, ts,
	ip,
	ss, adjp,
	DLEnum;

    if (sw == -1)
	sw = shiftwidth;
    if (sleft)
	sw = -sw;
    curr = bseekeol(curr);
    ip = curr;
    noerror = TRUE;
    do {
	DLEnum = sw + findDLE(ip, &np, bufmax,0);
	if (DLEnum >= 0 && DLEnum <= COLS && core[np] != EOL && np < bufmax) {
	    ts = DLEnum / tabsize;
	    ss = DLEnum % tabsize;
	    adjp = ts+ss+ip;
	    if (np-adjp < 0) {	/* expand the buf */
		moveright(&core[np], &core[adjp], bufmax-np);
		insert_to_undo(&undo, adjp, adjp - np);
	    }
	    else
		delete_to_undo(&undo, adjp, np - adjp);

	    endd += (adjp-np);
	    noerror = move_to_undo(&undo, ip, ts+ss);
	    fillchar(&core[ip], ts, TAB);
	    fillchar(&core[ip+ts], ss, 32);
	}
	else if (np > ip) {	/* remove the indent code */
	    noerror = delete_to_undo(&undo, ip, np-ip);
	    endd += (ip - np);
	}
	ip = 1 + fseekeol(ip);
    } while (noerror && ip < endd);
    if (!noerror)
	error();
    newc = skipws(curr);
    disp = curr;
    newend = endd;
    endY = setY(Min(newend, pend));
}

/* join <count> lines together */

void
join(int count)
{
    bool ok;
    int lp, first;

    if (lend < bufmax) {	/* are we in the buffer? */
	disp = lend;				/* start redraw here */
	newc = lend;
	do {					/* join until.. */
	    first = lend;
	    lp = skipws(1+first);
	    ok = delete_to_undo(&undo, 1+first, lp-(1+first));
	    if (ok) {
		ok = move_to_undo(&undo, first, 1);
		core[first] = ' ';		/* spaces between lines */
	    }
	    count--;
	    lend = fseekeol(first);
	} while (ok && count > 0);
	endY = MAGICNUMBER;
	newend = lend;
	if (!ok)
	    error();
    }
    else
	error();
}

void
squiggle(int endp, char c, bool dorepl)
{
    int i;

    if (endp >= curr) {
	ok = move_to_undo(&undo,curr,endp-curr+1);
	if (ok) {
	    for (i = curr;i<=endp;i++) {
		if (!dorepl) {		/* squiggle it to uc - lc */
		    if (core[i] >='A' && core[i] <='Z')
			core[i] += 32;
		    else if (core[i]>='a' && core[i]<='z')
			core[i] -= 32;
		}
		else
		    core[i] = c;
	    }
	    newend = Min(endp+1,lend);
	}
    }
}

void
bigreplace(void)
{
    int len, tsiz;

    tsiz = lend-curr;
    if (move_to_undo(&undo, curr, tsiz))
	if (EDITSIZE - bufmax > tsiz) {	/* enough room for temp copy? */
	    moveleft(&core[curr], &core[bufmax],lend-curr);
	    if (line(core, curr, lend, &len) != ESC)
		error();
	    newend = curr+len;
	    moveright(&core[bufmax+len], &core[newend], lend-newend);
	}
}

bool
put(bool before)
{
    endY = setY(curr);
    if (!before)
	if (yank.lines)
	    curr = Min(lend+1,bufmax);
	else
	    curr = Min(curr+1, lend);
    else if (yank.lines)
	curr = lstart;
    newc = disp = curr;
    return(putback(curr, &newend));
}

bool
execute(int start,int end)
{
    FILEDESC tf;
    FILE *f;
    char scratch[200];
    bool ret = FALSE;
    int size;
    os_pid_t child;
    static char tferror[] = "[tempfile error]";

    unless (os_mktemp(scratch, sizeof scratch, "lv")) {
	errmsg(tferror);
	return FALSE;
    }

    if ( (tf = OPEN_NEW(scratch)) == NOWAY ) {
	prints(tferror);
	return FALSE;
    }

    clrprompt();
    printch('!');
    unless ( lvgetline(instring, sizeof instring) )
	return FALSE;

    if ( (size = WRITE_TEXT(tf, core+start, end-start)) == (end-start) ) {
	if ( (f=os_cmdopen(instring, scratch, &child)) ) {
	    if ( deletion(start, end) && (insertfile(f, 1, start, &size) > 0) )
		ret = TRUE;
	    os_cmdclose(f, child);
	}
	else
	    error();
    }
    else
	prints(tferror);

    CLOSE_FILE(tf);
    os_unlink(scratch);

    return ret;
}

void
vitag(void)
{
    int start, endd;
    int newfile, rc;
    Tag loc;

    if ( bufmax == 0 || (class(core[curr]) != wordset) ) {
	error();
	return;
    }

    start = endd = curr;

    while ( start > 0 && (class(core[start]) == wordset) )
	--start;

    if ( class(core[start]) != wordset )
	++start;

    while ( endd < bufmax && (class(core[endd]) == wordset) )
	++endd;

    logit(("vitag: tag %.*s", endd-start, &core[start]));

    if ( find_tag(&core[start], endd-start, &loc) &&
	(newfile = addarg(loc.filename)) != F_UNSET ) {

	clrprompt();
	affirm = NO;
	if ( newfile == filenm || oktoedit(autowrite) ) {
	    push_tag(filenm, curr);
	    rc = gototag(newfile, loc.pattern);
	    if ( rc != ERR ) {
		maybe_refresh_screen(rc != SAMEFILE);
		return;
	    }
	    pop_tag();
	}
	dgotoxy(xp,yp);
    }
    error();
}



void
goback(void)
{
    Camefrom *loc = pop_tag();

    if ( loc ) {
	logit(("goback: newfile=%d (filenm=%d), curr=%d", loc->fileno, filenm, loc->cursor));

	if ( loc->fileno == filenm ) {
	    curr = loc->cursor;
	    maybe_refresh_screen(YES);
	    return;
	}
	clrprompt();
	affirm = NO;
	if ( oktoedit(autowrite) ) {
	    doinput(loc->fileno);
	    curr = Min(bufmax-1, loc->cursor);
	    maybe_refresh_screen(YES);
	    return;
	}
	dgotoxy(xp,yp);
    }
    error();
}

void
gcount(void)
{
    do {
	count = (count*10) + (ch-'0');
	readchar(); /* get a char to replace the one we dumped */
    } while ( count >= 0 && isdigit(ch) );
}

void
docommand(cmdtype cmd)
{
    cmdtype movecmd;	/* movement command for y, d, c */
    char    cmdch;
    int     oldc;	/* old count */
    int     endp;	/* end position before change */
    int     oldmaxY, newmaxY;	/* # lines at pend before/after */
    extern bool s_wrapped;

    resetX();				/* un-derange the cursor */
    oldc = newc = -1;
    endY = yp;
    newend = disp = curr;
    oldmaxY = setY(pend);
    ok = TRUE;				/* so far everything is working */
    cmdch = ch;
    if (cmd != UNDO_C && cmd != YANK_C) {
	if (macro<0)
	    zerostack(&undo);
	if (redoing != TRUE) {
	    rcp = rcb;		/* point at start of redo buffer */
	    if (count > 1) {	/* put in a count? */
		strcpy(rcb, ntoa(count));
		rcp += strlen(rcb);
	    }
	    *rcp++ = cmdch;	/* the command char goes in... */
	    xerox = TRUE;	/* hoist the magical flags */
	}
    }

    if (cmd <= YANK_C) {
	readchar();
	if ( isdigit(ch) && ch != '0' ) {
	    oldc = count;
	    count = 0;
	    gcount();				/* get a new count */
	    if (cmd == ADJUST_C)		/* special for >>,<< wierdness */
		swap(&count, &oldc);		/* reverse sw & count */
	    else
		count = count*Max(oldc,1);	/* combine them */
	}
	if (ch == cmdch) {		/* diddle lines */
	    yank.lines = TRUE;
	    endp = nextline(TRUE, curr, count);
	    curr = bseekeol(curr);
	    disp = curr;
	}
	else {				/* diddle 'things' */
	    yank.lines = FALSE;
	    movecmd = movemap[(unsigned int)ch];

	    if ( (ok = (findCP(curr,&endp,movecmd) == LEGALMOVE)) ) {
		if (curr > endp) {
		    swap(&curr,&endp);
		    ok = (cmd != CHANGE_C);
		}
		if (adjcurr[(unsigned int)movecmd])
		    curr++;
		if (adjendp[(unsigned int)movecmd])
		    endp++;
	    }
	    if (!ok) {
		if (ch != ESC)
		    error();
		goto killredo;
	    }
	}

	endY = setY(endp);
	newend = curr;
	disp = curr;
	switch (cmd) {
	    case EXEC_C:
		ok = execute(curr, endp);
		break;
	    case DELETE_C:
		ok = deletion(curr, endp);
		break;
	    case ADJUST_C:
		adjuster((cmdch == '<'), endp-1, oldc);
		break;
	    case CHANGE_C:
		if (endp <= pend+1) {
		    dgotoxy(setX(endp-1), setY(endp-1));
		    printch('$');
		    dgotoxy(xp, yp);
		}
		if (deletion(curr, endp)) {
		    newend = insertion(1, 0, &disp, &endY, TRUE);
		    ok = (newend >= 0);
		}
		else
		    ok = FALSE;
		break;
	    case YANK_C:
		if (!doyank(curr, endp))
		    error();
		return;	/* xerox will not be true, nor will redoing */
	}

    }
    else {
	endp = curr;
	endY = yp;

	switch (cmd) {
	    case I_AT_NONWHITE:
	    case A_AT_END:
	    case APPEND_C:
	    case INSERT_C:		/* variations on insert */
		if (cmd != INSERT_C) {
		    if (cmd == APPEND_C)
			curr = Min(curr+1, lend);
		    else if (cmd == A_AT_END)
			curr = lend;
		    else /* if (cmd == I_AT_NONWHITE) */
			curr = skipws(lstart);
		    xp = setX(curr);
		    dgotoxy(xp,yp);
		}
		newend = insertion(count, 0, &disp, &endY, TRUE);
		ok = (newend >= 0);
		break;
	    case OPEN_C:
	    case OPENUP_C:
		newend = insertion(1,setstep[ (cmd==OPENUP_C)&1 ],
						&disp,&endY,TRUE)-1;
		ok = (newend >= 0);
		break;
	    case REPLACE_C:
	    case TWIDDLE_C:
		if (cmd == REPLACE_C) {
		    if ((cmdch = readchar()) == ESC)
			goto killredo;
		}
		if (findCP(curr, &endp, GO_RIGHT) == LEGALMOVE)
		    squiggle(endp-1, cmdch, (cmd==REPLACE_C));
		break;
	    case PUT_BEFORE:
	    case PUT_AFTER:
		ok = put(cmd==PUT_AFTER);
		break;
	    case BIG_REPL_C:
		bigreplace();
		break;
	    case RESUBST_C:
		ok = FALSE;
		if (dst[0] != 0) {
		    newend = chop(curr, &lend, TRUE, &ok);
		    if (newend >= 0) {
			endY = setY(newend+strlen(dst));
			ok = TRUE;
		    }
		}
		break;
	    case JOIN_C:
		join(count);		/* join lines */
		break;
	    case UNDO_C:			/* undo last modification */
		ok = fixcore(&newend) >= 0;
		disp = newend;
		endY = MAGICNUMBER;
		break;
	}
    }

    if (ok) {
	setpos((newc<0)?newend:newc);
	newmaxY = setend();

	logit(("modification (%d) -> %d: endY = %d, setY(newend) = %d, " "oldmaxY = %d, newmaxY = %d",  cmd, ok, endY, setY(newend), oldmaxY, newmaxY));

	if (curr < ptop || curr > pend) {
	    yp = settop(LINES/2);
	    redisplay(TRUE);
	}
	else {
	    yp = setY(curr);
	    if ( (endY != setY(newend)) || (oldmaxY != newmaxY) )
					/* shuffled lines, refresh rest of page */
		refresh(setY(disp), setX(disp), disp, pend, TRUE);
	    else			/* refresh to end position */
		refresh(setY(disp), setX(disp), disp, newend, FALSE);
	}

	if (curr >= bufmax && bufmax > 0) {	/* adjust off end of buffer */
	    setpos(bufmax-1);
	    yp = setY(curr);
	}
	if (s_wrapped) {
	    prompt(FALSE, "search wrapped around end of buffer");
	    s_wrapped = 0;
	}
	else
	    clrprompt();
	modified = TRUE;
    }
    else {
	error();
killredo:
	rcb[0] = 0;
    }
    dgotoxy(xp, yp);
    if (xerox)
	*rcp = 0;	/* terminate the redo */
    redoing = FALSE;
    xerox = FALSE;
    core[bufmax] = EOL;
}

/* Initialize && execute a macro */

void
exmacro(void)
{
    int mp;

    mp = lookup(ch);
    if (mp > 0) {
	if (macro<0)
	    zerostack(&undo);
	insertmacro(mbuffer[mp].m_text, count);
    }
    else
	error();
}

/* redraw the screen w.r.t. the cursor */

void
zdraw(unsigned char code)
{
    int nl = ERR,
	np = (count>0)?to_index(count):curr;

    if (movemap[code] == CR_FWD)
	nl = 0;
    else if (movemap[code] == CR_BACK)
	nl = LINES-1;
    else if (code == '.')
	nl = LINES / 2;
    if (nl >= 0) {
	curr = np;
	yp = settop(nl);
	redisplay(TRUE);
	dgotoxy(xp,yp);
    }
    else
	error();
}

/* start up a built-in macro */

void
macrocommand(void)
{
    if (count > 1)
	strcpy(gcb, ntoa(count));
    else
	gcb[0] = 0;
    switch (ch) { /* which macro? */
	case 'x':			/* x out characters */
	    strcat(gcb,"dl"); break;
	case 'X':			/* ... backwards */
	    strcat(gcb,"dh"); break;
	case 's':			/* substitute over chars */
	    strcat(gcb,"cl"); break;
	case 'D':			/* delete to end of line */
	    strcat(gcb,"d$"); break;
	case 'C':			/* change ... */
	    strcat(gcb,"c$"); break;
	case 'Y':			/* yank ... */
	    strcat(gcb,"y$"); break;
	case '':			/* scroll up one page */
	    strcpy(gcb,"22"); break;
	case '':			/* ... down one page */
	    strcpy(gcb,"22"); break;
	case '':			/* scroll up one line */
	    strcpy(gcb,"1"); break;
	case '':			/* ... down one line */
	    strcpy(gcb,"1"); break;
	default:
            error();
            return;
	break;
    }
    if (macro<0)
	zerostack(&undo);
    insertmacro(gcb, 1);
}

/* scroll the window up || down */

void
scroll(bool down)
{
    int i;

    if (count <= 0)
	count = dofscroll;

    d_cursor(0);

    if (down) {
	curr = Min(bufmax-1, nextline(TRUE, curr, count));
	i = Min(bufmax-1, nextline(TRUE, pend, count));
	if (i > pend)
	    scrollforward(i);
    }
    else {
	curr = bseekeol(Max(0,nextline(FALSE, curr, count)));
	i = bseekeol(Max(0,nextline(FALSE, ptop, count)));
	if (i < ptop) {
	    if (canUPSCROLL)
		scrollback(i);
	    else {
		ptop = i;
		setend();
		redisplay(TRUE);
	    }
	}
    }

    d_cursor(1);

    setpos(skipws(curr));	/* initialize new position - first nonwhite */
    yp = setY(curr);
    dgotoxy(xp, yp);		/* go there */
}

exec_type
editcore(void)
{
    cmdtype cmd;
    extern bool s_wrapped;

    /* rcb[0] = 0; rcp = rcb; */

    maybe_refresh_screen(redraw);

    for (;;) {
	s_wrapped = 0;
	ch = readchar();			/* get a char */
	count = 0;
	if (isdigit(ch) && ch != '0')
	    gcount();			/* ... a possible count */
	switch (cmd = movemap[(unsigned int)ch]) {
	  case FILE_C:
	    wr_stat();			/* write file stats */
	    dgotoxy(xp, yp);
	    break;

	  case WINDOW_UP:
	  case WINDOW_DOWN:
	    scroll(cmd==WINDOW_UP);		/* scroll the window */
	    break;

	  case FULL_REDRAW_C:
	    dclearscreen();
	  case REDRAW_C:			/* redraw the window */
	    redisplay(TRUE);
	    dgotoxy(xp, yp);
	    break;

	  case MARKER_C:			/* set a marker */
	    ch = tolower(readchar());
	    if (ch >= 'a' && ch <= 'z')
		contexts[ch-'`'] = curr;
	    else if (ch != ESC)
		error();
	    break;

	  case TAG_C:
	    vitag();
	    break;

	  case POP_TAG:
	    goback();
	    break;

	  case REDO_C:
	    if (rcb[0] != 0) {
		zerostack(&undo);
		insertmacro(rcb, 1);
		redoing = TRUE;
	    }
	    break;

	  case REWINDOW:
	    zdraw(readchar());		/* shift the window */
	    break;

	  case DEBUG_C:			/* debugging stuff -- unused */
	    break;

	  case ZZ_C:			/* shortcut for :xit */
	    ch = readchar();
	    if (ch == 'Z')
		insertmacro(":x\r", 1);
	    else if (ch != ESC)
		error();
	    break;

	  case EDIT_C:		/* drop into line mode */
	    return E_EDIT;

	  case COLON_C:		/* do one exec mode command */
	    return E_VISUAL;

	  case HARDMACRO:
	    macrocommand();		/* 'hard-wired' macros */
	    break;

	  case SOFTMACRO:
	    exmacro();		/* run a macro */
	    break;

	  case INSMACRO:		/* macro for insert mode */
	  case BAD_COMMAND:
	    error();
	    break;

	  default:
	    if (cmd < DELETE_C)
		movearound(cmd);
	    else /*if (cmd < HARDMACRO)*/
		docommand(cmd);
	    break;
	}
	lastexec = 0;
    }
    /* never exits here */
}
