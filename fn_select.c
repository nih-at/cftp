/*
  fn_select -- bindable functions: selecting
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
#include "directory.h"
#include "functions.h"
#include "display.h"
#include "ftp.h"
#include "options.h"
#include "util.h"

int aux_enter(char *name);
int aux_download(char *name, long size);
int aux_view(char *name);



int
aux_enter(char *name)
{
    directory *d;

    d = ftp_cd(name);

    if (d == NULL)
	return -1;

    change_curdir(d);

    disp_dir(0, 0, cursel, 1);

    return 0;
}



int
aux_download(char *name, long size)
{
    int err;
    FILE *f;

    if ((f=fopen(basename(name), "w")) == NULL)
	return -2;
	
    err = ftp_retr(name, f, size, opt_mode);
    fclose(f);

    return err;
}



int
aux_view(char *name){
    int err;
    FILE *f;
	
    if ((f=disp_open(-1)) == NULL)
	return -2;

    err = ftp_retr(name, f, 0, 'a');

    err |= disp_close(f);
	
    return err;
}



void
fn_enter_get(char **args)
{
    char *name;
    int type;
    long size;

    if (args) {
	name = args[0];
	type = 'l';
	size = -1;
    }
    else {
	name = curdir->list[cursel].name;
	type = curdir->list[cursel].type;
	size = curdir->list[cursel].size;
    }

    switch (type) {
    case 'd':
	aux_enter(name);
	break;
    case 'f':
	aux_download(name, size);
	break;
    case 'l':
	if (aux_enter(name) == -1)
	    aux_download(name, size);
	break;
    default:
	disp_status("Can't download special files.");
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
    else {
	name = curdir->list[cursel].name;
	type = curdir->list[cursel].type;
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
	disp_status("Can't view special files.");
    }
}



void
fn_enter(char **args)
{
    char *name;
    int type;

    if (args) {
	name = args[0];
	type = 'l';
    }
    else {
	name = curdir->list[cursel].name;
	type = curdir->list[cursel].type;
    }

    switch (type) {
    case 'd':
    case 'l':
	aux_enter(name);
	break;
    default:
	disp_status("Can enter only directories.");
    }
}



void
fn_get(char **args)
{
    char *name;
    int type;
    long size;

    if (args) {
	name = args[0];
	type = 'l';
	size = -1;
    }
    else {
	name = curdir->list[cursel].name;
	type = curdir->list[cursel].type;
	size = curdir->list[cursel].size;
    }

    switch (type) {
    case 'f':
    case 'l':
	aux_download(name, size);
	break;
    default:
	disp_status("Can only download plain files.");
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
    else {
	name = curdir->list[cursel].name;
	type = curdir->list[cursel].type;
    }

    switch (type) {
    case 'f':
    case 'l':
	aux_view(name);
	break;
    default:
	disp_status("Can only view plain files.");
    }
}



void
fn_cdup(char **args)
{
    char *par;
    int sel;
	
    directory *d;

    if (strcmp(list->path, "/") == 0)
	return;

    d = ftp_cd("..");

    if (d == NULL)
	return;

    par = strrchr(list->path, '/');
    change_curdir(d);

    if (par && *(par++)!='\0')
	if ((sel=dir_find(list, par)) < 0)
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
	disp_status("");
	return;
    }

    d = ftp_cd(path);

    if (!args)
	free(path);
		
    if (d == NULL)
	return;

    change_curdir(d);
    
    disp_dir(list, 0, 0, 1);
}

