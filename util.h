#ifndef HAD_UTIL_H
#define HAD_UTIL_H

/*
  $NiH: util.h,v 1.14 2001/12/11 14:37:45 dillo Exp $

  util.h -- auxiliary functions
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



char *args_to_string(char **args);
char *canonical(char *path, char *current);
char *dirname(char *name);
char *get_anon_passwd(void);
char *local_exp(char *path);
char *mkhoststr(int passp, int urlp);
int set_file_blocking(int fd, int blocking);

#endif /* util.h */
