#include "levee.h"
#include <stdlib.h>

#if OS_UNIX

#include <string.h>
#include <sys/types.h>
#include <pwd.h>

char *
expand(char *f)
{
    char *name, *slash, *expanded;
    struct passwd *user;
    
    if ( !f )
	return 0;
    
    if ( *f != '~' )
	return 0;

    if ( f[1] == '/' ) {
	user = getpwuid(getuid());
	slash = f + 1;
    }
    else {
	name = f + 1;
	if ( (slash = strchr(name, '/')) == 0 )
	    return 0;
	
	*slash = 0;
	user = getpwnam(name);
	*slash = '/';
    }

    if ( !user )
	return 0;

    if (( expanded = malloc(strlen(user->pw_dir) + 1 + strlen(1+slash) + 1) ))
	sprintf(expanded, "%s/%s", user->pw_dir, 1+slash);

    return expanded;
}


#else

char *
expand(char *f)
{
    return 0;
}

#endif


FILE *
expandfopen(char *f, char *mode)
{
    char *expanded = expand(f);
    FILE *it;

    if ( expanded ) {
	it = fopen(expanded, mode);
	free(expanded);
	return it;
    }

    return fopen(f, mode);
}
