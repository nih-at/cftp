@ socket interface.

@(sockets.h@)
@<prototypes@>

@u
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

extern char *prg;


@ opening an active socket.

@d<prototypes@>
int sopen(char *host, char *service);

@u
int
sopen(char *host, char *service)
{
        int s;
        struct hostent *hp;
        struct sockaddr_in sa;
	u_short port;
	struct servent *serv;

	if ((serv = getservbyname(service, "tcp")) == NULL) {
		fprintf(stderr, "%s: can't get service %s: %s\n",
			prg, service, strerror(errno));
		return(-1);
	}
	port = (u_short)serv->s_port;

        if ((hp = gethostbyname(host)) == NULL) {
                fprintf(stderr, "%s: can't get host %s: %s\n",
                        prg, host, strerror(errno));
                return(-1);
        }
	memcpy(&sa.sin_addr, hp->h_addr, hp->h_length);
        sa.sin_family = hp->h_addrtype;
        sa.sin_port = port;
        if ((s = socket(hp->h_addrtype, SOCK_STREAM, 0)) < 0) {
                fprintf(stderr, "%s: can't allocate socket: %s\n",
                        prg, strerror(errno));
                return(-1);
        }
        if (connect(s, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
                fprintf(stderr, "%s: can't connect: %s\n",
                        prg, strerror(errno));
                return(-1);
        }
        return(s);
}


@ opening a passive socket (to any port).

@d<prototypes@>
int spassive(unsigned long *host, int *port);

@u
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
