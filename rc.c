#include <stdio.h>
#include <string.h>
#include "rc.h"



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



char **
rc_list(char *line)
{
    char *l[128], **list, *t;
    int i, n;

    list = l;
    n = 128;

    for (i=0; t=rc_token(&line); i++) {
	if (i >= n) {
	    n += 32;
	    if (n == 160) { /* list is alias for l */
		list = (char **)malloc(sizeof(char *)*n);
		memcpy(list, l, 128*sizeof(char *));
	    }
	    else
		list = (char **)realloc(list, (sizeof(char *)*n));
	}
	list[i] = strdup(t);
    }

    if (i == 0) {
	if (n != 128) /* list is allocated */
	    free(list);
	return NULL;
    }

    list[i] = NULL;

    if (n == 128) { /* list is alias for l */
	list = (char **)malloc(sizeof(char *)*(i+1));
	memcpy(list, l, sizeof(char *)*(i+1));
    }

    return list;
}
