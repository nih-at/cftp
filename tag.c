/*
  tag -- tagging
  Copyright (C) 1996 Dieter Baron

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
#include <string.h>
#include "tag.h"

#define tag_getdir(dir)	\
	((dirtags *)tag_do((filetags *)&tags, (dir), 0, NULL))
#define tag_insdir(dir)	\
	((dirtags *)tag_do((filetags *)&tags, (dir), 1, tag_newdir))

#define tag_filep(t, file)	\
	(tag_do((t), (file), 0, NULL) != NULL)
#define tag_insfile(t, file)	\
	(tag_do((t), (file), 1, tag_newfile))
#define tag_delfile(t, file)	\
	(tag_do((t), (file), -1, tag_freefile))

dirtags tags;
dirtags *curtags;



filetags *tag_do(filetags *root, char *name, int flag, filetags *(*mem)());
filetags *tag_newdir(filetags *dummy);
filetags *tag_freedir(filetags *d);
filetags *tag_newfile(filetags *dummy);
filetags *tag_freefile(filetags *f);



void
tag_changecurrent(char *dir)
{
    curtags = tag_getdir(dir);
}



int
tag_file(char *dir, char *file, long size, char type, int flag)
{
    extern char *ftp_lcwd;
    
    int filep;
    filetags *tl, *n;
 
    if (dir == NULL) {
	if (curtags == NULL) {
	    curtags = tag_insdir(ftp_lcwd);
	    if (curtags == NULL)
		return -1;
	}
	tl = curtags->tags;
    }
    else {
	dirtags *d;

	if ((d=tag_insdir(dir)) == NULL)
	    return -1;
	tl = d->tags;
    }

    filep = tag_filep(tl, file);
    
    if (filep && flag <= 0) {
	tag_delfile(tl, file);
	return 0;
    }
    else if (!filep && flag >= 0) {
	if (n=tag_insfile(tl, file)) {
	    n->size = size;
	    n->type = type;
	    return 1;
	}
	else
	    return 0;
    }
    else
	return filep;
}



filetags *
tag_do(filetags *root, char *name, int flag, filetags *(*mem)())
{
    filetags *n;
    int c = 1;

    while (root->next) {
	if ((c=strcmp(name, root->next->name)) <= 0)
	    break;
	root = root->next;
    }
	
    if (c == 0) {
	if (flag >= 0)
	    return root->next;
	else {
	    filetags *d = root->next;

	    if (d != NULL) {
		root->next = d->next;
		mem(d);
	    }
	    return NULL;
	}
    }

    if (flag <= 0)
	return NULL;

    n = mem(NULL);

    if (n != NULL) {
	n->name = strdup(name);
	n->next = root->next;
	root->next = n;
    }
    return n;
}



filetags *
tag_newdir(filetags *dummy)
{
    dirtags *d = (dirtags *)malloc(sizeof(dirtags));

    if (d) {
	d->next = NULL;
	d->tags = tag_newfile(NULL);
	d->name = NULL;
    }
    
    return (filetags *)d;
}



filetags *
tag_freedir(filetags *f)
{
    dirtags *d = (dirtags *)f;
    filetags *g;

    f=d->tags;
    while (f) {
	g=f;
	f=f->next;
	free(g->name);
	free(g);
    }
    free(d->name);
    free(d);

    return NULL;
}



filetags *
tag_newfile(filetags *dummy)
{
    filetags *f = (filetags *)malloc(sizeof(filetags));

    if (f) {
	f->name = NULL;
	f->next = NULL;
    }

    return f;
}



filetags *
tag_freefile(filetags *f)
{
    if (f) {
	free(f->name);
	free(f);
    }

    return NULL;
}



int
tag_anytags(void)
{
    dirtags *d;

    for (d=tags.next; d; d=d->next)
	if (d->tags && d->tags->next)
	    return 1;

    return 0;
}
