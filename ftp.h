#ifndef HAD_FTP_H
#define HAD_FTP_H

/*
  ftp.h -- ftp protocol functions
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



#ifndef SWIG

#include <stdio.h>

struct ftp_hist {
    char *line;
    struct ftp_hist *next;
};

extern struct ftp_hist *ftp_history;

extern char ftp_anon;
extern char *ftp_host, *ftp_prt, *ftp_user, *ftp_pass;
extern char **ftp_response;
extern char *ftp_pcwd;

#endif



#ifndef SWIG

void ftp_init(void);

directory *ftp_list(char *path);
directory *ftp_cd(char *wd, int force);
FILE *ftp_retr(char *file, int mode, long *startp, long *sizep);
FILE *ftp_stor(char *file, int mode);
int ftp_fclose(FILE *f);
char *ftp_gets(FILE *f);
int ftp_cat(FILE *fin, FILE *fout, long start, long size);

#endif

#ifdef SWIG
int ftp_open(char *host, char *port="ftp");
#else
int ftp_open(char *host, char *port);
#endif

int ftp_login(char *host, char *user, char *pass);
int ftp_reconnect(void);
int ftp_close(void);
int ftp_mkdir(char *path);
int ftp_site(char *cmd);
int ftp_noop(void);
char *ftp_pwd(void);

#endif /* ftp.h */
