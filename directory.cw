@ structure and routines to handle directory listings.

@(directory.h@)
@<types@>
@<prototypes@>

@u
#include <stdio.h>
#include <string.h>
#include "directory.h"
#include "ftp.h"
@<local types@>
@<local prototypes@>
@<local globals@>


@ a directory entry.

line is what is displayed, name is the actual name; type is either
'd'irecotry, sym'l'ink, 'f'ile or 'x' for unknown or special.

@d<types@>
typedef struct direntry {
	char *line;
	char *name, *link;
	long size;
	char type;
} direntry;


@ a directory

@d<types@>
typedef struct directory {
	char *path;
	int num;
	struct direntry *list;
} directory;


@ freeing a directory structure.

@d<prototypes@>
void dir_free(directory *d);

@u
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


@ the cache of directories.

a cache of cache_size directory listings is maintained, recycled on a
least recent use basis.

@d<local types@>

typedef struct dircache {
	directory *dir;

	struct dircache *next, *prev;
} dircache;

@d<local globals@>
dircache *cache_head = NULL, *cache_tail = NULL;
int cache_fill;
int cache_size = 30;


@ high level cache access function.

@d<prototypes@>
directory *get_dir(char *path);

@u
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


@ manipulating the queue.

@d<local prototypes@>
void cache_remove(dircache *d);
void cache_insert(dircache *d);

@u
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
