#include <string.h>

char *
basename(char *name)
{
    char *p = strrchr(name, '/');

    return (p ? p+1 : name);
}
