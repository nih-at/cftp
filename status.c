/*
  $NiH: status.c,v 1.19 2002/09/16 12:42:43 dillo Exp $

  status.c -- status line
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



#include <string.h>

#include "display.h"
#include "bindings.h"
#include "status.h"
#include "list.h"
#include "directory.h"
#include "tag.h"
#include "tty.h"

#define MAX_STATUS  8192
char status_line[MAX_STATUS];
struct status status;



void
status_init(void)
{
    status.host = status.remote.path = status.local.path = 0;
}



void
status_do(enum state when)
{
    int cols, space, l, l2;

    if (when != bs_none && when != binding_state)
	return;

    cols = tty_cols;
    if (cols >= MAX_STATUS)
	cols = MAX_STATUS-1;

    memset(status_line, ' ', cols);
    status_line[cols] = '\0';

    if (cols < 25) {
	switch (binding_state) {
	case bs_remote:
	    l = strlen(status.remote.path);
	    if (l > cols)
		strcpy(status_line, status.remote.path+(l-cols));
	    else
		strncpy(status_line, status.remote.path, l);
	    break;

	case bs_tag:
	    if (cols < 5)
		strncpy(status_line, "tag  ", cols);
	    else
		strncpy(status_line, "<tag>", 5);
	    break;

	default:
	    break;
	}
    }
    else {
	strncpy(status_line, "--cftp: ", 8);

	switch(status.percent) {
	case -1:
	    strcpy(status_line+cols-5, "All--");
	    break;
	case 0:
	    strcpy(status_line+cols-5, "Top--");
	    break;
	case 100:
	    strcpy(status_line+cols-5, "Bot--");
	    break;
	default:
	    sprintf(status_line+cols-5, "%2d%%--", status.percent);
	}

	space = cols-24;
	l = strlen(status.host);
	if (l > space) {
	    strncpy(status_line+8, status.host, space-3);
	    strncpy(status_line+space-3+8, "...", 3);
	    space = 0;
	}
	else {
	    strncpy(status_line+8, status.host, l);
	    space -= l;
	}
	switch (binding_state) {
	case bs_remote:
	    strncpy(status_line+cols-15, "<remote>--", 10);
	    if (status.remote.path) {
		l = strlen(status.remote.path);
		if (l+1 > space && space < (cols-24)/2) {
		    l2 = cols-24-l+1;
		    if (l > (cols-24)/2)
			l2 = (cols-24)/2;
		
		    strncpy(status_line+8+l2-3, "... ", 4);
		    space = cols-24-l2;
		}
		l2 = 8+(cols-24)-space+1;
		if (l+1 > space) {
		    strncpy(status_line+l2, "...", 3);
		    strncpy(status_line+l2+3,
			    status.remote.path+l-space+3+1,
			    space-4);
		}
		else
		    strncpy(status_line+l2, status.remote.path, l);
	    }
	    break;
	    
	case bs_tag:
	    strncpy(status_line+cols-15, "<tag>-----", 10);
	    break;

	default:
	    break;
	}
	
	if (!disp_quiet) {
	    tty_goto(0, tty_lines-2);
	    tty_standout();
	    fputs(status_line, stdout);
	    tty_standend();
	    fputc('\n', stdout);
	}
    }
}



extern enum state leave_tag;

void
enter_state(enum state state)
{
    switch (state) {
    case bs_remote:
	binding_state = bs_remote;
	list = (struct list *)curdir;
	list_do(1);
	status_do(bs_none);
	break;

    case bs_local:
	disp_status(DISP_STATUS, "state <local> not implemented yet");

    case bs_tag:
	if (!tag_anytags()) {
	    disp_status(DISP_STATUS, "no tags");
	    break;
	}
	if (binding_state != bs_tag)
	    leave_tag = binding_state;
	binding_state = bs_tag;
	list = (struct list *)&tags;
	list_do(1);
	status_do(bs_none);
	break;

    case bs_none:
	disp_status(DISP_STATUS, "can't enter state <global>");
	break;

    default:
	disp_status(DISP_STATUS, "no such state: %s", state);
    }
}
