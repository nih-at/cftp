@ unsorted auxiliary functions

@(util.h@)
@<prototypes@>

@u
#include <stdio.h>
#include <string.h>


@ canonifying pathnames; if current is NULL, relative to ftp_lcwd.

@d<prototypes@>
char *canonical(char *path, char *current);

@u
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


@ basename and dirname

@d<prototypes@>
char *basename(char *name);
char *dirname(char *name);

@u
char *
basename(char *name)
{
    char *p = strrchr(name, '/');

    return (p ? p+1 : name);
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
