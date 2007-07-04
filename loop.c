/*
  $NiH: loop.c,v 1.16 2002/09/16 12:42:36 dillo Exp $

  loop.c -- main loop
  Copyright (C) 1996-2002 Dieter Baron

  This file is part of cftp, a fullscreen ftp client
  The author can be contacted at <dillo@giga.or.at>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.
  3. The name of the author may not be used to endorse or promote
     products derived from this software without specific prior
     written permission.
 
  THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/



#include "directory.h"
#include "bindings.h"
#include "functions.h"
#include "display.h"
#include "tty.h"
#include "list.h"
#include "loop.h"
#include "tag.h"

int fnexit(void);

static int loop_readkey(void);

static int _loop_key;



void
loop()
{
    int c;
    struct binding *binding;
    function *f;

    _loop_key = -1;
    list_do(1);
    tty_lowleft();
    fflush(stdout);
    
    while ((c=loop_readkey()) != -1) {
	binding = get_function(c, bs_none);
	if (binding->fn != -1) {
	    f = functions+binding->fn;

	    if (f->type == FN_EXIT
		&& fnexit() == 0)
		return;

	    if (f->type != FN_PRE)
		disp_status(DISP_STATUS, "");

	    if (f->type != FN_EXIT)
		f->fn(binding->args);

	    if (f->type != FN_PRE)
		void_prefix();
	}
	else {
	    disp_status(DISP_STATUS, "");
	    void_prefix();
	}

	tty_lowleft();
	fflush(stdout);
    }
}



int
fnexit(void)
{
    int c, ret = 0;
    
    if (tag_anytags()) {
	c = read_char("Tags remaining; really quit? ");
	if (c != 'y' && c != 'Y')
	    ret = 1;
    }

    return ret;
}



static int
loop_readkey(void)
{
    int k;
    
    if (_loop_key != -1) {
	k = _loop_key;
	_loop_key = -1;
	return k;
    }

    return tty_readkey();
}

void
loop_putkey(int k)
{
    _loop_key = k;
}
	
