dnl  $NiH: mkoptexi.m4,v 1.6 2001/12/23 03:34:01 dillo Exp $
dnl
dnl  mkoptexi.m4 -- create options.texi from options.op
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
