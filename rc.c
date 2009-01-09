/*
  $NiH: rc.c,v 1.11 2001/12/11 14:37:38 dillo Exp $

  rc.c -- auxiliary functions for parsing .cftprc file
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
