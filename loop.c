/*
  loop -- main loop
  Copyright (C) 1996, 1997 Dieter Baron

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

int fnexit(void);



void
loop()
{
    int c, ret = 0;
    struct binding *binding;
    function *f;

    list_do(1);
	
    while ((c=tty_readkey()) != -1)
	binding = get_function(c, bs_none);
	if (binding->fn != -1) {
	    f = functions+binding->fn;

	    if (f->type == FN_EXIT
		&& fnexit() == 0)
		return;

	    if (f->type != FN_PRE)
		disp_status("");

	    if (f->type != FN_EXIT)
		f->fn(binding->args);

	    if (f->type != FN_PRE)
		void_prefix();
	}
	else {
	    disp_status("");
	    void_prefix();
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
