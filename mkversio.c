#include <stdio.h>
#include <stdlib.h>

int
main()
{
    FILE *version = fopen("VERSION", "r");
    char codeversion[80];
    char codecomment[160];

    if ( version == 0 ) {
	fprintf(stderr, "no VERSION file?\n");
	exit(1);
    }

    codecomment[0] = 0;
    if ( fscanf(version, "%80s %160[^\n]", codeversion, codecomment) >= 1 ) {
	FILE *version_c;

	if (( version_c = fopen("version.c", "w") )) {
	    fprintf(version_c, "char codeversion[] = \"%s\";\n"
			       "char codecomment[] = \"%s\";\n", codeversion, codecomment);
	    fclose(version_c);
	    exit(0);
	}
	fprintf(stderr, "unable to create version.c?\n");
	exit(1);
    }
    fprintf(stderr, "malformed version in VERSION?\n");
    exit(1);
}
