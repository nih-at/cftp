dnl  $NiH$
dnl
dnl  mkfntexi.m4 -- create functions.texi from fntable.fn
dnl  Copyright (C) 1996, 1997, 2000, 2001 Dieter Baron
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
	0  chapter menu
	1  chapter menu, all functions
	2  section menu + finished sections
	3  subsection nodes

changequote(<<,>>)

define(menuentry, dnl node, description
<<* <<$1>>::dnl
ifelse(eval(len(<<$1>>)<4), 1, <<	>>)dnl
ifelse(eval(len(<<$1>>)<12), 1, <<	>>)dnl
	<<$2>>>>)

define(function,dnl name, synopsis, function, flags, help-string, description
<<divert(1)dnl
menuentry(<<$1>>, <<$5>>)
divert(2)dnl
menuentry(<<$1>>, <<$5>>)
divert(3)dnl
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
<<divert(0)dnl
menuentry(<<$2>>,)
divert(2)dnl

@c =====================================================================
@node <<$2>>
<<@section>> <<$2>>
@cindex <<$2>>

@menu
divert(-1)>>)

define(endsec, dnl
<<divert(2)dnl
@end menu

undivert(3)dnl
divert(-1)>>)

divert(0)dnl
@c This file is automatically created from ``fntable.fn''; don't change
@c this file, change ``fntable.fn'' instead.

@c *********************************************************************
@node	 Functions
@chapter Functions

@menu
divert(1)dnl
All Functions Alphabetically
<-------
divert(-1)

define(endall,
<<divert(1)dnl
------->
@end menu

divert(0)
undivert>>)
