@ miscellaneous basic bindable functions.

@(fn_basic.fn@)
section(fn_basic, Basic Functions)
@<functions@>
endsec()


@u
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "util.h"
#include "directory.h"
#include "ftp.h"
#include "functions.h"
#include "display.h"
#include "rc.h"
#include "options.h"

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
	char *lwd, *exp;
	int freep;

	if (args) {
	    lwd = args[0];
	    freep = 0;
	}
	else {
	    lwd = read_string("local directory: ", 1);
	    freep = 1;
	}
	exp = local_exp(lwd);
	if (exp) {
	    chdir(exp);
	    free(exp);
	}
	if (freep)
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
	    cmd = read_string("! ", 1);

	escape_disp(0);
	if (cmd[0] != '\0') {
		system(cmd);
		printf("[Press return] ");
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
	char *cmd, *tok, *p, *line = NULL;
	int i, j;

	if (args) {
	    cmd = args[0];
	    args++;
	}
	else {
	    line = p = read_string(": ", 1);

	    if ((cmd=rc_token(&p)) == NULL) {
		disp_status("no function");
		return;
	    }
	}
	
	if ((i=find_function(cmd)) < 0) {
	    disp_status("unknown function: %s", cmd);
	    if (line)
		free(line);
	    return;
	}

	if (line) {
	    args = rc_list(p);
	}

	disp_status("");
	
	if (functions[i].fn)
	    functions[i].fn(args);

	if (line) {
	    free(line);
	    if (args) {
		for (i=0; args[i]; i++)
		    free(args[i]);
		free(args);
	    }
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


@ reconnecting

@d<functions@>
function(reconnect, , fn_reconnect, 0,
	 {reconnect to server},
 {Reopen connection to server after timeout.})

@u
void fn_reconnect(char **args)
{
    ftp_reconnect();
}


@ displaying last response.

@d<functions@>
function(response, , fn_response, 0,
	 {display last multiline response},
 {Display last N lines exchanged with ftp server.})

@u
void fn_response(char **args)
{
    struct ftp_hist *h;
    FILE *f;
    long i;

    if (ftp_history == NULL) {
	disp_status("no exchange available");
	return;
    }
    
    if ((f=disp_open(-1)) == NULL)
	return;

    for (h=ftp_history; h; h=h->next)
	fprintf(f, "%s\n", h->line);

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


@ binding keys

@d<functions@>
function(bind, {key [cmd [args @@dots{}]]}, fn_bind, FN_RC,
	 {bind key},
 {})

@u
int
fn_bind(char **args)
{
    extern int binding[];
    extern char **binding_args[], *binding_pool[];
    extern int binding_pool_len;

    int key, fn, i;
    char *line = NULL;
    char *cmd, *kname, **list, *p;

    
    if (args) {
	kname = args[0];
	cmd = args[1];
	    args += 2;
    }
    else {
	if (rc_inrc) {
	    rc_error("bind: no arguments given");
	    return;
	}
	line = p = read_string("bind ", 1);

	if ((kname=rc_token(&p)) == NULL) {
	    disp_status("no key");
	    free(line);
	    return;
	}
	cmd = rc_token(&p);
    }

    if ((key=parse_key(kname)) < 0) {
	(rc_inrc ? rc_error : disp_status)("unknown key: %s", kname);
	if (line)
	    free(line);
	return;
    }
	
    if (cmd) {
	if ((fn=find_function(cmd)) < 0) {
	    (rc_inrc ? rc_error : disp_status)("unknown function: %s", cmd);
	    if (line)
		free(line);
	    return;
	}
    }

    if (!rc_inrc)
	disp_status("");
	
    if (binding[key] > -1) {
	binding[key] = -1;
	if (binding_args[key]) {
	    if (binding_args[key] < binding_pool
		|| (binding_args[key] >=
		    binding_pool+binding_pool_len)) {
		for (i=0; binding_args[key][i]; i++)
		    free(binding_args[key][i]);
		free(binding_args);
	    }
	    binding_args[key] = NULL;
	}
    }

    if (cmd) {
	if (line)
	    list = rc_list(p);
	else {
	    for (i=0; args[i]; i++) {
		list = (char **)malloc(sizeof(char *)*(i+1));
		memcpy(list, args, sizeof(char *)*(i+1));
	    }
	}

	binding[key] = fn;
	binding_args[key] = list;
    }

    return;
}


@ setting user options.

@d<functions@>
function(set, [option [value]], fn_set, FN_RC,
	 {set option},
 {Set user option.})

@u
void fn_set(char **args)
{
    char *opt, *value;
    char *line = NULL, *p;
    char prompt[128];
    int ival, i;

    if (args) {
	opt = args[0];
	if (args[1])
	    value = args[1];
	else {
	    if (rc_inrc) {
		rc_error("no option value given");
		return;
	    }
	    sprintf(prompt, "set %s to: ", opt);
	    p = line = read_string(prompt, 1);
	    value = rc_token(&p);
	}
    }
    else {
	if (rc_inrc) {
	    rc_error("no option value given");
	    return;
	}
	p = line = read_string("set ", 1);
	opt = rc_token(&p);
	if (opt == NULL) {
	    disp_status("");
	    return;
	}
	value = rc_token(&p);
    }

    if (opt == NULL || opt[0] == '\0') {
	disp_status("no option.");
	return;
    }

    for (i=0; option[i].name; i++)
	if (strcasecmp(option[i].name, opt) == 0
	    || strcasecmp(option[i].shrt, opt) == 0)
	    break;

    if (option[i].name == NULL) {
	if (rc_inrc)
	    rc_error("unknown option: %s", opt);
	else
	    disp_status("unknown option: %s", opt);
	return;
    }

    switch (option[i].type) {
    case OPT_INT:
	if (value) {
	    ival = atoi(value);
	    if (option[i].func)
		option[i].func(ival, option[i].var.i);
	    else
		*(option[i].var.i) = ival;
	}
	
	if (!rc_inrc)
	    disp_status("%s set to %d", option[i].name,
			*(option[i].var.i));
	break;

    case OPT_CHR:
	if (value) {
	    ival = value[0];
	    if (option[i].func)
		option[i].func(ival, option[i].var.i);
	    else
		*(option[i].var.i) = ival;
	}
	
	if (!rc_inrc)
	    disp_status("%s set to `%c'", option[i].name,
			*(option[i].var.i));
	break;

    case OPT_STR:
	if (value) {
	    if (option[i].func)
		option[i].func(strdup(value), option[i].var.s);
	    else
		*(option[i].var.s) = strdup(value);
	}
	
	if (!rc_inrc)
	    disp_status("%s set to `%s'", option[i].name,
			*(option[i].var.s));
	break;
	
    case OPT_BOOL:
	if (value) {
	    if (value[0] == 't')
		ival = !*(option[i].var.i);
	    else if (value[0] == '0' || value[0] == 'n'
		|| strcasecmp(value, "off") == 0)
		ival = 0;
	    else
		ival = 1;
	    if (option[i].func)
		option[i].func(ival, option[i].var.i);
	    else
		*(option[i].var.i) = ival;
	}

	if (!rc_inrc)
	    disp_status("%s set to %s", option[i].name,
			(*(option[i].var.i) ? "on" : "off"));
	break;
    }

    if (line)
	free(line);

    return;
}


