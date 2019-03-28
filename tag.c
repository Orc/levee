/*
 * LEVEE, or Captain Video;  A vi clone
 *
 * Copyright (c) 1982-2019 David L Parsons
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

#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <errno.h>

/*
 * look up a tag
 */
int
find_tag(char *tag, int sztag, Tag *ret)
{
    FILE *tags = fopen("tags", "r");
    static char tagline[120];
    char *filename = 0;
    char *pattern = 0;
    int count=0;

    if ( tags == 0 ) {
	errno = EBADF;
	return 0;
    }
    
    while ( fgets(tagline, sizeof tagline, tags) ) {
	if ( memcmp(tagline, tag, sztag) == 0 && tagline[sztag] == '\t' ) {
	    filename = strtok(&tagline[sztag+1], "\t");
	    pattern = strtok(NULL, "\n");

	    fclose(tags);

	    if ( filename && pattern ) {
		ret->filename = filename;
		ret->pattern = pattern;
		return 1;
	    }
	    errno = EINVAL;
	    return 0;
	}
    }
    fclose(tags);
    errno = ENOENT;
    return 0;
}
