/*
  $NiH: sockets.c,v 1.24 2001/12/20 05:44:15 dillo Exp $

  sockets.c -- auxiliary socket functions
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
	disp_status(DISP_ERROR, "cannot get host/service %s/%s: %s\n",
		    host, service, gai_strerror(err));
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
    if (s < 0)
	disp_status(DISP_ERROR, "cannot %s: %s", cause, strerror(errno));

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
	disp_status(DISP_ERROR, "cannot get address info: %s\n",
		    gai_strerror(errno));
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

    disp_status(DISP_ERROR, "cannot %s: %s", cause, strerror(errno));
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

