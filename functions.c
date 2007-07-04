/*
  $NiH: functions.c,v 1.13 2002/09/16 12:42:34 dillo Exp $

  functions.c -- auxiliary functions for bindable function handling
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



#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "display.h"
#include "bindings.h"
#include "functions.h"
#include "options.h"

int prefix_arg, prefix_valid = 0;



void
void_prefix(void)
{
	prefix_valid = 0;
}



int
get_prefix(int deflt)
{
	return (prefix_valid ? prefix_arg : deflt);
}



int
get_prefix_args(char **args, int deflt)
{
    if (args)
	return atoi(args[0]);
    else
	return (prefix_valid ? prefix_arg : deflt);
}



void
add_prefix(int n)
{
	if (prefix_valid)
		prefix_arg = prefix_arg*10 + n;
	else {
		prefix_valid = 1;
		prefix_arg = n;
	}
	show_prefix();
}



void
set_prefix(int p)
{
    prefix_valid = 1;
    prefix_arg = p;
    show_prefix();
}



void
negate_prefix(void)
{
    return;
}



void
show_prefix(void)
{

	disp_status(DISP_STATUS, ": %d", prefix_arg);
}



int
find_function(char *f)
{
    int i;
    
    for (i=0; functions[i].name && strcmp(f, functions[i].name); i++)
	    ;

    return (functions[i].name ? i : -1);
}



enum state binding_state;

struct binding *
get_function(int nr, enum state state)
{
    struct binding *b;
    
    if (state == bs_none)
	state = binding_state;

    for (b=binding[nr].next; b; b=b->next)
	if (b->state == state)
	    return b;

    return &binding[nr];
}



char *binding_statename[] = {
    "<global>", "<remote>", "<local>", "<tag>", NULL
};

enum state
parse_state(char *name)
{
    int i;

    if (name[0] != '<' || name[strlen(name)-1] != '>')
	return bs_nostate;

    for (i=0; binding_statename[i]; i++)
	if (strcasecmp(name, binding_statename[i]) == 0)
	    return (enum state)i;

    return bs_unknown;
}
