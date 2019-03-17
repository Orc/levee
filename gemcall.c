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
 * Gemdos (Atari ST) bindings for levee (Alcyon/Sozobon C)
 */
#include "levee.h"

#if OS_ATARI
#include <atari\osbind.h>

os_dwrite(s,len)
char *s;
{
    return 0;
}


os_tty_setup()
{
    return 1;
}


unsigned
getKey()
/* get input from the keyboard. All odd keys (function keys, et al) that
 * do not produce a character have their scancode orred with $80 and returned.
 */
{
    unsigned c;
    long key;

    c = (key = Crawcin()) & 0xff;
    if (!c)
	c = (((unsigned)(key>>16))|0x80) & 0xff;
    return c;
} /* getKey */


/*
 * basename() returns the filename part of a pathname
 */
char *
basename(s)
register char *s;
{
    register char *p = s;
    
    for (p = s+strlen(s); --p > s; )
	if ( *p == '\\' || *p == ':' )
	    return p+1;
    return s;
} /* basename */
#endif

#endif /*OS_ATARI*/
