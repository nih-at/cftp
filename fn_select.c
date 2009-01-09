/*
  $NiH: fn_select.c,v 1.29 2003/05/13 15:46:08 dillo Exp $

  fn_select.c -- bindable functions: selecting
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



#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#include "directory.h"
#include "bindings.h"
#include "fntable.h"
#include "functions.h"
#include "display.h"
#include "ftp.h"
#include "options.h"
#include "util.h"
#include "list.h"
#include "tag.h"




int
aux_enter(char *name)
{
    directory *d;

    d = ftp_cd(name, 0);

    if (d == NULL)
	return -1;

    change_curdir(d);

    list_do(1);

    return 0;
}



int
aux_download(char *name, off_t size, int restart)
{
    int err;
    void *fin;
    FILE *fout;
    struct stat st;
    char *mode;
    off_t start, rsize;

    start = 0;
    
    if (restart) {
	if (stat(noalloc_basename(name), &st) == 0)
	    start = st.st_size;
    }

    if ((fin=ftp_retr(name, opt_mode, &start, &rsize)) == NULL)
	return -2;
    if (start > 0)
	mode = "a";
    else
	mode = "w";
    
    if ((fout=fopen(noalloc_basename(name), mode)) == NULL) {
	disp_status(DISP_STATUS, "can't %s `%s': %s",
		    (*mode == 'a' ? "append to" : "create"),
		    noalloc_basename(name), strerror(errno));
	return -2;
    }

    if (size == -1)
	size = rsize;

    err = ftp_cat(fin, fout, start, size, 0);

    err |= ftp_fclose(fin);
    
    if (fclose(fout)) {
	disp_status(DISP_STATUS, "error closing `%s': %s",
		    noalloc_basename(name), strerror(errno));
	return -2;
    }

    return err;
}



int
aux_pipe(char *name, off_t size, int mode, char *cmd, int quietp)
{
    int err;
    void *fin;
    FILE *fout;
	
    if ((fin=ftp_retr(name, (mode ? mode : opt_mode), NULL, NULL)) == NULL)
	return -2;

    if ((fout=disp_open(cmd, quietp)) == NULL)
	return -2;

    err = ftp_cat(fin, fout, 0, size, 0);

    err |= ftp_fclose(fin);

    err |= disp_close(fout, quietp);

    return err;
}



int
aux_upload(char *name)
{
    int err;
    off_t size;
    struct stat st;
    FILE *fin;
    void *fout;

    if ((fin=fopen(name, "r")) == NULL) {
	disp_status(DISP_STATUS, "can't open `%s': %s", name, strerror(errno));
	return -2;
    }

    if ((fout=ftp_stor(noalloc_basename(name), opt_mode)) == NULL)
	return -2;

    if (stat(name, &st) >= 0)
	size = st.st_size;
    else
	size = -1;
    
    err = ftp_cat(fin, fout, 0, size, 1);

    err |= ftp_fclose(fout);
    
    if (fclose(fin)) {
	disp_status(DISP_STATUS, "error closing `%s': %s",
		    name, strerror(errno));
	return -2;
    }

    return err;
}



void
fn_enter_get(char **args)
{
    char *name;
    int type;
    off_t size;

    if (args) {
	name = args[0];
	type = 'l';
	size = -1;
    }
    else if (curdir->len == 0)
	return;
    else {
	name = curdir->line[curdir->cur].name;
	type = curdir->line[curdir->cur].type;
	size = curdir->line[curdir->cur].size;
    }

    switch (type) {
    case 'd':
	aux_enter(name);
	break;
    case 'f':
	aux_download(name, size, 0);
	break;
    case 'l':
	if (aux_enter(name) == -1)
	    aux_download(name, size, 0);
	break;
    default:
	disp_status(DISP_STATUS, "Can't download special files.");
    }
}



void
fn_enter_view(char **args)
{
    char *name;
    int type;

    if (args) {
	name = args[0];
	type = 'l';
    }
    else if (curdir->len == 0)
	return;
    else {
	name = curdir->line[curdir->cur].name;
	type = curdir->line[curdir->cur].type;
    }

    switch (type) {
    case 'd':
	aux_enter(name);
	break;
    case 'f':
	aux_view(name);
	break;
    case 'l':
	if (aux_enter(name) == -1)
	    aux_view(name);
	break;
    default:
	disp_status(DISP_STATUS, "Can't view special files.");
    }
}



void fn_enter(char **args)
{
    char *name; int type;

    if (args) {
	name = args[0];
	type = 'l';
    }
    else if (curdir->len == 0)
	return;
    else {
	name = curdir->line[curdir->cur].name;
	type = curdir->line[curdir->cur].type;
    }

    switch (type) {
    case 'd':
    case 'l':
	aux_enter(name);
	break;
    default:
	disp_status(DISP_STATUS, "Can enter only directories.");
    }
}



void
fn_reload(char **args)
{
    directory *d;
    char *name;
    int sel;

    if (curdir->len > 0)
	name = strdup(curdir->line[curdir->cur].name);
    else
	name = NULL;

    d = ftp_cd(curdir->path, 1);
    if (d == NULL) {
	free(name);
	return;
    }

    curdir = NULL;
    change_curdir(d);

    if (name == NULL || (sel=dir_find(curdir, name)) < 0)
	sel = 0;
    free(name);
    
    aux_scroll(sel-(win_lines/2), sel, 1);
}



void
fn_get(char **args)
{
    char *name;
    int type;
    off_t size;

    if (args) {
	name = args[0];
	type = 'l';
	size = -1;
    }
    else if (curdir->len == 0)
	return;
    else {
	name = curdir->line[curdir->cur].name;
	type = curdir->line[curdir->cur].type;
	size = curdir->line[curdir->cur].size;
    }

    switch (type) {
    case 'f':
    case 'l':
	aux_download(name, size, 0);
	break;
    default:
	disp_status(DISP_STATUS, "Can only download plain files.");
    }
}



void
fn_view(char **args)
{
    char *name;
    int type;

    if (args) {
	name = args[0];
	type = 'l';
    }
    else if (curdir->len == 0)
	return;
    else {
	name = curdir->line[curdir->cur].name;
	type = curdir->line[curdir->cur].type;
    }

    switch (type) {
    case 'f':
    case 'l':
	aux_view(name);
	break;
    default:
	disp_status(DISP_STATUS, "Can only view plain files.");
    }
}



void
fn_pipe(char **args)
{
    char *name, *cmd, *line;
    int type, freecmdp, quietp;
    off_t size;

    line = NULL;
    cmd = NULL;
    freecmdp = 1;
    quietp = 1;
    
    if (args) {
	if (strcmp(args[0], "-q") == 0) {
	    quietp = 0;
	    args++;
	}
	if (strcmp(args[0], ":") == 0) {
	    if (curdir->len == 0)
		return;

	    name = curdir->line[curdir->cur].name;
	    type = curdir->line[curdir->cur].type;
	    size = curdir->line[curdir->cur].size;
	}
	else {
	    name = args[0];
	    type = 'l';
	    size = -1;
	}

	if (args[1])
	    cmd = args_to_string(args+1);
	else
	    cmd = read_string("| ", 1);
    }
    else {
	line = read_string("pipe: ", 1);
	if (line == NULL || line[0] == '\0') {
	    free(line);
	    return;
	}
	name = strtok(line, " \t");
	if (name == NULL || name[0] == '\0') {
	    free(line);
	    return;
	}
	if (strcmp(name, "-q") == 0) {
	    quietp = 0;
	    name = strtok(line, " \t");
	    if (name == NULL || name[0] == '\0') {
		free(line);
		return;
	    }
	}	    
	cmd = strtok(NULL, "\n");
	if (cmd == NULL || cmd[0] == '\0') {
	    cmd = read_string("| ", 1);
	}
	else
	    freecmdp = 0;
    }

    if (cmd == NULL || cmd[0] == '\0') {
	if (freecmdp)
	    free(cmd);
	free(line);
	disp_status(DISP_STATUS, "");
	return;
    }

    switch (type) {
    case 'f':
    case 'l':
	aux_pipe(name, size, 0, cmd, quietp);
	break;
    default:
	disp_status(DISP_STATUS, "Can only view plain files.");
    }
}



void
fn_cdup(char **args)
{
    char *par;
    int sel;
	
    directory *d;

    if (strcmp(curdir->path, "/") == 0)
	return;

    d = ftp_cd("..", 0);

    if (d == NULL)
	return;

    par = strrchr(curdir->path, '/');
    change_curdir(d);

    if (par && *(par++)!='\0')
	if ((sel=dir_find(curdir, par)) < 0)
	    sel = 0;
	    
    aux_scroll(sel-(win_lines/2), sel, 1);
}



void
fn_cd(char **args)
{
    char *path;
    directory *d;

    if (args)
	path = args[0];
    else
	path = read_string("directory: ", 1);
	
    if (path[0] == '\0') {
	disp_status(DISP_STATUS, "");
	return;
    }

    d = ftp_cd(path, 0);

    if (!args)
	free(path);
		
    if (d == NULL)
	return;

    change_curdir(d);
    
    list_do(1);
}



void
fn_put(char **args)
{
    char *name;

    if (args)
	name = args[0];
    else {
	name = read_string("put ", 1);
	if (name == NULL || name[0] == '\0')
	    return;
    }

    aux_upload(name);
}

