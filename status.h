#ifndef HAD_STATUS_H
#define HAD_STATUS_H

/*
  $NiH: status.h,v 1.9 2001/12/13 21:14:56 dillo Exp $

  status.h -- status line
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

#include "bindings.h"



struct status {
    char *host;
    int percent;
    struct {
	char *path;
    } remote;
    struct {
	char *path;
    } local;
/*  struct {
    } tag; */
};

extern struct status status;
extern char status_line[];



void status_init(void);
void status_do(enum state state);
void enter_state(enum state state);

#endif /* status.h */
