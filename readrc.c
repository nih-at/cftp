/*
  readrc.c -- read .cftprc file
  Copyright (C) 1996-2007 Dieter Baron

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
#include <errno.h>

#include "util.h"
#include "bindings.h"
#include "ftp.h"
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

    if ((home=getenv("HOME")) == NULL || strlen(home) > sizeof(b)-9)
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
		if (is_url(tok))
		    parse_url(tok, &ftp_proto, &user, &pass,
			      &host, &port, &wdir);
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
