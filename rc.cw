@ reading and parsing rc file

@u
#include <stdio.h>
#include <string.h>
#include "rc.h"

@(rc.h@)
#ifndef HAD_RC_H
#define HAD_RC_H

@<prototypes@>

#endif /* rc.h */


@ tokenizing an rc line

@d<prototypes@>
char *rc_token(char **linep);

@u
char *
rc_token(char **linep)
{
	char *tok, *p;
	
	if (*linep == NULL)
		return NULL;

	tok = *linep;

	if (tok[0] == '\0' || tok[0] == '\n') {
		*linep = NULL;
		return NULL;
	}
	
	if (tok[0] == '\'' || tok[0] == '\"') {
		tok++;
		if ((p=strchr(tok, tok[-1])) == NULL) {
			if (tok[strlen(tok)-1] == '\n')
				tok[strlen(tok)-1] = '\0';
			*linep = NULL;
		}
		else {
			*p = '\0';
			*linep = p+1+strspn(p+1, " \t\n");
		}
	}
	else {
		p = tok+strcspn(tok, " \t\n");
		if (*p == '\0')
		    *linep = NULL;
		else {
		    *p = '\0';
		    *linep = p+1+strspn(p+1, " \t\n");
		}
	}

	return tok;
}
