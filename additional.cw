@ additional routines that might be missing on some systems.

@ strdup (missing on nextstep)

@(strdup.c@)
#include <string.h>

char *
strdup(char *s)
{
	char *t;

	if (t=malloc(strlen(s)+1))
		strcpy(t, s);

	return t;
}


@ getcwd (missing on nextstep, if not in posix mode)

@(getcwd.c@)
#include <sys/types.h>

char *
getcwd(char *buf, size_t size)
{
	static char b[1024];

	getwd(b);

	if (buf == 0)
		return strdup(b);

	strncpy(buf, b, size);
	return buf;
}


@ getdomainname (missing on solaris)

@(getdomainname.c@)
#include <string.h>

int
getdomainname(char *name, int len)
{
	strncpy(name, DOMAINNAME, len);
	return 0;
}


@ strerror (missing on sunos)

@(strerror.c@)
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


@ fputchar

@(fputchar.c@)
#include <stdio.h>

int
fputchar(int c)
{
	putchar(c);
}
