/*
  $NiH: loop.c,v 1.15 2001/12/20 05:44:13 dillo Exp $

  loop.c -- main loop
  Copyright (C) 1996-2002 Dieter Baron

  This file is part of cftp, a fullscreen ftp client
  The author can be contacted at <dillo@giga.or.at>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/



#include "directory.h"
#include "bindings.h"
#include "functions.h"
#include "display.h"
#include "tty.h"
#include "list.h"
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
	
