dnl
dnl libics: Image Cytometry Standard file reading and writing.
dnl
dnl Copyright (C) 2000-2013, 2016 Cris Luengo and others
dnl Copyright 2015, 2016:
dnl   Scientific Volume Imaging Holding B.V.
dnl   Hilversum, The Netherlands.
dnl   https://www.svi.nl
dnl
dnl Script for automake and autoconfig.
dnl Written by Peter Verveer, Cris Luengo
dnl

dnl Library version number (make sure to also change it in 'libics.h'):
AC_INIT([libics],[1.7.1])
AC_CONFIG_SRCDIR([libics.h])
AC_CONFIG_HEADERS([config.h libics_conf.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE

AM_MAINTAINER_MODE

dnl Library versioning (CURRENT:REVISION:AGE)
dnl See the libtool manual for an explanation of the numbers
dnl
dnl Version history:
dnl ics-1.5.3  libics 0:0:0
dnl
dnl How to update library version number
dnl ====================================
dnl
dnl Update the version information only immediately before a public
dnl release of the software. More frequent updates are unnecessary.
dnl
dnl CURRENT: increment if the interface has additions, changes, removals.
dnl
dnl REVISION: increment any time the source changes; set to 0 if you
dnl incremented CURRENT
dnl
dnl AGE: increment if any interfaces have been added; set to 0 if any
dnl interfaces have been removed. removal has precedence over adding,
dnl so set to 0 if both happened.

ICS_LT_VERSION="0:0:0"
AC_SUBST(ICS_LT_VERSION)

AC_PROG_CC
LT_INIT
m4_warn([obsolete],
[The preprocessor macro `STDC_HEADERS' is obsolete.
  Except in unusual embedded environments, you can safely include all
  ISO C90 headers unconditionally.])dnl
# Autoupdate added the next two lines to ensure that your configure
# script's behavior did not change.  They are probably safe to remove.
AC_CHECK_INCLUDES_DEFAULT
AC_PROG_EGREP

AC_C_CONST
AC_TYPE_SIZE_T

AH_TEMPLATE([HAVE_STRTOK_R], [Define to 1 if the c library provides strtok_r])

# If this variable is not defined, libics_conf.h will revert to the old version.
AC_DEFINE([ICS_USING_CONFIGURE], [], [Using the configure script.])

dnl Check if you want to use .ids.gz and .ids.Z extensions
AC_ARG_ENABLE(gzext, AS_HELP_STRING([--disable-gz-extensions], [disable .ids.gz and .ids.Z extensions (enabled by default)]),,)

dnl Check if we should force the C locale
AC_ARG_ENABLE(c_locale, AS_HELP_STRING([--disable-c-locale], [disable force c locale (enabled by default)]),,)

dnl ---------------------------------------------------------------------------
dnl Check for ZLIB (copied from libtiff, with some additions)
dnl ---------------------------------------------------------------------------

HAVE_ZLIB=no

AC_ARG_ENABLE(zlib, AS_HELP_STRING([--disable-zlib], [disable Zlib usage (required for zip compression, enabled by default)]),,)
AC_ARG_WITH(zlib-include-dir, AS_HELP_STRING([--with-zlib-include-dir=DIR], [location of Zlib headers]),,)
AC_ARG_WITH(zlib-lib-dir, AS_HELP_STRING([--with-zlib-lib-dir=DIR], [location of Zlib library binary]),,)

if test "x$enable_zlib" != "xno" ; then

  if test "x$with_zlib_lib_dir" != "x" ; then
    LIBS="-L$with_zlib_lib_dir $LIBS"
  fi
  AC_CHECK_LIB(z, inflateEnd, [zlib_lib=yes], [zlib_lib=no],)
  if test "$zlib_lib" = "no" -a "x$with_zlib_lib_dir" != "x"; then
    AC_MSG_ERROR([Zlib library not found at $with_zlib_lib_dir])
  fi

  if test "x$with_zlib_include_dir" != "x" ; then
    CPPFLAGS="-I$with_zlib_include_dir $CPPFLAGS"
  fi
  AC_CHECK_HEADER(zlib.h, [zlib_h=yes], [zlib_h=no])
  if test "$zlib_h" = "no" -a "x$with_zlib_include_dir" != "x" ; then
    AC_MSG_ERROR([Zlib headers not found at $with_zlib_include_dir])
  fi

  if test "$zlib_lib" = "yes" -a "$zlib_h" = "yes" ; then
    HAVE_ZLIB=yes
  fi

fi

if test "$HAVE_ZLIB" = "yes" ; then
  AC_DEFINE(ICS_ZLIB, 1, [Whether to use zlib compression.])
  LIBS="-lz $LIBS"
fi
AM_CONDITIONAL([ICS_ZLIB], [test "x$HAVE_ZLIB" = "xyes"])

if test "x$enable_gz_extensions" != "xno" ; then
  AC_DEFINE(ICS_DO_GZEXT, 1, [Whether to search for files with .ids.gz or .ids.Z extension.])
fi
AM_CONDITIONAL([ICS_DO_GZEXT], [test "x$enable_gz_extensions" != "xno"])

if test "x$enable_c_locale" != "xno" ; then
  AC_DEFINE(ICS_FORCE_C_LOCALE, 1, [Whether to force the c locale for reading and writing.])
fi

dnl ---------------------------------------------------------------------------

dnl Check for _Float16 support in the compiler.
AC_MSG_CHECKING([for _Float16 support])
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM(
    [[
      _Float16 x = 1.0;
    ]]
  )],
  [AC_MSG_RESULT([yes])
   AC_DEFINE([HAVE_FLOAT16], [1], [Define if the compiler supports _Float16])],
  [AC_MSG_RESULT([no])]
)

dnl ---------------------------------------------------------------------------

dnl Check for -lm:
AC_CHECK_LIB(m, sqrt, [], [AC_MSG_ERROR([math lib is required])])

AC_CHECK_FUNC(strtok_r, [AC_DEFINE(HAVE_STRTOK_R, 1)], [])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

