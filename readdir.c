/*
  $NiH: readdir.c,v 1.25 2002/10/23 09:19:48 dillo Exp $

  readdir.c -- read directory listing
  Copyright (C) 1996-2002 Dieter Baron

  This file is part of cftp, a fullscreen ftp client
  The author can be contacted at <dillo@giga.or.at>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.
  3. The name of the author may not be used to endorse or promote
     products derived from this software without specific prior
     written permission.
 
  THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/



#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "directory.h"
#include "ftp.h"
#include "display.h"
#include "options.h"
#include "readdir.h"

static int parse_unix(direntry *de, char *line);
static int parse_ms(direntry *de, char *line);
static void init_parse_time(void);
static time_t parse_time(char *date);

typedef int (*parse_func)(direntry *, char *);

parse_func pfunc[] = {
    parse_unix, parse_ms
};
int npfunc = sizeof(pfunc)/sizeof(pfunc[0]);



#define DIR_STEP	1024

directory *
read_dir(FILE *f)
{
    directory *dir;
    direntry entry;
    int n, pf, ret;
    char *line;
    time_t oldt, newt;

    dir = dir_new();
    n = 0;

    pf = 0;
    oldt = 0;

    init_parse_time();

    while ((line=ftp_gets(f)) != NULL) {
	while ((ret=pfunc[pf](&entry, line)) == -1) {
	    pf++;
	    if (pf >= npfunc) {
		pf = 0;
		ret = 1;
		break;
	    }
	}
	if (ret == 0) {
	    dir_add(dir, &entry);
	    n++;
	}
	free(line);

	if ((newt=time(NULL)) != oldt) {
	    disp_status(DISP_STATUS, "listed %d", n);
	    oldt = newt;
	}
    }

#if 0
    if (n == 0) {
	dir->line = (direntry *)malloc(sizeof(direntry));
	dir->line->line = strdup("");
	dir->line->type = 'x';
	dir->line->name = strdup("");
	dir->line->link = NULL;
	n = 1;
    }
#endif

    return dir;
}



static int
parse_unix(direntry *de, char *line)
{
    char *p, *q;
	
    if (strncmp(line, "total ", 6) == 0)
	return 1;

    if (strcspn(line, " ") < 10
	|| (line[10]!=' ' && !isdigit(line[10])))
	return -1;

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
    de->size = strtoll(p, &q, 10);
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
	de->size = -1;
    }
    else {
	de->name = strdup(p);
	de->link = NULL;
    }

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



static int
parse_ms(direntry *de, char *line)
{
    struct tm tm;
	
    if (!(isdigit(line[0]) && isdigit(line[1]) && line[2] == '-'
	  && isdigit(line[3]) && isdigit(line[4]) && line[5] == '-'
	  && isdigit(line[6]) && isdigit(line[7]) && line[8] == ' '))
	return -1;

    if ((de->line=(char *)malloc(strlen(line)+2)) == NULL)
	return 1;
    
    de->line[0] = ' ';
    strcpy(de->line+1, line);

    if (strncmp(line+23, " <DIR>  ", 8) == 0) {
	de->type = 'd';
	de->size = -1;
    }
    else {
	de->type = 'f';
	de->size = strtoll(line+17, NULL, 10);
    }

    /* time */
    
    tm.tm_sec = tm.tm_wday = tm.tm_yday = 0;
    tm.tm_mon = atoi(line);
    tm.tm_mday = atoi(line+3);
    tm.tm_year = atoi(line+6);
    if (tm.tm_year < 50) {
	/* y2.05k problem */
	tm.tm_year += 100;
    }
    tm.tm_min = atoi(line+13);
    tm.tm_hour = atoi(line+10) % 12;
    if (line[15] == 'P')	
	tm.tm_hour += 12;
    
    
    de->mtime = mktime(&tm);

    de->name = strdup(line+39);

    return 0;
}

