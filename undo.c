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

#define BUFSZ	sizeof(undo.coreblock)
#define AVAIL(x)	((x)<<1)
#define INDEX(x)	((1+x)>>1)

bool
pushblock(struct undostack *u)
{
    if (u->blockp == 0)
	if ( (uwrite = OPEN_NEW(undobuf)) == NOWAY )
	    return FALSE;
    if (BUFSZ == WRITE_TEXT(uwrite, u->coreblock, BUFSZ)) {
	u->blockp++;
	u->ptr = 0;
	return TRUE;
    }
    return FALSE;
}

bool
pushw(struct undostack *u, int i)
{
    if (u->ptr >= PAGESIZE && !pushblock(u))
	return FALSE;
    u->coreblock[u->ptr++] = i;
    return TRUE;
}

bool
pushmem(struct undostack *u, int start, int size)
{
    int chunk;
    bool ok;

    ok = TRUE;
    while (ok && size > 0) {
	chunk = Min(size, AVAIL(PAGESIZE-u->ptr));
	moveleft(&core[start], (char*)&u->coreblock[u->ptr], chunk);
	size -= chunk;
	start += chunk;
	if (size > 0)
	    ok = pushblock(u);
	else
	    u->ptr += INDEX(chunk);
    }
    return ok;
}

void
zerostack(struct undostack *u)
{
    if (u->blockp > 0)
	CLOSE_FILE(uwrite);
    u->blockp = 0;			 /* initialize the stack */
    u->ptr = 0;
}

bool
uputcmd(struct undostack *u, int size, int start, int cmd)
{
    return(pushw(u, size) && pushw(u, start) && pushw(u, cmd));
}

void
insert_to_undo(struct undostack *u, int start, int size)
{
    if (uputcmd(u, size, start, U_DELC)) {
	fixmarkers(start, size);
	bufmax += size;
    }
    else
	error();
}

/* delete stuff from the buffer && put it into the undo stack */

bool
delete_to_undo(struct undostack *u, int start, int lump)
{
    if (lump <= 0)
	return TRUE;
    else if (pushmem(u,start,lump) && uputcmd(u,lump,start,U_ADDC)) {
	moveleft(&core[start+lump], &core[start], bufmax-(start+lump));
	fixmarkers(start,-lump);
	bufmax -= lump;
	return TRUE;
    }
    else
	return FALSE;
}

/* copy stuff into the undo buffer */

bool
move_to_undo(struct undostack *u, int start, int lump)
{
    return pushmem(u, start, lump) && uputcmd(u,lump,start,U_MOVEC);
}

bool
popblock(struct undostack *u)
{
    if (u->blockp > 0) {
	if (SEEK_POSITION(uread, (long)((--u->blockp)*BUFSZ), 0) < 0)
	    return FALSE;
	if (BUFSZ == READ_TEXT(uread, u->coreblock, BUFSZ)) {
	    u->ptr = PAGESIZE;
	    return TRUE;
	}
    }
    return FALSE;
}

bool
popw(struct undostack *u, int *i)
{
    if (u->ptr < 1 && !popblock(u))
	return FALSE;
    *i = u->coreblock[--u->ptr];
    return TRUE;
}

bool
popmem(struct undostack *u, int start, int size)
{
    int chunk, loc;
    bool ok;

    loc = start+size;		/* running backwards */
    ok = TRUE;
    while (ok && size > 0) {
	chunk = Min(size, AVAIL(u->ptr));
	size -= chunk;
	loc -= chunk;
	moveleft((char*)&u->coreblock[u->ptr-INDEX(chunk)], &core[loc], chunk);
	if (size > 0)
	    ok = popblock(u);
	else
	    u->ptr -= INDEX(chunk);
    }
    return(ok);
}

/* delete (I)nserted text */

bool
takeout(struct undostack *save_undo, int *curp)
{
    int lump;

    return popw(&undo,curp) && popw(&undo,&lump)
			    && delete_to_undo(save_undo,*curp,lump);
}

bool
copyover(struct undostack *save_undo, int *curp)
{
    int lump;

    return popw(&undo, curp) && popw(&undo, &lump)
			     && move_to_undo(save_undo, *curp, lump)
			     && popmem(&undo, *curp, lump);
}

bool
putin(struct undostack *save_undo, int *curp)
{
    int lump;

    if (popw(&undo,curp) && popw(&undo,&lump) && (bufmax+lump < EDITSIZE)) {
	insert_to_undo(save_undo, *curp, lump);
	moveright(&core[*curp], &core[*curp+lump], bufmax-*curp);
	if (popmem(&undo, *curp, lump))
	    return TRUE;
	else
	    moveleft(&core[*curp+lump], &core[*curp], bufmax-*curp);
    }
    return FALSE;
}

/* driver for undo -- returns last address modified || -1 if error */

int
fixcore(int *topp)
{
    int curp;
    static struct undostack save_undo;
    bool closeio, ok;
    int cch;

    if (undo.blockp > 0 || undo.ptr > 0) {
	closeio = (undo.blockp > 0);
	if (closeio) {			/* save diskfile */
	    CLOSE_FILE(uwrite);		/* close current undo file */
	    os_rename(undobuf,undotmp);
	    uread = OPEN_OLD(undotmp);	/* reopen it for reading */
	    if ( uread == NOWAY )
		return -1;
	}
	*topp = EDITSIZE+1;
	curp = -MAGICNUMBER;
	save_undo.blockp = save_undo.ptr = 0;
	ok = TRUE;
	while (ok && popw(&undo,&cch)) {
            switch (cch) {
	      case U_ADDC : ok = putin(&save_undo, &curp); break;
	      case U_MOVEC: ok = copyover(&save_undo, &curp); break;
	      case U_DELC : ok = takeout(&save_undo, &curp); break;
	    }
	    if (curp < *topp)
		*topp = curp;
	}
	if (curp >= 0)
	    undo = save_undo;
	if (closeio) {
	    CLOSE_FILE(uread);		/* Zap old buffer */
	    os_unlink(undotmp);
	}
	if (!ok)
	    error();
	return(curp);
    }
    return ERR;
}
