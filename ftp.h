#ifndef HAD_FTP_H
#define HAD_FTP_H

#include <stdio.h>

struct ftp_hist {
    char *line;
    struct ftp_hist *next;
};

extern struct ftp_hist *ftp_history;

extern char ftp_anon;
extern char *ftp_host, *ftp_prt, *ftp_user, *ftp_pass;
extern char **ftp_response;



int ftp_open(char *host, char *port);
int ftp_login(char *host, char *user, char *pass);
int ftp_reconnect(void);
int ftp_close(void);
directory *ftp_list(char *path);
directory *ftp_cd(char *wd);
int ftp_retr(char *file, FILE *fout, long size, int mode);
int ftp_noop(void);
char *ftp_pwd(void);
char *ftp_gets(FILE *f);

#endif /* ftp.h */
