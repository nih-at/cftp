 /*
  $NiH: fn_tag.c,v 1.32 2003/12/19 10:06:15 dillo Exp $

  fn_tag.c -- bindable functions: 
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

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#ifdef HAVE_FNMATCH
#include <fnmatch.h>
#else
#include <fnmatch_repl.h>
#endif

#include "directory.h"
#include "bindings.h"
#include "fntable.h"
#include "functions.h"
#include "display.h"
#include "options.h"
#include "ftp.h"
#include "tag.h"
#include "util.h"
#include "status.h"
#include "list.h"
#include "url.h"



void
fn_tag(char **args)
{
    char *dir, *file, *base;
    off_t size;
    char type;
    int tagged, i;

    if (args) {
	if (strcmp(args[0], "-t") == 0 || strcmp(args[0], "-u") == 0) {
	    /* works in <remote> only */

	    if (args[1] == NULL) {
		file = read_string("tag (wildcard): ", 1);
		if (file == NULL || file[0] == '\0') {
		    free(file);
		    return;
		}
	    }
	    else
		file = args[1];
	    
	    tagged = 0;
	    for (i=0; i<curdir->len; i++) {
		if (fnmatch(file, curdir->line[i].name, FNM_PERIOD) == 0)
		    tagged += tag_file(curdir->path,
				       curdir->line[i].name,
				       curdir->line[i].size,
				       curdir->line[i].type,
				       args[0][1] == 't' ? TAG_ON : TAG_OFF);
	    }

	    if (args[1] == NULL)
		free(file);
	}
	else {
	    dir = NULL;
	    file = canonical(args[0], NULL);
	    size = -1;
	    type = 'l';

	    base = noalloc_basename(file);
	    
	    i = strlen(curdir->path);
	    if ((base-file == 1 && i == 1)
		|| (i == base-file-1 && strncmp(curdir->path, file, i) == 0)) {
		i = dir_find(curdir, base);
		if (i >= 0) {
		    size = curdir->line[i].size;
		    type = curdir->line[i].type;
		}
	    }
	    tagged = tag_file(dir, file, size, type, TAG_TOGGLE);
	    free(file);
	}
    }
    else {
	/* works in <remote> only */

	if (curdir->len == 0)
	    return;
	
	dir = curdir->path;
	file = curdir->line[curdir->cur].name;
	size = curdir->line[curdir->cur].size;
	type = curdir->line[curdir->cur].type;

	tagged = tag_file(dir, file, size, type, TAG_TOGGLE);
    }
	
    if (tagged < -1)
	disp_status(DISP_STATUS, "%d files untagged", -tagged);
    else if (tagged == -1)
	disp_status(DISP_STATUS, "1 file untagged");
    else if (tagged == 1)
	disp_status(DISP_STATUS, "1 file tagged");
    else if (tagged > 1)
	disp_status(DISP_STATUS, "%d files tagged", tagged);
}



void
fn_cleartags(char **args)
{
    int i;

    tag_clear();

    for (i=0; i<curdir->len; i++)
	if (curdir->line[i].line[0] != ' ') {
	    curdir->line[i].line[0] = ' ';
	    if (binding_state == bs_remote)
		list_reline(i);
	}

    disp_status(DISP_STATUS, "all tags cleared");
}
    


void aux_scroll(int top, int sel, int force);

void
fn_gettags(char **args)
{
    enum state old_state;
    struct tagentry *t;
    int i, j;
    int restart;

    if (!tag_anytags()) {
	disp_status(DISP_STATUS, "no tags");
	return;
    }

    if (args && strcmp(args[0], "-c") == 0)
	restart = 1;
    else
	restart = 0;

    old_state = binding_state;
    if (old_state != bs_tag) {
	tags.cur = tags.top = 0;
	enter_state(bs_tag);
    }

    for (i=0; i<tags.len;) {
	t = tags.line+i;

	if (aux_download(t->name, t->size, restart) == 0) {
	    if (strlen(curdir->path) == t->dirl
		&& strncmp(t->name, curdir->path, t->dirl) == 0) {
		if ((j=dir_find(curdir, t->file)) >= 0)
		    curdir->line[j].line[0] = ' ';
	    }

	    tag_delete(i);
	    list_do(1);
	}
	else {
	    i++;
	    aux_scroll(i-(win_lines/2), i, 0);
	}
    }

    if (tags.cur >= tags.len)
	tags.cur = tags.len-1;

    if (old_state != bs_tag)
	enter_state(old_state);
    
    if (opt_beep)
	disp_beep();
}



void
fn_savetags(char **args)
{
    FILE *f;
    char *name;
    int i;
    int save_as_url;

    save_as_url = 0;
    
    if (!tag_anytags()) {
	disp_status(DISP_STATUS, "no tags");
	return;
    }

    if (args && args[0] != NULL && strcmp(args[0],"-u") == 0) {
	save_as_url=1;
	args++;
    }
    
    if (args && args[0] != NULL)
	name =  args[0];
    else {
	name = read_string("File: ", 1);
	if (name == NULL || name[0] == '\0') {
	    disp_status(DISP_STATUS, "");
	    return;
	}
    }
	    
    if ((f=fopen(name, "w")) == NULL) {
	disp_status(DISP_STATUS, "can't create `%s': %s",
		    name, strerror(errno));
	return;
    }

	for (i=0; i<tags.len && !ferror(f); i++)
	    if (save_as_url)
		fprintf(f,"ftp://%s%s\n",
			status.host,
			tags.line[i].name);
	    else
		fprintf(f, "%8lld  %c  %s\n",
			tags.line[i].size,
			tags.line[i].type,
			tags.line[i].name);

    if (ferror(f))
	disp_status(DISP_STATUS, "write error on `%s': %s",
		    name, strerror(errno));
    else
	disp_status(DISP_STATUS, "%d tag%s saved to `%s'%s", tags.len,
		    (tags.len == 1 ? "" : "s"), name,
		    (save_as_url ? " (as URL)" : ""));

    fclose(f);

    if (!args)
	free(name);
}



void
fn_loadtag(char **args)
{
    char *fname, *line, *p, *e;
    FILE *f;
    int count, len;
    char *name;
    off_t size;
    int type;
    
    if (args)
	fname = args[0];
    else {
	fname = read_string("File: ", 1);
	if (fname == NULL || fname[0] == '\0') {
	    disp_status(DISP_STATUS, "");
	    return;
	}
    }

    if ((f=fopen(fname, "r")) == NULL) {
	disp_status(DISP_STATUS, "can't open `%s': %s",
		    fname, strerror(errno));
	return;
    }

    count = 0;
    while ((line=ftp_gets(f)) != NULL) {
	p = line;
	size = strtoll(p, &e, 10);
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
	if ((len=strlen(p))) {
	    if (p[len-1] == '\n')
		p[len-1] = '\0';

	    name = canonical(p, NULL);

	    count += tag_file(NULL, name, size, type, TAG_ON);
		
	    free(name);
	}
    }

    if (ferror(f))
	disp_status(DISP_STATUS, "read error: %s\n", strerror(errno));
    else
	disp_status(DISP_STATUS,
		    (count == 1 ? "%d file tagged" : "%d files tagged"),
		    count);
    fclose(f);
}



void
fn_saveurls(char **args)
{
    FILE *f;
    char *name, *host, *p;
    int i, n, len;

    if (!tag_anytags()) {
	disp_status(DISP_STATUS, "no tags");
	return;
    }

    if (args)
	name =  args[0];
    else {
	name = read_string("File: ", 1);
	if (name == NULL || name[0] == '\0') {
	    disp_status(DISP_STATUS, "");
	    return;
	}
    }
	    
    if ((f=fopen(name, "w")) == NULL) {
	disp_status(DISP_STATUS, "can't create `%s': %s",
		    name, strerror(errno));
	return;
    }

    host = mkhoststr(1, 1);

    n = 0;
    p = NULL;
    for (i=0; i<tags.len && !ferror(f); i++) {
	len = url_enclen(tags.line[i].name, URL_UCHAR)+1;
	if (len > n) {
	    free(p);
	    p = malloc(len);
	    n = len;
	}
	fprintf(f, "ftp://%s%s", host, url_encode(p, tags.line[i].name,
						  URL_XCHAR));
    }
    free(p);
    free(host);

    if (ferror(f))
	disp_status(DISP_STATUS, "write error on `%s': %s",
		    name, strerror(errno));
    else
	disp_status(DISP_STATUS,
		    (tags.len == 1 ? "%d URL saved" : "%d URLs saved"),
		    tags.len);

    fclose(f);

    if (!args)
	free(name);
}
