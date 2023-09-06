#ifndef MPG123_INTSYM_H
#define MPG123_INTSYM_H
// Used to map lots of internal symbols to prefixed names to help static linking,
// reduciton potential naming conflicts. Now only stays here to redirect strerror
// if it does not exist in the system. Might just drop that, though.
// Probably it needs to be replaced by strerror_r() or strerror_s().
#include "config.h"

#ifndef HAVE_STRERROR
#define strerror INT123_strerror
#endif

#endif
