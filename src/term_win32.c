/*
	term_win32: Windows-specifc terminal functionality

	This is a very lightweight terminal library, just the minimum to

	- get at the width of the terminal (if there is one)
	- be able to read single keys being pressed for control
	- maybe also switch of echoing of input

	copyright 2008-2022 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Thomas Orgis and Jonathan Yong
*/


#include "config.h"
#include "compat.h"

#include "terms.h"

#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <wincon.h>

#include "debug.h"

// No fun for windows until we reorganize control character stuff.
int term_have_fun(int fd, int want_visuals)
{
	return 0;
}

// Also serves as a way to detect if we have an interactive terminal.
// It is important right now to really honour the fd given, usually
// either stdin or stderr.
int term_width(int fd)
{
	CONSOLE_SCREEN_BUFFER_INFO pinfo;
	HANDLE hStdout;
	DWORD handle;

	switch(fd){
	case STDIN_FILENO:
		handle = STD_INPUT_HANDLE;
		break;
	case STDOUT_FILENO:
		handle = STD_OUTPUT_HANDLE;
		break;
	case STDERR_FILENO:
		handle = STD_ERROR_HANDLE;
		break;
	default:
		return -1;
	}

	hStdout = GetStdHandle(handle);
	if(hStdout == INVALID_HANDLE_VALUE || hStdout == NULL)
		return -1;
	if(GetConsoleScreenBufferInfo(hStdout, &pinfo)){
		return pinfo.dwMaximumWindowSize.X;
	}
	return -1;
}

// TODO: term_setup(); term_restore(); term_get_key()
