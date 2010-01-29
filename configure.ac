#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([linrad], [0.3.8], [Leif Åsbrink])
AC_CONFIG_AUX_DIR(config)
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE([dist-bzip2])

# Checks for programs.
AC_PROG_CXX
AC_PROG_INSTALL

# Checks for header files.
AC_HEADER_STDC	    

# Checks for libraries: libasound2, libx11, libxext

PKG_CHECK_MODULES([ALSA], [alsa >= 1.0])
PKG_CHECK_MODULES([X11], [x11 >= 1.3])
PKG_CHECK_MODULES([XEXT], [xext >= 1.1])
PKG_CHECK_MODULES([XPROTO], [xproto >= 7.0])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE

# Checks for NASM
AC_PATH_PROG(NASM, nasm)

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

# Checks for library functions.
#AC_PROG_GCC_TRADITIONAL
#AC_CHECK_FUNCS([floor])
#AC_PROG_RANLIB