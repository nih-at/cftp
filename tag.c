/*
  tag -- tagging
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
#include <string.h>
#include "tag.h"
#include "directory.h"
#include "options.h"
#include "list.h"
#include "util.h"

struct taglist tags;
struct tagentry tags_s;

void _tag_delete(int n);
int _tag_insert(int n, struct tagentry *t, char *file, long size, char type);



int
tag_init(void)
{
    tags_s.next = tags_s.prev = &tags_s;
    tags.len = tags.cur = tags.top = 0;
    tags.size = sizeof(struct tagentry);
    tags.reallen = 1024;
    if ((tags.line=(struct tagentry *)malloc(tags.reallen
					     *sizeof(struct tagentry)))
	== NULL)
	return -1;
}



void
change_curdir(directory *dir)
{
    /* XXX: rewrite to new structures */

    extern struct dirtags *curtags;

    struct filetags *tags;
    int i;

    for (i=0; i<curdir->len; i++)
	curdir->line[i].line[0] = ' ';

/*    curtags = tag_getdir(dir, 0);

    if (curtags) {
	curtags = curtags->next;
	for (tags=curtags->tags.next; tags; tags=tags->next)
	    if ((i=dir_find(dir, tags->name)) >= 0)
		dir->line[i].line[0] = opt_tagchar;
    }
    */
    curdir = dir;
    list = (struct list *)curdir;
}



int
tag_file(char *dir, char *file, long size, char type, enum tagopt what)
{
    char *canon;
    struct tagentry *t;

    canon = NULL;

    if (dir == NULL) {
	canon = canonical(file, NULL);
    }
    else {
	if ((canon=(char *)malloc(strlen(dir)+strlen(file)+2)) == NULL)
	    return 0;
	sprintf(canon, "%s/%s", dir, file);
    }

    for (t=tags_s.next;
	 t != &tags_s && strncmp(dir, t->name, t->dirl) < 0;
	 t=t->next)
	;

    if (strncmp(dir, t->name, t->dirl) == 0) {
	if (what == TAG_ON) {
	    free(canon);
	    return 0;
	}

	_tag_delete(t-tags.line);
	return -1;
    }
    else {
	if (what == TAG_OFF) {
	    free(canon);
	    return 0;
	}

	_tag_insert(tags.len, t->prev, canon, size, type);
	return 1;
    }
}



void
tag_clear(void)
{
    int i;

    tags_s.next = tags_s.prev = &tags_s;
    
    for (i=0; i<tags.len; i++) {
	free(tags.line[i].line);
	free(tags.line[i].name);
    }

    tags.len = 0;
}



void
_tag_delete(int n)
{
    struct tagentry *t;

    t = tags.line[n];

    free(t->line);
    free(t->file);

    for (i=n; i>tags.len; i++) {
	tags.line[i] = tags.line[i+1];
	tags.line[i].prev->next--;
	tags.line[i].next->prev--;
    }
}



int
_tag_insert(int n, struct tagentry *t, char *file, long size, char type)
{
    int i;
    char *line;
    struct tagentry *u;

    if ((line=(char *)malloc(strlen(file)+14)) == NULL)
	return -1;

    /* XXX: tags.line overflow */
    
    for (i=tags.len; i>n; --i) {
	tags.line[i].prev->next++;
	tags.line[i].next->prev++;
	tags.line[i+1] = tags.line[i];
    }

    sprintf(line, "%8ld  %c  %s", size, type, file);

    u = tags.line[n];

    u->line = line;
    u->name = file;
    u->file = basename(file);
    u->dirl = tags.line[n].file - file;
    u->size = size;
    u->type = type;

    u->next = t->next;
    u->prev = t;
    t->next = u;
    u->next->prev = u;

    return 0;
}



int
tag_anytags(void)
{
    return (tags_s.next != &tags_s);
}
    
