/*
  $NiH: list.c,v 1.21 2002/09/27 16:35:01 dillo Exp $

  list.c -- display lists
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



#include <stdlib.h>
#include <string.h>

#include "display.h"
#include "list.h"
#include "tty.h"
#include "options.h"
#include "bindings.h"
#include "status.h"

struct list *list;

enum list_scrolltype { LIST_SCNONE, LIST_SCLINE, LIST_SCREGION };

enum list_scrolltype list_scrolltype;

static struct list *last_list;
static int last_top, last_sel;

void list_full(struct list *list);
void list_region(struct list *list, int up, int n);
void list_scroll(struct list *list, int up, int n);

void list_refill(struct list *list, int st, int n, int clreolp);

void list_desel(struct list *list, int top, int sel);
void list_sel(struct list *list);
void list_line(struct list *list, int i, int selp, int clreolp);



void
list_do(int full)
{
    int up, n, per;

    if (list == NULL)
	return;

    if (disp_quiet ||
	(!full && list->top == last_top && list->cur == last_sel))
	return;

    if (list != last_list) {
	full = 1;
	last_list = list;
    }

    if (list->len <= win_lines)
	per = -1;
    else
	per = (list->top*100)/(list->len-win_lines);
    if (per != status.percent) {
	status.percent = per;
	status_do(bs_none);
    }

    if (full || abs(last_top-list->top) >= win_lines-opt_scrlimit) {
	list_full(list);
    }
    else if (last_top != list->top) {
	if (last_sel >= list->top && last_sel < list->top+win_lines
	    && list_scrolltype)
	    list_desel(list, last_top, last_sel);

	if (list->top < last_top) {
	    up = 0;
	    n = last_top - list->top;
	}
	else {
	    up = 1;
	    n = list->top - last_top;
	}
	
	switch (list_scrolltype) {
	case LIST_SCREGION:
	    list_region(list, up, n);
	    break;
	case LIST_SCLINE:
	    list_scroll(list, up, n);
	    break;
	case LIST_SCNONE:
	    list_full(list);
	    break;
	}

	if (list->cur >= last_top && list->cur < last_top+win_lines
	    && list_scrolltype)
	    list_sel(list);
    }
    else {
	list_desel(list, last_top, last_sel);
	list_sel(list);
    }

    last_top = list->top;
    last_sel = list->cur;
}



void
list_full(struct list *list)
{
    int i;
    
    if (!*TTY_CAP(ce)) {
	tty_goto(0, win_top);
	tty_clreos(win_lines+2);
	status_do(bs_none);
	disp_restat();
    }
    tty_goto(0, win_top);
    list_refill(list, list->top, win_lines, *TTY_CAP(ce));
    if (*TTY_CAP(ce))
	for (i=list->len; i<win_lines; i++) {
	    tty_clreol();
	    putc('\n', stdout);
	}
}



void
list_region(struct list *list, int up, int n)
{
    /* 
    if (up)
	tty_goto(0, win_bottom);
    else
	tty_goto(0, win_top);
    */
	
    tty_scregion(win_top, win_bottom);
    
    if (up) {
	tty_goto(0, win_bottom);
	tty_scrollup(n, win_lines);
	tty_scregion(0, tty_lines-1);
	tty_goto(0, win_bottom-n+1);
	list_refill(list, list->top+win_lines-n, n, 0);
    }
    else {
	tty_goto(0, win_top);
	tty_scrolldown(n, win_lines);
	tty_scregion(0, tty_lines-1);
	tty_goto(0, win_top);
	list_refill(list, list->top, n, 0);
    }

    tty_lowleft();
}



void
list_scroll(struct list *list, int up, int n)
{
    if (up) {
	tty_goto(0, win_top);
	tty_dellines(n, win_lines+2);
	tty_goto(0, win_bottom+1);
	tty_clreos(2);
	tty_goto(0, win_lines-n+1);

	list_refill(list, list->top+win_lines-n, n, 0);
    }
    else {
	tty_goto(0, win_top);
	tty_inslines(n, win_lines+2);
	tty_goto(0, win_bottom+1);
	tty_clreos(2);
	tty_goto(0, win_top);
	
	list_refill(list, list->top, n, 0);
    }

    disp_restat();
}



void
list_refill(struct list *list, int top, int n, int clreolp)
{
    int i, end;

    end = top+n;

    for (i=top; i<end && i<list->len; i++)
	list_line(list, i, list->cur == i, clreolp);
}



void
list_desel(struct list *list, int top, int sel)
{
    if (sel < 0 || sel >= list->len)	/* happens on empty lists */
	return;
    
    tty_goto(0, win_top+sel-top);
    list_line(list, sel, 0, 0);
}




void
list_sel(struct list *list)
{
    if (list->cur < 0 || list->cur >= list->len)
	return;
    
    tty_goto(0, win_top+list->cur-list->top);
    list_line(list, list->cur, 1, 0);
}



void
list_line(struct list *list, int i, int selp, int clreolp)
{
    int l, cols;
    char *s, save;

    s = LIST_LINE(list, i)->line;
    l = strlen(s);
    cols = tty_cols;

    if (selp)
	tty_standout();
    
    if (l < cols) {
	fputs(s, stdout);
	if (selp)
	    tty_standend();
	if (clreolp)
	    tty_clreol();
	putc('\n', stdout);
    }
    else {
	save = s[cols];
	s[cols] = '\0';

	switch (tty_am) {
	case TTY_AMNONE:
	case TTY_AMXN:
	    puts(s);
	    break;
	case TTY_AM:
	    fputs(s, stdout);
	}

	s[cols] = save;

	if (selp)
	    tty_standend();
    }
}



void
list_init(void)
{
    last_list = list = NULL;

    if (*TTY_CAP(cs))
	list_scrolltype = LIST_SCREGION;
    else if ((*TTY_CAP(AL) || *TTY_CAP(al)) && (*TTY_CAP(DL) || *TTY_CAP(dl)))
	list_scrolltype = LIST_SCLINE;
    else
	list_scrolltype = LIST_SCNONE;
}



void
list_reline(int n)
{
    if (n < last_top || n > last_top+win_lines)
	return;

    tty_goto(0, win_top+n-last_top);
    list_line(last_list, n, (n == last_sel), 0);
}
