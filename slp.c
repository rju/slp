/* 
 slp - Simplify the writing of beamer tex slides by providing shorthand macros

 Copyright (C) 2012 Reiner Jung

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2, or (at your option)
 any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software Foundation,
 Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 #include <termios.h>
 #include <grp.h>
 #include <pwd.h>
 */

#include <stdio.h>
#include <sys/types.h>
#include <argp.h>
#include "system.h"
#include "scanner.h"
#include "parser.h"

#define EXIT_FAILURE 1

#if ENABLE_NLS
# include <libintl.h>
# define _(Text) gettext (Text)
#else
# define textdomain(Domain)
# define _(Text) Text
#endif
#define N_(Text) Text

char *xmalloc();
char *xrealloc();
char *xstrdup();

static error_t parse_opt(int key, char *arg, struct argp_state *state);
static void show_version(FILE *stream, struct argp_state *state);

/* argp option keys */
enum {
	DUMMY_KEY = 129
};

/* Option flags and variables.  These are initialized in parse_opt.  */

char *oname; /* --output=FILE */

static struct argp_option options[] = { { "output", 'o', N_("FILE"), 0,
		N_("Send output to FILE instead of standard output"), 0 }, { NULL,
		0, NULL, 0, NULL, 0 } };

/* The argp functions examine these global variables.  */
const char *argp_program_bug_address = "<rju@informatik.uni-kiel.de>";
void (*argp_program_version_hook)(FILE *, struct argp_state *) = show_version;

static struct argp argp =
		{ options, parse_opt, N_("[FILE...]"),
				N_("Simplify the writing of beamer tex slides by providing shorthand macros"),
				NULL, NULL, NULL };

int main(int argc, char **argv) {
	yyin = NULL;
	textdomain(PACKAGE);
	argp_parse(&argp, argc, argv, 0, NULL, NULL);

	if (yyin != NULL) {
		parse();
		fclose(yyin);
	}

	exit(0);
}

/* Parse a single option.  */
static error_t parse_opt(int key, char *arg, struct argp_state *state) {
	switch (key) {
	case ARGP_KEY_INIT:
		/* Set up default values.  */
		oname = "stdout";
		ofile = stdout;
		break;

	case 'o': /* --output */
		oname = xstrdup(arg);
		ofile = fopen(oname, "w");
		if (!ofile)
			argp_failure(state, EXIT_FAILURE, errno,
					_("Cannot open %s for writing"), oname);
		break;

	case ARGP_KEY_ARG: /* [FILE]... */
		if ((yyin = fopen(arg, "r")) == 0) {
			fprintf(stderr, "File not found: %s\n", arg);
			exit(1);
		}
		break;

	default:
		return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

/* Show the version number and copyright information.  */
static void show_version(FILE *stream, struct argp_state *state) {
	(void) state;
	/* Print in small parts whose localizations can hopefully be copied
	 from other programs.  */
	fputs(PACKAGE" "VERSION"\n", stream);
	fprintf(stream, _("Written by %s.\n\n"), "Reiner Jung");
	fprintf(stream, _("Copyright (C) %s %s\n"), "2012", "Reiner Jung");
	fputs(
			_("\
This program is free software; you may redistribute it under the terms of\n\
the GNU General Public License.  This program has absolutely no warranty.\n"),
			stream);
}
