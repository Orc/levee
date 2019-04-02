#include "levee.h"
#include "extern.h"
#include <stdio.h>
#include <stdlib.h>

FILE *
expandfopen(char *f, char *mode)
{
    char *fqname;
    FILE *it;

    logit(("expandfopen %s", f));

    if ( fqname = os_tilde(f) ) {
	logit(("expandfopen -> %s", fqname));
	it = fopen(fqname, mode);
	free(fqname);
	return it;
    }

    return fopen(f, mode);
}
