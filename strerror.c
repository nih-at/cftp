
#include <stdio.h>

extern char *sys_errlist[];
extern int sys_nerr;

int
strerror(int errno)
{
	static char buf[512];

	if (errno < sys_nerr)
		return sys_errlist[errno];
	
	sprintf(buf, "Unknown error: %u", errno);
	return buf;
}


