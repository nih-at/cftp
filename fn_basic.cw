@ miscanelous basic bindable functions.

@(fn_basic.fn@)
section(fn_basic, Basic Functions)
@<functions@>
endsec()


@u
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "directory.h"
#include "ftp.h"
#include "functions.h"
#include "display.h"
#include "rc.h"

extern int binding[];
extern function functions[];

@ exitting.

@d<functions@>
function(exit, [-f], 0, FN_EXIT,
	 {exit cftp},
{Close connection and exit @@sc{cftp}; if you have tagged files, @@sc{cftp}
asks for confirmation (unless the -f option is given).})

@ displaying version.

@d<functions@>
function(version, , fn_version, 0,
	 {display version number},
 {Display @@sc{cftp} version string.})

@u
extern char version[];

void fn_version(char **args)
{
	disp_status("%s", version);
}


@ redraw.

@d<functions@>
function(redraw, , fn_redraw, FN_PRE,
	 {redraw screen},
 {Clear and redraw screen.  Use this function if something messed up
your display.})

@u
void fn_redraw(char **args)
{
    disp_redraw();
}


@ help.

@d<functions@>
function(help, [key], fn_help, 0,
	 {display binding and help string for key},
 {Describe function bound to @@code{key}; @@code{key} is prompted for if
ommitted.})

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
function(lcd, [dir], fn_lcd, 0,
	 {change directory on local host},
 {Change directory on localhost; dir is prompted for if ommited.  The new current directory is printed.})

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
function(shell, {[cmd arg @@dots{}]}, fn_shell, 0,
	 {shell escape},
 {Execute shell command; if no command is given, it is prompted for.
Enter empty string to get an interactive shell.}) 

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
		fflush(stdout);
		disp_prompt_char();
	}
	else
		system("$SHELL -i");
	disp_status("");
	reenter_disp();
}


@ execute cftp command

@d<functions@>
function(colon, {[args @@dots{}]}, fn_colon, 0,
	 {execute cftp command},
 {Execute an arbitrary @@sc{cftp} command.})

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


@ deidling connection

@d<functions@>
function(deidle, , fn_deidle, 0,
	 {deidle connection},
 {Send a @@code{noop} to ftp server, thus resetting idle time on server.})

@u
void fn_deidle(char **args)
{
    ftp_noop();
}


@ displaying last response.

@d<functions@>
function(response, , fn_response, 0,
	 {display last multiline response},
 {Display last multiline response read from server.  This will be
replaced by an ability to few the last N lines exchanged with the
server.})

@u
void fn_response(char **args)
{
    FILE *f;
    long i;

    if (ftp_response == NULL) {
	disp_status("no response available");
	return;
    }
    
    if ((f=disp_open(-1)) == NULL)
	return;

    for (i=0; ftp_response[i]; i++)
	fprintf(f, "%s\n", ftp_response[i]);

    disp_close(f);
}


@ functions for entering digits

@d<functions@>
function(prefix, , fn_prefix, FN_PRE,
	 {prefix digit},
 {Enter digit of prefix argument to other commands.})


@u
int
fn_prefix(char **args)
{
    if (args == NULL) {
	/* emacs C-u */
	return;
    }
    else if (args[0][1] == '\0') {
	if (isdigit(args[0][0]))
	    add_prefix(args[0][0] - '0');
	else if (args[0][0] == '-')
	    negate_prefix();
    }
    else {
	int p = atoi(args[0]);

	if (p || args[0][0] == '0')
	    set_prefix(p);
    }
}
