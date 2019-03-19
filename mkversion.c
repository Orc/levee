#include <stdio.h>
#include <stdlib.h>

main()
{
    FILE *version = fopen("VERSION", "r");
    char codeversion[80];

    if ( version == 0 ) {
	fprintf(stderr, "no VERSION file?\n");
	exit(1);
    }

    if ( fscanf(version, "%80s", codeversion) == 1 ) {
	FILE *version_c;

	if (( version_c = fopen("version.c", "w") )) {
	    fprintf(version_c, "char codeversion[] = \"%s\";\n", codeversion);
	    fclose(version_c);
	    exit(0);
	}
	fprintf(stderr, "unable to create version.c?\n");
	exit(1);
    }
    fprintf(stderr, "malformed version in VERSION?\n");
    exit(1);
}
