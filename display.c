#include <stdarg.h>
#include <stdio.h>
#include "config.h"
#include "directory.h"
#include "display.h"
#include "ftp.h"
#include "tty.h"
#include "keys.h"

int disp_quiet = 0;
char head[8192], status[8192];

int oldtop, oldsel;
directory *olddir;



void disp_redir(directory *d, int top, int sel);
void disp_updir(directory *d, int oldtop, int oldsel, int top, int sel);
void disp_dndir(directory *d, int oldtop, int oldsel, int top, int sel);
void disp_sel(directory *d, int top, int sel, int selp);
void disp_restat(void);
void disp_rehead(void);
void win_line(char *line, int sel);



int
init_disp(void)
{
	int err;

	disp_quiet = 0;

	if (err=tty_setup())
	    return err;

	tty_redraw = disp_redraw;

	tty_clear();
	tty_hidecrsr();
}



void
exit_disp()
{
	tty_goto(0, tty_lines-1);
	tty_showcrsr();
	tty_restore();
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
	int i;

	if (disp_quiet)
		return;

	tty_clear();
	disp_rehead();

	tty_goto(0, 2);
	for (i=0; i<win_lines && i+oldtop < olddir->num; i++)
		win_line(olddir->list[i+oldtop].line, (i+oldtop == oldsel));

	disp_restat();
}



void
disp_dir(directory *d, int top, int sel, int newdir)
{
	if (newdir)
		olddir = d;
	
	if (disp_quiet) {
		if (newdir) {
			oldtop = top;
			oldsel = sel;
		}
		return;
	}
	
	if (!newdir && oldtop==top && oldsel==sel)
		return;

	if (!newdir && top == oldtop) {
		disp_sel(d, top, oldsel, 0);
		disp_sel(d, top, sel, 1);
	}
	else if (newdir || tty_getcap("dl") == NULL ||
		 abs(oldtop-top) > win_lines-2)
		disp_redir(d, top, sel);
	else if (top > oldtop)
		disp_updir(d, oldtop, oldsel, top, sel);
	else
		disp_dndir(d, oldtop, oldsel, top, sel);

	oldtop = top;
	oldsel = sel;
}



void
disp_reline(int line)
{
    int y = line - oldtop;

    if (y < 0 || y >= oldtop+win_lines)
	return;
    
    tty_goto(0, y+2);
    win_line(olddir->list[line].line, (line == oldsel));
}



void
disp_redir(directory *d, int top, int sel)
{
	int i;

	tty_goto(0, 2);
	tty_clreos(win_lines+2);

	for (i=0; i<win_lines && i+top < d->num; i++)
		win_line(d->list[i+top].line, (i+top == sel));

	disp_restat();
}



void
disp_updir(directory *d, int oldtop, int oldsel, int top, int sel)
{
	int i, n;

	n = top-oldtop;

	tty_goto(0, 2);
	for (i=0; i<n; i++)
		tty_delline(win_lines+2);
	tty_goto(0, 2+win_lines-n);
	tty_clreos(n+2);

	for (i=win_lines-n; i<win_lines && i+top<d->num; i++)
		win_line(d->list[i+top].line, (i+top == sel));

	if (oldsel >= top && sel != oldsel) {
		if (oldsel >= top)
			disp_sel(d, top, oldsel, 0);
		disp_sel(d, top, sel, 1);
	}

	disp_restat();
}



void
disp_dndir(directory *d, int oldtop, int oldsel, int top, int sel)
{
	int i, n;

	n = oldtop-top;

	tty_goto(0, 2);
	for (i=0; i<n; i++)
		tty_insline(win_lines+2);

	tty_goto(0, 2+win_lines);
	tty_clreos(2);
	tty_goto(0, 2);

	for (i=0; i<n && i+top<d->num; i++)
		win_line(d->list[i+top].line, (i+top == sel));

	if (oldsel-top< win_lines && sel != oldsel) {
		if (oldsel < top+win_lines)
			disp_sel(d, top, oldsel, 0);
		disp_sel(d, top, sel, 1);
	}

	disp_restat();
}



void
disp_sel(directory *d, int top, int sel, int selp)
{
	tty_goto(0, 2+sel-top);
	win_line(d->list[sel].line, selp);
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
		if (c == tty_verase && i > 0) {
			--i;
			if (echop)
			    printf("\b \b");
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
}



void
disp_status(char *fmt, ...)
{
	va_list argp;

	va_start(argp, fmt);
	vsprintf(status, fmt, argp);
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

	c = status[tty_cols-tty_noLP];
	status[tty_cols-tty_noLP] = '\0';
	fputs(status, stdout);
	fflush(stdout);
	status[tty_cols-tty_noLP] = c;
}



void
disp_head(char *fmt, ...)
{
	va_list argp;

	va_start(argp, fmt);
	vsprintf(head, fmt, argp);
	va_end(argp);
	
	disp_rehead();
}	



void
disp_rehead(void)
{
	char c;
	
	if (disp_quiet)
		return;

	tty_home();
	tty_clreol();

	c = head[tty_cols];
	head[tty_cols] = '\0';
	puts(head);
	fflush(stdout);
	head[tty_cols] = c;
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
disp_open(int lines)
{
    FILE *f;
    
    escape_disp(1);

    if ((f=popen("less", "w")) == NULL)
	return NULL;
    
    return f;
}



int
disp_close(FILE *f)
{
    int err;

    err = pclose(f);
    
    reenter_disp();

    return err;
}
