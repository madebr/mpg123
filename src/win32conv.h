/*
	Code that handles conversion of the windows command line arguments to UTF-8 as internal representation.
*/
#ifndef _MPG123_WIN32CONV_H_
#define _MPG123_WIN32CONV_H_

/**
 * Put the windows command line into argv / argc, encoded in UTF-8.
 * You are supposed to free up resources by calling win32_cmdline_free with the values you got from this one.
 * @return 0 on success, -1 on error */
int win32_cmdline_utf8(int * argc, char *** argv);

/**
 * Free up cmdline memory (the argv itself, theoretically hidden resources, too).
 */
void win32_cmdline_free(int argc, char **argv); 

#endif

