#ifndef HAD_KEYS_H
#define HAD_KEYS_H

/*
  keys.h -- parse and print key specifications
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



struct keyname {
    int key;
    char *name, *longname;
};

struct fnkey {
    char *name, *longname;
    char *cap, *seq;
};

extern struct keyname keyname[];
extern struct fnkey fnkey[];
extern int max_fnkey;



char *print_key(int key, int longp);
int parse_key(char *kn);

#endif /* keys.h */


