#ifndef HAD_URL_H
#define HAD_URL_H

/*
  $NiH$

  url.h -- functions to parse and create URLs
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



#define URL_UCHAR  0x1
#define URL_XCHAR  0x2



int parse_url(char *url, char **user, char **pass,
	      char **host, char **port, char **dir);
char *url_decode(char *d, const char *s);
int url_declen(char *s);
char *url_encode(char *d, const char *s, int which);
int url_enclen(char *s, int which);
int url_special(int c, int which);

#endif /* url.h */
