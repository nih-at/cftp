#ifndef HAD_FTP_H
#define HAD_FTP_H

/*
  $NiH: ftp.h,v 1.23 2001/12/20 05:53:59 dillo Exp $

  ftp.h -- ftp protocol functions
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



#include <stdio.h>

#include "methods.h"

struct ftp_hist {
    char *line;
    struct ftp_hist *next;
};

extern struct ftp_hist *ftp_history;

extern char **ftp_response;
extern char *ftp_lcwd;
extern char *ftp_pcwd;



int ftp_anon(void);
int ftp_cat(void *fin, void *fout, long start, long size, int upload);
directory *ftp_cd(char *wd, int force);
char *ftp_gets(FILE *f);
void ftp_hist(char *line);
char *ftp_host(void);
void ftp_init(void);
char *ftp_pass(void);
char *ftp_prt(void);
int ftp_reconnect(void);
void ftp_remember_host(char *host, char *port);
void ftp_remember_user(char *user, char *pass);
char *ftp_user(void);



#endif /* ftp.h */
