dnl  $NiH: mkmethods-c.m4,v 1.2 2001/12/13 21:25:26 dillo Exp $
dnl
dnl  mkmethods-c.m4 -- create methods.c from methods.mt
dnl  Copyright (C) 2001, 2002 Dieter Baron
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
