#ifndef H_LOCAL
#define H_LOCAL
/*
	local: some stuff for localisation

	Currently, this is just about determining if we got UTF-8 locale and
	checking output terminal properties.

	copyright 2008-2020 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Thomas Orgis, based on a patch by Thorsten Glaser.

	This got extended for the distinciton between a vague wish for UTF-8 in/output
	encoding from environment variables (utf8env) and a system actually being
	configured to correcty work with it (utf8loc). The configured locale is needed
	to enable proper (?) length computation and control character filtering.

	The fallback shall be either dumb UTF-8 with just filtering C0 and C1 control
	characters, or even full-on stripping to 7-bit ASCII.
*/

/* Pulled in by mpg123app.h! */

/* Set this to enforce UTF-8 output (utf8env will be 1, but utf8loc may not) */
extern int utf8force;

/* This is 1 if check_locale found some UTF-8 hint, 0 if not. */
extern int utf8env;

/* This is 1 if check_locale configured a locale with UTF-8 charset */
extern int utf8loc;

/* Check/set locale, set the utf8env variable.
   After calling this, a locale for LC_CTYPE should be configured,
   but one that is based on UTF-8 only if utf8loc is set. */
void check_locale(void);

/* Return non-zero if full terminal fun is desired/possible. */
int term_have_fun(int fd, int want_visuals);

/* Return width of terminal associated with given descriptor,
   -1 when there is none. */
int term_width(int fd);

#endif
