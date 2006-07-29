/*
	httpget.c: http communication

	copyright ?-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.de
	initially written Oliver Fromme
	old timestamp: Wed Apr  9 20:57:47 MET DST 1997

	Thomas' notes:
	
	I used to do 
	GET http://server/path HTTP/1.0

	But RFC 1945 says: The absoluteURI form is only allowed when the request is being made to a proxy.

	so I should not do that. Since name based virtual hosts need the hostname in the request, I still need to provide that info.
	Enter HTTP/1.1... there is a Host eader field to use (that mpg123 supposedly has used since some time anyway - but did it really work with my vhost test server)?
	Now
	GET /path/bla HTTP/1.1\r\nHost: host[:port]
	Should work, but as a funny sidenote:
	
	RFC2616: To allow for transition to absoluteURIs in all requests in future versions of HTTP, all HTTP/1.1 servers MUST accept the absoluteURI form in requests, even though HTTP/1.1 clients will only generate them in requests to proxies.
	
	I was already full-on HTTP/1.1 as I recognized that mpg123 then would have to accept the chunked transfer encoding.
	That is not desireable for its purpose... maybe when interleaving of shoutcasts with metadata chunks is supported, we can upgrade to 1.1.
	Funny aspect there is that shoutcast servers do not do HTTP/1.1 chunked transfer but implement some different chunking themselves...
*/

#include "config.h"
#if !defined(WIN32) && !defined(GENERIC)

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <netdb.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <ctype.h>

#include "config.h"
#include "mpg123.h"
#include "debug.h"

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif

void writestring (int fd, char *string)
{
	int result, bytes = strlen(string);

	while (bytes) {
		if ((result = write(fd, string, bytes)) < 0 && errno != EINTR) {
			perror ("write");
			exit (1);
		}
		else if (result == 0) {
			fprintf (stderr, "write: %s\n",
				"socket closed unexpectedly");
			exit (1);
		}
		string += result;
		bytes -= result;
	}
}

int readstring (char *string, int maxlen, FILE *f)
{
#if 0
	char *result;
#endif
	int pos = 0;

	while(pos < maxlen) {
		if( read(fileno(f),string+pos,1) == 1) {
			pos++;
			if(string[pos-1] == '\n') {
				break;
			}
		}
		else if(errno != EINTR) {
			fprintf (stderr, "Error reading from socket or unexpected EOF.\n");
			exit(1);
		}
	}

	string[pos] = 0;

	return pos;

#if 0
	do {
		result = fgets(string, maxlen, f);
	} while (!result  && errno == EINTR);
	if (!result) {
		fprintf (stderr, "Error reading from socket or unexpected EOF.\n");
		exit (1);
	}
#endif

}

void encode64 (char *source,char *destination)
{
  static char *Base64Digits =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  int n = 0;
  int ssiz=strlen(source);
  int i;

  for (i = 0 ; i < ssiz ; i += 3) {
    unsigned int buf;
    buf = ((unsigned char *)source)[i] << 16;
    if (i+1 < ssiz)
      buf |= ((unsigned char *)source)[i+1] << 8;
    if (i+2 < ssiz)
      buf |= ((unsigned char *)source)[i+2];

    destination[n++] = Base64Digits[(buf >> 18) % 64];
    destination[n++] = Base64Digits[(buf >> 12) % 64];
    if (i+1 < ssiz)
      destination[n++] = Base64Digits[(buf >> 6) % 64];
    else
      destination[n++] = '=';
    if (i+2 < ssiz)
      destination[n++] = Base64Digits[buf % 64];
    else
      destination[n++] = '=';
  }
  destination[n++] = 0;
}

/* VERY  simple auth-from-URL grabber */
int getauthfromURL(char *url,char *auth, size_t authlen)
{
  char *pos;

  *auth = 0;

  if (!(strncmp(url, "http://", 7)))
    url += 7;

  if( (pos = strchr(url,'@')) ) {
    int i;
    for(i=0;i<pos-url;i++) {
      if( url[i] == '/' )
         return 0;
    }
    if (pos-url >= authlen) {
      fprintf (stderr, "Error: authentication data exceeds max. length.\n");
      return -1;
    }
    strncpy(auth,url,pos-url);
    auth[pos-url] = 0;
    memmove(url,pos+1,strlen(pos+1)+1);
    return 1;
  }
  return 0;
}

char *url2hostport (char *url, char **hname, unsigned long *hip, unsigned int *port)
{
	char *cptr;
	struct hostent *myhostent;
	struct in_addr myaddr;
	int isip = 1;

	if (!(strncmp(url, "http://", 7)))
		url += 7;
	cptr = url;
	while (*cptr && *cptr != ':' && *cptr != '/') {
		if ((*cptr < '0' || *cptr > '9') && *cptr != '.')
			isip = 0;
		cptr++;
	}
	*hname = strdup(url); /* removed the strndup for better portability */
	if (!(*hname)) {
		*hname = NULL;
		return (NULL);
	}
	(*hname)[cptr - url] = 0;
	if (!isip) {
		if (!(myhostent = gethostbyname(*hname)))
			return (NULL);
		memcpy (&myaddr, myhostent->h_addr, sizeof(myaddr));
		*hip = myaddr.s_addr;
	}
	else
		if ((*hip = inet_addr(*hname)) == INADDR_NONE)
			return (NULL);
	if (!*cptr || *cptr == '/') {
		*port = 80;
		return (cptr);
	}
	*port = atoi(++cptr);
	if (*port > 65535) {
		/* Illegal port number. Ignore and use default. */
		*port = 80;
	}
	while (*cptr && *cptr != '/')
		cptr++;
	return (cptr);
}

char *proxyurl = NULL;
unsigned long proxyip = 0;
unsigned int proxyport;

/* That is not real ... I should really check the type of what I get! */
#define ACCEPT_HEAD "Accept: audio/mpeg, audio/x-mpegurl, audio/x-scpls, */*\r\n"
/* needed for HTTP/1.1 non-pipelining mode */
/* #define CONN_HEAD "Connection: close\r\n" */
#define CONN_HEAD ""

char *httpauth = NULL;
char *httpauth1 = NULL;

int http_open (char* url, char** content_type)
{
	/* TODO: make sure ulong vs. size_t is really clear! */
	/* TODO: change this whole thing until I stop disliking it */
	char *purl, *host, *request, *response, *sptr;
	char* request_url = NULL;
	size_t request_url_size = 0;
	size_t purl_size;
	size_t linelength, linelengthbase, tmp;
	unsigned long myip = 0;
	unsigned int myport = 0;
	int sock;
	int relocate, numrelocs = 0;
	struct sockaddr_in server;
	FILE *myfile;

	host = NULL;
	purl = NULL;
	request = NULL;
	response = NULL;
	if (!proxyip) {
		if (!proxyurl)
			if (!(proxyurl = getenv("MP3_HTTP_PROXY")))
				if (!(proxyurl = getenv("http_proxy")))
					proxyurl = getenv("HTTP_PROXY");
		if (proxyurl && proxyurl[0] && strcmp(proxyurl, "none")) {
			if (!(url2hostport(proxyurl, &host, &proxyip, &proxyport))) {
				fprintf (stderr, "Unknown proxy host \"%s\".\n",
					host ? host : "");
				sock = -1;
				goto exit;
			}
		}
		else
			proxyip = INADDR_NONE;
	}
	
	/* The length of purl is upper bound by 3*strlen(url) + 1 if
	 * everything in it is a space (%20) - or any encoded character */
	if (strlen(url) >= SIZE_MAX/3) {
		fprintf (stderr, "URL too long. Skipping...\n");
		sock = -1;
		goto exit;
	}
	/* +1 since we may want to add / if needed */
	purl_size = strlen(url)*3 + 1 + 1;
	purl = (char *)malloc(purl_size);
	if (!purl) {
		fprintf (stderr, "malloc() failed, out of memory.\n");
		exit (1);
	}
	/* make _sure_ that it is null-terminated */
	purl[purl_size-1] = 0;
	
	/*
	 * 2000-10-21:
	 * We would like spaces to be automatically converted to %20's when
	 * fetching via HTTP.
	 * -- Martin Sjögren <md9ms@mdstud.chalmers.se>
	 * Hm, why only spaces? Maybe one should do this http stuff more properly...
	 */
	if ((sptr = strchr(url, ' ')) == NULL) {
		strcpy (purl, url);
	} else {
		char *urlptr = url;
		purl[0] = '\0';
		do {
			strncat (purl, urlptr, sptr-urlptr);
			strcat (purl, "%20");
			urlptr = sptr + 1;
		} while ((sptr = strchr (urlptr, ' ')) != NULL);
		strcat (purl, urlptr);
	}
	/* now see if a terminating / may be needed */
	if(strchr(purl+(strncmp("http://", purl, 7) ? 0 : 7), '/') == NULL) strcat(purl, "/");
	
	/* some paranoia */
	if(httpauth1 != NULL) free(httpauth1);
	httpauth1 = (char *)malloc((strlen(purl) + 1));
	if(!httpauth1) {
		fprintf(stderr, "malloc() failed, out of memory.\n");
		exit(1);
	}
        if (getauthfromURL(purl,httpauth1,strlen(purl)) < 0) {
		sock = -1;
		goto exit;
	}

	/* "GET http://"		11
	 * " HTTP/1.0\r\nUser-Agent: <PACKAGE_NAME>/<PACKAGE_VERSION>\r\n"
	 * 				26 + PACKAGE_NAME + PACKAGE_VERSION
	 * ACCEPT_HEAD               strlen(ACCEPT_HEAD)
	 * "Authorization: Basic \r\n"	23
	 * "\r\n"			 2
	 */
	linelengthbase = 62 + strlen(PACKAGE_NAME) + strlen(PACKAGE_VERSION)
	                 + strlen(ACCEPT_HEAD) + strlen(CONN_HEAD);

	if(httpauth) {
		tmp = (strlen(httpauth) + 1) * 4;
		if (strlen(httpauth) >= SIZE_MAX/4 - 1 ||
		    linelengthbase + tmp < linelengthbase) {
			fprintf(stderr, "HTTP authentication too long. Skipping...\n");
			sock = -1;
			goto exit;
		}
		linelengthbase += tmp;
	}

	if(httpauth1) {
		tmp = (strlen(httpauth1) + 1) * 4;
		if (strlen(httpauth1) >= SIZE_MAX/4 - 1 ||
		    linelengthbase + tmp < linelengthbase) {
			fprintf(stderr, "HTTP authentication too long. Skipping...\n");
			sock = -1;
			goto exit;
		}
		linelengthbase += tmp;
	}

	do {
		/* save a full, valid url for later */
		if(request_url_size < (strlen(purl) + 8))
		{
			request_url_size = strlen(purl) + 8;
			if(request_url != NULL) free(request_url);
			request_url = (char*) malloc(request_url_size);
			if(request_url != NULL)	request_url[request_url_size-1] = '\0';
			else
			{
				fprintf(stderr, "malloc() failed, out of memory.\n");
				exit(1);
			}
		}
		/* used to be url here... seemed wrong to me (when loop advanced...) */
		if (strncasecmp(purl, "http://", 7) != 0)	strcpy(request_url, "http://");
		else request_url[0] = '\0';
		strcat(request_url, purl);

		char* ttemp;
		if (proxyip != INADDR_NONE) {
			myport = proxyport;
			myip = proxyip;

			linelength = linelengthbase + strlen(purl);
			if (linelength < linelengthbase) {
				fprintf(stderr, "URL too long. Skipping...\n");
				sock = -1;
				goto exit;
			}

			if(host) {
				tmp = 9 + strlen(host) + 5;
				if (strlen(host) >= SIZE_MAX - 14 ||
				    linelength + tmp < linelength) {
					fprintf(stderr, "Hostname info too long. Skipping...\n");
					sock = -1;
					goto exit;
				}
				/* "Host: <host>:<port>\r\n" */
				linelength += tmp;
			}

			/* Buffer is reused for receiving later on, so ensure
			 * minimum size. */
			linelength = (linelength < 4096) ? 4096 : linelength;
			/* ugly fix for an ugly memory leak */
			if(request != NULL) free(request);
			request = (char *)malloc((linelength + 1));
			if(response != NULL) free(response);
			response = (char *)malloc((linelength + 1));

			if ((request == NULL) || (response == NULL)) {
				fprintf (stderr, "malloc() failed, out of memory.\n");
				exit(1);
			}

			strcpy (request, "GET ");
			strcat(request, request_url);
		}
		else
		{
			if (host) {
				free (host);
				host = NULL;
			}
			sptr = url2hostport(purl, &host, &myip, &myport);
			if (!sptr)
			{
				fprintf (stderr, "Unknown host \"%s\".\n",
				host ? host : "");
				sock = -1;
				goto exit;
			}
			linelength = linelengthbase + strlen(sptr);
			if (linelength < linelengthbase) {
				fprintf(stderr, "URL too long. Skipping...\n");
				sock = -1;
				goto exit;
			}

			if(host) {
				tmp = 9 + strlen(host) + 5;
				if (strlen(host) >= SIZE_MAX - 14 ||
				    linelength + tmp < linelength) {
					fprintf(stderr, "Hostname info too long. Skipping...\n");
					sock = -1;
					goto exit;
				}
				/* "Host: <host>:<port>\r\n" */
				linelength += tmp;
			}
			
			/* Buffer is reused for receiving later on, so ensure
			 * minimum size. */
			linelength = (linelength < 4096) ? 4096 : linelength;
			/* ugly fix for an ugly memory leak */
			if(request != NULL) free(request);
			request = (char *)malloc((linelength + 1));
			if(response != NULL) free(response);
			response = (char *)malloc((linelength + 1));

			if ((request == NULL) || (response == NULL)) {
				fprintf (stderr, "malloc() failed, out of memory.\n");
				exit(1);
			}
			
			strcpy (request, "GET ");
			strcat (request, sptr);
		}
		
		/* hm, my test redirection had troubles with line break before HTTP/1.0 */
		if((ttemp = strchr(request,'\r')) != NULL) *ttemp = 0;
		if((ttemp = strchr(request,'\n')) != NULL) *ttemp = 0;
		sprintf (request + strlen(request),
		         " HTTP/1.0\r\nUser-Agent: %s/%s\r\n",
			 PACKAGE_NAME, PACKAGE_VERSION);
		if (host) {
			debug2("Host: %s:%u", host, myport);
			sprintf(request + strlen(request),
			        "Host: %s:%u\r\n", host, myport);
		}
/*		else
		{
			fprintf(stderr, "Error: No host! This must be an error! My HTTP/1.1 request is invalid.");
		} */
		strcat (request, ACCEPT_HEAD);
		strcat (request, CONN_HEAD);
		server.sin_family = AF_INET;
		server.sin_port = htons(myport);
		server.sin_addr.s_addr = myip;
		if ((sock = socket(PF_INET, SOCK_STREAM, 6)) < 0) {
			perror ("socket");
                       goto exit;
		}
		if (connect(sock, (struct sockaddr *)&server, sizeof(server))) {
			perror ("connect");
                       close(sock);
                       sock = -1;
                       goto exit;
		}

		if (httpauth1 || httpauth) {
			char *buf;
			strcat (request,"Authorization: Basic ");
			if(httpauth1) {
				buf=(char *)malloc((strlen(httpauth1) + 1) * 4);
				if(!buf) {
					fprintf(stderr, "Error allocating sufficient memory for http authentication.  Exiting.");
					exit(1);
				}
				encode64(httpauth1,buf);
				free(httpauth1);
				httpauth1 = NULL;
			} else {
				buf=(char *)malloc((strlen(httpauth) + 1) * 4);
				if(!buf) {
					fprintf(stderr, "Error allocating sufficient memory for http authentication.  Exiting.");
					exit(1);
				}
				encode64(httpauth,buf);
			}

			strcat (request, buf);
			strcat (request,"\r\n");
			free(buf);
		}
		strcat (request, "\r\n");
		
		debug1("<request>\n%s</request>",request);
		writestring (sock, request);
		if (!(myfile = fdopen(sock, "rb"))) {
			perror ("fdopen");
                       close(sock);
                       sock = -1;
                       goto exit;
		}
		relocate = FALSE;
		purl[0] = '\0';
		if (readstring (response, linelength-1, myfile)
		    == linelength-1) {
			fprintf(stderr, "Command exceeds max. length\n");
			close(sock);
			sock = -1;
			goto exit;
		}
		debug1("<response>\n%s</response>",response);
		if ((sptr = strchr(response, ' '))) {
			switch (sptr[1]) {
				case '3':
					relocate = TRUE;
				case '2':
					break;
				default:
                                       fprintf (stderr,
                                                "HTTP request failed: %s",
                                                sptr+1); /* '\n' is included */
                                       close(sock);
                                       sock = -1;
                                       goto exit;
			}
		}
		do {
			if (readstring (response, linelength-1, myfile)
			    == linelength-1) {
				fprintf(stderr, "URL exceeds max. length\n");
				close(sock);
				sock = -1;
				goto exit;
			}
			if (!strncmp(response, "Location: ", 10))
			{
				size_t needed_length;
				debug1("request_url:%s", request_url);
				/* initialized with full old url */
				char* prefix = request_url;
				
				if(strncmp(response, "Location: http://", 17))
				{
					char* ptmp = NULL;
					/* though it's not RFC (?), accept relative URIs as wget does */
					fprintf(stderr, "NOTE: no complete URL in redirect, constructing one\n");
					/* not absolute uri, could still be server-absolute */
					/* I prepend a part of the request... out of the request */
					if(response[10] == '/')
					{
						/* only prepend http://server/ */
						/* I null the first / after http:// */
						if((ptmp = strchr(prefix+7,'/')) != NULL)	ptmp[0] = 0;
					}
					else
					{
						/* prepend http://server/path/ */
						/* now we want the last / */
						ptmp = strrchr(prefix+7, '/');
						if(ptmp != NULL) ptmp[1] = 0;
					}
				}
				else prefix[0] = 0;
				debug1("prefix=%s", prefix);

				/* Isn't C string mangling just evil? ;-) */

				/* we want to allow urls longer than purl */
				/* eh, why *3 here? I don't see it that in this loop any x -> %yz conversion is done */
				needed_length = strlen(prefix) + strlen(response+10)*3+1;
				if(purl_size < needed_length)
				{
					purl_size = needed_length;
					purl = realloc(purl, purl_size);
					if(purl == NULL)
					{
						close(sock);
						sock = -1;
						goto exit;
					}
					/* Why am I always so picky about the trailing zero, nobody else seems to care? */
					purl[purl_size-1] = 0;
				}
				/* now that we ensured that purl is big enough, we can just hit it */
				strcpy(purl, prefix);
				strcat(purl, response+10);
			}
			else
			{
				/* watch out for content type */
				debug1("searching for content-type... %s", response);
				if(!strncasecmp("content-type:", response, 13))
				{
					if(content_type != NULL)
					{
						char *tmp = NULL;
						if((tmp = strchr(response, '\r')) != NULL ) tmp[0] = 0;
						if((tmp = strchr(response, '\n')) != NULL ) tmp[0] = 0;
						size_t len = strlen(response)-13;
						tmp = response+13;
						while(len && ((tmp[0] == ' ') || (tmp[0] == '\t')))
						{
							++tmp;
							--len;
						}
						if(len)
						{
							if(*content_type != NULL) free(*content_type);
							*content_type = (char*) malloc(len+1);
							if(*content_type != NULL)
							{
								strncpy(*content_type, tmp, len);
								(*content_type)[len] = 0;
								debug1("got type %s", *content_type);
							}
							else fprintf(stderr, "Error: canno allocate memory for content type!\n");
						}
					}
				}
			}
		} while (response[0] != '\r' && response[0] != '\n');
	} while (relocate && purl[0] && numrelocs++ < 10);
	if (relocate) {
		fprintf (stderr, "Too many HTTP relocations.\n");
               close(sock);
               sock = -1;
	}
exit:
	if(host != NULL) free(host);
	if(purl != NULL) free(purl);
	if(request != NULL) free(request);
	if(response != NULL) free(response);
	if(request_url != NULL) free(request_url);
	return sock;
}

#else /* defined(WIN32) || defined(GENERIC) */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern int errno;

#include "mpg123.h"

/* stubs for Win32 */

void writestring (int fd, char *string)
{
}

int readstring (char *string, int maxlen, FILE *f)
{
}

char *url2hostport (char *url, char **hname, unsigned long *hip, unsigned int *port)
{
}

char *proxyurl = NULL;
unsigned long proxyip = 0;
unsigned int proxyport;

#define ACCEPT_HEAD "Accept: audio/mpeg, audio/x-mpegurl, */*\r\n"

int http_open (char *url)
{
}
#endif

/* EOF */

