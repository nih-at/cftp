/*
  direcotry -- handle directory cache
  Copyright (C) 1996, 1997 Dieter Baron

  This file is part of cftp, a fullscreen ftp client
  The author can be contacted at <dillo@giga.or.at>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "directory.h"
#include "ftp.h"

directory *curdir;

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
	
	for (i=0; i<d->len; i++) {
		free(d->line[i].line);
		free(d->line[i].name);
		free(d->line[i].link);
	}
	free(d->line);
	free(d->path);
	free(d);
}



directory *
get_dir(char *path)
{
	dircache *d;
	directory *dir;
	
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

	dir = ftp_list(path);
	if (dir == NULL)
	    return NULL;

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

	dir->path = strdup(path);
	d->dir = dir;
	cache_insert(d);
	
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

    for (i=0; (i<dir->len &&
		 strcmp(dir->line[i].name, entry)); i++)
	;
    
    if (i == dir->len)
	i = -1;

    return i;
}
