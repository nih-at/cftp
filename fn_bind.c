/*
  $NiH: fn_bind.c,v 1.12 2002/09/16 12:42:30 dillo Exp $

  fn_bind.c -- bindable functions: bind
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



#include <stdlib.h>
#include <string.h>

#include "bindings.h"
#include "fntable.h"
#include "functions.h"
#include "display.h"
#include "keys.h"
#include "rc.h"



void
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
	    disp_status(DISP_STATUS, "no key");
	    free(line);
	    return;
	}
	if ((s2=parse_state(kname)) != bs_nostate) {
	    staten = kname;
	    state = s2;

	    if ((kname=rc_token(&p)) == NULL) {
		disp_status(DISP_STATUS, "no key");
		free(line);
		return;
	    }
	}
	cmd = rc_token(&p);
    }

    if (state == bs_unknown) {
	if (rc_inrc)
	    rc_error("unknown state: %s", staten);
	else
	    disp_status(DISP_STATUS, "unknown state: %s", staten);
	if (line)
	    free(line);
	return;
    }

    if ((key=parse_key(kname)) < 0) {
	if (rc_inrc)
	    rc_error("unknown key: %s", kname);
	else
	    disp_status(DISP_STATUS, "unknown key: %s", kname);
	if (line)
	    free(line);
	return;
    }
	
    if (cmd) {
	if ((fn=find_function(cmd)) < 0) {
	    if (rc_inrc)
		rc_error("unknown function: %s", cmd);
	    else
		disp_status(DISP_STATUS, "unknown function: %s", cmd);
	    if (line)
		free(line);
	    return;
	}
    }

    if (!rc_inrc)
	disp_status(DISP_STATUS, "");
	
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
