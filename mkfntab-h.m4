dnl  $NiH$
dnl
dnl  mkfntab-c.m4 -- create fntable.h from fntable.fn
dnl  Copyright (C) 1996, 2000, 2001 Dieter Baron
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

changequote(<<,>>)

define(function,dnl name, synopsis, function, flags, help-string, description
<<divert(0)dnl
ifelse(0,<<$3>>,,<<void <<$3>>(char **args);
>>)dnl
divert(-1)>>)

define(section,dnl file, name
<<divert(0)dnl
/* <<$2>>: <<$1>> */
divert(-1)>>)

define(endsec)

divert(0)dnl
<<#ifndef HAD_FNTABLE_H
#define HAD_FNTABLE_H

/*
   This file is automatically created from ``fntable.fn''; don't change
   this file, change ``fntable.fn'' instead.
*/

>>divert(-1)

define(endall,
<<divert(0)
#endif /* fntable.h */
divert(-1)>>)
