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

#include <unistd.h>

/* read in a file -- return TRUE -- read file
			    FALSE-- file too big
*/

int 
addfile(f, start, endd, size)
FILE *f;
int start;
int endd, *size;
{
    register int chunk;

    chunk = read(fileno(f), core+start, endd-start);

    *size = chunk;
    return chunk < endd-start;
}


/* write out a file -- return TRUE if ok. */

bool 
putfile(f, start, endd)
register FILE *f;
register int start, endd;
{
    int size = (endd-start);
    int ok;

#if USING_STDIO
    ok = fwrite(core+start, size, 1, f) == 1;
#else
    ok = write(fileno(f), core+start, size) == size;
#endif

    return ok;
}
