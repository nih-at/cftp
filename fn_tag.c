/*
  fn_tag -- bindable functions: 
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

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "directory.h"
#include "bindings.h"
#include "functions.h"
#include "display.h"
#include "options.h"
#include "ftp.h"
#include "tag.h"
#include "util.h"



void
fn_tag(char **args)
{
    char *dir, *file, *base;
    long size;
    char type;
    int tagged, i;

    if (args) {
	dir = NULL;
	file = canonical(args[0], NULL);
	size = -1;
	type = 'l';

	base = basename(file);

	i = strlen(curdir->path);
	if ((base-file == 1 && i == 1)
	    || i == base-file-1 && strncmp(curdir->path, file, i) == 0) {
	    i = dir_find(curdir->path, base);
	}
	else
	    i = -1;
    }
    else {
	/* works in <remote> only */
	
	dir = curdir->path;
	file = curdir->line[curdir->cur].name;
	size = curdir->line[curdir->cur].size;
	type = curdir->line[curdir->cur].type;

	i = curdir->cur;
    }
	
    tagged = tag_file(dir, file, size, type, TAG_TOGGLE);

    if (tagged && i > 0 && binding_state == bs_remote) {
	curdir->line[curdir->cur].line[0] =
	    (tagged < 0 ? ' ' : opt_tagchar);
	list_reline(i);
    }

    if (tagged < -1)
	disp_status("%d files untagged", -tagged);
    else if (tagged == -1)
	disp_status("1 file untagged");
    else if (tagged == 1)
	disp_status("1 file tagged");
    else if (tagged > 1)
	disp_status("%d files tagged", tagged);

    if (args)
	free(file);
}



void
fn_cleartags(char **args)
{
    struct dirtags *p, *q;
    int i;

    tag_clear();

    disp_status("all tags cleared");
}



void
fn_listtags(char **args)
{
#if 0
    FILE *f = NULL;
    dirtags *d;
    filetags *t;

    if (!tag_anytags()) {
	disp_status("no tags.");
	return;
    }

    if (args)
	if ((f=fopen(args[0], "w")) == NULL) {
	    disp_status("can't open `%s': %s", args[0], strerror(errno));
	    return;
	}
	
    for (d=tags.next; d; d=d->next)
	for (t=d->tags->next; t; t=t->next) {
	    if (f == NULL)
		if ((f=disp_open(-1)) == NULL)
		    return;
	    fprintf(f, "%8ld  %c  %s%s%s\n",
		    t->size,
		    t->type,
		    d->name,
		    (strcmp(d->name, "/") == 0 ? "" : "/"),
		    t->name);
	}

    if (f) {
	if (args)
	    fclose(f);
	else
	    disp_close(f);
    }
#endif
}
    


void
fn_gettags(char **args)
{
#if 0
    dirtags *d, *od;
    filetags *t, *o;
    char name[8192];
    int i, beepp;

    beepp = (tags.next != NULL);

    for (d=&tags; d->next;) {
	for (t=d->next->tags; t->next;) {
	    sprintf(name, "%s%s%s",
		    d->next->name,
		    (strcmp(d->next->name, "/") == 0 ? "" : "/"),
		    t->next->name);
	    if (aux_download(name, t->next->size) == 0) {
		if (strcmp(d->next->name, curdir->path) == 0
		    && ((i=dir_find(curdir, t->next->name)) >= 0)) {
		    curdir->line[i].line[0] = ' ';
		    /* XXX: disp_reline(i); */
		}
		free(t->next->name);
		o = t->next;
		t->next = o->next;
		free(o);
	    }
	    else
		t = t->next;
	}
	if (d->next->tags->next == NULL) {
	    od = d->next;
	    free(od->tags);
	    free(od->name);
	    d->next = od->next;
	    if (od == curtags)
		curtags = NULL;
	    free(od);
	}
	else
	    d = d->next;
    }
    if (beepp && opt_beep) {
	fputc('\a', stdout);
	fflush(stdout);
    }
#endif
}



void
fn_loadtag(char **args)
{
    char *fname, *line, *p, *e;
    FILE *f;
    int count, len;
    char *name, *dir, *file;
    long size;
    int type;
    
    if (args)
	fname = args[0];
    else
	fname = read_string("File: ", 1);

    if ((f=fopen(fname, "r")) == NULL) {
	disp_status("can't open `%s': %s", fname, strerror(errno));
	return;
    }

    count = 0;
    while ((line=ftp_gets(f)) != NULL) {
	p = line;
	size = strtol(p, &e, 10);
	if (size < -1 ||
	    e == NULL || e == p || (*e != '\0' && !isspace(*e))) {
	    size = -1;
	}
	else
	    p = e;

	p += strspn(p, " \t\n");
	if (isspace(p[1])) {
	    switch (p[0]) {
	    case 'l':
	    case 'd':
		type = p[0];
		break;
	    case 'f':
	    case '-':
	    case 'p':
		type = 'f';
		break;
	    default:
		type = 'x';
	    }
	    p++;
	}
	else
	    type = 'l';

	p += strspn(p, " \t\n");
	if (len=strlen(p)) {
	    if (p[len-1] == '\n')
		p[len-1] = '\0';

	    name = canonical(p, NULL);
	    count += tag_file(NULL, name, size, type, TAG_ON);
	    free(name);
	}
    }

    if (ferror(f))
	disp_status("read error: %s\n", strerror(errno));
    else
	disp_status("%d file%s tagged", count, (count == 1 ? "" : "s"));

    fclose(f);
}
