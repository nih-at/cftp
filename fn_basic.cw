@ miscanelous basic bindable functions.

@(fn_basic.fn@)
; fn_basic
@<functions@>

@u
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "directory.h"
#include "functions.h"
#include "display.h"
#include "rc.h"

extern int binding[];
extern function functions[];

@ exitting.

@d<functions@>
  { 0, "exit", FN_EXIT, "exit cftp" }


@ displaying version.

@d<functions@>
  { fn_version, "version", 0, "display version number" }

@u
extern char version[];

void fn_version(char **args)
{
	disp_status("%s", version);
}


@ redraw.

@d<functions@>
  { fn_redraw, "redraw", FN_PRE, "redraw screen" }

@u
void fn_redraw(char **args)
{
	disp_redraw();
}


@ help.

@d<functions@>
  { fn_help, "help", 0, "display binding and help string for key" }

@u
void fn_help(char **args)
{
	int c, i;

	if (args)
	    c = parse_key(args[0]);
	else
	    c = read_char("Key: ");
	
	if ((i=binding[c]) == -1) {
		disp_status("[%s] key is unbound", print_key(c, 0));
		return;
	}

	disp_status("[%s] %s: %s", print_key(c, 0),
		    functions[i].name, functions[i].help);
}


@ change local directory.

@d<functions@>
  { fn_lcd, "lcd", 0, "change directory on local host" }

@u
void fn_lcd(char **args)
{
	char *lwd;

	if (args)
	    lwd = args[0];
	else
	    lwd = read_string("local directory: ");
	chdir(lwd);
	free(lwd);

	lwd = getcwd(NULL, 1024);
	disp_status("Current local directory: %s", lwd);
	free(lwd);
}


@ shell escape.

@d<functions@>
  { fn_shell, "shell", 0, "shell escape" }

@u
void fn_shell(char **args)
{
	char *cmd, b[128];

	if (args)
	    cmd = args[0];
	else
	    cmd = read_string("! ");

	escape_disp(0);
	if (cmd[0] != '\0') {
		system(cmd);
		printf("[Press any key] ");
		fflush(stdin);
		disp_prompt_char();
	}
	else
		system("$SHELL -i");
	reenter_disp();
}


@ execute cftp command

@d<functions@>
  { fn_colon, "colon", 0, "execute cftp command" }

@u
void fn_colon(char **args)
{
	char *arg[128], *cmd, *tok, *p, *line = NULL;
	int i, j;

	if (args) {
	    cmd = args[0];
	    args++;
	}
	else {
	    line = p = read_string(": ");

	    if ((cmd=rc_token(&p)) == NULL) {
		disp_status("no function");
		return;
	    }
	}
	
	for (i=0; functions[i].name && strcmp(functions[i].name, cmd); i++)
	    ;
	if (functions[i].name == NULL) {
	    disp_status("unknown function: %s", cmd);
	    return;
	}

	if (line) {
	    for (j=0; tok=rc_token(&p); j++)
		arg[j] = strdup(tok);
	    if (j)
		args = arg;
	    else
		args = NULL;
	}

	disp_status("");
	
	if (functions[i].fn)
	    functions[i].fn(args);

	if (line) {
	    free(line);
	    for (i=0; i<j; i++)
		free(arg[i]);
	}
}
