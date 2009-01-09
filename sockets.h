#ifndef HAD_SOCKET_H
#define HAD_SOCKET_H

/*
  $NiH: sockets.h,v 1.9 2001/12/11 14:37:41 dillo Exp $

  sockets.h -- auxiliary socket functions
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



int sopen(char *host, char *service, int family);
int spassive(int family, struct sockaddr *addr, int *lenp);

const char *sockaddr_ntop(struct sockaddr *sa);



#ifndef HAVE_GETADDRINFO
/* provide definitions for getaddrinfo replacement */
struct addrinfo {
    int     ai_flags;     /* AI_PASSIVE, AI_CANONNAME, AI_NUMERICHOST */
    int     ai_family;    /* PF_xxx */
    int     ai_socktype;  /* SOCK_xxx */
    int     ai_protocol;  /* 0 or IPPROTO_xxx for IPv4 and IPv6 */
    size_t  ai_addrlen;   /* length of ai_addr */
    char   *ai_canonname; /* canonical name for nodename */
    struct sockaddr  *ai_addr; /* binary address */
    struct addrinfo  *ai_next; /* next structure in linked list */
};

#define AI_PASSIVE 0x00000001

#define EAI_ADDRFAMILY   1      /* address family for hostname not supported */
#define EAI_AGAIN        2      /* temporary failure in name resolution */
#define EAI_BADFLAGS     3      /* invalid value for ai_flags */
#define EAI_FAIL         4      /* non-recoverable failure in name resolution */
#define EAI_FAMILY       5      /* ai_family not supported */
#define EAI_MEMORY       6      /* memory allocation failure */
#define EAI_NODATA       7      /* no address associated with hostname */
#define EAI_NONAME       8      /* hostname nor servname provided, or not known
				 */
#define EAI_SERVICE      9      /* servname not supported for ai_socktype */
#define EAI_SOCKTYPE    10      /* ai_socktype not supported */
#define EAI_SYSTEM      11      /* system error returned in errno */
#define EAI_BADHINTS    12
#define EAI_PROTOCOL    13
#define EAI_MAX         14

int getaddrinfo(const char *nodename, const char *servname,
		const struct addrinfo *hints, struct addrinfo **res);
void freeaddrinfo(struct addrinfo *ai);
char *gai_strerror(int ecode);

#endif /* !HAVE_GETADDRINFO */

#endif /* socket.h */


