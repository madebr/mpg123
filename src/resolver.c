/*
	resolver.c: TCP network stuff, for IPv4 and IPv6
	Oh, and also some URL parsing... extracting host name and such.

	copyright 2008 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written Thomas Orgis (based on httpget.c)

	The idea is to have everything involving the game between URLs and IPs/connections separated here.
	I begin with the outsourcing of IPv4 stuff, then make the stuff generic.
*/

#include "mpg123app.h"

#ifdef NETWORK
#include "true.h"
#include "resolver.h"
#include <netdb.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include "debug.h"

int split_url(mpg123_string *url, mpg123_string *auth, mpg123_string *host, mpg123_string *port, mpg123_string *path)
{
	size_t pos  = 0; /* current position in input URL */
	size_t pos2 = 0; /* another position in input URL */
	char *part  = NULL; /* a part of url we work on */
	int ret = TRUE; /* return code */
	/* Zeroing the output strings; not freeing to avoid unnecessary mallocs. */
	if(auth) auth->fill = 0;
	if(host) host->fill = 0;
	if(port) port->fill = 0;
	if(path) path->fill = 0;

	if(!url || !url->fill || url->p[url->fill-1] != 0)
	{
		error("URL string is not good! (Programmer's fault!?)");
		return FALSE;
	}
	if (!(strncmp(url->p+pos, "http://", 7)))
	pos += 7; /* Drop protocol. */

	/* Extract user[:passwd]@... */
	if( (part = strchr(url->p+pos,'@')) )
	{
		size_t i;
		size_t partlen = part - url->p - pos;
		int have_auth = TRUE;
		/* User names or passwords don't have "/" in them (?), the "@" wasn't for real if we find such. */
		for(i=0;i<partlen;i++)
		{
			if(url->p[pos+i] == '/' )
			{
				have_auth = FALSE;
				break;
			}
		}
		if(have_auth)
		{
			if(auth != NULL && !mpg123_set_substring(auth, url->p, pos, partlen))
			{
				error("Cannot set auth string (out of mem?).");
				return FALSE;
			}
			pos += partlen+1; /* Continuing after the "@". */
		}
	}
	
	/* Extract host name or IP. */
#ifdef IPV6
	if(url->p[pos] == '[')
	{ /* It's possibly an IPv6 url in [ ] */
		++pos;
		if( (part = strchr(url->p+pos,']')) != NULL)
		{
			pos2 = part-url->p;
		}
		else { error("Malformed IPv6 URL!"); return FALSE; }
	}
	else
#endif
	for(pos2=pos; pos2 < url->fill-1; ++pos2)
	{
		char a = url->p[pos2];
		if( a == ':' || a == '/') break;
	}
	/* At pos2 there is now either a delimiter or the end. */
debug4("hostname between %lu and %lu, %lu chars of %s", (unsigned long)pos, (unsigned long)pos2, (unsigned long)(pos2-pos), url->p + pos);
	if(host != NULL && !mpg123_set_substring(host, url->p, pos, pos2-pos))
	{
		error("Cannot set host string (out of mem?).");
		return FALSE;
	}
	pos = pos2;

	/* Now for the port... */
	if(url->p[pos] == ':')
	{
		pos += 1; /* We begin _after_ the ":". */
		for(pos2=pos; pos2 < url->fill-1; ++pos2)
		if( url->p[pos2] == '/' ) break;

		/* Check for port being numbers? Not sure if that's needed. */
		if(port) ret = mpg123_set_substring(port, url->p, pos, pos2-pos);
		pos = pos2;
	}
	else if(port) ret = mpg123_set_string(port, "80");

	if(!ret)
	{
		error("Cannot set port string (out of mem?).");
		return FALSE;
	}

	/* Now only the path is left. */
	if(path) ret = mpg123_set_substring(path, url->p, pos, url->fill-1-pos);

	if(!ret) error("Cannot set path string (out of mem?)");

	return ret;
}

/* So, this then is the only routine that should know about IPv4 or v6 in future. */
int open_connection(mpg123_string *host, mpg123_string *port)
{
#ifndef IPV6 /* The legacy code for IPv4. No real change to keep all compatibility. */
#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif
	struct sockaddr_in server;
	struct hostent *myhostent;
	struct in_addr myaddr;
	int isip = 1;
	char *cptr = host->p;
	int sock = -1;
	if(param.verbose>1) fprintf(stderr, "Note: Attempting connection to %s\n", host->p);
	/* Resolve to IP; parse port number. */
	while(*cptr) /* Iterate over characters of hostname, check if it's an IP or name. */
	{
		if ((*cptr < '0' || *cptr > '9') && *cptr != '.')
		{
			isip = 0;
			break;
		}
		cptr++;
	}
	if(!isip)
	{ /* Name lookup. */
		if (!(myhostent = gethostbyname(host->p))) return -1;

		memcpy (&myaddr, myhostent->h_addr, sizeof(myaddr));
		server.sin_addr.s_addr = myaddr.s_addr;
	}
	else  /* Just use the IP. */
	if((server.sin_addr.s_addr = inet_addr(host->p)) == INADDR_NONE)
	return -1;

	server.sin_port = htons(atoi(port->p));
	server.sin_family = AF_INET;

	if((sock = socket(PF_INET, SOCK_STREAM, 6)) < 0)
	{
		error1("Cannot create socket: %s", strerror(errno));
		return -1;
	}
	if(connect(sock, (struct sockaddr *)&server, sizeof(server)))
	{
		error1("Cannot connect to server: %s", strerror(errno));
		return -1;
	}
#else /* Host lookup and connection in a protocol independent manner. */
	struct addrinfo hints;
	struct addrinfo *addr, *addrlist;
	int addrcount, sock = -1;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family   = AF_UNSPEC; /* We accept both IPv4 and IPv6 ... and perhaps IPv8;-) */
	hints.ai_socktype = SOCK_STREAM;

	addrcount = getaddrinfo(host->p, port->p, &hints, &addrlist);

	if(addrcount <0)
	{
		error3("Resolving %s:%s: %s", host->p, port->p, gai_strerror(addrcount));
		return -1;
	}

	addr = addrlist;
	while(addr != NULL)
	{
		sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
		if(sock >= 0)
		{
			if(connect(sock, addr->ai_addr, addr->ai_addrlen) == 0)
			break;

			close(sock);
			sock=-1;
		}
		addr=addr->ai_next;
	}
	if(sock < 0) error2("Cannot resolve %s:%s!", host->p, port->p);

	freeaddrinfo(addrlist);
#endif
	return sock; /* Hopefully, that's an open socket to talk with. */
}

#else /* NETWORK */

void resolver_dummy_without_sense()
{
	/* Some compilers don't like empty source files. */
}

#endif
