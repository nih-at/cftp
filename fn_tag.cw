@ tagging files;

@(fn_tag.fn@)
; fn_tag
@<functions@>

@u
#include <stdlib.h>
#include "directory.h"
#include "functions.h"
#include "display.h"
#include "options.h"
#include "tag.h"
#include "util.h"


@d<functions@>
  { fn_tag, "tag", 0, "tag/untag file for later download" }

@u
void
fn_tag(char **args)
{
    char *name, *dir, *file;
    long size;
    int tagged, i;

    if (args) {
	name= canonical(args[0], NULL);
	dir = dirname(name);
	file = basename(name);
	size = -1;
    }
    else {
	name = NULL;
	dir = NULL;
	file = curdir->list[cursel].name;
	size = curdir->list[cursel].size;
    }
	
    if ((tagged=tag_file(dir, file, size, 0)) < 0)
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


@ listing tags.

@d<functions@>
  { fn_listtags, "list-tags", 0, "list tagged files" }

@u
void
fn_listtags(char **args)
{
    FILE *f = NULL;
    dirtags *d;
    filetags *t;

    for (d=tags.next; d; d=d->next)
	for (t=d->tags->next; t; t=t->next) {
	    if (f == NULL)
		if ((f=disp_open(-1)) == NULL)
		    return;
	    fprintf(f, "%8ld  %s%s%s\n",
		    t->size,
		    d->name,
		    (strcmp(d->name, "/") == 0 ? "" : "/"),
		    t->name);
	}

    if (f)
	disp_close(f);
}
    

@ getting taged files

@d<functions@>
  { fn_gettags, "get-tags", 0, "get tagged files" }

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
		    && (i=dir_find(curdir, t->next->name))) {
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
