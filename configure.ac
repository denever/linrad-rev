#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([linrad], [0.3.8], [Leif Åsbrink])
AC_CONFIG_AUX_DIR(config)
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE([dist-bzip2])

# Checks for programs.
AC_PROG_CXX
AM_PROG_CC_C_O
AC_PROG_INSTALL

# Checks for header files.
AC_HEADER_STDC

# Checks for libraries: libasound2, libx11, libxext, libxproto
# On Debian: libasound2-dev, libx11-dev, x11proto-core-dev
PKG_CHECK_MODULES([ALSA], [alsa >= 1.0], AC_DEFINE([HAVE_ALSA], [1], ['If alsa is present']))
PKG_CHECK_MODULES([X11], [x11 >= 1.3])
PKG_CHECK_MODULES([XEXT], [xext >= 1.1], AC_DEFINE([HAVE_SHM], [1], ['If shm is installed']))
PKG_CHECK_MODULES([XPROTO], [xproto >= 7.0], AC_DEFINE([HAVE_SHM], [1], ['If shm is installed']))

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE

# Checks for NASM
AC_CHECK_PROG([NASM], [nasm], [nasm])
#AC_ARG_VAR([NASM],['NASM Assembler'])
if test -z "$NASM"; then
   AC_MSG_ERROR('Missing NASM assembler. Please install')
fi
# Sets variables for LIBS

AC_SUBST(ALSA_LIBS)
AC_SUBST(X11_LIBS)
AC_SUBST(XEXT_LIBS)
AC_SUBST(XPROTO_LIBS)

# Sets variables for CFLAGS
AC_SUBST(ALSA_CFLAGS)
AC_SUBST(X11_CFLAGS)
AC_SUBST(XEXT_CFLAGS)
AC_SUBST(XPROTO_CFLAGS)


AC_CONFIG_FILES([Makefile
		 include/Makefile
		 src/Makefile])
AC_OUTPUT
