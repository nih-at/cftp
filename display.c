/*
  display -- display functions
  Copyright (C) 1996, 1997, 1998, 1999, 2000 Dieter Baron

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



#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "display.h"
#include "directory.h"
#include "ftp.h"
#include "tty.h"
#include "keys.h"
#include "list.h"
#include "bindings.h"
#include "status.h"

int disp_quiet = 0;
int disp_active = 0;
char d_status[8192];



void win_line(char *line, int sel);



int
init_disp(void)
{
    int err;

    disp_quiet = 0;
    disp_active = 1;

    if ((err=tty_setup()))
	return err;

    tty_redraw = disp_redraw;

    tty_clear();
    tty_hidecrsr();

    return 0;
}



void
exit_disp()
{
    tty_goto(0, tty_lines-1);
    tty_showcrsr();
    tty_restore();
    disp_active = 0;
    printf("\n");
}



void
escape_disp(int clearp)
{
    if (disp_quiet >= 0) {
	tty_showcrsr();
	if (clearp) {
	    tty_clear();
	    fflush(stdout);
	}
	else
	    tty_goto(0, tty_lines-1);
	tty_restore();
	if (!clearp)
	    printf("\n");
    }
    --disp_quiet;
}



void
reenter_disp(void)
{
    if (++disp_quiet >= 0) {
	disp_quiet = 0;
	tty_setup();
	tty_hidecrsr();
	disp_redraw();
    }
}



void
disp_redraw(void)
{
    if (disp_quiet)
	return;

    tty_clear();
    list_do(1);
    status_do(bs_none);
    disp_restat();
}



char *
read_string(char *prompt, int echop)
{
    char *line;
    int c, i, x;
	
    line = (char *)malloc(tty_cols+1);
	
    tty_showcrsr();
    disp_status("%s", prompt);
    x = strlen(prompt);

    i = 0;
    while ((c=getchar())!='\n' && c!=EOF) {
	if (c == tty_verase) {
	    if (i > 0) {
		--i;
		if (echop)
		    printf("\b \b");
	    }
	}
	else if (c == tty_vwerase && i > 0) {
	    while (i>0 && line[--i] == ' ')
		;
	    while (i>0 && line[i-1] != ' ')
		--i;
	    if (echop) {
		tty_goto(x+i, tty_lines-1);
		tty_clreol();
	    }
	}
	else if (c == tty_vkill && i > 0) {
	    i = 0;
	    if (echop) {
		tty_goto(x, tty_lines-1);
		tty_clreol();
	    }
	}
	else if (i<tty_cols) {
	    line[i++] = c;
	    if (echop)
		putchar(c);
	}
	fflush(stdout);
    }

	line[i]	= '\0';

	tty_hidecrsr();
	fflush(stdout);
	
	return line;
    }



int
read_char(char *prompt)
{
	int c;

	tty_showcrsr();
	disp_status("%s", prompt);
	
	c = tty_readkey();
	printf(print_key(c, 0));

	tty_hidecrsr();
	fflush(stdout);

	return c;
}



int
disp_prompt_char(void)
{
	int c;
	
	tty_cbreak();
	while ((c=tty_readkey()) != EOF && c != '\n')
	    ;
	/*escape_disp(0);*/

	return 0;
}



void
disp_status(char *fmt, ...)
{
    va_list argp;

    va_start(argp, fmt);
    vsprintf(d_status, fmt, argp);
    va_end(argp);
	
    disp_restat();
}	



void
disp_restat(void)
{
	char c;
	
	if (disp_quiet)
		return;

	tty_goto(0, tty_lines-1);
	tty_clreol();

	c = d_status[tty_cols-tty_noLP];
	d_status[tty_cols-tty_noLP] = '\0';
	fputs(d_status, stdout);
	fflush(stdout);
	d_status[tty_cols-tty_noLP] = c;
}



void
win_line(char *line, int sel)
{
	int i, l;
	char save;

	if (sel)
		tty_standout();

	i = tty_cols;
	l = strlen(line);
	
	if (l < i)
		printf("%s\n", line);
	else {
		if (l >i) {
			save = line[i];
			line[i] = '\0';
		}
		printf("%s", line);
		if (!tty_am)
			putchar('\n');
		if (l > i)
			line[i] = save;
	}

	if (sel)
		tty_standend();
}



FILE *
disp_open(char *cmd, int quietp)
{
    FILE *f;

    if (quietp)
	escape_disp(1);

    if ((f=popen(cmd, "w")) == NULL) {
	if (quietp)
	    reenter_disp();
	return NULL;
    }
    
    return f;
}



int
disp_close(FILE *f, int quietp)
{
    int err;

    err = pclose(f);

    if (quietp)
	reenter_disp();

    return err;
}



void
disp_beep(void)
{
    if (!disp_quiet) {
    	fputc('\a', stdout);
	fflush(stdout);
    }
}
