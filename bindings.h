#ifndef HAD_BINDINGS_H
#define HAD_BINDINGS_H

/*
  bindings.h -- key bindings
  Copyright (C) 1996, 1997, 1998, 2000 Dieter Baron

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



enum state {
    bs_nostate = -10, bs_unknown,
    bs_quit = -1, bs_none = 0,
    bs_remote, bs_local, bs_tag
};

struct binding {
    struct binding *next;
    enum state state;
    int fn;
    char **args;
};



extern enum state binding_state;
extern struct binding binding[];

extern char *binding_statename[];

extern struct binding binding_pool[];
extern char *binding_argpool[];
extern int binding_npool, binding_nargpool;



enum state parse_state(char *name);

#endif /* bindings.h */
