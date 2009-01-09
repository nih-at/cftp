dnl  $NiH: mkoptexi.m4,v 1.6 2001/12/23 03:34:01 dillo Exp $
dnl
dnl  mkoptexi.m4 -- create options.texi from options.op
dnl  Copyright (C) 1996-2002 Dieter Baron
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
dnl
divert(-1)

diverts:
	1: chapter menu
	2: sections
	3: rcs ids

changequote(<<,>>)

define(rcsid, dnl id
<<divert(3)@c   $1
divert(-1)>>)

rcsid(<<$NiH: mkoptexi.m4,v 1.6 2001/12/23 03:34:01 dillo Exp $>>)


define(menuentry, dnl node, description
<<* <<$1>>::dnl
ifelse(eval(len(<<$1>>)<4), 1, <<	>>)dnl
ifelse(eval(len(<<$1>>)<12), 1, <<	>>)dnl
	<<$2>>>>)

define(option,dnl name, short, variable, function, type, default, values, help, doku
<<divert(1)dnl
menuentry(<<$1>>, <<$8>>)
divert(2)dnl
@c ---------------------------------------------------------------------
@node <<$1>>
@section <<$1>> -- <<$8>>
@cindex <<$1>>

@itemize @bullet
@item
Option: <<$1>>
@item 
Short: <<$2>>
@item
Default: <<$6>>
@end itemize

<<$9>>

divert(-1)>>)

define(endall,
<<divert(0)dnl
<<@c This file is automatically created from ``options.op''; don't change
@c this file, change ``options.op'' instead.
@c
@c Created from:>>
undivert(3)dnl
<<
@c *********************************************************************
@node	 Options
@chapter Options

@menu>>
undivert(1)dnl
<<@end menu
>>
undivert(2)>>)
