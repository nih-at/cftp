/*
  fn_bind -- bindable functions: bind
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



#include "bindings.h"
#include "display.h"
#include "functions.h"
#include "keys.h"
#include "rc.h"



int
fn_bind(char **args)
{
    enum state state, s2;
    int key, fn, i;
    char *line = NULL;
    char *staten, *kname, *cmd, **list, *p;
    struct binding *b;

    state = bs_none;

    if (args) {
	if ((s2=parse_state(args[0])) != bs_nostate) {
	    staten = args[0];
	    state = s2;
	    args++;
	}
	kname = args[0];
	cmd = args[1];
	args += 2;
    }
    else {
	if (rc_inrc) {
	    rc_error("bind: no arguments given");
	    return;
	}
	line = p = read_string("bind ", 1);

	if ((kname=rc_token(&p)) == NULL) {
	    disp_status("no key");
	    free(line);
	    return;
	}
	if ((s2=parse_state(kname)) != bs_nostate) {
	    staten = kname;
	    state = s2;

	    if ((kname=rc_token(&p)) == NULL) {
		disp_status("no key");
		free(line);
		return;
	    }
	}
	cmd = rc_token(&p);
    }

    if (state == bs_unknown) {
	(rc_inrc ? rc_error : disp_status)("unknown state: %s", staten);
	if (line)
	    free(line);
	return;
    }

    if ((key=parse_key(kname)) < 0) {
	(rc_inrc ? rc_error : disp_status)("unknown key: %s", kname);
	if (line)
	    free(line);
	return;
    }
	
    if (cmd) {
	if ((fn=find_function(cmd)) < 0) {
	    (rc_inrc ? rc_error : disp_status)("unknown function: %s", cmd);
	    if (line)
		free(line);
	    return;
	}
    }

    if (!rc_inrc)
	disp_status("");
	
    b = get_function(key, state);

    if (b->state == state) {
	b->fn = -1;
	if (b->args) {
	    if (b->args < binding_argpool
		|| (b->args >= binding_argpool+binding_nargpool)) {
		for (i=0; b->args[i]; i++)
		    free(b->args[i]);
		free(b->args);
		b->args = NULL;
	    }
	}
    }
    else {
	if ((b=(struct binding *)malloc(sizeof(struct binding))) == NULL)
	    return;
	b->fn = -1;
	b->args = NULL;
	b->state = state;

	b->next = binding[key].next;
	binding[key].next = b;
    }

    if (cmd) {
	if (line)
	    list = rc_list(p);
	else {
	    for (i=0; args[i]; i++)
		;
	    if (i > 0) {
		list = (char **)malloc(sizeof(char *)*(i+1));
		memcpy(list, args, sizeof(char *)*(i+1));
	    }
	    else
		list = NULL;
	}

	b->fn = fn;
	b->args = list;
    }

    return;
}
