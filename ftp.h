#ifndef HAD_FTP_H
#define HAD_FTP_H

/*
  $NiH: ftp.h,v 1.18 2001/12/11 14:37:32 dillo Exp $

  ftp.h -- ftp protocol functions
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



#include <stdio.h>

struct ftp_hist {
    char *line;
    struct ftp_hist *next;
};

extern struct ftp_hist *ftp_history;

extern char **ftp_response;
extern char *ftp_lcwd;
extern char *ftp_pcwd;



int ftp_anon(void);
int ftp_cat(FILE *fin, FILE *fout, long start, long size, int upload);
directory *ftp_cd(char *wd, int force);
int ftp_close(void);
int ftp_cwd(char *path);
int ftp_fclose(FILE *f);
char *ftp_gets(FILE *f);
void ftp_hist(char *line);
void ftp_histf(char *fmt, ...);
char *ftp_host(void);
void ftp_init(void);
directory *ftp_list(char *path);
int ftp_login(char *user, char *pass);
int ftp_mkdir(char *path);
int ftp_open(char *host, char *port);
int ftp_noop(void);
char *ftp_pass(void);
char *ftp_prt(void);
char *ftp_pwd(void);
int ftp_reconnect(void);
FILE *ftp_retr(char *file, int mode, long *startp, long *sizep);
int ftp_site(char *cmd);
FILE *ftp_stor(char *file, int mode);
char *ftp_user(void);

#endif /* ftp.h */
