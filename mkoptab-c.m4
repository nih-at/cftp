dnl  $NiH: mkoptab-c.m4,v 1.9 2001/12/23 03:34:01 dillo Exp $
dnl
dnl  mkoptab-c.m4 -- create options.c from options.op
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
	1: options array
	3: declarations, definitions and initializations
	4: rcs ids

changequote(<<,>>)

define(rcsid, dnl id
<<divert(4)    $1
divert(-1)>>)

rcsid(<<$NiH: mkoptab-c.m4,v 1.9 2001/12/23 03:34:01 dillo Exp $>>)

dnl struct uoption {
dnl     char *name, *short;
dnl     char *help;
dnl     int type;
dnl     union optvar var;
dnl     void (*func)();
dnl };

define(type,dnl t, (i)nt, (c)har, (s)tring, (b)oolean, (e)num
<<ifelse(<<$1>>, i, <<$2>>, <<$1>>, c, <<$3>>, <<$1>>, s, <<$4>>, <<$1>>, b, <<$5>>, <<$6>>)>>)

define(option,dnl name, short, variable, function, type, default, values, help, doku
<<divert(1)dnl
  { "<<$1>>" , "<<$2>>", "<<$8>>",
    type(<<$5>>, OPT_INT, OPT_CHR, OPT_STR, OPT_BOOL, OPT_ENUM), dnl
(void *)&<<$3>>, <<$4>>, ifelse(<<$5>>, e, openum_<<$7>>, NULL) dnl
},
divert(3)dnl
type(<<$5>>, <<int >>, <<int >>, <<char *>>, <<int >>, <<int >>)<<$3>> dnl
= <<$6>>;
ifelse(<<$4>>, NULL, , extern void <<$4>><<()>>;
)
divert(-1)>>)

define(values,dnl name, values
<<divert(3)dnl
char *openum_<<$1>>[] = { <<$2>>, NULL };
divert(-1)>>)

define(endall, dnl
<<divert(0)dnl
/*
   This file is automatically created from ``options.op''; don't make
   changes to this file, change ``options.op'' instead.

  Created from:
undivert(4)dnl
*/

#include <stddef.h>
#include "options.h"

undivert(3)dnl

<<struct uoption option[] = {>>
undivert(1)dnl
<<  NULL, NULL, NULL, 0, NULL, NULL
};>>
divert(-1)>>)
