dnl  $NiH: mkfntab-h.m4,v 1.6 2001/12/23 03:52:33 dillo Exp $
dnl
dnl  mkfntab-c.m4 -- create fntable.h from fntable.fn
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

changequote(<<,>>)

define(rcsid, dnl id
<<divert(2)    $1
divert(-1)>>)

rcsid(<<$NiH: mkfntab-h.m4,v 1.6 2001/12/23 03:52:33 dillo Exp $>>)

define(function,dnl name, synopsis, function, flags, help-string, description
<<divert(1)dnl
ifelse(0,<<$3>>,,<<void <<$3>>(char **args);
>>)dnl
divert(-1)>>)

define(section,dnl file, name
<<divert(1)dnl
/* <<$2>>: <<$1>> */
divert(-1)>>)

define(endsec)

define(endall, dnl
<<divert(0)dnl
<<#ifndef HAD_FNTABLE_H
#define HAD_FNTABLE_H

/*
   This file is automatically created from ``fntable.fn''; don't change
   this file, change ``fntable.fn'' instead.

   Created from:>>
undivert(2)dnl
<<*/>>

undivert(1)dnl

<<#endif /* fntable.h */>>
divert(-1)>>)
