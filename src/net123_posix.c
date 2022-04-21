/*
	net123_posix: network (HTTP(S)) streaming for mpg123 via fork+exec

	This avoids linking any network code directly into mpg123, just using external
	tools at runtime.

	I will start with hardcoded wget, will add curl and some parameters later.
*/


#include "config.h"
#include "net123.h"

// Just for parameter struct that we use for HTTP auth and proxy info.
#include "mpg123app.h"

#include "compat.h"
#include "debug.h"
#include <sys/types.h>
#include <unistd.h>

// Those are set via environment variables:
// http_proxy
// https_proxy
//    If set, the http_proxy and https_proxy variables should contain the
//    URLs of the proxies for HTTP and HTTPS connections respectively.
// ftp_proxy
//    same
// wget --user=... --password=... 
// Alternatively: Have them in .netrc.

struct net123_handle_struct
{
	FILE *pipept;
	pid_t worker;
};

// Zero line end in given string, return 0 if one was found, -1 if not.
static int chomp(char *line, size_t n)
{
	int gotend = 0;
	for(size_t i=0; i<sizeof(line) && line[i]; ++i)
	{
		if(line[i] == '\r' || line[i] == '\n')
		{
			line[i] = 0;
			return 0;
		}
	}
	error("skipping excessively long header");
	return -1;
}

// Combine two given strings into one newly allocated one.
// Use: (--parameter=, value) -> --parameter=value
static char *catstr(const char *par, const char *value)
{
	char *res = malloc(strlen(par)+strlen(value)+1);
	if(res)
	{
		res[0] = 0;
		strcat(res, par);
		strcat(res, value);
	}
	return res;
}

net123_handle *net123_open(const char *url, const char * const * client_head, const char * const *head, char **val)
{
	char line[4096];
	int fd[2];
	int hi = -1; // index of header value that might get a continuation line
	net123_handle *nh = malloc(sizeof(net123_handle));
	if(!nh)
		return NULL;
	nh->pipept = NULL;
	nh->worker = 0;
	errno = 0;
	if(pipe(fd))
	{	
		merror("failed creating a pipe: %s", strerror(errno));
		free(nh);
		return NULL;
	}

	nh->worker = fork();
	if(nh->worker == -1)
	{
		merror("fork failed: %s", strerror(errno));
		free(nh);
		return NULL;
	}

	if(nh->worker == 0)
	{
		close(fd[0]);
		int errfd = open("/dev/null", O_WRONLY);
		int infd  = open("/dev/null", O_RDONLY);
		dup2(intfd, STDIN_FILENO);
		dup2(fd[1], STDOUT_FILENO);
		dup2(errfd, STDERR_FILENO);
		// child
		// Construct command line, this needs 
		// Proxy environment variables can just be set in the user and inherited here, right?
		const char* wget_args[] =
		{
			"wget" // begins with program name
		,	"--output-document=-"
		,	"--quiet"
		,	"--save-headers"
		};
		size_t cheads = 0;
		while(client_head && client_head[cheads]){ ++cheads; }
		// Get the count of argument strings right!
		// Fixed args + client headers [+ auth] + URL + NULL
		size_t argc = sizeof(wget_args)/sizeof(char*)+cheads+1;
		char *httpauth = NULL;
		char *user = NULL;
		char *password = NULL;
		if(param.httpauth && (httpauth = compat_strdup(param.httpauth)))
		{
			char *sep = strchr(param.httpauth, ':');
			if(sep)
			{
				argc += 2;
				*sep = 0;
				user = httpauth;
				password = sep+1;
			}
		}
		char ** argv = malloc(sizeof(char*)*(argc+1));
		if(!argv)
		{
			error("failed to allocate argv");
			exit(1);
		}
		int an = 0;
		for(;an<sizeof(wget_args)/sizeof(char*); ++an)
			argv[an] = compat_strdup(wget_args[an]);
		argv[an++] = compat_strdup("wget");
		argv[an++] = compat_strdup("--user-agent=" PACKAGE_NAME "/" PACKAGE_VERSION);
		for(size_t ch=0; ch < cheads; ++ch)
			argv[an++] = catstr("--header=", client_header[ch]);
		if(user)
			argv[an++] = catstr("--user=", user);
		if(password)
			argv[an++] = catstr("--password=", password);
		argv[an++] = compat_strdup(url);
		argv[an++] = NULL;
		errno = 0;
		execvp("wget", argv);
		merror("cannot execute wget: %s", strerror(errno));
		exit(1);
	}
	// parent
	errno = 0;
	close(fd[1]);
	nh->pipept = fdopen(fd[0], "r");
	if(!nh->pipept)
	{
		merror("failed to open stream for worker pipe: %s", strerror(errno));
		net123_close(nh);
		free(nh);
		return NULL;
	}
	// fork, exec
	// read headers, end at the empty line
	// use bufferd I/O and getline?
	while(fgets(line, sizeof(line), nh->pipept))
	{
		if(chomp(line, sizeof(line))
		{
			error("skipping excessively long header");
			while(fgets(line, sizeof(line), nh->pipept) && chomp(line, sizeof(line)))
			{
				# skip, skip
			}
			continue;
		}
		if(line[0] == 0)
		{
			break; // This is the content separator line.
		}
		// Only store headers if we got names and storage locations.
		if(!head || !val)
			continue;
		if(hi >= 0 && (line[0] == ' ' || line[0] == '\t'))
		{
			debug("header continuation");
			// nh continuation line, appending to already stored value.
			char *v = line+1;
			while(*v == ' ' || *v == '\t'){ ++v; }
			val[hi] = safer_realloc(val[hi], strlen(val[hi])+strlen(v)+1);
			if(!val[hi])
			{
				error("failed to grow header value for %s", head[hi]);
				hi = -1;
				continue;
			}
			strcat(val[hi], v);
		}
		char *n = line;
		char *v = strchr(line, ':');
		if(!v)
			continue; // No proper header line.
		// Got a header line.
		*v = 0; // Terminate the header name.
		mdebug("got header: %s", n);
		++v; // Value starts after : and whitespace.
		while(*v == ' ' || *v == '\t'){ ++v; }
		for(hi = 0; head[hi] != NULL; ++hi)
		{
			if(!strcasecmp(n, head[hi]))
				break;
		}
		if(head[hi] == NULL)
		{
			debug("skipping uninteresting header");
			hi = -1;
			continue;
		}
		debug("storing value for %s", head[hi]);
		val[hi] = safer_realloc(val[hi], strlen(v)+1);
		if(!val[hi])
		{
			error("failed to allocate header value storage");
			hi = -1;
			continue;
		}
		val[hi][0] = 0;
		strcat(val[hi], v);
	}
	// If we got here, things are somewhat cool.
	// Caller sees trouble already if no headers are returned, or the read doesn't give.
	return nh;
}

int net123_read(net123_handle *nh, void *buf, size_t bufsize, size_t *gotbytes);
{
	if(!nh || (bufsize && !buf))
		return -1;
	size_t got = fread(buf, 1, bufsize, nh->pipept);
	if(gotbytes)
		*gotbytes = got;
	if(!got && ferror(nh->pipept))
		return -1;
	return 0;
}

void net123_close(net123_handle *nh)
{
	if(!nh)
		return;
	if(nh->worker)
	{
		kill(nh->worker, SIGKILL);
		errno = 0;
		if(waitpid(nh->worker, NULL, 0) < 0)
			merror("failed to wait for worker process: %s", strerror(errno));
	}
	if(nh->pipept)
		fclose(nh->pipept);
	free(nh);
}

