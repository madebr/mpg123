/*
	sysutil: generic utilities to interact with the OS (signals, paths)

	copyright ?-2014 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp (dissected/renamed by Thomas Orgis)
*/

#include "mpg123app.h"
#include <sys/stat.h>
#include "sysutil.h"

#include "common/debug.h"

#if 0
/* removed the strndup for better portability */
/*
 *   Allocate space for a new string containing the first
 *   "num" characters of "src".  The resulting string is
 *   always zero-terminated.  Returns NULL if malloc fails.
 */
char *strndup (const char *src, int num)
{
	char *dst;

	if (!(dst = (char *) malloc(num+1)))
		return (NULL);
	dst[num] = '\0';
	return (strncpy(dst, src, num));
}
#endif


size_t dir_length(const char *path)
{
	const char * slashpos = strrchr(path, '/');
	return (slashpos ? slashpos-path : 0);
}

/*
 *   Split "path" into directory and filename components.
 *
 *   Return value is 0 if no directory was specified (i.e.
 *   "path" does not contain a '/'), OR if the directory
 *   is the same as on the previous call to this function.
 *
 *   Return value is 1 if a directory was specified AND it
 *   is different from the previous one (if any).
 */

int split_dir_file (const char *path, const char **dname,  const char **fname)
{
	static char *lastdir = NULL;
	static size_t lastlen = 0;
	size_t dlen = dir_length(path);

	if (dlen) {
		*fname = path + dlen + 1; // a one-byte separator is implied
		if (lastdir && dlen == lastlen && !strncmp(lastdir, path, dlen)) {
			/***   same as previous directory   ***/
			*dname = lastdir;
			return 0;
		}
		// a temporary pointer to be able to modify the string before const
		// Care to optimize that strdup to only copy directory part?
		char *pre_dname = INT123_compat_strdup(path); /* , 1 + slashpos - path); */
		*dname = pre_dname;
		if(!(*dname)) {
			perror("failed to allocate memory for dir name");
			return 0;
		}
		pre_dname[dlen+1] = 0; // keep separator (or not?)
		if (lastdir)
			free (lastdir);
		lastdir = pre_dname;
		lastlen = dlen;
		return 1;
	}
	else {
		/***   no directory specified   ***/
		if (lastdir) {
			free (lastdir);
			lastdir = NULL;
		};
		lastlen = 0;
		*dname = NULL;
		*fname = path;
		return 0;
	}
}
