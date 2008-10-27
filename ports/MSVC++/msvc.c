/*
	msvc: libmpg123 add-ons for MSVC++

	copyright 1995-2008 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org

	originally written by Patrick Dehne (inspired by libmpg123/readers.c)

*/

#include "mpg123lib_intern.h"

#include <tchar.h>
#include <fcntl.h>
#include <io.h>

#include "debug.h"

int mpg123_topen(mpg123_handle *fr, const _TCHAR *path)
{
	int filept; /* descriptor of opened file/stream */

	if((filept = _topen(path, O_RDONLY|O_BINARY)) < 0)
	{
		/* Will not work with unicode path name
		   if(NOQUIET) error2("Cannot open file %s: %s", path, strerror(errno)); */

		if(NOQUIET) error1("Cannot open file: %s", strerror(errno));
		fr->err = MPG123_BAD_FILE;
		return filept; /* error... */
	}

	if(mpg123_open_fd(fr, filept) == MPG123_OK) {
		/* Tell mpg123 that it is allowed to close the fd and be happy. */
		fr->rdat.flags |= READER_FD_OPENED;
		return MPG123_OK;
	}
	else
	{
		close(filept);
		return MPG123_ERR;
	}
}
