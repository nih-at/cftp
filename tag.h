#ifndef HAD_TAG_H
#define HAD_TAG_H

/*
  tag.h -- tagging
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



struct dirtags {
    char *name;
    struct dirtags *next;
    struct filetags *tags;
};

struct filetags {
    char *name;
    struct filetags *next;
    long size;
    char type;
};

typedef struct dirtags dirtags;
typedef struct filetags filetags;

extern dirtags tags;
extern dirtags *curtags;



void tag_changecurrent(char *dir);
int tag_file(char *dir, char *file, long size, char type, int flag);
int tag_anytags(void);

#endif /* tag.h */
