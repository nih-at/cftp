#ifndef HAD_FUNCTIONS_H
#define HAD_FUNCTIONS_H

/*
  functions.h -- auxiliary functions for bindable function handling
  Copyright (C) 1996 Dieter Baron

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



typedef struct function {
	void (*fn)();
	char *name;
	int type;
	char *help;
} function;

extern function functions[];

#define FN_PRE	1
#define FN_EXIT	-1



extern directory *curdir;
extern int curtop, cursel;



void void_prefix(void);
int get_prefix(int deflt);
int get_prefix_args(char **args, int deflt);
void add_prefix(int n);
void set_prefix(int p);
void negate_prefix(void);
void show_prefix(void);
void change_curdir(directory *dir);
int find_function(char *f);
void aux_scroll(int top, int sel, int force);

#endif /* functions.h */
