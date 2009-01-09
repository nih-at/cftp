dnl  $NiH: acinclude.m4,v 1.7 2002/09/16 12:42:28 dillo Exp $
dnl
dnl  acinclude.m4 -- test for autoconf
dnl  Copyright (C) 2000-2002 Dieter Baron
dnl
dnl  This file is part of cftp, a fullscreen ftp client
dnl  The author can be contacted at <dillo@giga.or.at>
dnl
dnl  Redistribution and use in source and binary forms, with or without
dnl  modification, are permitted provided that the following conditions
dnl  are met:
dnl  1. Redistributions of source code must retain the above copyright
dnl     notice, this list of conditions and the following disclaimer.
dnl  2. Redistributions in binary form must reproduce the above copyright
dnl     notice, this list of conditions and the following disclaimer in
dnl     the documentation and/or other materials provided with the
dnl     distribution.
dnl  3. The name of the author may not be used to endorse or promote
dnl     products derived from this software without specific prior
dnl     written permission.
dnl 
dnl  THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
dnl  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
dnl  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
dnl  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
dnl  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
dnl  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
dnl  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
dnl  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
dnl  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
dnl  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
dnl  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

dnl Usage:
dnl NIH_CHECK_STRUCT(includes, struct, action-if-found, action-if-not-found)

AC_DEFUN([NIH_CHECK_STRUCT],
[AC_MSG_CHECKING(for struct $2)
AC_CACHE_VAL(nih_cv_check_struct_$2,
[AC_TRY_COMPILE([$1], [struct $2 var;],
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

dnl Usage:
dnl NIH_CHECK_STRUCT_MEMBER(includes, struct, member, a-if-fnd, a-if-not-fnd)

AC_DEFUN([NIH_CHECK_STRUCT_MEMBER],
[AC_MSG_CHECKING(for member $3 of struct $2)
AC_CACHE_VAL(nih_cv_check_struct_member_$2_$3,
[AC_TRY_COMPILE([$1], [struct $2 *var; var->$3],
 [nih_cv_check_struct_member_$2_$3=yes],
 [nih_cv_check_struct_member_$2_$3=no])])
if test "x$nih_cv_check_struct_member_$2_$3" = xyes; then
AC_MSG_RESULT(yes)
ifelse([$4], ,
[AC_DEFINE([HAVE_STRUCT_MEMBER_]translit($2, [a-z], [A-Z])[_]translit($3,
 [a-z], [A-Z]),
  1,[Define if you have member $3 in struct $2])],
[$4])
else
AC_MSG_RESULT(no)
ifelse([$5], , , [$5])
fi])

dnl Usage:
dnl NIH_CHECK_DECL(includes, variable, a-if-fnd, a-if-not-fnd)
AC_DEFUN([NIH_CHECK_DECL],
[AC_MSG_CHECKING(for declaration of $2)
AC_CACHE_VAL(nih_cv_check_decl_$2,
[AC_TRY_COMPILE([$1], [$2 = 0;],
 [nih_cv_check_decl_$2=yes],
 [nih_cv_check_decl_$2=no])])
if test "x$nih_cv_check_decl_$2" = xyes; then
AC_MSG_RESULT(yes)
ifelse([$4], ,
[AC_DEFINE([HAVE_DECL_]translit($2, [a-z], [A-Z]),
  1,[Define if you have member $3 in struct $2])],
[$4])
else
AC_MSG_RESULT(no)
ifelse([$5], , , [$5])
fi])
