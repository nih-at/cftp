@ tagging files;

@(fn_tag.fn@)
section(fn_tag, Tagging Functions)
@<functions@>
endsec()


@u
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "directory.h"
#include "functions.h"
#include "display.h"
#include "options.h"
#include "ftp.h"
#include "tag.h"
#include "util.h"


@d<functions@>
function(tag, [file], fn_tag, 0,
	 {tag/untag file for later download},
 {})


@u
void
fn_tag(char **args)
{
    char *name, *dir, *file;
    long size;
    char type;
    int tagged, i;

    if (args) {
	name= canonical(args[0], NULL);
	dir = dirname(name);
	file = basename(name);
	size = -1;
	type = 'l';
    }
    else {
	name = NULL;
	dir = NULL;
	file = curdir->list[cursel].name;
	size = curdir->list[cursel].size;
	type = curdir->list[cursel].type;
    }
	
    if ((tagged=tag_file(dir, file, size, type, 0)) < 0)
	return;
    
    if (args) {
	if (strcmp(dir, curdir->path) != 0)
	    i = -1;
	else
	    i = dir_find(curdir, file);
    }
    else
	i = cursel;

    if (i >= 0) {
	if (tagged)
	    curdir->list[i].line[0] = opt_tagchar;
	else
	    curdir->list[i].line[0] = ' ';

	disp_reline(i);
    }

    free(name);
    free(dir);
}


@ clear tags.

@d<functions@>
function(clear-tags, , fn_cleartags, 0,
	 {clear all tags},
 {})

@u
void
fn_cleartags(char **args)
{
    dirtags *p, *q;
    int i;

    p = tags.next;
    while (p) {
	q = p->next;
	tag_freedir(p);
	p = q;
    }

    tags.next = NULL;
    curtags = NULL;

    for (i=0; i<curdir->num; i++)
	if (curdir->list[i].line[0] != ' ') {
	    curdir->list[i].line[0] = ' ';
	    disp_reline(i);
	}
}

@ listing tags.

@d<functions@>
function(list-tags, [file], fn_listtags, 0,
	 {list tagged files},
 {})


@u
void
fn_listtags(char **args)
{
    FILE *f = NULL;
    dirtags *d;
    filetags *t;

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
}
    

@ getting taged files

@d<functions@>
function(get-tags, {}, fn_gettags, 0,
	 {get tagged files},
 {})


@u
void
fn_gettags(char **args)
{
    dirtags *d, *od;
    filetags *t, *o;
    char name[8192];
    int i;

    for (d=&tags; d->next;) {
	for (t=d->next->tags; t->next;) {
	    sprintf(name, "%s%s%s",
		    d->next->name,
		    (strcmp(d->next->name, "/") == 0 ? "" : "/"),
		    t->next->name);
	    if (aux_download(name, t->next->size) == 0) {
		if (strcmp(d->next->name, curdir->path) == 0
		    && ((i=dir_find(curdir, t->next->name)) >= 0)) {
		    curdir->list[i].line[0] = ' ';
		    disp_reline(i);
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
}


@ read in tags list

@d<functions@>
function(load-tags, [file], fn_loadtag, 0,
	 {load tags list from file},
 {})


@u
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
	fname = read_string("File: ");

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

	    name = canonical(p, (ftp_anon ? "/" : "~"));
	    file = basename(name);
	    dir = dirname(name);
	    tag_file(dir, file, size, type, 1);
	    if (strcmp(curdir->path, dir) == 0
		&& (len=dir_find(curdir, file)) >= 0
		&& curdir->list[len].line[0] != opt_tagchar) {
		curdir->list[len].line[0] = opt_tagchar;
		disp_reline(len);
	    }

	    count++;
	}
    }

    if (ferror(f))
	disp_status("read error: %s\n", strerror(errno));
    else
	disp_status("%d files tagged", count);

    fclose(f);
}
