dnl  $NiH: mkfntexi.m4,v 1.5 2001/12/23 03:52:34 dillo Exp $
dnl
dnl  mkfntexi.m4 -- create functions.texi from fntable.fn
dnl  Copyright (C) 1996-2002 Dieter Baron
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
