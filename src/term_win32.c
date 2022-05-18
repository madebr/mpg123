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
#include <io.h>
#include <ctype.h>
#include <windows.h>
#include <wincon.h>

#include "debug.h"

static HANDLE consoleinput = INVALID_HANDLE_VALUE;
static HANDLE getconsole(void){
  if(consoleinput == INVALID_HANDLE_VALUE){
    consoleinput = GetStdHandle(STD_ERROR_HANDLE);
  }
  return consoleinput;
}

// No fun for windows until we reorganize control character stuff.
int term_have_fun(int fd, int want_visuals)
{
        return 0;
}

static DWORD lastmode;
static int modeset;
int term_setup(void)
{
  DWORD mode, r;
  HANDLE c = getconsole();
  if(c == INVALID_HANDLE_VALUE) return -1;

  r = GetConsoleMode(c, &mode);
  if(!r) return -1;
  lastmode = mode;
  modeset = 1;

  mode |= ENABLE_LINE_INPUT|ENABLE_PROCESSED_INPUT|ENABLE_WINDOW_INPUT;
  mode &= ~(ENABLE_ECHO_INPUT|ENABLE_QUICK_EDIT_MODE|ENABLE_MOUSE_INPUT);

  r = SetConsoleMode(c, mode);
  return r ? -1 : 0;
}

void term_restore(void){
  HANDLE c = getconsole();
  if(modeset && c != INVALID_HANDLE_VALUE)
    SetConsoleMode(c, lastmode);
}

static int width_cache[3] = { -1, -1, -1};
int term_width(int fd)
{
  CONSOLE_SCREEN_BUFFER_INFO pinfo;
  HANDLE h;
  if(fd < 3 && width_cache[fd] != -1)
    return width_cache[fd];

  h = (HANDLE) _get_osfhandle(fd);

  if(h == INVALID_HANDLE_VALUE || h == NULL)
    return -1;
  if(GetConsoleScreenBufferInfo(h, &pinfo)){
    if(fd < 3)
      width_cache[fd] = pinfo.dwMaximumWindowSize.X;
    return pinfo.dwMaximumWindowSize.X;
   }
  return -1;
}

int term_present(void){
  return _fileno(stderr) != -2 ? 1 : 0;
}

/* Get the next pressed key, if any.
   Returns 1 when there is a key, 0 if not. */
int term_get_key(int do_delay, char *val){
  INPUT_RECORD record;
  HANDLE input;
  DWORD res;

  input = getconsole();
  if(input == NULL || input == INVALID_HANDLE_VALUE)
    return 0;

  while(WaitForSingleObject(input, do_delay ? 10 : 0) == WAIT_OBJECT_0){
    do_delay = 0;
    if(!ReadConsoleInput(input, &record, 1, &res))
      return 0;
    if(record.EventType == KEY_EVENT && record.Event.KeyEvent.bKeyDown){
      *val = record.Event.KeyEvent.uChar.AsciiChar;
      return 1;
    }
  }

  return 0;
}
