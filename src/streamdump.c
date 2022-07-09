/*
	streamdump: Dumping a copy of the input data.

	This evolved into the generic I/O interposer for direct file or http stream
	access, with explicit buffering for getline.

	copyright 2010-2019 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp
*/

#include "streamdump.h"
#include <fcntl.h>
#include <errno.h>
#include "debug.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif

/* Stream dump descriptor. */
static int dump_fd = -1;

// Read without the buffer. This is used to fill the buffer explicitly in getline.
// This is the function that finally wraps around all the different types of input.

static ssize_t stream_read_raw(struct stream *sd, void *buf, size_t count)
{
	ssize_t ret = -1;
#ifdef NET123
	if(sd->nh)
		ret = net123_read(sd->nh, buf, count);
#endif
#ifdef WANT_WIN32_SOCKETS
	if(sd->fd >= 0 && sd->network)
		ret = win32_net_read(sd->fd, buf, count);
#endif
	if(sd->fd >= 0) // plain file or network socket
		ret = (ssize_t) unintr_read(sd->fd, buf, count);
	return ret;
}

static ssize_t stream_read(struct stream *sd, void *buf, size_t count)
{
	if(!sd)
		return -1;
	char *bbuf = buf;
	ssize_t ret = 0;
	if(count > SSIZE_MAX)
		return -1;
	while(count)
	{
		size_t get = 0;
		if(sd->fill)
		{ // drain the buffer
			get = sd->fill > count ? count : sd->fill;
			memcpy(bbuf, sd->bufp, get);
			sd->fill -= get;
			sd->bufp += get;
		} else
		{ // get it from the source
			ssize_t rret = stream_read_raw(sd, bbuf, count);
			if(rret < 0)
				return ret > 0 ? ret : -1;
			if(rret == 0)
				return ret;
			get = rret;
		}
		bbuf  += get;
		count -= get;
		ret   += get;
	}
	return ret;
}

static off_t stream_seek(struct stream *sd, off_t pos, int whence)
{
	if(!sd || sd->network)
		return -1;
	return lseek(sd->fd, pos, whence);
}

// Read into the stream buffer, look for line endings there, copy stuff.
// This should work with \n and \r\n sequences ... even just \r sequences?
// Yes, either \r or \n ends a line, a following \n or \r is just swallowed.
// Need to catch the case where the buffer ends with \r and the next buffer
// contents start with the matching \n, and the other way round.
ssize_t stream_getline(struct stream *sd, mpg123_string *line)
{
	if(!sd || !line)
		return -1;
	line->fill = 0; // this is EOF
	char lend = 0;
	while(1)
	{
		mdebug("getline loop with %d", sd->fill);
		// If we got just an \r, we need to ensure that we swalloed the matching \n,
		// too. This implies that we got a line stored already.
		if(sd->fill && lend)
		{
			// finish skipping over an earlier line end
			if( (*sd->bufp == '\n' || *sd->bufp == '\r') && *sd->bufp != lend)
			{
				++sd->bufp;
				--sd->fill;
			}
			// whatever happened, no half-line-end lurking anymore
			return line->fill;
		}
		if(sd->fill)
		{
			// look for line end here, copy things
			size_t i = 0;
			while(i < sd->fill && sd->bufp[i] != '\n' && sd->bufp[i] != '\r')
				++i;
			// either found an end, or hit the end
			if(!mpg123_add_substring(line, sd->bufp, 0, i))
				return -1; // out of memory
			// skip over a line end if found
			if(i == sd->fill)
			{
				// not done yet, refill, please
				sd->fill = 0;
			} else
			{
				// got end and stored complete line, but need to go on to capture full line end
				lend = sd->bufp[i]; // either \r or \n
				sd->bufp += i+1;
				sd->fill -= i+1;
			}
		} else
		{
			debug("re-filling buffer");
			// refill buffer
			ssize_t ret = stream_read_raw(sd, sd->buf, sizeof(sd->buf));
			mdebug("raw read return: %zd", ret);
			if(ret < 0)
				return -1;
			else if(ret == 0)
				return line->fill; // A line ends at end of file.
			else
			{
				sd->fill = ret;
				sd->bufp = sd->buf;
			}
		}
	}
}

int stream_parse_headers(struct stream *sd)
{
	int ret = 0;
	mpg123_string line;
	mpg123_init_string(&line);
	mpg123_string icyint;
	mpg123_init_string(&icyint);
	const char   *head[] = { "content-type",        "icy-name",        "icy-url",        "icy-metaint" };
	mpg123_string *val[] = { &sd->htd.content_type, &sd->htd.icy_name, &sd->htd.icy_url, &icyint };
	int hn = sizeof(head)/sizeof(char*);
	int hi = -1;
	int got_ok = 0;
	int got_line = 0;
	debug("parsing headers");
	while(stream_getline(sd, &line) > 0)
	{
		got_line = 1;
		mdebug("HTTP in: %s", line.p);
		if(line.p[0] == 0)
		{
			break; // This is the content separator line.
		}
		// React to HTTP error codes, but do not enforce an OK being sent as Shoutcast
		// only produces very minimal headers, not even a HTTP response code.
		if(!strncasecmp("http/", line.p, 5))
		{
			// HTTP/1.1 200 OK
			char *tok = line.p;
			while(*tok && *tok != ' ' && *tok != '\t')
				++tok;
			while(*tok && (*tok == ' ' || *tok == '\t'))
				++tok;
			if(tok && *tok != '2')
			{
				merror("HTTP error response: %s", line.p);
				ret = -1;
				break;
			} else if(tok && *tok == '2')
			{
				if(param.verbose > 2)
					fprintf(stderr, "Note: got a positive HTTP response\n");
				got_ok = 1;
			}
		}
		if(hi >= 0 && (line.p[0] == ' ' || line.p[0] == '\t'))
		{
			debug("header continuation");
			// nh continuation line, appending to already stored value.
			char *v = line.p+1;
			while(*v == ' ' || *v == '\t'){ ++v; }
			if(!mpg123_add_string(val[hi], v))
			{
				merror("failed to grow header value for %s", head[hi]);
				hi = -1;
				continue;
			}
		}
		char *n = line.p;
		char *v = strchr(line.p, ':');
		if(!v)
			continue; // No proper header line.
		// Got a header line.
		*v = 0; // Terminate the header name.
		if(param.verbose > 2)
			fprintf(stderr, "Note: got header: %s\n", n);
		++v; // Value starts after : and whitespace.
		while(*v == ' ' || *v == '\t'){ ++v; }
		for(hi = 0; hi<hn; ++hi)
		{
			if(!strcasecmp(n, head[hi]))
				break;
		}
		if(hi == hn)
		{
			debug("skipping uninteresting header");
			hi = -1;
			continue;
		}
		if(param.verbose > 2)
			fprintf(stderr, "Note: storing HTTP header %s: %s\n", head[hi],  v);
		got_ok = 1; // When we got some header to store, things seem fine.
		if(!mpg123_set_string(val[hi], v))
		{
			error("failed to allocate header value storage");
			hi = -1;
			continue;
		}
	}
	if(icyint.fill)
	{
		sd->htd.icy_interval = atol(icyint.p);
		if(param.verbose > 1)
			fprintf(stderr, "Info: ICY interval %li\n", (long)sd->htd.icy_interval);
	}
	if(!got_line)
	{
		error("no data at all from network resource");
		ret = -1;
	} else if(!got_ok)
	{
		error("missing positive server response");
		ret = -1;
	}
	mpg123_free_string(&icyint);
	mpg123_free_string(&line);
	return ret;
}

static void stream_init(struct stream *sd)
{
	sd->bufp = sd->buf;
	sd->fill = 0;
	sd->network = 0;
	sd->fd = -1;
#ifdef NET123
	sd->nh = NULL;
#endif
	httpdata_init(&sd->htd);
}

struct stream *stream_open(const char *url)
{
	struct stream *sd = malloc(sizeof(struct stream));
	if(!sd)
		return NULL;
	stream_init(sd);
	mdebug("opening resource %s", url);
	if(!strcmp(url, "-"))
	{
		sd->fd = STDIN_FILENO;
		compat_binmode(STDIN_FILENO, TRUE);
	}
#ifdef NET123
	else if(!strncasecmp("http://", url, 7) || !strncasecmp("https://", url, 8))
	{
		sd->network = 1;
		// Network stream with header parsing.
		const char *client_head[] = { NULL, NULL, NULL };
		client_head[0] = param.talk_icy ? icy_yes : icy_no;
		mpg123_string accept;
		mpg123_init_string(&accept);
		append_accept(&accept);
		client_head[1] = accept.p;
		sd->nh = net123_open(url, client_head);
		if(!sd->nh || stream_parse_headers(sd))
		{
			stream_close(sd);
			return NULL;
		}
	}
#elif defined(NETWORK)
	else if(!strncasecmp("http://", url, 7))
	{
#ifdef WANT_WIN32_SOCKETS
		sd->fd = win32_net_http_open(url, &sd->htd);
#else
		sd->fd = http_open(url, &sd->htd);
#endif
		if(sd->fd < 0)
		{
			stream_close(sd);
			return NULL;
		}
	}
#endif
	else
	{
		// plain file access
		if(!strncasecmp("file://", url, 7))
			url+= 7; // Might be useful to prepend file scheme prefix for local stuff.
		errno = 0;
		sd->fd = compat_open(url, O_RDONLY|O_BINARY);
		if(sd->fd < 0)
		{
			merror("failed to open file: %s: %s", url, strerror(errno));
			stream_close(sd);
			return NULL;
		}
	}
	return sd;
}

void stream_close(struct stream *sd)
{
	if(!sd)
		return;
#ifdef NET123
	if(sd->nh)
		net123_close(sd->nh);
#endif
#ifdef WANT_WIN32_SOCKETS
	if(sd->fd >= 0 && sd->network)
	{
		if(sd->fd != SOCKET_ERROR)
			win32_net_close(sd->fd);
	}
#endif
	if(sd->fd >= 0) // plain file or network socket
		close(sd->fd);
	httpdata_free(&sd->htd);
	free(sd);
}

/* Read data from input, write copy to dump file. */
static ssize_t dump_read(void *handle, void *buf, size_t count)
{
	struct stream *sd = handle;
	ssize_t ret = stream_read(sd, buf, count);
	if(ret > 0 && dump_fd > -1)
	{
		ret = unintr_write(dump_fd, buf, ret);
	}
	return ret;
}

/* Also mirror seeks, to prevent messed up dumps of seekable streams. */
static off_t dump_seek(void *handle, off_t pos, int whence)
{
	struct stream *sd = handle;
	off_t ret = stream_seek(sd, pos, whence);
	if(ret >= 0 && dump_fd > -1)
	{
		ret = lseek(dump_fd, pos, whence);
	}
	return ret;
}

/* External API... open and close. */
int dump_setup(struct stream *sd, mpg123_handle *mh)
{
	int ret = MPG123_OK;
	int do_replace = 0; // full replacement with handle

	// paranoia: if buffer active, ensure handle I/O
	if(sd->fill)
		do_replace = 1;
#ifdef NET123
	if(sd->nh)
		do_replace = 1;
#endif
	if(param.streamdump)
	{
		do_replace = 1;
		// open freshly or keep open
		if(dump_fd < 0)
		{
			if(!param.quiet)
				fprintf(stderr, "Note: Dumping stream to %s\n", param.streamdump);
			dump_fd = compat_open(param.streamdump, O_CREAT|O_TRUNC|O_RDWR);
		}
		if(dump_fd < 0)
		{
			error1("Failed to open dump file: %s\n", strerror(errno));
			return -1;
		}
#ifdef WIN32
		_setmode(dump_fd, _O_BINARY);
#endif
	}

	if( MPG123_OK != mpg123_param(mh, MPG123_ICY_INTERVAL
	,	param.icy_interval ? param.icy_interval : sd->htd.icy_interval, 0) )
		error1("Cannot set ICY interval: %s", mpg123_strerror(mh));
	if(param.icy_interval > 0 && param.verbose > 1)
		fprintf(stderr, "Info: Forced ICY interval %li\n", param.icy_interval);

	if(do_replace)
	{
		mpg123_replace_reader_handle(mh, dump_read, dump_seek, NULL);
		ret = mpg123_open_handle(mh, sd);
	} else
	{
#ifdef WANT_WIN32_SOCKETS
		if(sd->network)
			win32_net_replace(mh);
		else // ensure libmpg123 is using its own reader otherwise
#endif
			mpg123_replace_reader(mh, NULL, NULL);
		ret = mpg123_open_fd(mh, sd->fd);
	}
	if(ret != MPG123_OK)
	{
		error1("Unable to replace reader/open track for stream dump: %s\n", mpg123_strerror(mh));
		dump_close(sd);
		return -1;
	}
	else return 0;
}

void dump_close(void)
{
	if(dump_fd > -1) compat_close(dump_fd);

	dump_fd = -1;
}
