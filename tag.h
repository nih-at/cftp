#ifndef HAD_TAG_H
#define HAD_TAG_H

/*
  $NiH$

  tag.h -- tagging
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



#include "directory.h"

struct tagentry {
    char *line, *name;
    struct tagentry *next, *prev;
    char *file;
    int dirl;
    long size;
    char type;
};

struct taglist {
    int len, top, cur;
    int size;
    struct tagentry *line;
    long reallen;
};

enum tagopt { TAG_ON, TAG_OFF, TAG_TOGGLE };

extern struct taglist tags;



int tag_init(void);
int tag_file(char *dir, char *file, long size, char type, enum tagopt what);
void tag_delete(int n);
void tag_clear(void);
int tag_anytags(void);

void change_curdir(directory *dir);

#endif /* tag.h */
