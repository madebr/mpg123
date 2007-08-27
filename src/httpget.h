/*
	httpget: HTTP input routines (the header)

	copyright 2007 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Thomas Orgis

	Note about MIME types:
	You feed debunk_mime() a MIME string and it classifies it as it is relevant for mpg123.
	In httpget.c are the MIME class lists, which may be appended to to support more bogus MIME types.
*/

#ifndef _HTTPGET_H_
#define _HTPPGET_H_

/* There is a whole lot of MIME types for the same thing.
   the function will reduce it to a combination of these flags */
#define IS_FILE 1
#define IS_LIST 2
#define IS_M3U  4
#define IS_PLS  8
int debunk_mime(const char* mime);

extern char *proxyurl;
extern unsigned long proxyip;
/* takes url and content type string address, opens resource, returns fd for data, allocates and sets content type */
extern int http_open (char* url, char** content_type);
extern char *httpauth;

#endif
