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

/*
 * os2 (and bound) interface for levee (Borland c++)
 */
#include "levee.h"
#include "extern.h"

#if OS_OS2

#include <signal.h>

#define INCL_DOSFILEMGR
#include <os2.h>
#include <ctype.h>


os_dwrite(s,len)
char *s;
{
    return 0;
}


os_initialize()
{
    Erasechar = '\b';	/* ^H */
    Eraseline = 21;	/* ^U */

    return 1;
}


/* get a key, mapping certain control sequences
 */

getKey()
{
    register c;

    c = getch();

    if (c == 0 || c == 0xe0)
	switch (c=getch()) {
	case 'K': return LTARROW;
	case 'M': return RTARROW;
	case 'H': return UPARROW;
	case 'P': return DNARROW;
	case 'I': return 'U'-'@';	/* page-up */
	case 'Q': return 'D'-'@';	/* page-down */
	default : return 0;
	}
    return c;
}


/* don't allow interruptions to happen
 */

void
set_input()
{
    signal(SIGINT, SIG_IGN);
} /* nointr */


/* have ^C do what it usually does
 */

void
reset_input()
{
    signal(SIGINT, SIG_DFL);
} /* allowintr */


#if !HAVE_BASENAME
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
	    return p;
    return s;
} /* basename */

#endif


static void
lowercase(s)
char *s;
{
    while (*s) {
	if (isupper(*s))
	    *s += 32;
	s++;
    }
}


/*
 * glob() expands a wildcard, via calls to _dos_findfirst/_next()
 * and pathname retention.
 */

/* local function to add a single file (broken out so I can implement
 * wildcards using _findfirst/_findnext)
 */
static int
glob_addfile(char *file, int count, glob_t *result)
{
    char **newlist;

    logit(("os_glob: adding %s", file));
    if ( count >= result->gl_pathalloc ) {
	result->gl_pathalloc += 50;
	logit(("os_glob: expanding gl_pathv (old pathv=%p, count=%d, pathc=%d)",
		    result->gl_pathv, count, result->gl_pathc));
	if ( result->gl_pathv )
	    newlist = realloc(result->gl_pathv,
			      result->gl_pathalloc * sizeof result->gl_pathv[0]);
	else
	    newlist = calloc(result->gl_pathalloc, sizeof result->gl_pathv[0]);

	unless (newlist)
	    return GLOB_NOSPACE;

	result->gl_pathv = newlist;
    }

    unless (result->gl_pathv[count-1] = malloc(strlen(file)+2))
	return GLOB_NOSPACE;

    strcpy(result->gl_pathv[count-1], file);
    if ( result->gl_flags & GLOB_MARK )
	strcat(result->gl_pathv[count-1], "/");
    result->gl_pathv[count] = 0;
    result->gl_pathc++;
    result->gl_matchc = 1;
    logit(("os_glob: count=%d, pathc=%d,matchc=%d",
	    count, result->gl_pathc, result->gl_matchc));
    return 0;
}


/* local function to wildcard expand a filename
 */
static int
glob_wildcard(path, permit_nomatch, count, dta)
char *path;
int permit_nomatch;
int count;
glob_t *dta;
{
    char *path_bfr;		/* full pathname to return */
    char *file_part;		/* points at file - for filling */

    static FILEFINDBUF3 finfo[50];/* OS/2 dta */
    int dir;			/* directory handle */
    register result;		/* status from DosFindxxx */

    ULONG wanted;

    static char isdotpattern;	/* looking for files starting with . */
    static char isdotordotdot;	/* special case . or .. */
    int idx;			/* for looping through finfo */

    unless (path)
	return -1;

    unless (path_bfr = malloc(strlen(path) + 256))
	return -1;

    strcpy(path_bfr, path);
    file_part = basename(path_bfr);

    /* set up initial parameters for DosFindFirst()
     */
    dir = HDIR_CREATE;

    if (isdotpattern = (*file_part == '.')) {
	/* _dos_findfirst() magically expands . and .. into their
	 * directory names.  Admittedly, there are cases where
	 * this can be useful, but this is not one of them. So,
	 * if we find that we're matching . and .., we just
	 * special-case ourselves into oblivion to get around
	 * this particular bit of DOS silliness.
	 */
	if (file_part[1] == 0 || (file_part[1] == '.' && file_part[2] == 0))
	    return glob_addfile(path, count, dta);

    }

#define OS_FINDFLAGS	0x39

    wanted = sizeof finfo / sizeof finfo[0];
    result = DosFindFirst(path, &dir, OS_FINDFLAGS,
			finfo, wanted, &wanted, FIL_STANDARD);

    if ( result != 0 ) {
	free(path_bfr);
	DosFindClose(dir);
	return permit_nomatch ? glob_addfile(path,count,dta) : -1;
    }

    do {
	for (idx = 0; idx < wanted; idx++ ) {
	    logit(("glob_wildcard: file=%s", finfo[idx].achName));
	    if (finfo[idx].achName[0] == '.' && !isdotpattern)
		continue;

	    strcpy(file_part, finfo[idx].achName);

	    if (result = glob_addfile(path_bfr, count, dta)) {
		free(path_bfr);
		DosFindClose(dir);
		return -1;
	    }
	    ++count;
	}

	wanted = sizeof finfo / sizeof finfo[0];
	result = DosFindNext(dir, finfo, wanted, &wanted);

    } while ( result == 0 );
    DosFindClose(dir);
    free(path_bfr);
    return 0;
} /* glob_wildcard */


/*
 * non-sorting glob with os2 wildcarding
 */
int
os_glob(const char* pattern, int flags, glob_t *result)
{
    int count = 1 + (result->gl_flags & GLOB_DOOFFS ? result->gl_offs : 0);

#if LOGGING
    if ( result->gl_flags & GLOB_APPEND )
	logit(("os_glob: add %s to arglist", pattern));
    else
	logit(("os_glob: create new arglist, initialized with %s", pattern));
#endif

    if ( result->gl_pathc == 0 )
	result->gl_flags = flags;

    if ( result->gl_flags & GLOB_APPEND )
	count += result->gl_pathc;

    unless ( flags & GLOB_NOMAGIC ) {
	if ( strcspn(pattern, "?*") < strlen(pattern) ) {
	    int stat;
	    result->gl_flags |= GLOB_MAGCHAR;

	    stat = glob_wildcard(pattern,
				 result->gl_flags & GLOB_NOCHECK,
				 count,
				 result);

	    return stat ? GLOB_NOMATCH : 0;
	}
	else
	    result->gl_flags &= ~GLOB_MAGCHAR;
    }

    return glob_addfile((char*)pattern, count, result);
}


/*
 * clean up a glob_t after use.
 */
void
os_globfree(glob_t *collection)
{
    int x, start;

    start = collection->gl_flags & GLOB_DOOFFS ? collection->gl_offs : 0;

    for (x=0; x < collection->gl_pathc; x++)
	if ( collection->gl_pathv[start+x] )
	    free(collection->gl_pathv[start+x]);
    if ( collection->gl_pathc )
	free(collection->gl_pathv);
    memset(collection, 0, sizeof(collection[0]));
}


char *
os_tilde(char *name)
{
    /* OS/2 is a single-user system with no home directory (sigh) */
    return 0;
}


os_cclass(c)
unsigned int c;
{
    if (c == '\t' && !list)
	return CC_TAB;
    if (c == 127 || c < ' ')
	return CC_CTRL;
    if (c & 0x80)
	return CC_OTHER;
    return CC_PRINT;
}

#endif
