/*
  $NiH$

  readrc -- read .cftprc file
  Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001 Dieter Baron

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
#include <errno.h>

#include "util.h"
#include "bindings.h"
#include "functions.h"
#include "rc.h"
#include "url.h"



int
readrc(char **userp, char **passp, char **hostp, char **portp, char **wdirp, 
       int check_alias)
{
    FILE *f;
    char b[8192], *p, *tok, *q, *home;
    char *user, *pass, *host, *port, *wdir;

    if ((home=getenv("HOME")) == NULL)
	home = "";
    sprintf(b, "%s/.cftprc", home);
    
    if ((f=fopen(b, "r")) == NULL) {
	if (errno == ENOENT)
	    return 0;
	return -1;
    }

    rc_inrc = 1;
    rc_lineno = 0;
    rc_filename = "~/.cftprc";

    while (fgets(b, 8192, f)) {
	rc_lineno++;

	if (b[0] == '#')
	    continue;

	p = b;
	if ((tok=rc_token(&p)) == NULL)
	    continue;

	if (strcasecmp(tok, "alias") == 0) {
	    if (check_alias == 0)
		continue;
	    
	    if ((tok=rc_token(&p)) == NULL
		|| strcasecmp(tok, *hostp) != 0)
		continue;

	    if ((tok=rc_token(&p)) != NULL) {
		if (strncmp(tok, "ftp://", 6) == 0) {
		    parse_url(tok, &user, &pass, &host, &port, &wdir);
		}
		else {
		    user = pass = port = NULL;
		    host = strdup(tok);

		    if ((tok=rc_token(&p)) != NULL)
			wdir = strdup(tok);
		    else
			wdir = NULL;
		}

		if (!*userp)
		    *userp = user;
		else
		    free(user);
		if (!*passp)
		    *passp = pass;
		else
		    free(pass);
		free(*hostp);
		*hostp = host;
		if (!*portp)
		    *portp = port;
		else
		    free(port);
		if (wdir) {
		    if (*wdirp) {
			if (wdir[strlen(wdir)-1] == '/')
			    wdir[strlen(wdir)-1] = '\0';
			q = *wdirp;
			*wdirp = (char *)malloc(strlen(q)+strlen(wdir)+2);
			sprintf(*wdirp, "%s%s%s",
				wdir, ((q[0] == '/') ? "" : "/"), q);
			free(q);
			free(wdir);
		    }
		    else {
			free(*wdirp);
			*wdirp = wdir;
		    }
		}
	    }
	}
	else {
	    int func;
	    char **args;
	    
	    if ((func=find_function(tok)) == -1) {
		rc_error("unknown command: %s", tok);
		continue;
	    }

	    if ((functions[func].type & FN_RC) == 0) {
		rc_error("command %s not allowed in rc file", tok);
		continue;
	    }

	    args = rc_list(p);

	    functions[func].fn(args);
	}
    }

    fclose(f);

    rc_inrc = 0;

    return 0;
}
