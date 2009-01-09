/*
  $NiH: fn_scroll.c,v 1.22 2002/09/16 12:42:31 dillo Exp $

  fn_scroll.c -- bindable functions: scrolling
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



#include <string.h>

#include "directory.h"
#include "bindings.h"
#include "fntable.h"
#include "functions.h"
#include "display.h"
#include "tty.h"
#include "list.h"
#include "loop.h"
#include "options.h"

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
	if (opt_wrap)
	    sel = (list->cur+n) % list->len;
	else
	    sel = list->cur+n;
	if (list->top > sel || list->top <= sel-win_lines)
	    top = sel-win_lines+1;
	else
	    top = list->top;
    }
    else {
	if (opt_wrap) {
	    sel = list->cur-((-n) % list->len);
	    if (sel < 0)
		sel += list->len;
	}
	else
	    sel = list->cur + n;
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

    if (n == 0)
	return;
    
    if (n > 0) {
	if (list->cur == list->len-1) {
	    if (opt_wrap)
		list->top = list->cur = 0;
	    else
		return;
	}
	else if (list->top >= list->len - win_lines)
	    list->cur = list->len-1;
	else {
	    top = list->top + n;
	    if (top > list->len) {
		if (opt_wrap) {
		    list->cur = list->top
			+ ((list->len-list->top)/win_lines)*win_lines;
		    list->top = list->len - win_lines;
		}
		else {
		    list->cur = list->len-1;
		    list->top = list->len - win_lines;
		}
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
	else if (list->top == 0) {
	    if (opt_wrap) {
		list->top = list->len - win_lines;
		if (list->top < 0)
		    list->top = 0;
		list->cur = list->len-1;
	    }
	    else
		return;
	}
	else {
	    /* XXX: inconsistent with forward scrolling */
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

    char b[1024], *p;
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

    disp_status(DISP_STATUS, "");
    
    return;
}
