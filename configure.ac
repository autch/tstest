#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.67])
AC_INIT([tstest], [0.001], [autch@autch.net])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_SRCDIR([mpegts_types.h])
AC_CONFIG_HEADERS([config.h])

AC_SYS_LARGEFILE

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h memory.h stdint.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_INT64_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memchr memset strchr])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
