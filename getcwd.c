/*
  getcwd -- get working directory pathname
  Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001 Dieter Baron

  The author can be contacted at <dillo@giga.or.at>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/



#include <sys/types.h>

char *
getcwd(char *buf, size_t size)
{
	static char b[1024];

	getwd(b);

	if (buf == 0)
		return strdup(b);

	strncpy(buf, b, size);
	return buf;
}


