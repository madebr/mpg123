/*
	local: some stuff for localisation

	Currently, this is just about determining if we got UTF-8 locale and
	checking output terminal properties.

	copyright 2008-2020 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Thomas Orgis, based on a patch by Thorsten Glaser.
*/

#include "config.h"
#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif
#ifdef HAVE_LANGINFO_H
#include <langinfo.h>
#endif
#include "compat.h"

#ifdef __EMX__
/* Special ways for OS/2 EMX */
#include <stdlib.h>
#else
/* POSIX stuff */
#ifdef HAVE_TERMIOS
#include <termios.h>
#include <sys/ioctl.h>
#endif
#endif

#include "local.h"

#include "debug.h"

int utf8force = 0; // enforce UTF-8 workings
int utf8env = 0; // produce UTF-8 text output
int utf8loc = 0; // have actual UTF-8 locale (so that mbstowcs() works)

static int term_is_fun = -1;

/* Check some language variable for UTF-8-ness. */
static int is_utf8(const char *lang);

void check_locale(void)
{
	if(utf8force)
		utf8env = 1;
	else
	{
		const char *cp;

		/* Check for env vars in proper oder. */
		if((cp = getenv("LC_ALL")) == NULL && (cp = getenv("LC_CTYPE")) == NULL)
		cp = getenv("LANG");

		if(is_utf8(cp))
			utf8env = 1;
	}

#if defined(HAVE_SETLOCALE) && defined(LC_CTYPE)
	/* To query, we need to set from environment... */
	if(
		   is_utf8(setlocale(LC_CTYPE, ""))
		// If enforced, try to set an UTF-8 locale that hopefully exists.
		|| (utf8force && is_utf8(setlocale(LC_CTYPE, "C.UTF-8")))
		|| (utf8force && is_utf8(setlocale(LC_CTYPE, "en_US.UTF-8")))
	)
	{
		utf8env = 1;
		utf8loc = 1;
	}
#endif
#if defined(HAVE_NL_LANGINFO) && defined(CODESET)
	/* ...langinfo works after we set a locale, eh? So it makes sense after setlocale, if only. */
	if(is_utf8(nl_langinfo(CODESET)))
	{
		utf8env = 1;
		utf8loc = 1;
	}
#endif

	debug2("UTF-8 env %i: locale: %i", utf8env, utf8loc);
}

static int is_utf8(const char *lang)
{
	if(lang == NULL) return 0;

	/* Now, if the variable mentions UTF-8 anywhere, in some variation, the locale is UTF-8. */
	if(   strstr(lang, "UTF-8") || strstr(lang, "utf-8")
	   || strstr(lang, "UTF8")  || strstr(lang, "utf8")  )
	return 1;
	else
	return 0;
}

int term_have_fun(int fd, int want_visuals)
{
	if(term_is_fun > -1)
		return term_is_fun;
	else
		term_is_fun = 0;
#ifdef HAVE_TERMIOS
	if(term_width(fd) > 0 && want_visuals)
	{
		/* Only play with non-dumb terminals. */
		char *tname = compat_getenv("TERM");
		if(tname)
		{
			if(strcmp(tname, "") && strcmp(tname, "dumb"))
				term_is_fun = 1;
			free(tname);
		}
	}
#endif
	return term_is_fun;
}

/* Also serves as a way to detect if we have an interactive terminal. */
int term_width(int fd)
{
#ifdef __EMX__
/* OS/2 */
	int s[2];
	_scrsize (s);
	if (s[0] >= 0)
		return s[0];
#else
#ifdef HAVE_TERMIOS
/* POSIX */
	struct winsize geometry;
	geometry.ws_col = 0;
	if(ioctl(fd, TIOCGWINSZ, &geometry) >= 0)
		return (int)geometry.ws_col;
#endif
#endif
	return -1;
}
