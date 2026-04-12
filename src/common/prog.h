#ifndef TERMUS_PROG_H
#define TERMUS_PROG_H

#include "common/compiler.h"

/* set in beginning of main */
extern char *program_name;

struct option {
	/* short option or 0 */
	int short_opt;

	/* long option or NULL */
	const char *long_opt;

	/* does option have an argument */
	int has_arg;
};

/*
 * arg: returned argument if .has_arg is 1
 *
 * returns: index to options array or -1 of no more options
 */
int get_option(char **argv[], const struct option *options, char **arg);

void warn(const char *format, ...) TERMUS_FORMAT(1, 2);
void warn_errno(const char *format, ...) TERMUS_FORMAT(1, 2);
void die(const char *format, ...) TERMUS_FORMAT(1, 2) TERMUS_NORETURN;
void die_errno(const char *format, ...) TERMUS_FORMAT(1, 2) TERMUS_NORETURN;

#endif
