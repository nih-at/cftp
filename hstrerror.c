/*
  hstrerror -- get resolver error message string
  Copyright (C) 1996, 1997 Dieter Baron

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



#include <stdio.h>

static char *_h_errlist[] = {
    "Error 0",
    "Unknown host",			/* 1 HOST_NOT_FOUND */
    "Host name lookup failure",		/* 2 TRY_AGAIN */
    "Unknown server error",		/* 3: NO_RECOVERY */
    "No address associated with name",	/* 4: NO_ADDRESS */
    "Service unavailable",		/* 5 AIX: SERVICE_UNAVAILABLE */
};

static int _h_nerr = sizeof(_h_errlist)/sizeof(_h_errlist[0]);



char *
hstrerror(int err)
{
    static char b[40];

    if (err < 0 || err > _h_nerr) {
	sprintf(b, "Unknown resolver error %d", err);
	return b;
    }

    return _h_errlist[err];
}
