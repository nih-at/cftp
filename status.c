/*
  status.h -- status line
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



#include "display.h"
#include "bindings.h"
#include "status.h"
#include "tty.h"

char status_line[8192];
struct status status;



void
status_init(void)
{
    status.host = status.remote.path = status.local.path = 0;
}

void
status_do(enum state when)
{
    int cols, space, l;

    cols = tty_cols;
    space = cols - 20;

    if (when != bs_none && when != binding_state)
	return;

/*    if (opt_emacs_status) { */
    if (1) {
	strcpy(status_line, "--cftp: ");
	l = strlen(status.host);
	if (l > space-30) {
	    strncpy(status_line+8, status.host, space-33);
	    strcpy(status_line+space-33+8, "...");
	    space = 30;
	}
	else {
	    strcpy(status_line+8, status.host);
	    space -= l;
	}
	memset(status_line+cols-12-space, ' ', space+2);
	switch (binding_state) {
	case bs_remote:
	    strcpy(status_line+cols-10, "<remote>--");
	    l = strlen(status.remote.path);
	    if (l > space) {
		strncpy(status_line+cols-space-11, "...", 3);
		strncpy(status_line+cols-space-8,
			status.remote.path+l-space+3,
			space-3);
	    }
	    else
		strncpy(status_line+cols-space-11, status.remote.path, l);
	    break;
	    
	case bs_tag:
	    strcpy(status_line+cols-10, "<tag>-----");
	    break;
	}
	
	if (!disp_quiet) {
	    tty_goto(0, tty_lines-2);
	    tty_standout();
	    fputs(status_line, stdout);
	    tty_standend();
	}
    }

    /* XXX: handle non-emacs-mode */
}



void
opt_set_status(int *optval)
{
    /* XXX */
}
