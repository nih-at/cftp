/*
  sockets -- auxiliary socket functions
  Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001 Dieter Baron

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
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>

#include "config.h"
#include "display.h"
#include "sockets.h"

extern char *prg;



int
sopen(char *host, char *service, int family)
{
    struct addrinfo hints, *res0, *res;
    int s, err;
    char *cause;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = family;
    hints.ai_socktype = SOCK_STREAM;
    
    if ((err=getaddrinfo(host, service, &hints, &res0)) != 0) {
	if (disp_active)
	    disp_status("cannot get host/service %s/%s: %s\n",
			host, service, gai_strerror(err));
	else
	    fprintf(stderr, "%s: can't get host/service %s/%s: %s\n",
		    prg, host, service, gai_strerror(err));
	return -1;
    }

    s = -1;
    for (res = res0; res; res = res->ai_next) {
	if ((s=socket(res->ai_family, res->ai_socktype,
		      res->ai_protocol)) < 0) {
	    cause = "create socket";
	    continue;
	}

	if (connect(s, res->ai_addr, res->ai_addrlen) < 0) {
	    cause = "connect";
	    close(s);
	    s = -1;
	    continue;
	}

	/* okay we got one */
	break;
    }
    if (s < 0) {
	if (disp_active)
	    disp_status("cannot %s: %s", cause, strerror(errno));
	else
	    fprintf(stderr, "%s: cannot %s: %s\n",
		    prg, cause, strerror(errno));
    }
    freeaddrinfo(res0);

    return s;
}



int
spassive(int family, struct sockaddr *addr, int *lenp)
{
    struct addrinfo hints, *res, *res0;
    int s;
    char *cause;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = family;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, "0", &hints, &res0) != 0) {
	if (disp_active)
	    disp_status("cannot get address info: %s\n",
			gai_strerror(errno));
	else
	    fprintf(stderr, "%s: cannot get address info: %s\n",
		    prg, gai_strerror(errno));
	return -1;
    }
	
    for (res = res0; res; res = res->ai_next) {
	if ((s=socket(res->ai_family, res->ai_socktype,
		      res->ai_protocol)) < 0) {
	    cause = "create socket";
	    continue;
	}

	if (bind(s, res->ai_addr, res->ai_addrlen) < 0) {
	    cause = "bind";
	    close(s);
	    continue;
	}

	if (getsockname(s, addr, lenp) == -1) {
	    cause = "get socket name";
	    close(s);
	    continue;
	}

	if (listen(s, 1) < 0) {
	    cause = "listen";
	    close(s);
	    continue;
	}
	
	freeaddrinfo(res0);
	return s;
    }
    
    freeaddrinfo(res0);

    if (disp_active)
	disp_status("cannot %s: %s", cause, strerror(errno));
    else
	fprintf(stderr, "%s: cannot %s: %s",
		prg, cause, strerror(errno));
    return -1;
}



const char *
sockaddr_ntop(struct sockaddr *sa)
{
#ifdef HAVE_GETNAMEINFO

    static char addrbuf[NI_MAXHOST];
    int len;

#ifdef HAVE_STRUCT_MEMBER_SOCKADDR_SA_LEN
    len = sa->sa_len;
#else
    if (sa->sa_family == AF_INET)
	len = sizeof(struct sockaddr_in);
    else
	len = sizeof(struct sockaddr_in6);
#endif

    if (getnameinfo(sa, len, addrbuf, sizeof(addrbuf),
            NULL, 0, NI_NUMERICHOST) == 0)
        return addrbuf;
    else
        return NULL;

#else /* !HAVE_GETNAMEINFO */

    struct sockaddr_in *si;
    if (sa->sa_family != AF_INET)
	return NULL;

    si = (struct sockaddr_in *)sa;

    return inet_ntoa(si->sin_addr);

#endif
}

