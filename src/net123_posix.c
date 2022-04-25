/*
	net123_posix: network (HTTP(S)) streaming for mpg123 via fork+exec

	This avoids linking any network code directly into mpg123, just using external
	tools at runtime.

	I will start with hardcoded wget, will add curl and some parameters later.
*/


#include "config.h"
#include "net123.h"
// for strings
#include "mpg123.h"

// Just for parameter struct that we use for HTTP auth and proxy info.
#include "mpg123app.h"

#include "compat.h"

#include "debug.h"
#include <sys/types.h>
#include <sys/wait.h>
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
	int fd;
	pid_t worker;
};

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

net123_handle *net123_open(const char *url, const char * const * client_head)
{
	int fd[2];
	int hi = -1; // index of header value that might get a continuation line
	net123_handle *nh = malloc(sizeof(net123_handle));
	if(!nh)
		return NULL;
	nh->fd = -1;
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
		dup2(fd[1], STDOUT_FILENO);
		int infd  = open("/dev/null", O_RDONLY);
		dup2(infd,  STDIN_FILENO);
		// child
		// Construct command line, this needs 
		// Proxy environment variables can just be set in the user and inherited here, right?
		const char* wget_args[] =
		{
			"wget" // begins with program name
		,	"--output-document=-"
#ifndef DEBUG
		,	"--quiet"
#endif
		,	"--save-headers"
		};
		size_t cheads = 0;
		while(client_head && client_head[cheads]){ ++cheads; }
		// Get the count of argument strings right!
		// Fixed args + agent + client headers [+ auth] + URL + NULL
		size_t argc = sizeof(wget_args)/sizeof(char*)+1+cheads+1+1;
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
		argv[an++] = compat_strdup("--user-agent=" PACKAGE_NAME "/" PACKAGE_VERSION);
		for(size_t ch=0; ch < cheads; ++ch)
			argv[an++] = catstr("--header=", client_head[ch]);
		if(user)
			argv[an++] = catstr("--user=", user);
		if(password)
			argv[an++] = catstr("--password=", password);
		argv[an++] = compat_strdup(url);
		argv[an++] = NULL;
		errno = 0;
		if(param.verbose > 2)
		{
			char **a = argv;
			fprintf(stderr, "HTTP helper command:\n");
			while(*a)
			{
				fprintf(stderr, " %s\n", *a);
				++a;
			}
		}
#ifndef DEBUG
		int errfd = open("/dev/null", O_WRONLY);
		dup2(errfd, STDERR_FILENO);
#endif
		execvp(argv[0], argv);
		merror("cannot execute %s: %s", argv[0], strerror(errno));
		exit(1);
	}
	// parent
	if(param.verbose > 1)
		fprintf(stderr, "Note: started network helper with PID %"PRIiMAX"\n", (intmax_t)nh->worker);
	errno = 0;
	close(fd[1]);
	nh->fd = fd[0];
	return nh;
}

size_t net123_read(net123_handle *nh, void *buf, size_t bufsize)
{
	if(!nh || (bufsize && !buf))
		return 0;
	return unintr_read(nh->fd, buf, bufsize);
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
		else if(param.verbose > 1)
			fprintf(stderr, "Note: network helper %"PRIiMAX" finished\n", (intmax_t)nh->worker);
	}
	if(nh->fd > -1)
		close(nh->fd);
	free(nh);
}

