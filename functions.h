#ifndef HAD_FUNCTIONS_H
#define HAD_FUNCTIONS_H

/*
  $NiH$

  functions.h -- auxiliary functions for bindable function handling
  Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001 Dieter Baron

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



struct function {
	void (*fn)();
	char *name;
	int type;
	char *help;
};

typedef struct function function;

extern function functions[];

#define FN_PRE	1
#define FN_RC   2
#define FN_EXIT	-1




void void_prefix(void);
int get_prefix(int deflt);
int get_prefix_args(char **args, int deflt);
void add_prefix(int n);
void set_prefix(int p);
void negate_prefix(void);
void show_prefix(void);
int find_function(char *f);
struct binding *get_function(int key, enum state state);

void aux_scroll(int top, int sel, int force);
int aux_enter(char *name);
int aux_download(char *name, long size, int restart);
int aux_pipe(char *name, long size, int mode, char *cmd, int quietp);
#define aux_view(name)	(aux_pipe((name), -1, 'a', opt_pager, 1))

#endif /* functions.h */
