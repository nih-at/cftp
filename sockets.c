/*
  sockets -- auxiliary socket functions
  Copyright (C) 1996, 1997, 1998, 1999 Dieter Baron

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
#include <errno.h>

#include "display.h"

extern char *prg;

#ifndef H_ERRNO_DECLARED
extern int h_errno;
#endif



int
sopen(char *host, char *service)
{
    struct addrinfo hints, *res0, *res;
    int s, err;
    char *cause;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
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
	    disp_status("cannot %s: %s\n",
			cause, strerror(errno));
	else
	    fprintf(stderr, "%s: cannot %s: %s\n",
		    prg, cause, strerror(errno));
    }
    freeaddrinfo(res0);

    return s;
}



int
spassive(unsigned long *host, int *port)
{
    int s, len;
    struct sockaddr_in locaddr;
    extern int getport();
    
    if ((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
	fprintf(stderr, "%s: can't allocate socket: %s\n",
		prg, strerror(errno));
	return(-1);
    }
    
    locaddr.sin_family = AF_INET;
    locaddr.sin_addr.s_addr = INADDR_ANY;
    memset(locaddr.sin_zero, 0, 8);
    locaddr.sin_port = 0;
    
    if (bind(s, (struct sockaddr *)&locaddr, sizeof(struct sockaddr_in))
	== -1) {
	fprintf(stderr, "%s: can't bind socket: %s\n",
		prg, strerror(errno));
	return(-1);
    }
    
    len = sizeof(locaddr);
    if (getsockname(s, (struct sockaddr *)&locaddr, &len) == -1) {
	fprintf(stderr, "%s: can't get socket name: %s\n",
		prg, strerror(errno));
	return(-1);
    }
    *host = (unsigned long)locaddr.sin_addr.s_addr;
    *port = ntohs(locaddr.sin_port);
    
    if (listen(s, 1) == -1) {
	fprintf(stderr, "%s: can't listen on socket: %s\n",
		prg, strerror(errno));
	return(-1);
    }
    
    return s;
}
