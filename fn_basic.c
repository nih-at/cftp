/*
  fn_basic -- bindable functions: basics
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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "util.h"
#include "directory.h"
#include "ftp.h"
#include "functions.h"
#include "display.h"
#include "rc.h"
#include "options.h"

extern int binding[];
extern function functions[];


extern char version[];

void fn_version(char **args)
{
	disp_status("%s", version);
}



void fn_redraw(char **args)
{
    disp_redraw();
}



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



void fn_lcd(char **args)
{
	char *lwd, *exp;
	int freep;

	if (args) {
	    lwd = args[0];
	    freep = 0;
	}
	else {
	    if (rc_inrc) {
		rc_error("lcd: no directory specified");
		return;
	    }
	    lwd = read_string("local directory: ", 1);
	    freep = 1;
	}
	exp = local_exp(lwd);
	if (exp) {
	    if (chdir(exp) < 0 && rc_inrc)
		rc_error("rc: cannot cd to %s: %s",
			 lwd, strerror(errno));
	    free(exp);
	}
	if (freep)
	    free(lwd);

	lwd = getcwd(NULL, 1024);
	if (!rc_inrc)
	    disp_status("Current local directory: %s", lwd);
	free(lwd);
}



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



void fn_deidle(char **args)
{
    ftp_noop();
}



void fn_reconnect(char **args)
{
    ftp_reconnect();
}



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

