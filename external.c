/*
 * LEVEE, or Captain Video;  A vi clone
 *
 * Copyright (c) 1982-2018 David L Parsons
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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <paths.h>

FILE *
cmdopen(char *cmdline, char *input, pid_t *child)
{
    pid_t job;
    int io[2];


    if ( pipe(io) < 0 )
	return NULL;

    if ( (job = fork()) < 0 ) {
	close(io[0]);
	close(io[1]);
	return NULL;
    }

    if ( job < 0 )
	return NULL;
    else if ( job == 0 ) {
	/* child */
	int ifd = open(input, O_RDONLY);

	close(io[0]);
	if ( (ifd < 0) || (dup2(ifd, 0) < 0) ) {
	    close(io[1]);
	    exit(1);
	}
	dup2(io[1], 1);
	dup2(1,2);
	
	execl("/bin/sh", "sh", "-c", cmdline, 0L);
	close(io[1]);
	exit(1);
    }
    else {
	/* parent */
	close(io[1]);
	*child = job;
	return fdopen(io[0], "r");
    }
}


int
cmdclose(FILE *input, pid_t child)
{
    int status;

    waitpid(child, &status, 0);
    fclose(input);

    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}


#ifdef DEBUG

main(argc, argv)
char **argv;
{
    FILE *f;
    pid_t child;
    register int c;
    int count = 0;
    int status;


    if ( (f = cmdopen(argc > 1 ? argv[1] : "sort", "README", &child)) != 0 ) {
	while ( (c=getc(f)) != EOF ) {
	    count++;
	    putchar(c);
	}
	status = cmdclose(f,child);
	if ( status != 0 )
	    printf("exit status %d -- ", status);
	printf("read %d character%s\n", count, (count!=1)?"s":"");
    }
}
#endif
