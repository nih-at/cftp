/*
  readdir -- read directory listing
  Copyright (C) 1996, 1997, 1998 Dieter Baron

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



#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "directory.h"
#include "ftp.h"
#include "options.h"

static int parse_unix(direntry *de, char *line);
static void init_parse_time(void);
static time_t parse_time(char *date);



#define DIR_STEP	1024

directory *
read_dir(FILE *f)
{
    directory *dir;
    direntry *list;
    int i, n, sz;
    char *line;

    dir = malloc(sizeof(directory));
    list = NULL;
    sz = n = 0;

    init_parse_time();

    while ((line=ftp_gets(f)) != NULL) {
	if (n >= sz) {
	    sz += DIR_STEP;
	    if (list == NULL)
		list = malloc(sizeof(direntry)*sz);
	    else
		list = realloc(list, sizeof(direntry)*sz);
	}
	if (parse_unix(list+n, line) == 0) {
	    list[n].pos = n;
	    n++;
	}
    }

    if (n == 0) {
	dir->line = (direntry *)malloc(sizeof(direntry));
	dir->line->line = strdup("");
	dir->line->type = 'x';
	dir->line->name = strdup("");
	dir->line->link = NULL;
	n = 1;
    }
    else {
	dir->line = malloc(sizeof(direntry)*n);
	for (i=0; i<n; i++)
	    dir->line[i] = list[i];
    }
    dir->sorted = 0;
    dir->len = n;
    dir->size = sizeof(struct direntry);
    dir->top = dir->cur = 0;

    return dir;
}



static int
parse_unix(direntry *de, char *line)
{
    char *p, *q;
	
    if (strncmp(line, "total ", 6) == 0)
	return 1;

    if ((de->line=(char *)malloc(strlen(line)+2)) == NULL)
	return 1;
    
    de->line[0] = ' ';
    strcpy(de->line+1, line);

    switch (line[0]) {
    case 'l':
    case 'd':
	de->type = line[0];
	break;
    case '-':
    case 'p':
	de->type = 'f';
	break;
    default:
	de->type = 'x';
    }

    strtok(line+10, " ");	/* skip perms, links */
    strtok(NULL, " ");	/* owner */
    strtok(NULL, " ");	/* group */
    if ((p=strtok(NULL, " ")) == NULL) {
	free(line);
	return 1;
    }
    de->size = strtol(p, &q, 10);
			/* size  */
    if (p == q)
	de->size = -1;
    else if ((p=strtok(NULL, " ")) == NULL) {
	free(line);
	return 1;
    }

    p[12] = '\0';
    de->mtime = parse_time(p);

    p += 13;
    
    if (de->type == 'l') {
	de->size = -1;
	q = p + strlen(p) - de->size;
	if (strncmp(q-4, " -> ", 4) == 0) {
	    q[-4] = '\0';
	    de->name = strdup(p);
	    de->link = strdup(q);
	}
	else {
	    if ((q=strstr(p, " -> ")) == NULL) {
		/* unknown link format */
		de->name = strdup(p);
		de->link = NULL;
	    }
	    else {
		*q = '\0';
		de->name = strdup(p);
		de->link = strdup(q+4);
	    }
	}
    }
    else {
	de->name = strdup(p);
	de->link = NULL;
    }

    free(line);	

    if (de->type == 'd' && (strcmp(de->name, "..")==0 ||
			    strcmp(de->name, ".")==0)) {
	free(de->name);
	return 1;
    }

    return 0;
}



static struct tm now;

static void
init_parse_time(void)
{
    struct tm *p;
    time_t t;
    
    t = time(NULL);
    p = gmtime(&t);
    now = *p;
}

static time_t
parse_time(char *date)
{
    static char *mon[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
			   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
    struct tm tm;
    int i;

    tm = now;
    tm.tm_sec = tm.tm_wday = tm.tm_yday = 0;

    for (i=0; i<12; i++)
	if (strncasecmp(date, mon[i], 3) == 0) {
	    tm.tm_mon = i;
	    break;
	}

    if (i==12)
	return 0;

    tm.tm_mday = atoi(date+4);
    if (tm.tm_mday == 0)
	return 0;

    if (date[9] == ':') {
	tm.tm_hour = atoi(date+7);
	tm.tm_min = atoi(date+10);

	if (tm.tm_mon < now.tm_mon
	    || (tm.tm_mon == now.tm_mon && tm.tm_mday <= now.tm_mday))
	    tm.tm_year = now.tm_year;
	else
	    tm.tm_year = now.tm_year - 1;
    }
    else {
	tm.tm_year = atoi(date+7);
	if (tm.tm_year == 0)
	    return 0;
	tm.tm_year -= 1900;
	tm.tm_hour = tm.tm_min = 0;
    }

    return mktime(&tm);
}
