
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


