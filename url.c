/*
  $NiH: url.c,v 1.6 2002/09/16 12:42:45 dillo Exp $

  url.c -- functions to parse and create URLs
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



#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "url.h"

#define R URL_UCHAR
#define B (URL_XCHAR|URL_UCHAR)

static char _url_spec[256] = {
    B, B, B, B, B, B, B, B,  B, B, B, B, B, B, B, B,  /* 0x00 - 0x0f */
    B, B, B, B, B, B, B, B,  B, B, B, B, B, B, B, B,  /* 0x10 - 0x1f */
    B, 0, B, B, 0, B, R, 0,  0, 0, 0, 0, 0, 0, 0, R,  /* 0x20 - 0x2f */
    0, 0, 0, 0, 0, 0, 0, 0,  0, 0, R, R, B, R, B, R,  /* 0x30 - 0x3f */
    R, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,  /* 0x40 - 0x4f */
    0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, B, B, B, B, 0,  /* 0x50 - 0x5f */
    B, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,  /* 0x60 - 0x6f */
    0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, B, B, B, B, B,  /* 0x70 - 0x7f */
    B, B, B, B, B, B, B, B,  B, B, B, B, B, B, B, B,  /* 0x80 - 0x8f */
    B, B, B, B, B, B, B, B,  B, B, B, B, B, B, B, B,  /* 0x90 - 0x9f */
    B, B, B, B, B, B, B, B,  B, B, B, B, B, B, B, B,  /* 0xa0 - 0xaf */
    B, B, B, B, B, B, B, B,  B, B, B, B, B, B, B, B,  /* 0xb0 - 0xbf */
    B, B, B, B, B, B, B, B,  B, B, B, B, B, B, B, B,  /* 0xc0 - 0xcf */
    B, B, B, B, B, B, B, B,  B, B, B, B, B, B, B, B,  /* 0xd0 - 0xdf */
    B, B, B, B, B, B, B, B,  B, B, B, B, B, B, B, B,  /* 0xe0 - 0xef */
    B, B, B, B, B, B, B, B,  B, B, B, B, B, B, B, B,  /* 0xf0 - 0xff */
};

#undef R
#undef B



int
url_special(int c, int which)
{
    if (c < 0 || c >= 256)
	return 1;
    else
	return _url_spec[c] & which;
}



int
url_enclen(char *s, int which)
{
    int len;

    len=0;
    while (*s)
	len += url_special(*(s++), which) ? 3 : 1;

    return len;
}



char *
url_encode(char *d, const char *s, int which)
{
    static char hex[] = "0123456789ABCDEF";

    char *p;

    for (p=d; *s; s++) {
	if (url_special(*s, which)) {
	    p[0] = '%';
	    p[1] = hex[((*s)>>4)&0xf];
	    p[2] = hex[(*s)&0xf];
	    p += 3;
	}
	else
	    *(p++) = *s;
    }
    *p = '\0';

    return d;
}



int
url_declen(char *s)
{
    int len;

    for (len=0; *s; len++) {
	if (s[0] == '%' && s[1] && isxdigit(s[1]) && s[2] && isxdigit(s[2]))
	    s += 3;
	else
	    s++;
    }

    return len;
}



char *
url_decode(char *d, const char *s)
{
    unsigned char *p, x[3];

    x[2] = 0;
    p = (unsigned char *)d;
    while (*s) {
	if (s[0] == '%' && s[1] && isxdigit(s[1]) && s[2] && isxdigit(s[2])) {
	    x[0] = s[1];
	    x[1] = s[2];
	    *p = strtoul((char *)x, NULL, 16);
	    s += 3;
	}
	else
	    *(p++) = *(s++);
    }
    *p = '\0';

    return d;
}



int
parse_url(char *url, int *proto, char **user, char **pass,
	  char **host, char **port, char **dir)
{
    char *p, *q, *h, *r;

    if (strncmp(url, "ftp://", 6) == 0) {
	url += 6;
	*proto = 0;
    }
    else if (strncmp(url, "sftp://", 7) == 0) {
	url += 7;
	*proto = 1;
    }
    else
	return -1;

    if ((p=strchr(url, '/')) != NULL)
	*(p++) = '\0';
    else
	p = url+strlen(url);
    
    if ((q=strrchr(url, '@')) != NULL) {
	*q = '\0';
	if ((r=strchr(url, ':')) != NULL) {
	    *(r++) = '\0';
	    *pass = malloc(url_declen(r)+1);
	    url_decode(*pass, r);
	}
	*user = malloc(url_declen(url)+1);
	url_decode(*user, url);
	url = q+1;
    }

    if (url[0] == '[' && (h=strchr(url, ']')) != NULL
	&& (h[1] == ':' || h[1] == '\0')) {
	url++;
	*(h++) = '\0';
    }
    else
	h = url;

    if ((q=strchr(h, ':')) != NULL) {
	*q = '\0';
	if (*(q+1) != '\0') {
	    *port = malloc(url_declen(q+1)+1);
	    url_decode(*port, q+1);
	}
    }

    *host = malloc(url_declen(url)+1);
    url_decode(*host, url);

    if (p && *p != '\0') {
	*dir = malloc(url_declen(p)+1);
	url_decode(*dir, p);
    }

    return 0;
}



int
is_url(char *str)
{
    return (strncmp(str, "ftp://", 6) == 0
#ifdef USE_SFTP
	    || strncmp(str, "sftp://", 7) == 0
#endif
	);
}
