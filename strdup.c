
#include <string.h>

char *
strdup(char *s)
{
	char *t;

	if (t=malloc(strlen(s)+1))
		strcpy(t, s);

	return t;
}


