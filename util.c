/*
  util -- auxiliary functions
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



#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pwd.h>



char *
canonical(char *path, char *current)
{
    char *canon, *p, *w, *d;

    extern char *ftp_lcwd;
    
    if (path[0] == '/' || path[0] == '~') {
	canon = strdup(path);
	for (p=canon+strlen(canon)-1; p>canon && *p=='/'; --p)
			*p='\0';
    }
    else {
	if (current == NULL)
	    current = ftp_lcwd;

	canon = (char *)malloc(strlen(current)+strlen(path)+2);
	strcpy(canon, current);
	w = strdup(path);

	for (p=w+strlen(w); *p=='/'; --p)
	    *p='\0';

	for (d=strtok(w, "/"); d; d=strtok(NULL, "/")) {
	    if (strcmp(d, "..") == 0) {
		p = strrchr(canon, '/');
		if (p != NULL) {
		    if (p != canon)
			*p = '\0';
		    else
			strcpy(canon, "/");
		}
	    }
	    else if (strcmp(d, ".") != 0) {
		if (canon[strlen(canon)-1] != '/')
		    strcat(canon, "/");
		strcat(canon, d);
	    }
	}
		
	free(w);
    }

    return canon;
}



char *
dirname(char *name)
{
    char *dir;
    char *p = strrchr(name, '/');

    if (p == NULL || p == name)
	return strdup("/");

    if ((dir=(char *)malloc(p-name+1)) != NULL) {
	strncpy(dir, name, p-name);
	dir[p-name] = '\0';
    }

    return dir;
}



char *
local_exp(char *path)
{
    char *home, *s, *p;
    struct passwd *pw;
	    
    if (path[0] != '~')
	return strdup(path);

    if (path[1] == '/' || path[1] == '\0') {
	s = path+1;

	if ((home=getenv("HOME")) == NULL) {
	    if ((pw=getpwuid(getuid())) == NULL
		|| (home=pw->pw_dir) == NULL)
		return NULL;
	}
    }
    else {
	if (s=strchr(path, '/'))
	    *s = '\0';
	if ((pw=getpwnam(path+1)) == NULL
	    || (home=pw->pw_dir) == NULL) {
	    if (s)
		*s = '/';
	    return NULL;
	}
	if (s)
	    *s = '/';
	else
	    s = path+strlen(path);
    }

    if ((p=(char *)malloc(strlen(home)+strlen(s)+1)) == NULL)
	return NULL;

    sprintf(p, "%s%s", home, s);

    return p;
}
