/*
  $NiH: rc.c,v 1.11 2001/12/11 14:37:38 dillo Exp $

  rc.c -- auxiliary functions for parsing .cftprc file
  Copyright (C) 1996-2002 Dieter Baron

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
#include <stdarg.h>
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

    for (i=0; (t=rc_token(&line)); i++) {
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



int rc_inrc = 0;
int rc_lineno;
char *rc_filename;

extern char *prg;

void
rc_error(char *fmt, ...)
{
    va_list argp;

    fprintf(stderr, "%s:%s:%d: ", prg, rc_filename, rc_lineno);
    va_start(argp, fmt);
    vfprintf(stderr, fmt, argp);
    va_end(argp);
    fputc('\n', stderr);
}
