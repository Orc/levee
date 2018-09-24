#ifndef _EXTERNAL_D
#define _EXTERNAL_D

#include <stdio.h>
#include <sys/wait.h>

extern FILE *cmdopen(char *, char *, pid_t *);
extern int  cmdclose(FILE *, pid_t);

#endif/*_EXTERNAL_D*/
