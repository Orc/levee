#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>


static FILE *logfile = 0;

void
logit(char *fmt, ...)
{
    va_list args;
    char *line, *p;


    if ( logfile == 0 )
	logfile = fopen("levee.log", "w");

    va_start(args, fmt);

    if ( vasprintf(&line, fmt, args) >= 0 ) {
	for (p=line; *p; p++ )
	    if ( *p >= ' ' && *p < 127 )
		fputc(*p, logfile);
	    else
		fprintf(logfile, "%%%02x", (unsigned int)(*p));
	fputc('\n', logfile);
	free(line);
    }
    return 0;
}
