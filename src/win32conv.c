#include "config.h"
#define WIN32_LEAN_AND_MEAN 1
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <windows.h>
#include <winnls.h>
#include <shellapi.h>

#include "mpg123.h"
#include "debug.h"
#include "compat.h"
#include "win32conv.h"

int win32_cmdline_utf8(int * argc, char *** argv)
{
	wchar_t **argv_wide;
	char *argvptr;
	int argcounter;

	/* That's too lame. */
	if(argv == NULL || argc == NULL) return -1;

	argv_wide = CommandLineToArgvW(GetCommandLineW(), argc);
	if(argv_wide == NULL){ error("Cannot get wide command line."); return -1; }

	*argv = (char **)calloc(sizeof (char *), *argc);
	if(*argv == NULL){ error("Cannot allocate memory for command line."); return -1; }

	for(argcounter = 0; argcounter < *argc; argcounter++)
	{
		win32_wide_utf8(argv_wide[argcounter], (const char **)&argvptr, NULL);
		(*argv)[argcounter] = argvptr;
	}
	LocalFree(argv_wide); /* We don't need it anymore */

	return 0;
}

void win32_cmdline_free(int argc, char **argv)
{
	int i;

	if(argv == NULL) return;

	for(i=0; i<argc; ++i) free(argv[i]);
}
