/*
  $NiH: fn_basic.c,v 1.28 2002/09/16 12:42:30 dillo Exp $

  fn_basic.c -- bindable functions: basics
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



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "util.h"
#include "directory.h"
#include "ftp.h"
#include "bindings.h"
#include "fntable.h"
#include "functions.h"
#include "display.h"
#include "rc.h"
#include "options.h"
#include "tty.h"
#include "list.h"
#include "tag.h"
#include "status.h"
#include "keys.h"

extern char version[];

void fn_version(char **args)
{
	disp_status(DISP_STATUS, "%s", version);
}



void fn_redraw(char **args)
{
    if (args && strcmp(args[0], "-c") == 0) {
	aux_scroll(list->cur-(win_lines/2), list->cur, -1);
    }
    disp_redraw();
}



void
fn_showname(char **args)
{
    char *tmp;
    
    if (args && strcmp(args[0], "-u") == 0) {
	switch (binding_state) {
	case bs_remote:
	    disp_status(DISP_STATUS, "ftp://%s%s%s%s",
			status.host, curdir->path,
			(strcmp(curdir->path, "/") == 0 ? "" : "/"),
			LIST_LINE(list, list->cur)->name);
	    break;
	case bs_tag:
	    disp_status(DISP_STATUS, "ftp://%s%s", status.host,
			LIST_LINE(list, list->cur)->name);
	    break;
	default:
	    disp_status(DISP_STATUS, "");
	}
    }
    else if (args && strcmp(args[0], "-l") == 0) {
	switch (binding_state) {
	case bs_remote:
	    tmp = ((direntry *)LIST_LINE(list, list->cur))->link;
	    disp_status(DISP_STATUS, "%s", tmp ? tmp : "");
	    break;
	default:
	    disp_status(DISP_STATUS, "");
	}
    }
    else 
	disp_status(DISP_STATUS, "%s", LIST_LINE(list, list->cur)->name);
}



void fn_help(char **args)
{
    static char spaces[] = "                ";

    struct binding *b;
    FILE *f;
    int what, c, i, j, l;
    char *s, *t;

    what = read_char("Help on <f>unction, <k>ey, or <o>ption "
		     "(shift for list)? ");

    switch (what) {
    case 'f':
	s = read_string("Function: ", 1);
	if (s == NULL || s[0] == '\0') {
	    free(s);
	    disp_status(DISP_STATUS, "");
	    return;
	}
	    
	if ((i=find_function(s)) == -1)
	    disp_status(DISP_STATUS, "no such function: %s", s);
	else
	    disp_status(DISP_STATUS, "%s: %s",
			functions[i].name, functions[i].help);

	free(s);
	break;

    case 'F':
	if ((f=disp_open(opt_pager, 1)) == NULL)
	    return;

	for (i=0; functions[i].name; i++) {
	    fprintf(f, "%-16.16s %s\n", functions[i].name, functions[i].help);
	}

	disp_close(f, 1);
	disp_status(DISP_STATUS, "");
	break;

    case 'k':
	c = read_char("Key: ");

	b = get_function(c, bs_none);
	if ((i=b->fn) == -1)
	    disp_status(DISP_STATUS, "[%s%s] key is unbound",
			(binding_state != bs_none ?
			 binding_statename[binding_state] : ""),
			print_key(c, 0));
	else {
	    char *buf;
	    int cols;

	    cols = tty_cols;
	    
	    if ((buf=(char *)malloc(cols+5)) == NULL)
		break;

	    sprintf(buf, "[%s%s] %s",
			(b->state != bs_none ?
			 binding_statename[b->state] : ""),
			print_key(c, 0),
			functions[i].name);
	    if (b->args) {
		int i, l;

		l = strlen(buf);
		for (i=0; b->args[i]; i++)
		    if (strlen(b->args[i])+l+2 > cols) {
			strcpy(buf+l, " ...");
			break;
		    }
		    else {
			sprintf(buf+l, " %s", b->args[i]);
			l += strlen(b->args[i])+1;
		    }
	    }

	    disp_status(DISP_STATUS, "%s", buf);
	    free(buf);
	}
	break;

    case 'K':
	if ((f=disp_open(opt_pager, 1)) == NULL)
	    return;

	for (i=0; i<max_fnkey+256; i++) {
	    for (b=&binding[i]; b; b=b->next) {
		if (b->fn != -1) {
		    s = (b->state != bs_none ?
			 binding_statename[b->state] : "");
		    t = print_key(i, 0);
		    l = strlen(s)+strlen(t);
		    fprintf(f, "%s%s%s%s",
			    s, t, spaces+((l>16) ? 16 : l),
			    functions[b->fn].name);
		    if (b->args) {
			for (j=0; b->args[j]; j++)
			    fprintf(f, " %s", b->args[j]);
		    }
		    fputc('\n', f);
		}
	    }
	}

	disp_close(f, 1);
	disp_status(DISP_STATUS, "");
	break;

    case 'o':
	s = read_string("Option: ", 1);
	if (s == NULL || s[0] == '\0') {
	    free(s);
	    disp_status(DISP_STATUS, "");
	    return;
	}
	    
	for (i=0; option[i].name; i++)
	    if (strcasecmp(option[i].name, s) == 0
		|| strcasecmp(option[i].shrt, s) == 0)
		break;
	
	if (option[i].name == NULL)
	    disp_status(DISP_STATUS, "no such option: %s", s);
	else {
	    char *buf;
	    
	    switch(option[i].type) {
	    case OPT_INT:
		if ((buf=(char *)malloc(30)) == NULL)
		    break;
		sprintf(buf, "(int) %d", *(option[i].var.i));
		break;
		
	    case OPT_CHR:
		if ((buf=(char *)malloc(30)) == NULL)
		    break;
		sprintf(buf, "(char) `%c'", *(option[i].var.i));
		break;

	    case OPT_STR:
		if ((buf=(char *)malloc(strlen(*(option[i].var.s))+10))
		    == NULL)
		    break;
		sprintf(buf, "(str) ``%s''", *(option[i].var.s));
		break;

	    case OPT_BOOL:
		if ((buf=(char *)malloc(30)) == NULL)
		    break;
		sprintf(buf, "(bool) %s",
			(*(option[i].var.i) ? "on" : "off"));
		break;

	    case OPT_ENUM:
		if ((buf=(char *)malloc(strlen(option[i].values[*(option[i].var.i)])+10))
		    == NULL)
		    break;
		sprintf(buf, "(enum) %s",
			option[i].values[*(option[i].var.i)]);
		break;
	    default:
		buf = NULL;
	    }

	    if (buf == NULL)
		break;

	    disp_status(DISP_STATUS, "%s (%s) [%s]: %s", 
			option[i].name, option[i].shrt,
			buf, option[i].help);
	}
	free(s);
	break;
	
    case 'O':
	if ((f=disp_open(opt_pager, 1)) == NULL)
	    return;
	
	for (i=0; option[i].name; i++) {
	    if (i > 0)
		fputc('\n', f);
	    
	    fprintf(f, "%s %-15.15s",
		     option[i].shrt, option[i].name);

	    switch(option[i].type) {
	    case OPT_INT:
		fprintf(f, "(int) %d\n", *(option[i].var.i));
		break;
		
	    case OPT_CHR:
		fprintf(f, "(char) `%c'\n", *(option[i].var.i));
		break;

	    case OPT_STR:
		fprintf(f, "(str) ``%s''\n", *(option[i].var.s));
		break;
		
	    case OPT_BOOL:
		fprintf(f, "(bool) %s\n",
			(*(option[i].var.i) ? "on" : "off"));
		break;
		
	    case OPT_ENUM:
		fprintf(f, "(enum) %s\n",
			option[i].values[*(option[i].var.i)]);
		break;
	    }
	    fprintf(f, "        %s\n", option[i].help);
	}

	disp_close(f, 1);
	disp_status(DISP_STATUS, "");
	
    default:
	disp_status(DISP_STATUS, "");
    }
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

	if (!rc_inrc) {
	    lwd = getcwd(NULL, 1024);
	    disp_status(DISP_STATUS, "Current local directory: %s", lwd);
	    free(lwd);
	}
}



void fn_shell(char **args)
{
	char *cmd;

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
	disp_status(DISP_STATUS, "");
	reenter_disp();
}



void fn_colon(char **args)
{
	char *cmd, *p, *line = NULL;
	int i;

	if (args) {
	    cmd = args[0];
	    args++;
	}
	else {
	    line = p = read_string(": ", 1);
	    if (line == NULL || line[0] == '\0') {
		free(line);
		disp_status(DISP_STATUS, "");
		return;
	    }

	    if ((cmd=rc_token(&p)) == NULL) {
		disp_status(DISP_STATUS, "no function");
		return;
	    }
	}
	
	if ((i=find_function(cmd)) < 0) {
	    disp_status(DISP_STATUS, "unknown function: %s", cmd);
	    if (line)
		free(line);
	    return;
	}

	if (line) {
	    args = rc_list(p);
	}

	disp_status(DISP_STATUS, "");
	
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
    ftp_deidle();
}



void fn_site(char **args)
{
    char *line;
    
    if (args == NULL) {
	line = read_string("site ", 1);
	if (line == NULL || line[0] == '\0') {
	    free(line);
	    disp_status(DISP_STATUS, "");
	    return;
	}
    }
    else
	line = args_to_string(args);

    ftp_site(line);

    free(line);
}



void fn_reconnect(char **args)
{
    ftp_reconnect();
}



void fn_response(char **args)
{
    struct ftp_hist *h;
    FILE *f;

    if (ftp_history == NULL) {
	disp_status(DISP_STATUS, "no exchange available");
	return;
    }
    
    if ((f=disp_open(opt_pager, 1)) == NULL)
	return;

    for (h=ftp_history; h; h=h->next)
	fprintf(f, "%s\n", h->line);

    disp_close(f, 1);
}



void
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
	    disp_status(DISP_STATUS, "");
	    return;
	}
	value = rc_token(&p);
    }

    if (opt == NULL || opt[0] == '\0') {
	disp_status(DISP_STATUS, "no option.");
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
	    disp_status(DISP_STATUS, "unknown option: %s", opt);
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
	    disp_status(DISP_STATUS, "%s set to %d", option[i].name,
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
	    disp_status(DISP_STATUS, "%s set to `%c'", option[i].name,
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
	    disp_status(DISP_STATUS, "%s set to `%s'", option[i].name,
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
	    disp_status(DISP_STATUS, "%s set to %s", option[i].name,
			(*(option[i].var.i) ? "on" : "off"));
	break;

    case OPT_ENUM:
	if (value) {
	    for (ival=0; option[i].values[ival]; ival++) {
		if (strcasecmp(value, option[i].values[ival]) == 0)
		    break;
	    }
	    if (option[i].values[ival] == NULL && !rc_inrc) {
		disp_status(DISP_STATUS, "unknown value %s for option %s",
			    value, option[i].name);
		break;
	    }

	    if (option[i].func)
		option[i].func(ival, option[i].var.i);
	    else
		*(option[i].var.i) = ival;
	}

	if (!rc_inrc)
	    disp_status(DISP_STATUS, "%s set to %s", option[i].name,
			option[i].values[(*(option[i].var.i))]);
	break;
    }
    
    if (line)
	free(line);

    return;
}



enum state leave_tag;

void
fn_state(char **args)
{
    char *state, *line;
    enum state st;
    int freestatep;

    freestatep = 0;
    line = NULL;

    if (args) {
	if (args[0][0] != '<') {
	    if ((state=(char *)malloc(strlen(args[0])+3)) == NULL)
		return;
	    sprintf(state, "<%s>", args[0]);
	    freestatep = 1;
	}
	else
	    state = args[0];
    }
    else {
	line = read_string("State: ", 1);
	if (line == NULL || line[0] == '\0')
	    return;
	if (line[0] != '<') {
	    if ((state=(char *)malloc(strlen(line)+3)) == NULL)
		return;
	    sprintf(state, "<%s>", line);
	    freestatep = 1;
	}
	else
	    state = line;
    }

    st = parse_state(state);
    if (st == binding_state)
	return;
    
    enter_state(st);

    if (freestatep)
	free(state);
    free(line);
}	



void
fn_leave_tag(char **args)
{
    if (binding_state != bs_tag) {
	disp_status(DISP_STATUS, "not in <tag> mode");
	return;
    }

    enter_state(leave_tag);
}



void
fn_mkdir(char **args)
{
    char *name;
    
    if (args == NULL) {
	name = read_string("Directory: ", 1);
	if (name == NULL || name[0] == '\0') {
	    disp_status(DISP_STATUS, "");
	    free(name);
	    return;
	}
    }
    else
	name = args[0];

    ftp_mkdir(name);

    if (args == NULL)
	free(name);

    return;
}




void
fn_rmdir(char **args)
{
    char *name;
    
    if (args == NULL) {
	name = read_string("Directory: ", 1);
	if (name == NULL || name[0] == '\0') {
	    disp_status(DISP_STATUS, "");
	    free(name);
	    return;
	}
    }
    else
	name = args[0];

    ftp_rmdir(name);

    if (args == NULL)
	free(name);

    return;
}
