/*
  fn_scroll -- bindable functions: scrolling
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



#include <string.h>

#include "directory.h"
#include "bindings.h"
#include "functions.h"
#include "display.h"
#include "tty.h"
#include "list.h"
#include "loop.h"

void aux_scroll(int top, int sel, int force);
void aux_scroll_line(int n);
void aux_scroll_page(int n);



void
aux_scroll(int top, int sel, int force)
{
    if (sel < 0)
	sel = 0;
    if (sel >= list->len)
	sel = list->len -1;

    if (top > sel)
	top = sel;
    if (top <= sel - win_lines)
	top = sel - win_lines + 1;
	
    if (top > list->len - win_lines)
	top = list->len - win_lines;
    if (top < 0)
	top = 0;

    list->top = top;
    list->cur = sel;

    if (force >= 0)
	list_do(force);
}



void
aux_scroll_line(int n)
{
    int sel, top;

    if (n == 0 || list->len == 0)
	return;

    if (n > 0) {
	sel = (list->cur+n) % list->len;
	if (list->top > sel || list->top <= sel-win_lines)
	    top = sel-win_lines+1;
	else
	    top = list->top;
    }
    else {
	sel = list->cur-((-n) % list->len);
	if (sel < 0)
	    sel += list->len;
	if (list->top > sel || list->top <= sel-win_lines)
	    top = sel;
	else
	    top = list->top;
    }
    aux_scroll(top, sel, 0);
}



void
aux_scroll_page(int n)
{
    int top;
    
    if (list->len <= win_lines || n == 0)
	return;

    if (n > 0) {
	if (list->top >= list->len - win_lines)
	    list->top = list->cur = 0;
	else {
	    top = list->top + n;
	    if (top > list->len) {
		list->cur = list->top
		    + ((list->len-list->top)/win_lines)*win_lines;
		list->top = list->len - win_lines;
	    }
	    else if (top > list->len - win_lines) {
		list->cur = top;
		list->top = list->len - win_lines;
	    }
	    else
		list->top = list->cur = top;
	}
    }
    else if (n < 0) {
	if (list->cur > list->top)
	    list->cur = list->top;
	else if (list->top == 0)
	    list->top = list->cur = list->len - win_lines;
	else {
	    top = list->top + n;
	    if (top < 0)
		top = 0;
	    list->top = list->cur = top;
	}
    }

    list_do(0);
}



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



void
fn_goto(char **args)
{
    int n;

    n = get_prefix_args(args, list->len)-1;
    if (n == -1)
	n = list->len-1;
	
    aux_scroll(n-(win_lines/2), n, 0);
}



#define FN_IS_BACK	0x1
#define FN_IS_FAIL	0x2
#define FN_IS_WRAP	0x4

int
aux_isread(int state, char *s)
{
    char b[2048];

    sprintf(b, "%s%s%sI-search%s: %s",
	   (state & FN_IS_FAIL ? "Failing " : ""),
	   (state & FN_IS_WRAP ? (state & FN_IS_FAIL ? "w" : "W") : ""),
	   (state & FN_IS_WRAP ? "rapped " : ""),
	   (state & FN_IS_BACK ? " backward" : ""),
	   s);

    return read_char(b);
}

void
fn_isearch(char **args)
{

    char b[1024], *prompt, *p;
    int n, c, start, current, research, state, wrap;

    state = 0;

    if (args && strcmp(args[0], "-r") == 0) {
	state = FN_IS_BACK;
    }
    
    b[0] = '\0';
    p = b;
    n = 0;

    start = current = list->cur;

    while ((c=aux_isread(state, b)) != '\n' && c != 7 /* ^G */) {
	research = 1;
	if (c == tty_verase) {
	    if (p > b) {
		*(--p) = '\0';
		current = start;
		state &= ~FN_IS_WRAP;
	    }
	    else
		research = 0;
	}
	else if (c == tty_vwerase) {
	    while (p>b && *(--p) == ' ')
		;
	    while (p>b && *(p-1) != ' ')
		--p;

	    *p = '\0';

	    current = start;
	    state &= ~FN_IS_WRAP;
	}
	else if (c == tty_vkill) {
	    p = b;
	    *p = '\0';
	    
	    current = start;
	    state &= ~FN_IS_WRAP;
	}
	else if (c == 19 /* ^S */) {
	    if ((state & FN_IS_FAIL) && !(state & FN_IS_BACK))
		state |= FN_IS_WRAP;
	    state &= ~FN_IS_BACK;
	    research = 2;
	}
	else if (c == 18 /* ^R */) {
	    if ((state & FN_IS_FAIL) && (state & FN_IS_BACK))
		state |= FN_IS_WRAP;
	    state |= FN_IS_BACK;
	    research = 2;
	}
	else if (c == 12 /* ^L */) {
	    disp_redraw();
	    research = 0;
	}
	else if (c >= 32 && c < 127) {
	    *(p++) = c;
	    *p = '\0';
	}
	else {
	    loop_putkey(c);
	    return;
	}

	if (research) {
	    n = current;
	    state &= ~FN_IS_FAIL;
	    
	    if (!(state & FN_IS_BACK)) {
		if (research == 2)
		    n++;

		wrap = 0;
		while (!(state & FN_IS_FAIL)) {
		    if (wrap && n == current+1) {
			state |= FN_IS_FAIL;
			break;
		    }
		    if (n >= list->len) {
			if (state & FN_IS_WRAP) {
			    wrap = 1;
			    n = 0;
			}
			else {
			    state |= FN_IS_FAIL;
			    break;
			}
		    }
		    if (strstr(LIST_LINE(list, n)->name, b) != NULL)
			break;
		    n++;
		}
	    }
	    else {
		if (research == 2)
		    --n;

		wrap = 0;
		while (!(state & FN_IS_FAIL)) {
		    if (wrap && n == current-1) {
			state |= FN_IS_FAIL;
			break;
		    }
		    if (n < 0) {
			if (state & FN_IS_WRAP) {
			    n = list->len-1;
			    wrap = 1;
			}
			else {
			    state |= FN_IS_FAIL;
			    break;
			}
		    }
		    if (strstr(LIST_LINE(list, n)->name, b) != NULL)
			break;
		    --n;
		}
	    }
	}
	if (!(state & FN_IS_FAIL)) {
	    current = n;
	    if (current >= list->top && current < list->top+win_lines)
		aux_scroll(list->top, current, 0);
	    else
		aux_scroll(current-(win_lines/2), current, 0);
	}
    }
    if (c == 7 /* ^G */) {
	if (start >= list->top && start < list->top+win_lines)
	    aux_scroll(list->top, start, 0);
	else
	    aux_scroll(start-(win_lines/2), start, 0);
    }

    disp_status("");
    
    return;
}
