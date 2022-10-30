#include "levee.h"
#include "extern.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#if HAVE_ERRNO_H
#include <errno.h>
#endif

void undefine(int i);
void fixupline(int dft);
void doinput(int fileptr);

/* CAUTION: these make exec not quite recursive */
int  high,low;		/* low && high end of command range */
bool affirm;		/* cmd! */

static char noalloc[] = " (out of memory)";


/*
 * do a newline and set flags.
 */
#define exprintln()	(zotscreen=YES),println()

void
plural(int num, char *string)
{
    printi(num);
    prints(string);
    if (num != 1)
	printch('s');
} /* plural */


void
clrmsg(void)
{
    dgotoxy(0, -1);
    dclear_to_eol();
} /* clrmsg */


void
errmsg(char *msg)
{
    dgotoxy(0, -1);
    prints(msg);
    dclear_to_eol();
} /* errmsg */


/* get a space-delimited token */

static char *execstr;

void
setarg(char *s)
{
    execstr = s;
}

static
char * getarg(void)
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
version(void)
/* version: print which version of levee we are... */
{
    errmsg("levee (c)");prints(codeversion);
} /* version */


/* figure out a address range for a command */
char *
findbounds(char *ip)
{
    /* get the low address */
    logit(("findbounds: low (%d,&[%s],0)", curr, ip));
    if ( (low = findparse(curr, &ip, 0)) > ERR )
	low = bseekeol(low);		/* at start of line */
    else if (low != ERR_UNKNOWN) {
	prints(expr_errstring(low));
	return 0;
    }

    if (*ip == ',') {		/* high address? */
	int offset = 0;

	ip++;

	if ( *ip == '/' || *ip == '+' )
	    offset=1;
	else if ( *ip == '?' || *ip == '-' )
	    offset=-1;

	logit(("findbound: high (%d, &[%s], %d", low, ip, offset));

	unless ( (high = findparse(curr, &ip, offset)) > ERR ) {
	    prints(expr_errstring(high));
	    return 0;
	}
	high = fseekeol(high);
    }
    logit(("findbounds: low=%d, high=%d", low, high));
    return(ip);
}


void
show_args(void)
/* show_args: print the argument list */
{
    register int i;
    dgotoxy(0, -1);
    logit(("show_args: there %s %d argument%s",
	(args.gl_pathc == 1) ? "is" : "are",
	args.gl_pathc,
	(args.gl_pathc == 1) ? "." : "s."));
    for (i=0; i < args.gl_pathc; i++) {
	if (curpos.x+strlen(args.gl_pathv[i]) >= COLS)
	    exprintln();
	else if (i > 0)
	    printch(' ');
	if (filenm == i) d_highlight(1);
	prints(args.gl_pathv[i]);
	if (filenm == i) d_highlight(0);
    }
} /* show_args */

void
setcmd(void)
{
    bool no = NO,b;
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
printone(int i)
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
printall(void)
{
    int i;
    for (i = 0; i < MAXMACROS; i++)
	if (mbuffer[i].token != 0)
	    printone(i);
} /* printall */


/* :map ch text */
void
map(bool insert)
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
undefine(int i)
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
unmap(void)
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
findarg(register char *name)
{
    int j;
    for (j = 0; j < args.gl_pathc; j++)
	if (strcmp(args.gl_pathv[j],name) == 0)
	    return j;
    return -1;
} /* findarg */


/* add a filename to the arglist */
int
addarg(register char *name)
{
    int where;
    int rc;

    if ((where = findarg(name)) >= 0 )
	return where;

    rc = os_glob(name, GLOB_APPEND|GLOB_NOMAGIC, &args);
    return rc ? F_UNSET : args.gl_pathc-1;
} /* addarg */


/* s/[src]/dst[/options] */
/* s& */
void
cutandpaste(void)
{
    bool askme  = NO,
	 printme= NO,
	 glob   = NO;
    int newcurr = -1;
    int oldcurr = curr;
    int  num;
    char delim;
    int  lastlow;
    register char *ip;
    register char *dp;

    zerostack(&undo);
    logit(("cutandpaste: [%s]", execstr));
    ip = execstr;
    if (*ip != '&') {
	delim = *ip++;
	unless ( ip = makepat(ip,delim) )	/* get search */
	    goto splat;

	dp = dst;
	while (*ip && *ip != delim) {
	    if (*ip == '\\' && ip[1] != 0)
		*dp++ = *ip++;
	    *dp++ = *ip++;
	}
	*dp = 0;

	logit(("cutandpaste: replacement = [%s]", dst));

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
    lastlow = 0;
    num = 0;
    do {
	unless ( (low = chop(low, &high, NO, &askme)) > ERR )
	    break;

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

	if ( low < lastlow ) {
	    logit(("low went backwards: lastlow=%d, low=%d", lastlow, low));
	    errmsg("low went backwards?");
	    break;
	}
	lastlow = low;
    } while (low > ERR);
    if (num > 0) {
	exprintln();
	plural(num," substitution");
    }
    curr = (newcurr > -1) ? newcurr : oldcurr;
    //fixupline((newcurr > -1) ? newcurr : oldcurr);
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
inputf(register char *fname, bool newbuf)
{
    FILE *f;
    int fsize,		/* bytes read in */
	rc;

    if (newbuf)
	readonly = is_viewer;

    zerostack(&undo);

    if ( newbuf ) {
	modified = NO;
	low = 0;
    }
    else
	fixupline(bseekeol(curr));

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
	    readonly=YES;
	}
	if (newbuf) newfile = NO;
    }
    if (newbuf) {
	fillchar(contexts, sizeof(contexts), -1);
	bufmax = fsize;
    }
    if (startcmd) {
	int addr;

	if ( (addr = findparse(low, &startcmd, 0)) > ERR ) {
	    if (*startcmd == 0 || *startcmd == ';' )
		curr = addr;
	}
	else {
	    prints(" [");
	    prints(expr_errstring(addr));
	    printch(']');
	}
	startcmd = 0;
    }
    else
	curr = low;
} /* inputf */


/* Change a file's name (for autocopy). */
void
backup(char *name)
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
outputf(char *fname,int entire_file)
{
    bool whole;
    FILE *f;
    int status;
    zerostack(&undo);		/* undo doesn't survive past write */
    if ( entire_file ) {
	high = bufmax-1;
	low = 0;
    }
    else  {
	if (high < 0)
	    high = (low < 0) ? (bufmax-1) : (fseekeol(low));
	if (low < 0)
	    low  = 0;
    }

    logit(("outputf: entire_file = %d, low=%d, high=%d", entire_file, low, high));

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
oktoedit(int writeold)
/* check and see if it is ok to edit a new file */
/* automatically write out changes? */
{
    unless (modified)			/* no modifications?  Yes. */			/* otherwise?  Yes. */
	return YES;

    if (readonly || is_viewer) {	/* readonly or in viewer mode? No. */
	errmsg(fisro);
	return NO;
    }

    if (affirm)				/* :{cmd}! ? Yes. */
	return YES;

					/* no autowrite or filename?  No. */
    unless (writeold && (filenm != F_UNSET) ) {
	errmsg(fismod);
	return NO;
    }
    unless (outputf(args.gl_pathv[filenm], YES))
	return NO;			/* write error?  No. */
    prints(", ");
    return YES;				/* otherwise?  Yes. */
} /* oktoedit */


/* write out all || part of a file */
bool
writefile(void)
{
    char *name;
    int fileptr = filenm;

    unless (name=getarg())
	name = (filenm == F_UNSET) ? NULL : args.gl_pathv[filenm];

    unless (name) {
	errmsg("no file to write");
	return NO;
    }

    unless (outputf(name, NO))
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
editfile(void)
{
    char *name;	/* file to edit */
    int newpc = F_UNSET;

    if ((name = getarg()) && *name == '+') {
	startcmd = name[1] ? 1+name : "$";
	logit(("editfile: startcmd=%s",startcmd));
	name = getarg();
    }

    if ( name ) {
	logit((":edit %s", name));
	if ((newpc = addarg(name)) == F_UNSET) {
	    errmsg(noalloc);
	    return;
	}
#if LOOSELY_COMPATABLE
	/* copy the rest of the files into the arglist
	 */
	while (name = getarg())
	    addarg(name);
#endif
    }
    else {
	logit((":edit with no args: filenm=%d", filenm));
	if ( filenm == F_UNSET ) {
	    errmsg("Nothing to edit");
	    return;
	}
	newpc = filenm;
    }

    if ( oktoedit(NO) )
	doinput(newpc);
}


void
dotag(void)
{
    char *tag;
    Tag result;
    int fileptr;
    unless (oktoedit(autowrite))
	return;

    unless (tag = getarg()) {
	errmsg("No tag");
	return;
    }
    logit(("dotag: tag %s", tag));
    unless ( find_tag(tag, strlen(tag), &result) ) {
	errmsg("Can't find tag");
	return;
    }
    logit(("dotag: filename=%s, pattern=%s", result.filename, result.pattern));

    push_tag(filenm, curr);
    clrmsg();

    fileptr = addarg(result.filename);

    if ( gototag(fileptr, result.pattern) == SAMEFILE )
	fixupline(bseekeol(curr));
}


void
poptag(void)
{
    Camefrom *loc = pop_tag();

    unless (loc) {
	errmsg("empty tag stack");
	return;
    }

    if ( loc->fileno == filenm ) {
	curr = loc->cursor;
	redraw = YES;
	return;
    }

    unless ( oktoedit(autowrite) )
	return;

    clrprompt();
    inputf(args.gl_pathv[loc->fileno], YES);

    curr = Min(loc->cursor, bufmax-1);
    redraw = YES;
}


void
doinput(int fileptr)
{
    inputf(args.gl_pathv[fileptr], YES);

    if ( fileptr != filenm ) {
	altnm = filenm;
	filenm = fileptr;
    }
}


void
toedit(int count)
{
    if (count > 1) {
	printi(count);
	prints(" files to edit; ");
    }
}


void
readfile(void)
{
    char *name;

    if ( *execstr == '!' ) {
	int size=0;
	FILE *f;
	os_pid_t child;

	if ( *++execstr ) {
	    if ( (f=os_cmdopen(execstr, NULL, &child)) ) {
		insertfile(f, 1, curr, &size);
		os_cmdclose(f, child);
	    }
	}
	else
	    errmsg("usage :r[ead] !cmd");
    }
    else if ( (name=getarg()) )
	inputf(name,NO);
    else
	errmsg("no file to read");
}


void
nextfile(bool prev)
{
    char *name = getarg();
    int rc, i, current;

    current = filenm;

    if ( prev || (name && (strcmp(name, "-") == 0)) ) {
	/* :n - ; move back, ignore other arguments
	 */
	if ( current <= 0 ) {
	    errmsg("(no prev files)");
	    return;
	}
	unless ( oktoedit(autowrite) )
	    return;
	--current;
    }
    else if ( name == 0 ) {
	/* :n w/o argument; move forward
	 */
	if ( current+1 >= args.gl_pathc ) {/* beware: gl_pathc is unsigned */
	    errmsg("(no more files)");
	    return;
	}
	unless ( oktoedit(autowrite) )
	    return;
	current++;
    }
    else {
	/* :n file {...}
	 */
	glob_t files;

	unless (oktoedit(autowrite))
	    return;

	fillchar(&files, sizeof files, 0);

	do {
	    if (rc = os_glob(name, GLOB_APPEND|GLOB_NOMAGIC, &files)) {
		errmsg(noalloc);
		os_globfree(&files);
		return;
	    }
	} while ( name = getarg() );

	if ( (files.gl_pathc == 1) && (i = findarg(files.gl_pathv[0])) >= 0) {

	    /* jump to existing file */
	    ;
	}
	else {
	    /* toss the old args; s-l-o-w-l-y copy the new ones in */
	    os_globfree(&args);
	    for ( i=0; i < files.gl_pathc; i++ ) {
		rc = os_glob(files.gl_pathv[i], GLOB_APPEND|GLOB_NOMAGIC, &args);
		if ( rc ) {
		    errmsg(noalloc);
		    break;
		}
	    }
	    current = 0;
	}
	if ( files.gl_pathc > 0 )
	    os_globfree(&files);
    }

    if ( current != F_UNSET )
	doinput(current);
    else {
	/* if we got here and there was an error, we are screwed
	 * and need to go back to an empty screen
	 */
	newfile = YES;
    }
}


/*
 * set up low, high; set dot to high
 */
void
fixupline(int dft)
{
    int newpos;

    if (low < 0)
	low = dft;
    if (high < 0)
	high = fseekeol(low);
    else if (high < low) {		/* flip high & low */
	int tmp;
	tmp = high;
	high = low;
	low = tmp;
    }
    newpos = (high<=0)?low:high;
    //if (low >= ptop && low < pend) {
    if (newpos >= ptop && newpos < pend) {
	setpos(skipws(newpos));
	yp = setY(curr);
    }
    else {
	curr = newpos;
	redraw = YES;
    }
}


void
whatline(void)
{
    printi(to_line((low < 0) ? (bufmax-1) : low));
    if (high >= 0) {
	printch(',');
	printi(to_line(high));
    }
}


void
print(void)
{
    goto justwrite;
    do {
	exprintln();
    justwrite:
	writeline(-1, 0, low);
	low = fseekeol(low) + 1;
    } while (low < high);
    exprintln();
}

/* move to different line */
/* execute lines from a :sourced || .lvrc file */


bool
do_file(char *fname,exec_type *mode)
{
    char line[120];
    FILE *fp;

    if ((fp = expandfopen(fname,"r")) != NULL) {
	++indirect;
	while (fgets(line,120,fp) && indirect) {
	    strtok(line, "\n");
	    logit(("do_file: exec [%s]", line));
	    if (*line && exec(line,mode) )
		break;
	}
	fclose(fp);
	if ( indirect )
	    --indirect;
	return YES;
    }
    return NO;
}


void
doins(bool flag)
{
    int i;
    curr = low;
    exprintln();
    low = insertion(1,setstep[flag],&i,&i,NO)-1;
    if (low >= 0)
	curr = low;
    redraw = YES;
}

/* parse the command line for lineranges && a command */
static int
parse(char **inp, char *default_cmd)
{
    int j,k;
    char *cmd, *token;

    token = *inp;

    low = high = ERR;
    affirm = 0;

    while (isspace(*token))
	++token;

    unless (*token)
	token = default_cmd;

    if (*token == '%') {
	findbounds("1,$");
	++token;
    }
    else if ( (token=findbounds(token)) == 0 )
	return ERR;

    while (isspace(*token))
	++token;

    if ( *token == 0 )
	return EX_CR;
    else if ( *token == '"' )
	return (low > ERR) ? EX_PR : EX_COMMENT;

    cmd = token;
    j = 0;
    while (isalpha(*token)) {
	token++;
	j++;
    }

    if (*token == '!') {
	if ( token == cmd )
	    j++;	/* ! command */
	else
	    affirm++;	/* otherwise a emphatic command (e!, for instance) */
	token++;
    }
    else if (*token == '=' && j == 0) {
	++token;
	++j;
    }

    while (isspace(*token))
	++token;

    (*inp) = token;

    if (j==0)
	return ERR;

    logit(("parse: cmd is [%.*s]", j, cmd));
    for (k=0; excmds[k].name; k++)
	if (excmds[k].active && strncmp(cmd, excmds[k].name, j) == 0)
	    return k;
    return EX_UNKNOWN;
}


/* expand '%' and '#' into gl_pathv[filenm] & gl_pathv[altnm] */
int expand_err;

#define EXP_OK 0	/* all % & #'s expanded */
#define EXP_ALT 1	/* tried to expand # w/o altname */
#define EXP_FILE 2	/* tried to expand % w/o filename */
#define EXP_MEM 3	/* ran out of memory during expansion */

char *
expand_line(char *cmd)
{
    static char *expanded = 0;
    static int sz_expanded = 0;
    char *new, *r, *cp;

    char *fileptr = (filenm == F_UNSET) ? 0 : args.gl_pathv[filenm];
    char *altptr  = (altnm == F_UNSET) ? 0 : args.gl_pathv[altnm];
    int count_percent = 0,
	count_hash = 0,
	sz_fileptr = fileptr ? strlen(fileptr) : 0,
	sz_altptr  = altptr ? strlen(altptr) : 0,
	size = strlen(cmd)+1;

    /* first scan across the command line counting up #'s and %'s */
    for (r = cmd; r < cmd + size; r++ ) {
	if ( *r == '%' )
	    count_percent++;
	else if ( *r == '#' )
	    count_hash++;
    }

    expand_err = EXP_OK;

    /* if the cmdline doesn't need to be expanded just return it */
    unless (count_percent+count_hash)
	return cmd;

    if ( count_percent && !fileptr ) {
	expand_err = EXP_FILE;
	return 0;
    }

    if ( count_hash && !altptr ) {
	expand_err = EXP_ALT;
	return 0;
    }

    /* find the needed size for the expanded filename and,
     * if needed, expand the buffer to fit it
     */
    size += (count_percent*sz_fileptr) + (count_hash*sz_altptr);

    logit(("expand_line: needed size = %d", size));

    if ( size > sz_expanded ) {
	if ( expanded )
	    new = realloc(expanded, size);
	else
	    new = malloc(size);

	unless (new) {
	    expand_err = EXP_MEM;
	    return 0;
	}

	expanded = new;
	sz_expanded = size;
    }


    /* replace all %'s & #'s */
    for (cp = cmd, r = expanded; *cp; ++cp ) {
	if ( *cp == '%' ) {
	    memcpy(r, fileptr, sz_fileptr);
	    r += sz_fileptr;
	}
	else if ( *cp == '#' ) {
	    memcpy(r, altptr, sz_altptr);
	    r += sz_altptr;
	}
	else
	    *r++ = *cp;
    }
    *r = 0;
    logit(("expand_line: expanded = %s", expanded));

    return expanded;
}

 /* inner loop of execmode: returns TRUE if it expects to do more
  * editing, FALSE if it received a quit command.
  */
int
exec(char *cmd,exec_type *mode)
{
    int  what;
    int  exit_now = NO;
    bool ok = YES;
    char *expanded;

    what = parse(&cmd, (*mode==E_EDIT) ? "+1" : "");

    if ( what == EX_UNKNOWN || what <= ERR ) {
	if ( what == EX_UNKNOWN )
	    errmsg("Not an editor command.");
	return NO;
    }

    if ( excmds[what].expand ) {
	unless (expanded = expand_line(cmd)) {
	    switch ( expand_err ) {
	    default:    /* if all else fails, blame Canada^Wmalloc */
			errmsg("Out of memory");
			return NO;
	    case EXP_ALT:
			errmsg("No alternate file for #");
			return NO;
	    case EXP_FILE:
			errmsg("No file for %");
			return NO;
	    }
	}
	setarg(expanded);
    }
    else
	setarg(cmd);

    if ( what != EX_CR && (*mode) == E_EDIT )
	exprintln();

    switch (what) {
	case EX_QUIT:				/* :quit */
	    if (affirm || what == lastexec || !modified)
		exit_now = YES;
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
	    if (is_viewer || (readonly && !affirm) )
		prints(fisro);
	    else if (writefile())
		exit_now = (what == EX_WQ);
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
	    if ((cmd = getarg()) && !do_file(cmd, mode)) {
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

	    if (!affirm && (args.gl_pathc-filenm > 1)) {
	    	/* more files to edit? */
		printch('(');
		plural(args.gl_pathc-filenm-1," more file");
		prints(" to edit)");
	    }
	    else
		exit_now = YES;
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
		    filenm = new_pc;
		}
	    }
	    wr_stat();
	    break;
	case EX_SET:				/* :set */
	    setcmd();
	    break;
	case EX_CR:
	case EX_PR:				/* :print */
	    //fixupline(bseekeol(curr));
	    fixupline(curr);
	    if ((*mode) == E_EDIT || what == EX_PR)
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
	    redraw = YES;
	    lstart = bseekeol(curr);
	    lend = fseekeol(curr);
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
	    if ( (filenm > 0) && (args.gl_pathc > 0) && oktoedit(autowrite) )
		doinput(0);
	    break;
	case EX_TAG:
	    dotag();
	    break;
	case EX_POPTAG:
	    poptag();
	    break;
    }
    lastexec = what;
    if (!ok) {
	errmsg(excmds[what].name);
	prints(" error");
    }
    return (exit_now) ? YES : NO;
} /* exec */
