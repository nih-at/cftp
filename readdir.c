#include "directory.h"
#include "ftp.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int parse_unix(direntry *de, char *line);



#define DIR_STEP	1024

directory *
read_dir(FILE *f)
{
	directory *dir;
	direntry *list;
	int i, n, sz;
	char *line;

	dir = malloc(sizeof(directory));
	list = NULL;
	sz = n = 0;
	
	while ((line=ftp_gets(f)) != NULL) {
		if (n >= sz) {
			sz += DIR_STEP;
			if (list == NULL)
				list = malloc(sizeof(direntry)*sz);
			else
				list = realloc(list, sizeof(direntry)*sz);
		}
		if (parse_unix(list+n, line) == 0)
			n++;
	}

	if (n == 0) {
		dir->list = (direntry *)malloc(sizeof(direntry));
		dir->list->line = strdup("");
		dir->list->type = 'x';
		dir->list->name = dir->list->link = NULL;
		n = 1;
	}
	else {
		dir->list = malloc(sizeof(direntry)*n);
		for (i=0; i<n; i++)
			dir->list[i] = list[i];
	}
	dir->num = n;

	return dir;
}



int
parse_unix(direntry *de, char *line)
{
    char *p, *q;
	
    if (strncmp(line, "total ", 6) == 0)
	return 1;

    if ((de->line=(char *)malloc(strlen(line)+2)) == NULL)
	return 1;
    
    de->line[0] = ' ';
    strcpy(de->line+1, line);

    switch (line[0]) {
    case 'l':
    case 'd':
	de->type = line[0];
	break;
    case '-':
    case 'p':
	de->type = 'f';
	break;
    default:
	de->type = 'x';
    }

    strtok(line+10, " ");	/* skip perms, links */
    strtok(NULL, " ");	/* owner */
    strtok(NULL, " ");	/* group */
    if ((p=strtok(NULL, " ")) == NULL) {
	free(line);
	return 1;
    }
    de->size = strtol(p, &q, 10);
			/* size  */
    if (p == q)
	de->size = -1;
    else if ((p=strtok(NULL, " ")) == NULL) {
	free(line);
	return 1;
    }
    p += 13;
    
    if (de->type == 'l') {
	q = p + strlen(p) - de->size;
	q[-4] = '\0';
	de->name = strdup(p);
	de->link = strdup(q);
	de->size = -1;
    }
    else {
	de->name = strdup(p);
	de->link = NULL;
    }

    free(line);	

    if (de->type == 'd' && (strcmp(de->name, "..")==0 ||
			    strcmp(de->name, ".")==0)) {
	free(de->name);
	return 1;
    }

    return 0;
}
