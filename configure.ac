#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT(CVM, 0.0.1, NULL)
AM_INIT_AUTOMAKE(CVM, 0.0.1)
LT_INIT()
AC_OUTPUT(Makefile src/Makefile tests/Makefile)
AC_CONFIG_SRCDIR([src/errors.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB(check, tcase_name, [AC_DEFINE(HAVE_TCASE_NAME, 1, [Define to 1 if libcheck has tcase_name function])])
AC_SEARCH_LIBS(GC_malloc, gc, [], AC_MSG_ERROR([library 'gc' is required. See http://www.hboehm.info/gc/]))


# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC

AC_PROG_RANLIB

PKG_CHECK_MODULES([CHECK], [check >= 0.9.4])

AC_OUTPUT
