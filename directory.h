#ifndef HAD_DIRECTORY_Y
#define HAD_DIRECTORY_Y

/*
  directory.h -- handle directory cache
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



struct direntry {
	char *line, *name;
	char *link;
	long size;
	char type;
};

typedef struct direntry direntry;

struct directory {
    int len, top, cur;
    int size;
    struct direntry *line;
    char *path;
};

typedef struct directory directory;



void dir_free(directory *d);
directory *get_dir(char *path);
int dir_find(directory *dir, char *entry);



extern directory *curdir;

#endif /* directory.h */
