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

// Question: Keep abstract or predefine the struct with a void *handle inside?
// The header handling stuff should be common code, constructing and parsing
// header lines. Or do external libs do that for us?

struct net123_handle_struct;
typedef struct net123_handle_struct net123_handle;

net123_handle * net123_new(void);

// Open a new URL, store headers.
int net123_open(net123_handle *nh, const char *url)
int net123_close(net123_handle *nh);

// Set a client header to be sent on each request.
int net123_client_header(net123_handle *nh, const char *name,  const char *value);
// Case-insenstive header name we are interested in, others are dropped.
// If not provided, all headers are stored.
int net123_keep_header(net123_handle *nh, const char *name);
// Get count and names of stored server headers.
// Set after each successful net123_open().
size_t net123_server_headers(net123_handle *nh, const char **names);
// Fetch value of named header. Returns MPG123_OK if found and value non-null.
int net123_header_value(net123_handle *nh, const char *name, const char **value);

// Set authentication data to use.
int net123_auth(net123_handle *nh, const char *user, const char *password);

// MPG123_OK or error code returned
int net123_read(net123_handle *nh, void *buf, size_t bufsize, size_t *gotbytes);

#endif
