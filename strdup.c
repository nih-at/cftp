/*
  $NiH: strdup.c,v 1.9 2001/12/11 14:37:42 dillo Exp $

  strdup.c -- save a copy of a string
  Copyright (C) 1996-2002 Dieter Baron

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



#include <stdlib.h>
#include <string.h>

char *
strdup(char *s)
{
	char *t;

	if (t=malloc(strlen(s)+1))
		strcpy(t, s);

	return t;
}


