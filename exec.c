#include "levee.h"
#include "extern.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <strings.h>
#if HAVE_ERRNO_H
#include <errno.h>
#endif

void undefine();
void fixupline();
void doinput();

/*
 * do a newline and set flags.
 */
#define exprintln()	(zotscreen=YES),println()

void
plural(num,string)
int num;
char *string;
{
    printi(num);
    prints(string);
    if (num != 1)
	printch('s');
} /* plural */


void
clrmsg()
{
    dgotoxy(0, -1);
    dclear_to_eol();
} /* clrmsg */


void
errmsg(msg)
char *msg;
{
    dgotoxy(0, -1);
    prints(msg);
    dclear_to_eol();
} /* errmsg */


/* get a space-delimited token */
char *execstr;			/* if someone calls getarg in the	*/
				/* wrong place, death will come...	*/
char *
getarg()
{
    char *rv;
    rv = execstr;
    while (*execstr && !isspace(*execstr))
	++execstr;
    if (*execstr) {
	*execstr++ = 0;
	while (isspace(*execstr))
	    ++execstr;
    }
    return (*rv) ? rv : NULL;
} /* getarg */


void
version()
/* version: print which version of levee we are... */
{
    errmsg("levee (c)");prints(codeversion);
} /* version */


void
show_args()
/* show_args: print the argument list */
{
    register int i;
    dgotoxy(0, -1);
    logit("show_args: there %s %d argument%s",
	(args.gl_pathc == 1) ? "is" : "are",
	args.gl_pathc,
	(args.gl_pathc == 1) ? "." : "s.");
    for (i=0; i < args.gl_pathc; i++) {
	if (curpos.x+strlen(args.gl_pathv[i]) >= COLS)
	    exprintln();
	else if (i > 0)
	    printch(' ');
	if (pc == i) d_highlight(1);
	prints(args.gl_pathv[i]);
	if (pc == i) d_highlight(0);
    }
} /* show_args */

void
setcmd()
{
    bool no = NO,b;
#if 0
    int len,i;
#endif
    char *arg, *num;
    struct variable *vp;

    if ( (arg = getarg()) ) {
	do {
	    if (*arg != 0) {
		if ( (num = strchr(arg, '=')) ) {
		    b = NO;
		    *num++ = 0;
		}
		else {				/* set [no]opt */
		    b = YES;
		    if (arg[0]=='n' && arg[1]=='o') {
			arg += 2;
			no = NO;
		    }
		    else
			no = YES;
		}
		for(vp=vars;vp->u && strcmp(arg,vp->v_name)
				  && strcmp(arg,vp->v_abbr); vp++)
		    ;
		if (!vp->u || vp->v_flags & V_CONST) {
		    errmsg("Can't set ");
		    prints(arg);
		}
		else {
		    int j;

		    if (b)
			if (vp->v_tipe == VBOOL)
			    vp->u->valu = no;
			else
			       goto badsettype;
		else if (vp->v_tipe == VSTR) {
		    if (vp->u->strp)
			free(vp->u->strp);
		    vp->u->strp = (*num) ? strdup(num) : NULL;
		}
		else
		    if (*num && (j=atoi(num)) >= 0)
			    vp->u->valu = j;
			else {
		  badsettype:
			    errmsg("bad set type");
			    continue;
			}
		    redraw |= vp->v_flags & V_DISPLAY;
		}
	    }
	} while ( (arg = getarg()) );
    }
    else {
	version(); exprintln();
	for(vp=vars;vp->u;vp++) {
	    if ( vp->v_flags & V_ALIAS )
		continue;
	    switch (vp->v_tipe) {
		case VBOOL:
		    if (!vp->u->valu)
			prints("no");
		    prints(vp->v_name);
		    break;
		case VSTR:
		    if (!vp->u->strp)
			prints("no ");
		    prints(vp->v_name);
		    if (vp->u->strp) {
			dgotoxy(10, -1);
			prints(" = ");
			prints(vp->u->strp);
		    }
		    break;
		default:
		    prints(vp->v_name);
		    dgotoxy(10, -1);
		    prints(" = ");
		    printi(vp->u->valu);
		    break;
	    }
	    exprintln();
	}
    }
} /* setcmd */


/* print a macro */
void
printone(i)
int i;
{
    if (i >= 0) {
	exprintln();
	printch(mbuffer[i].token);
	dgotoxy(3, -1);
	if (movemap[(unsigned int)(mbuffer[i].token)] == INSMACRO)
	    prints("!= ");
	else
	    prints(" = ");
	prints(mbuffer[i].m_text);
    }
} /* printone */


/* print all the macros */
void
printall()
{
    int i;
    for (i = 0; i < MAXMACROS; i++)
	if (mbuffer[i].token != 0)
	    printone(i);
} /* printall */


/* :map ch text */
void
map(insert)
bool insert;
{
    char *macro, c;
    int i;
    		/* get the macro */
    if ((macro=getarg()) == NULL) {
	printall();
	return;
    }
    if (strlen(macro) > 1) {
	errmsg("macros must be one character");
	return;
    }
    c = macro[0];
    if (*execstr == 0)
	printone(lookup(c));
    else {
	if ((i = lookup(0)) < 0)
	    errmsg("macro table full");
	else if (c == ESC || c == ':') {
	    errmsg("can't map ");
	    printch(c);
	}
	else if (*execstr != 0) {
	    undefine(lookup(c));
	    mbuffer[i].token = c;
	    mbuffer[i].m_text = strdup(execstr);
	    mbuffer[i].oldmap = movemap[(unsigned int)c];
	    if (insert)
		movemap[(unsigned int)c] = INSMACRO;
	    else
		movemap[(unsigned int)c] = SOFTMACRO;
	}
    }
} /* map */


void
undefine(i)
int i;
{
    char *p;
    if (i >= 0) {
	movemap[(unsigned int)(mbuffer[i].token)] = mbuffer[i].oldmap;
	mbuffer[i].token = 0;
	p = mbuffer[i].m_text;
	free(p);
	mbuffer[i].m_text = 0;
    }
} /* undefine */


int
unmap()
{
    int i;
    char *arg;

    if ( (arg=getarg()) ) {
	if (strlen(arg) == 1) {
	    undefine(lookup(*arg));
	    return YES;
	}
	if (strcmp(arg,"all") == 0) {
	    for (i = 0; i < MAXMACROS; i++)
		if (mbuffer[i].token != 0)
			undefine(i);
	    return YES;
	}
    }
    return NO;
} /* unmap */


/* return argument # of a filename */
int
findarg(name)
register char *name;
{
    int j;
    for (j = 0; j < args.gl_pathc; j++)
	if (strcmp(args.gl_pathv[j],name) == 0)
	    return j;
    return -1;
} /* findarg */


/* add a filename to the arglist */
int
addarg(name)
register char *name;
{
    int where;
    int rc;

    if ((where = findarg(name)) >= 0 )
	return where;

    rc = os_glob(name, GLOB_APPEND|GLOB_NOMAGIC, &args);
    return rc ? F_UNSET : args.gl_pathc-1;
} /* addarg */


/* get a file name argument (substitute alt file for #) */
char *
getname()
{
    register char *name;

    if ( (name = getarg()) ) {
	if ( 0 == strcmp(name,"#") ) {
	    if ( altnm == F_UNSET ) {
		errmsg("no alt name");
		return NULL;
	    }
	    else
		return args.gl_pathv[altnm];
	}
    }
    return name;
} /* getname */


/* CAUTION: these make exec not quite recursive */
int  high,low;		/* low && high end of command range */
bool affirm;		/* cmd! */
/* s/[src]/dst[/options] */
/* s& */
void
cutandpaste()
{
    bool askme  = NO,
	 printme= NO,
	 glob   = NO;
    int newcurr = -1;
    int oldcurr = curr;
    int  num;
    char delim;
    register char *ip;
    register char *dp;

    zerostack(&undo);
    ip = execstr;
    if (*ip != '&') {
	delim = *ip;
	ip = makepat(1+ip,delim);			/* get search */
	if (ip == NULL)
	    goto splat;
	dp = dst;
	while (*ip && *ip != delim) {
	    if (*ip == '\\' && ip[1] != 0)
		*dp++ = *ip++;
	    *dp++ = *ip++;
	}
	*dp = 0;
	if (*ip == delim) {
	    while (*++ip)
		switch (*ip) {
		    case 'q':
		    case 'c': askme = YES;	break;
		    case 'g': glob = YES;	break;
		    case 'p': printme= YES;	break;
		}
	}
    }
    if (*lastpatt == 0) {
splat:	errmsg("bad substitute");
	return;
    }
    fixupline(bseekeol(curr));
    num = 0;
    do {
	low = chop(low, &high, NO, &askme);
	if (low > -1) {
	    redraw = YES;
	    num++;
	    if (printme) {
		exprintln();
		writeline(-1,-1,bseekeol(low));
	    }
	    if (newcurr < 0)
		newcurr = low;
	    if (!glob)
		low = 1+fseekeol(low);
	}
    } while (low >= 0);
    if (num > 0) {
	exprintln();
	plural(num," substitution");
    }
    fixupline((newcurr > -1) ? newcurr : oldcurr);
} /* cutandpaste */


/* quietly read in a file (and mark it in the undo stack)
 */
int
insertfile(FILE *f, int insert, int at, int *fsize)
{
    int high,
	onright,
	rc=0;

    onright = (bufmax-at);
    high = EDITSIZE-onright;

    if ( insert && (onright > 0) )
	 moveright(&core[at], &core[high], onright);

    rc = addfile(f, at, high, fsize);

    if ( (rc == 0) && (*fsize < 0) ) {
	rc = -1;
	*fsize=0;
    }
    if ( insert ) {
	if ( *fsize ) 
	    insert_to_undo(&undo, at, *fsize);
	modified = YES;
	if (onright > 0)
	    moveleft(&core[high], &core[at+(*fsize)], onright);
    }
    redraw = YES;
    return rc;
} /* insertfile */



void
inputf(fname, newbuf)
register char *fname;
bool newbuf;
{
    FILE *f;
    int fsize,		/* bytes read in */
	rc;

    if (newbuf)
	readonly = NO;

    zerostack(&undo);

    if ( newbuf ) {
	modified = NO;
	low = 0;
    }
    else {
	fixupline(bseekeol(curr));
    }

    printch('"');
    prints(fname);
    prints("\" ");
    if ((f=expandfopen(fname, "r")) == NULL) {
	prints("[New file]");
	fsize = 0;
	if (newbuf)
	    newfile = YES;
	redraw = TRUE;
    }
    else {
	rc = insertfile(f, !newbuf, low, &fsize);
	fclose(f);

	if ( rc > 0 )
	    plural(fsize, " byte");
	else if ( rc < 0 )
	    prints("[read error]");
	else {
	    prints("[overflow]");
	    readonly=1;
	}
	if (newbuf) newfile = NO;
    }
    if (newbuf) {
	fillchar(contexts, sizeof(contexts), -1);
	bufmax = fsize;
    }
    if (startcmd) {
	count = 1;
	if (*findparse(startcmd,&curr,low) != 0 || curr < 0)
	    curr = low;
	startcmd = 0;
    }
    else
	curr = low;
} /* inputf */


/* Change a file's name (for autocopy). */
void
backup(name)
char *name;
{
    char *back, *expanded;
    int ok = NO;

    if ( expanded = os_tilde(name) )
	name = expanded;

    if ( back = os_backupname(name) ) {
	os_unlink(back);
	ok = os_rename(name, back) == 0;
	free(back);
    }

    unless (ok) {
	prints(" (");
	prints(strerror(errno));
	prints(")");
    }

    if ( expanded )
	free(expanded);
} /* backup */


bool
outputf(fname)
char *fname;
{
    bool whole;
    FILE *f;
    int status;
    zerostack(&undo);		/* undo doesn't survive past write */
    if (high < 0)
	high = (low < 0) ? (bufmax-1) : (fseekeol(low));
    if (low < 0)
	low  = 0;

    high++;
    printch('"');
    prints(fname);
    prints("\" ");
    whole = (low == 0 && high >= (bufmax-1));
    if (whole && autocopy)
	backup(fname);
    if ( (f=expandfopen(fname, "w")) ) {
	status = putfile(f, low, high);
	fclose(f);
	if (status) {
	    plural(high-low," byte");
	    if (whole)
		modified = NO;
	    return(YES);
	}
	else {
	    prints("[write error]");
	    os_unlink(fname);
	}
    }
    else
	prints(fisro);
    return(NO);
} /* outputf */


int
oktoedit(writeold)
/* check and see if it is ok to edit a new file */
int writeold;	/* automatically write out changes? */
{
    if (modified && !affirm) {
	if (readonly) {
	    errmsg(fisro);
	    return NO;
	}
	else if (writeold && (filenm != F_UNSET) ) {
	    if (!outputf(args.gl_pathv[filenm]))
		return NO;
	    printch(',');
	}
	else {
	    errmsg(fismod);
	    return NO;
	}
    }
    return YES;
} /* oktoedit */


/* write out all || part of a file */
bool
writefile()
{
    char *name;
    int fileptr = filenm;

    unless (name=getname())
	name = (filenm == F_UNSET) ? NULL : args.gl_pathv[filenm];

    unless (name) {
	errmsg("no file to write");
	return NO;
    }

    unless (outputf(name))
	return NO;

    if ( (filenm == F_UNSET) && ((fileptr=addarg(name)) == F_UNSET) )
	errmsg("error expanding argument list!");

    if ( fileptr != F_UNSET ) {
	altnm = filenm;
	filenm = fileptr;
    }

    return YES;
}


void
editfile()
{
    char *name = NULL;	/* file to edit */
    int newpc = F_UNSET;

    if ((name = getname()) && *name == '+') {
	startcmd = name[1] ? 1+name : "$";
	name = getname();
    }

    if ( name == NULL ) {
	logit(":edit with no args: pc=%d", pc);
	if ( pc == F_UNSET ) {
	    errmsg("Nothing to edit");
	    return;
	}
	newpc = pc;
    }
    else {
	logit(":edit %s", name);
	if ((newpc = addarg(name)) == F_UNSET) {
	    errmsg("file allocation error");
	    return;
	}
#if LOOSELY_COMPATABLE
	/* copy the rest of the files into the arglist
	 */
	while (name = getname())
	    addarg(name);
#endif
    }

    if ( oktoedit(NO) )
	doinput(newpc);
}


void
doinput(fileptr)
int fileptr;
{
    inputf(args.gl_pathv[fileptr], YES);
    if ( fileptr != filenm ) {
	altnm = filenm;
	filenm = fileptr;
    }
}


void
toedit(count)
int count;
{
    if (count > 1) {
	printi(count);
	prints(" files to edit; ");
    }
}


void
readfile()
{
    char *name;

    if ( (name=getarg()) )
	inputf(name,NO);
    else
	errmsg("no file to read");
}


void
nextfile(prev)
bool prev;
{
    char *name = getarg();
    int rc, i;

    if ( prev || (name && (strcmp(name, "-") == 0)) ) {
	/* :n - ; move back, ignore other arguments
	 */
	if ( pc <= 0 ) {
	    errmsg("(no prev files)");
	    return;
	}
	else if ( oktoedit(autowrite) )
	    --pc;
    }
    else if ( name == 0 ) {
	/* :n w/o argument; move forward
	 */
	if ( pc+1 >= args.gl_pathc ) {	/* beware: gl_pathc is unsigned */
	    errmsg("(no more files)");
	    return;
	}
	else if ( oktoedit(autowrite) )
	    pc++;
    }
    else {
	/* :n file {...}
	 */
	glob_t files;

	unless (oktoedit(autowrite))
	    return;

	memset(&files, 0, sizeof files);

	do {
	    if (rc = os_glob(name, GLOB_APPEND|GLOB_NOMAGIC, &files)) {
		errmsg("memory allocation error");
		os_globfree(&files);
		return;
	    }
	} while ( name = getarg() );

	if ( (files.gl_pathc == 1) && (pc = findarg(files.gl_pathv[0])) >= 0) {

	    /* jump to existing file */
	    ;
	}
	else {
	    /* toss the old args; s-l-o-w-l-y copy the new ones in */
	    pc = 0;
	    os_globfree(&args);
	    for ( i=0; i < files.gl_pathc; i++ ) {
		rc = os_glob(files.gl_pathv[i], GLOB_APPEND|GLOB_NOMAGIC, &args);
		if ( rc ) {
		    errmsg("(memory allocation error)");
		    break;
		}
	    }
	}
	if ( files.gl_pathc > 0 )
	    os_globfree(&files);
    }

    if ( pc >= 0 && pc < args.gl_pathc )
	doinput(pc);
    else {
	/* if we got here and there was an error, we are screwed
	 * and need to go back to an empty screen
	 */
	newfile = YES;
    }
}


/*
 * set up low, high; set dot to low
 */
void
fixupline(dft)
int dft;
{
    if (low < 0)
	low = dft;
    if (high < 0)
	high = fseekeol(low)+1;
    else if (high < low) {		/* flip high & low */
	int tmp;
	tmp = high;
	high = low;
	low = tmp;
    }
    if (low >= ptop && low < pend) {
	setpos(skipws(low));
	yp = setY(curr);
    }
    else {
	curr = low;
	redraw = YES;
    }
}


void
whatline()
{
    printi(to_line((low < 0) ? (bufmax-1) : low));
    if (high >= 0) {
	printch(',');
	printi(to_line(high));
    }
}


void
print()
{
    do {
	exprintln();
	writeline(-1, 0, low);
	low = fseekeol(low) + 1;
    } while (low < high);
    exprintln();
}

/* move to different line */
/* execute lines from a :sourced || .lvrc file */


bool
do_file(fname,mode,noquit)
char *fname;
exec_type *mode;
bool *noquit;
{
    char line[120];
    FILE *fp;

    if ((fp = expandfopen(fname,"r")) != NULL) {
	indirect = YES;
	while (fgets(line,120,fp) && indirect) {
	    strtok(line, "\n");
	    if (*line != 0)
		exec(line,mode,noquit);
	}
	indirect = YES;
	fclose(fp);
	return YES;
    }
    return NO;
}


void
doins(flag)
bool flag;
{
    int i;
    curr = low;
    exprintln();
    low = insertion(1,setstep[flag],&i,&i,NO)-1;
    if (low >= 0)
	curr = low;
    redraw = YES;
}


/* figure out a address range for a command */
char *
findbounds(ip)
char *ip;
{
    ip = findparse(ip, &low, curr);	/* get the low address */
    if (low >= 0) {
	low = bseekeol(low);		/* at start of line */
	if (*ip == ',') {		/* high address? */
	    ip++;
	    count = 0;
	    ip = findparse(ip, &high, curr);
	    if (high >= 0) {
		high = fseekeol(high);
		return(ip);
	    }
	}
	else
	    return(ip);
    }
    return(0);
}


/* parse the command line for lineranges && a command */
int
parse(inp)
char *inp;
{
    int j,k;
    char cmd[80];
    low = high = ERR;
    affirm = 0;
    if (*inp == '%') {
	moveright(inp, 2+inp, 1+strlen(inp));
	inp[0]='1';
	inp[1]=',';
	inp[2]='$';
    }
    while (isspace(*inp))
	++inp;
    if (strchr(".$-+0123456789?/`'", *inp))
	if (!(inp=findbounds(inp))) {
	    errmsg("bad address");
	    return ERR;
	}
    while (isspace(*inp))
	++inp;
    j = 0;
    while (isalpha(*inp))
	cmd[j++] = *inp++;
    if (*inp == '!') {
	if (j == 0)
	    cmd[j++] = '!';
	else
	    affirm++;
	inp++;
    }
    else if (*inp == '=' && j == 0)
	cmd[j++] = '=';
    while (isspace(*inp))
	++inp;
    execstr = inp;
    if (j==0)
	return EX_CR;
    for (k=0; excmds[k].name; k++)
	if (excmds[k].active && strncmp(cmd, excmds[k].name, j) == 0)
	    return k;
    return ERR;
}


/* inner loop of execmode */
void
exec(cmd, mode, noquit)
char *cmd;
exec_type *mode;
bool *noquit;
{
    int  what;
    bool ok;

    what = parse(cmd);
    ok = YES;
    if (redraw) {
	lstart = bseekeol(curr);
	lend = fseekeol(curr);
    }
    switch (what) {
	case EX_QUIT:				/* :quit */
	    if (affirm || what == lastexec || !modified)
		*noquit = NO;
	    else
		errmsg(fismod);
	    break;
	case EX_READ:				/* :read */
	    clrmsg();
	    readfile();
	    break;
	case EX_EDIT:				/* :read, :edit */
	    clrmsg();
	    editfile();
	    break;
	case EX_WRITE:
	case EX_WQ :				/* :write, :wq */
	    clrmsg();
	    if (readonly && !affirm)
		prints(fisro);
	    else if (writefile() && what==EX_WQ)
		*noquit = NO;
	    break;
	case EX_PREV:
	case EX_NEXT:				/* :next */
	    clrmsg();
	    nextfile(what==EX_PREV);
	    break;
	case EX_SUBS:				/* :substitute */
	    cutandpaste();
	    break;
	case EX_SOURCE:				/* :source */
	    if ((cmd = getarg()) && !do_file(cmd, mode, noquit)) {
		errmsg("cannot open ");
		prints(cmd);
	    }
	    break;
	case EX_XIT:
	    clrmsg();
	    if (modified) {
		if (readonly) {
		    prints(fisro);
		    break;
		}
		else if (!writefile())
		    break;
	    }

	    if (!affirm && (args.gl_pathc-pc > 1)) {
	    	/* more files to edit? */
		printch('(');
		plural(args.gl_pathc-pc-1," more file");
		prints(" to edit)");
	    }
	    else
		*noquit = NO;
	    break;
	case EX_MAP:
	    map(affirm);
	    break;
	case EX_UNMAP:
	    ok = unmap();
	    break;
	case EX_FILE:				/* :file */
	    if ( (cmd=getarg()) ) {		/* :file name */
		int new_pc;
		if ( (new_pc = addarg(cmd)) != F_UNSET ) {
		    altnm = filenm;
		    filenm = pc = new_pc;
		}
	    }
	    wr_stat();
	    break;
	case EX_SET:				/* :set */
	    setcmd();
	    break;
	case EX_CR:
	case EX_PR:				/* :print */
	    fixupline(bseekeol(curr));
	    if (what == EX_PR)
		print();
	    break;
	case EX_LINE:				/* := */
	    whatline();
	    break;
	case EX_DELETE:
	case EX_YANK:				/* :delete, :yank */
	    yank.lines = YES;
	    fixupline(lstart);
	    zerostack(&undo);
	    if (what == EX_DELETE)
		ok = deletion(low,high);
	    else
		ok = doyank(low,high);
	    redraw = YES;
	    break;
	case EX_PUT:				/* :put */
	    fixupline(lstart);
	    zerostack(&undo);
	    ok = putback(low, &high);
	    redraw = YES;
	    break;
	case EX_VI:				/* :visual */
	    *mode = E_VISUAL;
	    if (*execstr) {
		clrmsg();
		nextfile(NO);
	    }
	    break;
	case EX_EX:
	    *mode = E_EDIT;			/* :execmode */
	    break;
	case EX_INSERT:
	case EX_OPEN:				/* :insert, :open */
	    if (indirect)
		ok = NO;		/* kludge, kludge, kludge!!!!!!!!!! */
	    else {
		zerostack(&undo);
		fixupline(lstart);
		doins(what == EX_OPEN);
	    }
	    break;
	case EX_CHANGE:				/* :change */
	    if (indirect)
		ok = NO;		/* kludge, kludge, kludge!!!!!!!!!! */
	    else {
		zerostack(&undo);
		yank.lines = YES;
		fixupline(lstart);
		if (deletion(low,high))
		    doins(YES);
		else
		    ok = NO;
	    }
	    break;
	case EX_UNDO:				/* :undo */
	    low = fixcore(&high);
	    if (low >= 0) {
		redraw = YES;
		curr = low;
	    }
	    else ok = NO;
	    break;
	case EX_ARGS:				/* :args */
	    show_args();
	    break;
	case EX_VERSION:			/* version */
	    version();
	    break;
	case EX_ESCAPE:			/* shell escape hack */
	    zotscreen = YES;
	    exprintln();
	    if (*execstr) {
		reset_input();
		os_subshell(execstr);
		set_input();
	    }
	    else
		prints("incomplete shell escape.");
	    break;
	case EX_REWIND:
	    clrmsg();
	    if ( (pc > 0) && (args.gl_pathc > 0) && oktoedit(autowrite) )
		doinput(pc=0);
	    break;
	default:
	    prints(":not an editor command.");
	    break;
    }
    lastexec = what;
    if (!ok) {
	errmsg(excmds[what].name);
	prints(" error");
    }
} /* exec */

