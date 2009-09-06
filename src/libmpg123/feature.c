#include "config.h"
#include "mpg123.h"

int mpg123_feature(const enum mpg123_feature_set key)
{
	switch(key)
	{
		case mpg123_feature_utf8open:
		#ifdef WANT_WIN32_UNICODE
		  return 1;
		#else
		  return 0;
		#endif /* WANT_WIN32_UNICODE */
		default: return 0;
	}
}
