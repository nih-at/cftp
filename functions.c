/*
  functions -- auxiliary functions for bindable function handling
  Copyright (C) 1996, 1997, 1998, 1999 Dieter Baron

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

	disp_status(": %d", prefix_arg);
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
	    return i;

    return bs_unknown;
}
