dnl  $NiH: mkmethods-c.m4,v 1.2 2001/12/13 21:25:26 dillo Exp $
dnl
dnl  mkmethods-c.m4 -- create methods.c from methods.mt
dnl  Copyright (C) 2001, 2002 Dieter Baron
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

define(rcsid, dnl id
<<divert(3)    $1
divert(-1)>>)

rcsid(<<$NiH: mkmethods-c.m4,v 1.2 2001/12/13 21:25:26 dillo Exp $>>)

define(method, dnl rettype, name, args, docu
<<divert(1)    rftp_<<$2>>,
divert(2)    sftp_<<$2>>,
divert(-1)>>)

define(endall, dnl
<<divert(0)dnl
/*
  This file is automatically created from ``methods.mt''; don't make
  changes to this file, change ``methods.mt'' instead.

  Created from:
undivert(3)dnl
*/

#include <stddef.h>

#include "config.h"

#ifdef USE_SFTP
#include "methods.h"

struct ftp_methods ftp_methods[] = {
{
undivert(1)dnl
},
{
undivert(2)dnl
}
};

#endif

int ftp_proto;

divert(-1)>>)
