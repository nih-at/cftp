dnl  $NiH: mkfntab-c.m4,v 1.4 2001/12/13 21:14:52 dillo Exp $
dnl
dnl  mkfntab-c.m4 -- create fntable.c from fntable.fn
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

diverts:
	1: functions array
	2: rcs ids

changequote(<<,>>)

define(rcsid, dnl id
<<divert(2)    $1
divert(-1)>>)

rcsid(<<$NiH$>>)

define(function,dnl name, synopsis, function, flags, help-string, description
<<divert(1)dnl
  <<$3>> , "<<$1>>", <<$4>>, "<<$5>>" ,
divert(-1)>>)

define(section,dnl file, name
<<divert(1)dnl
/* <<$2>>: <<$1>> */
divert(-1)>>)

define(endsec)

define(endall, dnl
<<divert(0)dnl
<</*
   This file is automatically created from ``fntable.fn''; don't change
   this file, change ``fntable.fn'' instead.

   Created from:>>
undivert(2)dnl
<<*/

#include "directory.h"
#include "bindings.h"
#include "functions.h"
#include "fntable.h"

function functions[] = {
>>
undivert(1)dnl
<</* end marker */
  { 0, 0, 0, 0 }
};>>
divert(-1)>>)
