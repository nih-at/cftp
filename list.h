#ifndef HAD_LIST_Y
#define HAD_LIST_Y

/*
  list.h -- display lists
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



struct listentry {
	char *line, *name;
};

struct list {
    int len, top, cur;
    int size;
    void *line;
};

#define LIST_LINE(list, i)	\
	((struct listentry *)((char *)(list)->line+(list)->size*(i)))

extern struct list *list;	/* currently displayed list */



void list_init(void);
void list_do(int full);
void list_reline(int line);

#endif /* list.h */


