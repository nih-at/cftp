#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "rc.h"

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

    while (fgets(b, 8192, f)) {
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
		    parse_url(tok, &user, &host, &port, &wdir);
		    pass = NULL;
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

	/* XXX: other commands */
    }

    fclose(f);

    return 0;
}
