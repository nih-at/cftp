/*
  url -- functions to parse and create URLs
  Copyright (C) 1996, 1997, 1998, 1999, 2000 Dieter Baron

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



#include <ctype.h>
#include <stdlib.h>
#include <string.h>

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
	    *p = strtoul(x, NULL, 16);
	    s += 3;
	}
	else
	    *(p++) = *(s++);
    }
    *p = '\0';

    return d;
}



int
parse_url(char *url, char **user, char **pass,
	  char **host, char **port, char **dir)
{
    char *p, *q, *h, *r;

    if (strncmp(url, "ftp://", 6) != 0)
	return -1;
    url+=6;

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
