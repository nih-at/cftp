/*
  sockets -- auxiliary socket functions
  Copyright (C) 1996, 1997, 1998 Dieter Baron

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
        int s;
        struct hostent *hp;
        struct sockaddr_in sa;
	u_short port;
	struct servent *serv;

	if ((serv = getservbyname(service, "tcp")) == NULL) {
	    if ((port=atoi(service)) == 0 && service[0] != '0') {
		if (disp_active)
		    disp_status("unknown service: %s", service);
		else
		    fprintf(stderr, "%s: unknown service: %s\n",
			    prg, service);
		return(-1);
	    }
	    else
		port = htons(port);
	}
	else
	    port = (u_short)serv->s_port;

        if ((hp = gethostbyname(host)) == NULL) {
	    if (disp_active)
		disp_status("can't get host %s: %s",
			    host, hstrerror(h_errno));
	    else
                fprintf(stderr, "%s: can't get host %s: %s\n",
                        prg, host, hstrerror(h_errno));
	    return(-1);
        }
	memcpy(&sa.sin_addr, hp->h_addr, hp->h_length);
        sa.sin_family = hp->h_addrtype;
        sa.sin_port = port;
        if ((s = socket(hp->h_addrtype, SOCK_STREAM, 0)) < 0) {
	    if (disp_active)
		disp_status("can't allocate socket: %s\n",
			    strerror(errno));
	    else
                fprintf(stderr, "%s: can't allocate socket: %s\n",
                        prg, strerror(errno));
	    return(-1);
        }
        if (connect(s, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
	    if (disp_active)
		disp_status("can't connect: %s", strerror(errno));
	    else
                fprintf(stderr, "%s: can't connect: %s\n",
                        prg, strerror(errno));
	    return(-1);
        }
        return(s);
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
