#ifndef HAD_FTP_H
#define HAD_FTP_H

/*
  $NiH: ftp.h,v 1.24 2002/09/16 12:42:33 dillo Exp $

  ftp.h -- ftp protocol functions
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
int ftp_cat(void *fin, void *fout, off_t start, off_t size, int upload);
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
