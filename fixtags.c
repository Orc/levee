/*
 * replace the weird ctags tag for main() with an actual tag
 * that can be picked by a non-heroic search for the tag main
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


int
main()
{
    char line[200];
    char *tag, *file, *pattern;
    char *found;


    while (fgets(line, sizeof line, stdin)) {
	tag = strtok(line, "\t");
	file = strtok(0, "\t");
	pattern = strtok(0, "\n");

	if ( tag[0] == 'M' ) {
	    found = strstr(pattern, "main(");

	    if ( found && ((found == pattern) || !isalnum(found[-1])) ) {
		printf("%s\t%s\t%s\n", "main", file, pattern);
		continue;
	    }
	}
	printf("%s\t%s\t\%s\n", tag, file, pattern);
    }
    exit(0);
}
