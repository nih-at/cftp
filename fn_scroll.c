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



#include "directory.h"
#include "functions.h"
#include "display.h"
#include "tty.h"

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

    disp_dir(list, top, sel, force);
}



void
aux_scroll_line(int n)
{
    int sel, top;

    if (n == 0)
	return;

    if (n > 0) {
	sel = (list->sel+n) % list->len;
	if (list->top > sel || list->top <= sel-win_lines)
	    top = sel-win_lines+1;
	else
	    top = list->top;
    }
    else {
	sel = list->sel-((-n) % list->len);
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
    top = list->top + n;

    if (top < 0)
	top = 0;
    if (top > list->len-win_lines)
	top = list->len-win_lines;
    
    disp_dir(list, top, top, 0);
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

    n = get_prefix_args(args, curdir->num)-1;
    if (n == -1)
	n = curdir->num-1;
	
    aux_scroll(n-(win_lines/2), n, 0);
}



void
fn_isearch(char **args)
{
    char b[1024], *p;
    int n, c, start, current, origin;

    strcpy(b, "isearch: ");
    p = b+9;
    n = 0;

    origin = start = current = cursel;

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
	else if (c == 19 /* ^S */) {
	    start = current;
	    current++;
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
