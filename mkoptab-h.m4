dnl  $NiH: mkoptab-h.m4,v 1.7 2001/12/13 21:14:54 dillo Exp $
dnl
dnl  mkoptab-h.m4 -- create options.h from options.op
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
	1: extern declarations
	3: rcs ids

changequote(<<,>>)

define(rcsid, dnl id
<<divert(3)    $1
divert(-1)>>)

rcsid(<<$NiH: mkoptab-h.m4,v 1.7 2001/12/13 21:14:54 dillo Exp $>>)

define(type,dnl t, (i)nt, (c)har, (s)tring, (b)oolean (e)num
<<ifelse(<<$1>>, i, <<$2>>, <<$1>>, c, <<$3>>, <<$1>>, s, <<$4>>, <<$1>>, b, <<$5>>, <<$6>>)>>)

define(option,dnl name, short, variable, function, type, default, values, help, doku
<<divert(1)dnl
extern type(<<$5>>, <<int >>, <<int >>, <<char *>>, <<int >>, <<int >>)<<$3>>;
divert(-1)>>)

define(values,dnl name, values
)


define(endall, dnl
<<divert(0)dnl
<<#ifndef HAD_OPTIONS_H
#define HAD_OPTIONS_H
/*
   This file is automatically created from ``options.op''; don't make
   changes to this file, change ``options.op'' instead.

  Created from:>>
undivert(3)dnl
<<*/

#define OPT_INT 0
#define OPT_CHR 1
#define OPT_STR 2
#define OPT_BOOL 3
#define OPT_ENUM 4

union optvar {
    int *i;
    char **s;
};

struct uoption {
    char *name, *shrt;
    char *help;
    int type;
    union optvar var;
    void (*func)();
    char **values;
};

extern struct uoption option[];>>
undivert(1)dnl

<<#endif /* options.h */>>
divert(-1)>>)

