#ifndef HAD_DIRECTORY_Y
#define HAD_DIRECTORY_Y

/*
  $NiH: directory.h,v 1.12 2001/12/12 04:43:32 dillo Exp $

  directory.h -- handle directory cache
  Copyright (C) 1996-2002 Dieter Baron

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



#include <time.h>

struct direntry {
    char *line, *name;
    char *link;
    long size;
    time_t mtime;
    int pos;
    char type;
};

typedef struct direntry direntry;

struct directory {
    int len, top, cur;
    int size;
    struct direntry *line;
    char *path;
    int alloc_len;
    int sorted;
};

typedef struct directory directory;



int dir_add(directory *dir, direntry *entry);
int dir_find(directory *dir, char *entry);
void dir_free(directory *d);
directory *dir_new(void);
void dir_sort(directory *dir, int sort_type);
directory *get_dir(char *path, int force);



extern directory *curdir;

#endif /* directory.h */
