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

@<local prototypes@>


@ entering a directory.

@d<local prototypes@>
int aux_enter(void);

@u
int
aux_enter(void)
{
	directory *d;

	d = ftp_cd(curdir->list[cursel].name);

	if (d == NULL)
		return -1;

	curdir = d;
	cursel = curtop = 0;

	disp_dir(curdir, curtop, cursel, 1);

	return 0;
}


@ downloading a file.

@d<local prototypes@>
int aux_download(void);

@u
int
aux_download(void)
{
	int err;
	FILE *f;

	if ((f=fopen(curdir->list[cursel].name, "w")) == NULL)
		return -2;
	
	err = ftp_retr(curdir->list[cursel].name, f,
		       curdir->list[cursel].size, opt_mode);
	fclose(f);

	return err;
}


@ viewing a flie.

@d<local prototypes@>
int aux_view(void);

@u
int
aux_view(void){
	int err;
	FILE *f;
	
	if ((f=popen("less", "w")) == NULL)
		return -2;

	escape_disp(1);
	
	err = ftp_retr(curdir->list[cursel].name, f,
		       curdir->list[cursel].size, 'a');
	pclose(f);

	reenter_disp();

	return err;
}


@ selecting an entry: entering (dir) or downloading (file).

@d<functions@>
  { fn_enter_get, "enter/get", 0, "enter directory or get file" }

@u
void
fn_enter_get()
{
	switch (curdir->list[cursel].type) {
	case 'd':
		aux_enter();
		break;
	case 'f':
		aux_download();
		break;
	case 'l':
		if (aux_enter() == -1)
			aux_download();
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
fn_enter_view()
{
	switch (curdir->list[cursel].type) {
	case 'd':
		aux_enter();
		break;
	case 'f':
		aux_view();
		break;
	case 'l':
		if (aux_enter() == -1)
			aux_view();
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
fn_enter()
{
	switch (curdir->list[cursel].type) {
	case 'd':
	case 'l':
		aux_enter();
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
fn_get()
{
	switch (curdir->list[cursel].type) {
	case 'f':
	case 'l':
		aux_download();
		break;
	default:
		disp_status("Can only download plain files.\n");
	}
}

@ viewing a file.

@d<functions@>
  { fn_view, "view", 0, "view file" }

@u
void
fn_view()
{
	switch (curdir->list[cursel].type) {
	case 'f':
	case 'l':
		aux_view();
		break;
	default:
		disp_status("Can only view plain files.\n");
	}
}


@ leaving a directory.

@d<functions@>
  { fn_cdup, "leave", 0, "leave current directory" }

@u
void
fn_cdup()
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
	curdir = d;
	cursel = 0, curtop = 0;

	if (par && *(par++)!='\0') {
		for (sel=0; (sel<curdir->num &&
			     strcmp(curdir->list[sel].name, par)); sel++)
			;
		if (sel == curdir->num)
			sel = 0;
	}
	aux_scroll(sel-(win_lines/2), sel, 1);
}


@ going to a directory.

@d<functions@>
  { fn_cd, "cd", 0, "change directory" }

@u
void
fn_cd()
{
	char *path;
	directory *d;

	path = read_string("directory: ");
	
	d = ftp_cd(path);
	free(path);
		
	if (d == NULL)
		return;

	curdir = d;
	cursel = curtop = 0;

	disp_dir(curdir, curtop, cursel, 1);
}

