/*
	local: some stuff for localisation

	Currently, this is just about determining if we got UTF-8 locale.

	copyright 2008-2019 by the mpg123 project - free software under the terms of the LGPL 2.1
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
#include "debug.h"

int utf8force = 0; // enforce UTF-8 workings
int utf8env = 0; // produce UTF-8 text output
int utf8loc = 0; // have actual UTF-8 locale (so that mbstowcs() works)

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
