@ bindable functions for selecting files and changing directories.

@(fn_select.fn@)
; fn_select
@<functions@>

@u
#include <stdio.h>
#include <string.h>
#include "directory.h"
#include "functions.h"
#include "display.h"
#include "ftp.h"
#include "options.h"
#include "util.h"

@<local prototypes@>


@ entering a directory.

@d<local prototypes@>
int aux_enter(char *name);

@u
int
aux_enter(char *name)
{
	directory *d;

	d = ftp_cd(name);

	if (d == NULL)
		return -1;

	change_curdir(d);
	cursel = curtop = 0;

	disp_dir(curdir, curtop, cursel, 1);

	return 0;
}


@ downloading a file.

@d<local prototypes@>
int aux_download(char *name, long size);

@u
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


@ viewing a flie.

@d<local prototypes@>
int aux_view(char *name);

@u
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


@ selecting an entry: entering (dir) or downloading (file).

@d<functions@>
  { fn_enter_get, "enter/get", 0, "enter directory or get file" }

@u
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


@ selecting an entry: entering (dir) or viewing (file)

@d<functions@>
  { fn_enter_view, "enter/view", 0, "enter directory or view file" }

@u
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


@ entering a directory.

@d<functions@>
  { fn_enter, "enter", 0, "enter directory" }

@u
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


@ downloading a file.

@d<functions@>
  { fn_get, "get", 0, "get file" }

@u
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

@ viewing a file.

@d<functions@>
  { fn_view, "view", 0, "view file" }

@u
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


@ leaving a directory.

@d<functions@>
  { fn_cdup, "leave", 0, "leave current directory" }

@u
void
fn_cdup(char **args)
{
	char *par;
	int sel;
	
	directory *d;

	if (strcmp(curdir->path, "/") == 0)
		return;

	d = ftp_cd("..");

	if (d == NULL)
		return;

	par = strrchr(curdir->path, '/');
	change_curdir(d);
	cursel = 0, curtop = 0;

	if (par && *(par++)!='\0')
	    if ((sel=dir_find(curdir, par)) < 0)
		sel = 0;
	    
	aux_scroll(sel-(win_lines/2), sel, 1);
}


@ going to a directory.

@d<functions@>
  { fn_cd, "cd", 0, "change directory" }

@u
void
fn_cd(char **args)
{
    char *path;
    directory *d;

    if (args)
	path = args[0];
    else
	path = read_string("directory: ");
	
    d = ftp_cd(path);

    if (!args)
	free(path);
		
    if (d == NULL)
	return;

    change_curdir(d);
    cursel = curtop = 0;
    
    disp_dir(curdir, curtop, cursel, 1);
}

