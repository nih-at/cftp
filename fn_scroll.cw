@ bindable functions for scrolling directory listsings.

@(fn_scroll.fn@)
section(fn_scroll, Scrolling Functions)
@<functions@>
endsec()

@u
#include "directory.h"
#include "functions.h"
#include "display.h"
#include "tty.h"

@<local prototypes@>


@ this function restrains a request for placement to possible values
and does the actual move.  set force to force a complete update
(e.g. if you change the dirlist)

@d<local prototypes@>
void aux_scroll(int top, int sel, int force);

@u
void
aux_scroll(int top, int sel, int force)
{
	if (sel < 0)
		sel = 0;
	if (sel >= curdir->num)
		sel = curdir->num - 1;

	if (top > sel)
		top = sel;
	if (top <= sel - win_lines)
		top = sel - win_lines + 1;
	
	if (top > curdir->num - win_lines)
		top = curdir->num - win_lines;
	if (top < 0)
		top = 0;

	curtop = top;
	cursel = sel;

	disp_dir(curdir, curtop, cursel, force);
}

@d<local prototypes@>
void aux_scroll_line(int n);

@u
void
aux_scroll_line(int n)
{
	int sel, top;

	if (n == 0)
		return;
	if (n > 0) {
		sel = (cursel+n) % curdir->num;
		if (curtop > sel || curtop <= sel-win_lines)
			top = sel-win_lines+1;
		else
			top = curtop;
	}
	else {
		sel = cursel-((-n)%curdir->num);
		if (sel < 0)
			sel += curdir->num;
		if (curtop > sel || curtop <= sel-win_lines)
			top = sel;
		else
			top = curtop;
	}
	aux_scroll(top, sel, 0);
}


@d<local prototypes@>
void aux_scroll_page(int n);

@u
void
aux_scroll_page(int n)
{
	aux_scroll(curtop+n, cursel+n, 0);
}


@d<functions@>
function(down, [n], fn_down, 0,
	 {move N lines down (default: 1)},
 {Move cursor down N lines, wrapping around at bottom.  (N is the
prefix argument.)})

function(up, [n], fn_up, 0,
	 {move N lines up (default: 1)},
 {Move cursor up N lines, wrapping around at top.  (N is the prefix
argument.)})

function(page-down, [n], fn_pg_down, 0,
	 {move N screenfulls down (default: 1)},
 {Move cursor down N screenfulls, or to bottom.  (N is the prefix
argument.)})

function(page-up, [n], fn_pg_up, 0,
	 {move N screenfulls up (default: 1)},
 {Move cursor up N screenfulls, or to top.  (N is the prefix argument.)})


@u
void
fn_down(char **args)
{
	aux_scroll_line(get_prefix_args(args, 1));
}

void
fn_up(char **args)
{
	aux_scroll_line(-get_prefix_args(args, 1));
}

void
fn_pg_down(char **args)
{
	aux_scroll_page(get_prefix_args(args, 1)*win_lines);
}

void
fn_pg_up(char **args)
{
	aux_scroll_page(-get_prefix_args(args, 1)*win_lines);
}


@ goto line.

@d<functions@>
function(goto, [line], fn_goto, 0,
	 {goto line N (default: last)},
 {Move cursor to line N.  (N is the prefix argument.)})


@u
void
fn_goto(char **args)
{
	int n;

	n = get_prefix_args(args, curdir->num)-1;
	if (n == -1)
		n = curdir->num-1;
	
	aux_scroll(n-(win_lines/2), n, 0);
}


@ incremental search

@d<functions@>
function(isearch, , fn_isearch, 0,
	 {incremnetal search},
 {Search directory listing incrementally.})


@u
void
fn_isearch(char **args)
{
    char b[1024], *p;
    int n, c, start, current;

    strcpy(b, "isearch: ");
    p = b+9;
    n = 0;

    start = current = cursel;

    while ((c=read_char(b)) != '\n' && c != 7 /* ^G */) {
	if (c == tty_verase) {
	    *(--p) = '\0';
	    current = start;
	}
	else if (c == tty_vkill) {
	    p = b+9;
	    *p = '\0';
	    current = start;
	}
	else {
	    *(p++) = c;
	    *p = '\0';
	}

	for (n = current;
	     n < curdir->num && strstr(curdir->list[n].name, b+9) == NULL;
	     n++)
	    ;

	if (n < curdir->num) {
	    current = n;
	    if (current >= curtop && current < curtop+win_lines)
		aux_scroll(curtop, current, 0);
	    else
		aux_scroll(current-(win_lines/2), current, 0);
	}
    }
    if (c == 7 /* ^G */) {
	    if (start >= curtop && start < curtop+win_lines)
		aux_scroll(curtop, start, 0);
	    else
		aux_scroll(start-(win_lines/2), start, 0);
    }

    disp_status("");
    
    return;
}
