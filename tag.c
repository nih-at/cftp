/*
  tag -- tagging
  Copyright (C) 1996, 1997, 1998, 1999 Dieter Baron

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
#include "bindings.h"

struct taglist tags;
struct tagentry tags_s;

static int _tag_insert(int n, struct tagentry *t, char *file,
		       long size, char type);
static int _tag_update_curdir(struct tagentry *u, enum tagopt what);



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

    return 0;
}



void
change_curdir(directory *dir)
{
    struct tagentry *t;
    int i, cmp, c;

    if (curdir) {
	for (i=0; i<curdir->len; i++)
	    curdir->line[i].line[0] = ' ';

	cmp = 1;

	for (t=tags_s.next; t != &tags_s && cmp >= 0; t = t->next) {
	    c = t->name[t->dirl];
	    t->name[t->dirl] = '\0';
	    cmp = strcmp(dir->path, t->name);
	    t->name[t->dirl] = c;
	    if (cmp == 0 && (i=dir_find(dir, t->file)) >= 0) {
		dir->line[i].line[0] = opt_tagchar;
	    }
	}
	curdir->top = curdir->cur = 0;
    }
    
    curdir = dir;
    if (binding_state == bs_remote)
	list = (struct list *)curdir;
}



int
tag_file(char *dir, char *file, long size, char type, enum tagopt what)
{
    char *canon, c;
    struct tagentry *t;
    int freedirp, cmp;

    canon = NULL;

    if (dir == NULL) {
	canon = canonical(file, NULL);
	if ((dir=dirname(canon)) == NULL) {
	    free(canon);
	    return 0;
	}
	freedirp = 1;
	file = (char *)basename(canon);
    }
    else {
	if ((canon=(char *)malloc(strlen(dir)+strlen(file)+2)) == NULL)
	    return 0;
	sprintf(canon, "%s%s%s", dir,
		(strcmp(dir, "/") == 0 ? "" : "/"), file);
    }

    cmp = 1;

    for (t=tags_s.next; t != &tags_s; t = t->next) {
	c = t->name[t->dirl];
	t->name[t->dirl] = '\0';
	cmp = strcmp(dir, t->name);
	t->name[t->dirl] = c;
	if (cmp == 0)
	    if ((cmp=strcmp(file, t->file)) <= 0)
		break;
    }

    if (t != &tags_s && cmp == 0) {
	if (what == TAG_ON) {
	    free(canon);
	    return 0;
	}

	tag_delete(t-tags.line);
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
	_tag_update_curdir(&tags.line[i], TAG_OFF);
	free(tags.line[i].line);
	free(tags.line[i].name);
    }

    tags.len = 0;
}



void
tag_delete(int n)
{
    int i;
    struct tagentry *t;

    t = tags.line+n;

    _tag_update_curdir(t, TAG_OFF);

    free(t->line);
    free(t->name);

    t->next->prev = t->prev;
    t->prev->next = t->next;

    --tags.len;
    
    for (i=n; i<tags.len; i++) {
	tags.line[i] = tags.line[i+1];
	tags.line[i].prev->next--;
	tags.line[i].next->prev--;
    }
}



static int
_tag_insert(int n, struct tagentry *t, char *file, long size, char type)
{
    int i;
    char *line;
    struct tagentry *u;

    if ((line=(char *)malloc(strlen(file)+14)) == NULL)
	return -1;

    if (tags.len >= tags.reallen) {
	tags.reallen += 1024;
	if ((tags.line=(struct tagentry *)realloc(tags.line, tags.reallen
						  *sizeof(struct tagentry)))
	    == NULL)
	    return -1;
    }
    
    for (i=tags.len; i>n; --i) {
	tags.line[i].prev->next++;
	tags.line[i].next->prev++;
	tags.line[i+1] = tags.line[i];
    }

    sprintf(line, "%8ld  %c  %s", size, type, file);

    u = tags.line+n;

    u->line = line;
    u->name = file;
    u->file = (char *)basename(file);
    u->dirl = tags.line[n].file - file;
    if (u->dirl > 1)
	--u->dirl;
    u->size = size;
    u->type = type;

    u->next = t->next;
    u->prev = t;
    t->next = u;
    u->next->prev = u;

    _tag_update_curdir(u, TAG_ON);

    tags.len++;
    
    return 0;
}



static int
_tag_update_curdir(struct tagentry *u, enum tagopt what)
{
    int i;

    if (strlen(curdir->path) == u->dirl
	&& strncmp(curdir->path, u->name, u->dirl) == 0) {
	i = dir_find(curdir, u->file);
	
	if (i >= 0
	    && (curdir->line[i].line[0] !=
		(what == TAG_ON) ? opt_tagchar : ' ')) {
	    curdir->line[i].line[0] =
		(what == TAG_ON) ? opt_tagchar : ' ';
	    if (binding_state == bs_remote)
		list_reline(i);
	}
    }
}



int
tag_anytags(void)
{
    return (tags_s.next != &tags_s);
}
    
