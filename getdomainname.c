
#include <string.h>

int
getdomainname(char *name, int len)
{
	strncpy(name, DOMAINNAME, len);
	return 0;
}


