/*
  $NiH: util.c,v 1.22 2002/09/17 14:58:19 dillo Exp $

  util.c -- auxiliary functions
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



#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

#include "directory.h"
#include "ftp.h"
#include "options.h"
#include "util.h"
#include "url.h"



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
xdirname(const char *name)
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
	if ((s=strchr(path, '/')))
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



char *
args_to_string(char **args)
{
    int len, i;
    char *s;

    len = 0;
    for (i=0; args[i]; i++)
	len += strlen(args[i])+1;

    if ((s=(char *)malloc(len)) == NULL)
	return NULL;

    len = 0;
    for (i=0; args[i]; i++) {
	strcpy(s+len, args[i]);
	len += strlen(args[i]);
	s[len++] = ' ';
    }
    s[len-1] = '\0';

    return s;
}



char *
get_anon_passwd(void)
{
    char pass[8192], host[1024], domain[1024];
    struct passwd *pwd;

    if (!opt_user_anon_passwd)
	return strdup("anonymous@");
    else {
	pwd = getpwuid(getuid());

	if (pwd)
	    sprintf(pass, "%s@", pwd->pw_name);
	else
	    strcpy(pass, "unknown@");
	
	gethostname(host, 1023);
#ifdef HAVE_GETDOMAINNAME
	getdomainname(domain, 1023);
#else
	domain[0] = '\0';
#endif
	
	if (strcmp(domain, "(none)") != 0 && domain[0] != '\0') {
	    if (domain[0] != '.')
		sprintf(pass+strlen(pass), "%s.%s", host, domain);
	    else
		sprintf(pass+strlen(pass), "%s%s", host, domain);
	}
	else if (strchr(host, '.'))
	    strcat(pass, host);
	
	return strdup(pass);
    }
}



char *
mkhoststr(int passp, int urlp)
{
    int len, i, n;
    char *str[7], *hs, *t;
    int encode[7], slen[7];

    n = 0;
    len = 0;
    if ((ftp_user() && !ftp_anon()) || (passp && ftp_pass())) {
	str[n] = ftp_user();
	encode[n++] = urlp;
	if (passp && ftp_pass()) {
	    str[n] = ":";
	    encode[n++] = 0;
	    str[n] = ftp_pass();
	    encode[n++] = urlp;
	}
	str[n] = "@";
	encode[n++] = 0;
    }
    str[n] = ftp_host();
    encode[n++] = 1;
    if (ftp_prt()) {
	str[n] = ":";
	encode[n++] = 0;
	str[n] = ftp_prt();
	encode[n++] = urlp;
    }

    len = 0;
    for (i=0; i<n; i++) {
	if (encode[i])
	    slen[i] = url_enclen(str[i], URL_UCHAR);
	else
	    slen[i] = strlen(str[i]);
	len += slen[i];
    }
    len++;

    t = hs = (char *)malloc(len);

    len = 0;
    for (i=0; i<n; i++) {
	if (encode[i])
	    url_encode(t, str[i], URL_UCHAR);
	else
	    strcpy(t, str[i]);
	t += slen[i];
    }
    *t = '\0';

    return hs;
}



char *
noalloc_basename(char *name)
{
    char *p;
    int len;

    if (name == NULL || *name == '\0')
	return ".";

    len = strlen(name);

    if (strspn(name, "/") == len)
	return name + len-1;

    while (name[len-1] == '/')
	name[--len] = '\0';
    
    p = strrchr(name, '/');

    if (p) {
	if (*p == '\0')
	    return p;
	else
	    return p+1;
    }
    return name;
}



/*
  Set FD to blocking if BLOCKING, non-blocking otherwise.  Return 0 if
  FD was in non-blocking mode, 1 if it was in blocking mode, -1 in
  case of error.
*/

int
set_file_blocking(int fd, int blocking)
{
    int ret, flags;
    
    if ((flags=fcntl(fd, F_GETFL, 0)) == -1)
	return -1;

    ret = flags & O_NONBLOCK ? 0 : 1;

    if (blocking)
	flags &= ~O_NONBLOCK;
    else
	flags |= O_NONBLOCK;

    if (fcntl(fd, F_SETFL, flags) == -1)
	return -1;

    return ret;
}
