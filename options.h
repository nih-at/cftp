#ifndef HAD_OPTIONS_H
#define HAD_OPTIONS_H

/*
  options.h -- user setable options
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



#define OPT_INT 0
#define OPT_CHR 1
#define OPT_STR 2

union optvar {
    int *i;
    char **s;
};

struct option {
    char *name, *shrt;
    char *help;
    int type;
    union optvar var;
    void (*func)();
};

extern struct option option[];

extern int opt_mode;
extern int opt_tagchar;
extern int ftp_hist_size;

#endif /* options.h */
