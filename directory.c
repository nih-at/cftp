#include <stdio.h>
#include <string.h>
#include "directory.h"
#include "ftp.h"

typedef struct dircache {
	directory *dir;

	struct dircache *next, *prev;
} dircache;

dircache *cache_head = NULL, *cache_tail = NULL;
int cache_fill;
int cache_size = 30;

void cache_remove(dircache *d);
void cache_insert(dircache *d);



void
dir_free(directory *d)
{
	int i;

	if (d == NULL)
		return;
	
	for (i=0; i<d->num; i++) {
		free(d->list[i].line);
		free(d->list[i].name);
		free(d->list[i].link);
	}
	free(d->list);
	free(d->path);
	free(d);
}



directory *
get_dir(char *path)
{
	dircache *d;
	
	/* initialize queue */
	if (cache_head == NULL) {
		cache_head = (dircache *)malloc(sizeof(dircache));
		cache_tail = (dircache *)malloc(sizeof(dircache));
		cache_head->prev = cache_tail->prev = cache_head;
		cache_head->next = cache_tail->next = cache_tail;
		cache_head->dir = cache_tail->dir = NULL;
		cache_fill = 0;
	}

	/* find dir */
	for (d=cache_head->next; (d!=cache_tail &&
				  strcmp(path, d->dir->path) != 0);
	     d=d->next)
		;

	if (d != cache_tail) {
		/* found: put d to front of queue and return it */
		cache_remove(d);
		cache_insert(d);

		return d->dir;
	}

	/* not found: if cache full, recycle last entry */
	if (cache_fill >= cache_size) {
		d = cache_tail->prev;
		cache_remove(d);
		dir_free(d->dir);
	}
	else {
		d = (dircache *)malloc(sizeof(dircache));
		cache_fill++;
	}

	d->dir = ftp_list(path);
	if (d->dir) {
		d->dir->path = strdup(path);
		cache_insert(d);
	}
	
	return d->dir;
}



void
cache_remove(dircache *d)
{
	d->prev->next = d->next;
	d->next->prev = d->prev;
	d->next = d->prev = NULL;
}



void
cache_insert(dircache *d)
{
	d->prev = cache_head;
	d->next = cache_head->next;
	cache_head->next->prev = d;
	cache_head->next = d;
}



int
dir_find(directory *dir, char *entry)
{
    int i;

    for (i=0; (i<dir->num &&
		 strcmp(dir->list[i].name, entry)); i++)
	;
    
    if (i == dir->num)
	i = -1;

    return i;
}
