/*
  $NiH: tag.c,v 1.37 2003/05/13 15:46:08 dillo Exp $

  tag.c -- tagging
  Copyright (C) 1996-2002 Dieter Baron

  This file is part of cftp, a fullscreen ftp client
  The author can be contacted at <dillo@giga.or.at>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.
  3. The name of the author may not be used to endorse or promote
     products derived from this software without specific prior
     written permission.
 
  THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#include "tag.h"
#include "directory.h"
#include "options.h"
#include "list.h"
#include "util.h"
#include "bindings.h"

struct taglist tags;
struct tagentry tags_s;

static int _tag_insert(int n, struct tagentry *t, char *file,
		       off_t size, char type);
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
    int i, cmp;

    if (curdir) {
	for (i=0; i<curdir->len; i++)
	    curdir->line[i].line[0] = ' ';

	cmp = 1;

	for (t=tags_s.next; t != &tags_s && cmp >= 0; t = t->next) {
	    cmp = strncmp(dir->path, t->name, t->dirl);
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
tag_file(char *dir, char *file, off_t size, char type, enum tagopt what)
{
    char *canon;
    struct tagentry *t;
    int cmp;

    canon = NULL;

    if (dir == NULL)
	canon = canonical(file, NULL);
    else {
	if ((canon=(char *)malloc(strlen(dir)+strlen(file)+2)) == NULL)
	    return 0;
	sprintf(canon, "%s%s%s", dir,
		(strcmp(dir, "/") == 0 ? "" : "/"), file);
    }

    cmp = 1;

    for (t=tags_s.next; t != &tags_s; t = t->next) {
	if ((cmp=strcmp(canon, t->name)) <= 0)
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
_tag_insert(int n, struct tagentry *t, char *file, off_t size, char type)
{
    int i;
    char *line;
    struct tagentry *u;

    if ((line=(char *)malloc(strlen(file)+14)) == NULL)
	return -1;

    if (tags.len >= tags.reallen) {
	tags.reallen += 1024;
	if ((u=(struct tagentry *)realloc(tags.line, tags.reallen
						  *sizeof(struct tagentry)))
	    == NULL)
	    return -1;

	if (u != tags.line) {
	    /* tags cannot be empty since tags.line is full */ 
	    tags_s.next = u + (tags_s.next - &tags.line[0]);
	    tags_s.prev = u + (tags_s.prev - &tags.line[0]);
	    for (i=0; i<tags.len; i++) {
		if (u[i].prev != &tags_s)
		    u[i].prev = u + (tags.line[i].prev - &tags.line[0]);
		if (u[i].next != &tags_s)
		    u[i].next = u + (tags.line[i].next - &tags.line[0]);
	    }
	    tags.line = u;
	}
    }
    
    for (i=tags.len; i>n; --i) {
	tags.line[i].prev->next++;
	tags.line[i].next->prev++;
	tags.line[i+1] = tags.line[i];
    }

    sprintf(line, "%8lld  %c  %s", size, type, file);

    u = tags.line+n;

    u->line = line;
    u->name = file;
    u->file = noalloc_basename(file);
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

    return 0;
}



int
tag_anytags(void)
{
    return (tags_s.next != &tags_s);
}
    
