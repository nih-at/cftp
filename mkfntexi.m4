dnl  $NiH: mkfntexi.m4,v 1.5 2001/12/23 03:52:34 dillo Exp $
dnl
dnl  mkfntexi.m4 -- create functions.texi from fntable.fn
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
	1  chapter menu
	2  chapter menu, all functions
	3  section menu + finished sections
	4  subsection nodes
	5  rcs ids

changequote(<<,>>)

define(rcsid, dnl id
<<divert(5)@c   $1
divert(-1)>>)

rcsid(<<$NiH: mkfntexi.m4,v 1.5 2001/12/23 03:52:34 dillo Exp $>>)

define(menuentry, dnl node, description
<<* <<$1>>::dnl
ifelse(eval(len(<<$1>>)<4), 1, <<	>>)dnl
ifelse(eval(len(<<$1>>)<12), 1, <<	>>)dnl
	<<$2>>>>)

define(function,dnl name, synopsis, function, flags, help-string, description
<<divert(2)dnl
menuentry(<<$1>>, <<$5>>)
divert(3)dnl
menuentry(<<$1>>, <<$5>>)
divert(4)dnl
@c ---------------------------------------------------------------------
@node <<$1>>
@subsection <<$1>> -- <<$5>>
@cindex <<$1>>

@itemize @bullet
@item
Synopsis: <<$1>> <<$2>>
dnl @item
dnl Default Binding: 
@end itemize

<<$6>>

divert(-1)>>)

define(section,dnl file, name
<<divert(1)dnl
menuentry(<<$2>>,)
divert(3)dnl

@c =====================================================================
@node <<$2>>
<<@section>> <<$2>>
@cindex <<$2>>

@menu
divert(-1)>>)

define(endsec, dnl
<<divert(3)dnl
@end menu

undivert(4)dnl
divert(-1)>>)

define(endall, dnl
<<divert(0)dnl
<<@c This file is automatically created from ``fntable.fn''; don't change
@c this file, change ``fntable.fn'' instead.
@c
@c Created from:>>
undivert(5)dnl

<<@c *********************************************************************
@node	 Functions
@chapter Functions

@menu>>
undivert(1)dnl

<<All Functions Alphabetically
<------->>
undivert(2)dnl
<<------->
@end menu>>

undivert(3)dnl
divert(-1)>>)
