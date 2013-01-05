dnl Process this file with autoconf to produce a configure script.

dnl init

AC_INIT([cftp],[0.12a],[cftp@nih.at])
AC_CONFIG_SRCDIR([fn_bind.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE

AC_ARG_ENABLE(sftp,
    [  --enable-sftp           enable support for sftp protocol],
    USE_SFTP=no, USE_SFTP=yes)

# We want these before the checks, so the checks can modify their values.
test -z "$CFLAGS" && CFLAGS='-g' auto_cflags=1

AC_PROG_CC

dnl # If we're using gcc and the user hasn't specified CFLAGS, add -O to CFLAGS.
dnl test -n "$GCC" && test -n "$auto_cflags" && CFLAGS="$CFLAGS -O"

if test "$USE_SFTP" = "yes"
then
    AC_DEFINE([USE_SFTP], [1], [Define if you want sftp support])
fi

dnl programs

dnl libraries

AC_SEARCH_LIBS([socket], [socket], [],
	       [AC_MSG_ERROR([cannot find socket functions])],
	       [-lnsl])
if test "$ac_cv_search_socket" = "-lsocket"; then
LIBS="$LIBS -lnsl"
fi

AC_SEARCH_LIBS([tgetent], [termcap curses ncurses], [],
	       [AC_MSG_ERROR([cannot find termcap (emulation) library])])

AC_SEARCH_LIBS([inet_aton], [resolv],
	       [AC_DEFINE([HAVE_INET_ATON], [1],
			 [Define to 1 if you have the `inet_aton' function.])])

dnl AC_CHECK_LIB(resolv, main)


dnl header files

AC_CHECK_HEADERS(libgen.h sys/select.h termcap.h)

if test "$ac_cv_header_termcap_h" = yes; then
NIH_CHECK_DECL([#include "termcap.h"], ospeed)
fi

NIH_CHECK_DECL([#include "netdb.h"], h_errno)

dnl typedefs
dnl structures

NIH_CHECK_STRUCT([#include <sys/types.h>
#include <sys/socket.h>], sockaddr_storage)
NIH_CHECK_STRUCT_MEMBER([#include <sys/types.h>
#include <sys/socket.h>], sockaddr, sa_len)

dnl compiler characteristics

dnl library functions

AC_REPLACE_FUNCS(fnmatch fputchar getaddrinfo \
		 getcwd inet_aton strdup strerror)
AC_CHECK_FUNCS(getdomainname getnameinfo)

AC_CHECK_FUNCS(tparam, [tparam=1], [tparam=0])
if test "$tparam" -ne 1
then
	AC_CHECK_FUNCS(tparm, , [AC_LIBOBJ(tparam)])
fi

AC_CHECK_FUNCS(getopt_long, , [AC_LIBOBJ(getopt) AC_LIBOBJ(getopt1)])

need_snprintf=no
AC_CHECK_FUNCS(snprintf vsnprintf vasprintf, , [need_snprintf=yes])
if test $need_snprintf = yes
then
        AC_LIBOBJ(snprintf)
fi


dnl system services

dnl output
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
