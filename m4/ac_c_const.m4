# The AC_C_CONST from autoconf-2.61 is still broken with missing initialization.
# gcc -Wall -Werror -pedantic fails with:
# conftest.c: In function 'main':
# conftest.c:39: warning: 't' is used uninitialized in this function
# conftest.c:55: warning: 'b' is used uninitialized in this function
# conftest.c:61: warning: 'cs[0]' is used uninitialized in this function
# Fixing that locally for now... of course I am not sure if this breakes some test for weird compiler bugs... but it is a nasty showstopper when one has careful CFLAGS.

# AC_C_CONST
# ----------
AN_IDENTIFIER([const],  [AC_C_CONST])
AC_DEFUN([AC_C_CONST],
[AC_CACHE_CHECK([for an ANSI C-conforming const], ac_cv_c_const,
[AC_COMPILE_IFELSE([AC_LANG_PROGRAM([],
[[/* FIXME: Include the comments suggested by Paul. */
#ifndef __cplusplus
  /* Ultrix mips cc rejects this.  */
  typedef int charset[2];
  const charset cs = {0,0}; /* ThOr: initialized */
  /* SunOS 4.1.1 cc rejects this.  */
  char const *const *pcpcc;
  char **ppc;
  /* NEC SVR4.0.2 mips cc rejects this.  */
  struct point {int x, y;};
  static struct point const zero = {0,0};
  /* AIX XL C 1.02.0.0 rejects this.
     It does not let you subtract one const X* pointer from another in
     an arm of an if-expression whose if-part is not a constant
     expression */
  const char *g = "string";
  pcpcc = &g + (g ? g-g : 0);
  /* HPUX 7.0 cc rejects these. */
  ++pcpcc;
  ppc = (char**) pcpcc;
  pcpcc = (char const *const *) ppc;
  { /* SCO 3.2v4 cc rejects this.  */
    char *t = ""; /* ThOr: initialized */
    char const *s = 0 ? (char *) 0 : (char const *) 0;

    *t++ = 0;
    if (s) return 0;
  }
  { /* Someone thinks the Sun supposedly-ANSI compiler will reject this.  */
    int x[] = {25, 17};
    const int *foo = &x[0];
    ++foo;
  }
  { /* Sun SC1.0 ANSI compiler rejects this -- but not the above. */
    typedef const int *iptr;
    iptr p = 0;
    ++p;
  }
  { /* AIX XL C 1.02.0.0 rejects this saying
       "k.c", line 2.27: 1506-025 (S) Operand must be a modifiable lvalue. */
    struct s { int j; const int *ap[3]; };
    struct s *b = 0; b->j = 5; /* ThOr: initialized */
  }
  { /* ULTRIX-32 V3.1 (Rev 9) vcc rejects this */
    const int foo = 10;
    if (!foo) return 0;
  }
  return !cs[0] && !zero.x;
#endif
]])],
		   [ac_cv_c_const=yes],
		   [ac_cv_c_const=no])])
if test $ac_cv_c_const = no; then
  AC_DEFINE(const,,
	    [Define to empty if `const' does not conform to ANSI C.])
fi
])# AC_C_CONST
