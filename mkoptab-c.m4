dnl  $NiH: mkoptab-c.m4,v 1.9 2001/12/23 03:34:01 dillo Exp $
dnl
dnl  mkoptab-c.m4 -- create options.c from options.op
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
