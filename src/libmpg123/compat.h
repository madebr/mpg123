/* compat: compatibility stuff */

/* realloc, size_t */
#include <stdlib.h>

void *safe_realloc(void *ptr, size_t size);
#ifndef HAVE_STRERROR
const char *strerror(int errnum);
#endif
