#ifndef HAD_RC_H
#define HAD_RC_H

/*
  rc.h -- auxiliary functions for parsing .cftprc file
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



char *rc_token(char **linep);
char **rc_list(char *line);

extern int rc_inrc;
extern int rc_lineno;
extern char *rc_filename;

void rc_error(char *fmt, ...);

#endif /* rc.h */
