dnl  $NiH: mkmethods-h.m4,v 1.2 2001/12/13 21:25:27 dillo Exp $
dnl
dnl  mkmethods-h.m4 -- create methods.h from methods.mt
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

diverts:
	0	output
	1	struct ftp_methods
	2	USE_SFTP method defines
	3	!USE_SFTP method defines
	4	rftp method prototypes
	5	sftp method prototypes
	6	rcsids

changequote(<<,>>)

define(rcsid, dnl id
<<divert(6)    $1
divert(-1)>>)

rcsid(<<$NiH: mkmethods-h.m4,v 1.2 2001/12/13 21:25:27 dillo Exp $>>)

define(method, dnl rettype, name, args, docu
<<divert(1)   <<$1>> (*fn_<<$2>>)(<<$3>>);
divert(2)#define ftp_$2 (ftp_methods[ftp_proto].fn_$2)
divert(3)#define ftp_$2 rftp_$2
divert(4)<<$1>> rftp_<<$2>>(<<$3>>);
divert(5)<<$1>> sftp_<<$2>>(<<$3>>);
divert(-1)>>)

define(endall, dnl
<<divert(0)dnl
#ifndef _HAD_METHODS_H
#define _HAD_METHODS_H

/*
  This file is automatically created from ``methods.mt''; don't make
  changes to this file, change ``methods.mt'' instead.

  Created from:
undivert(6)dnl
*/

#include <stddef.h>

#include "config.h"
#include "directory.h"

undivert(4)dnl

#ifdef USE_SFTP

struct ftp_methods {
undivert(1)dnl
};

extern struct ftp_methods ftp_methods[];

undivert(5)dnl

undivert(2)dnl

#else /* !USE_SFTP */

undivert(3)dnl

#endif /* !USE_SFTP */

extern int ftp_proto;

#endif /* methods.h */
divert(-1)>>)
