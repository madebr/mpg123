/*
	net123: network (HTTP(S)) streaming for mpg123 using external code

	copyright 2022 by the mpg123 project --
	free software under the terms of the LGPL 2.1,
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Thomas Orgis

	This is supposed to be a binding to some system facilty to
	get HTTP/HTTPS streaming going. The goal is to not be responsible
	for HTTP protocol handling or even TLS in mpg123 code.
	Maybe this could also stream via other protocols ... maybe even
	SSH, whatever.

	For POSIX platforms, this shall refer to external binaries that
	do all the network stuff. For others, some system library or
	other facility shall provide the minimal required functionality.

	We could support seeking using HTTP ranges. But the first step is
	to be able to just make a request to a certain URL and get the
	server response headers followed by the data (be it a playlist file
	or the MPEG stream itself).

	We need to support:
	- client headers (ICY yes or no, client name)
	- HTTP auth parameters
*/
#ifndef _MPG123_NET123_H_
#define _MPG123_NET123_H_

// The network implementation defines the struct for private use.
// The purpose if just to keep enough context to be able to
// call net123_read() and net123_close() afterwards.
struct net123_handle_struct;
typedef struct net123_handle_struct net123_handle;

// Just prepare storage and whatever intiializations are needed.
net123_handle * net123_new(void);
// Free resources, implying a close, too.
void net123_del(net123_handle *nh);

// TODO: decide if mpg123_strings should be used

// Open stream from URL, parsing headers and storing the selected ones.
// nh: handle
// url: stream URL
// client_head: NULL-terminated list of client header lines
// head: NULL-terminated list of response header field names (case-insensitive)
// val: matching storage for header values, individual entries being nulled by the call
//   and only those with new values allocated and set
// HTTP auth parameters are taken from mpg123 parameter struct behind the scenes or from
// the URL itself by the backend (ponder that, maybe just always put user:pw@host in there, if set?)
int net123_open(net123_handle *nh, const char *url, const char **client_head, const char **head, char **val);
// MPG123_OK or error code returned
int net123_read(net123_handle *nh, void *buf, size_t bufsize, size_t *gotbytes);
int net123_close(net123_handle *nh);

#endif
