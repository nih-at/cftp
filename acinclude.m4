dnl  acinclude.m4 -- test for autoconf
dnl  Copyright (C) 2000 Dieter Baron
dnl
dnl  This file is part of cftp, a fullscreen ftp client
dnl  The author can be contacted at <dillo@giga.or.at>
dnl
dnl  This program is free software; you can redistribute it and/or modify
dnl  it under the terms of the GNU General Public License as published by
dnl  the Free Software Foundation; either version 2 of the License, or
dnl  (at your option) any later version.
dnl
dnl  This program is distributed in the hope that it will be useful,
dnl  but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl  GNU General Public License for more details.
dnl
dnl  You should have received a copy of the GNU General Public License
dnl  along with this program; if not, write to the Free Software
dnl  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

dnl Usage:
dnl NIH_CHECK_STRUCT(includes, struct, action-if-found, action-if-not-found)

AC_DEFUN(NIH_CHECK_STRUCT,
[AC_MSG_CHECKING(for struct $2)
AC_CACHE_VAL(nih_cv_check_struct_$2,
[AC_TRY_RUN([$1], [$2 var;],
 [nih_cv_check_struct_$2=yes], [nih_cv_check_struct_$2=no])])
if test "x$nih_cv_check_struct_$2" = xyes; then
AC_MSG_RESULT(yes)
ifelse([$3], ,
[AC_DEFINE([HAVE_STRUCT_]translit($2, [a-z], [A-Z]),
  1,[Define if you have struct $2])],
[$3])
else
AC_MSG_RESULT(no)
ifelse([$4], , , [$4])
fi])

