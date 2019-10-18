#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>


static FILE *logfile = 0;

void
logger(char *fmt, ...)
{
    va_list args;
    char *p, *endp;
#if HAVE_VASPRINTF
    char *line;
#else
    char biglogbuf[1024];
#endif
    int size;

    if ( logfile == 0 ) {
	logfile = fopen("levee.log", "w");
	setvbuf(logfile, 0, _IOLBF, 0);
    }

    va_start(args, fmt);

#if HAVE_VASPRINTF
    if ( (size = vasprintf(&line, fmt, args)) > 0 ) {
	p = line;
	endp = &line[size];
    }
#else
    if ( (size = vsprintf(biglogbuf, fmt, args)) > 0 ) {
	p = biglogbuf;
	endp = &biglogbuf[size];
    }
#endif

    if (size > 0) {
	for ( ; p < endp; p++ ) {
	    if ( *p >= ' ' && *p < 127 )
		fputc(*p, logfile);
	    else
		fprintf(logfile, "%%%02x", (unsigned int)(*p));
	}
	fputc('\n', logfile);
#if HAVE_VASPRINTF
	free(line);
#endif
    }
    va_end(args);
}
