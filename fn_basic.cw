@ miscanelous basic bindable functions.

@(fn_basic.fn@)
; fn_basic
@<functions@>

@u
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "directory.h"
#include "functions.h"
#include "display.h"

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

void fn_version()
{
	disp_status("%s", version);
}


@ redraw.

@d<functions@>
  { fn_redraw, "redraw", FN_PRE, "redraw screen" }

@u
void fn_redraw()
{
	disp_redraw();
}


@ help.

@d<functions@>
  { fn_help, "help", 0, "display binding and help string for key" }

@u
void fn_help()
{
	int c, i;

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
  { fn_lcd, "local-cd", 0, "change directory on local host" }

@u
void fn_lcd(void)
{
	char *lwd;

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
void fn_shell(void)
{
	char *cmd, b[128];

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
