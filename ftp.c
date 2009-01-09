/*
  $NiH: ftp.c,v 1.84 2004/03/08 12:22:05 dillo Exp $

  ftp.c -- ftp protocol functions
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



#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include "directory.h"
#include "display.h"
#include "sockets.h"
#include "ftp.h"
#include "readdir.h"
#include "options.h"
#include "util.h"
#include "bindings.h"
#include "status.h"
#include "signals.h"



char *ftp_lcwd;
char *ftp_pcwd;
char ftp_curmode;		/* current transfer mode (' ' for unknown) */
char *ftp_last_resp;
static char *_ftp_host, *_ftp_port, *_ftp_user, *_ftp_pass;
				/* host, port, user, passwd (for reconnect) */
static int _ftp_anon;		/* whether we're using anonymous ftp */

struct ftp_hist *ftp_history;	/* command/response history */
struct ftp_hist *ftp_hist_last; /* tail of history */
int ftp_hist_cursize;		/* size of history (for dynamic growth) */

static int _ftp_keptresp;
static int ftp_dosnames;	/* server sends directory names in
				   DOS format */

char **ftp_response;
long ftp_response_size;

FILE *conin, *conout;		/* control connection to server */

#ifdef HAVE_STRUCT_SOCKADDR_STORAGE
static struct sockaddr_storage _ftp_addr;
#else
static struct sockaddr _ftp_addr;
#endif
struct sockaddr *ftp_addr = (struct sockaddr *)&_ftp_addr;
				/* local ip address (for port commands) */



struct _ftp_transfer_stats {
    off_t got, start, size;
    int secs, stall_sec;
    int ncur, ccur;
    off_t *cur;
};



static int ftp_abort(FILE *fin);
static FILE *ftp_accept(int fd, char *mode);
static int ftp_gethostaddr(int fd);
static int ftp_mode(char m);
static int ftp_port(void);
static int ftp_put(char *fmt, ...);
static int ftp_resp(void);
static void ftp_unresp(int resp);

static int _ftp_ascii2host(char *buf, char *buf2, int n, int *trail_cr);
static int _ftp_host2ascii(char *buf, char *buf2, int n, int *trail_cr);

static void _ftp_transfer_stats_init(struct _ftp_transfer_stats *tr,
				     off_t start, off_t size, int ncur);
static void _ftp_transfer_stats_cleanup(struct _ftp_transfer_stats *tr);
static void _ftp_update_transfer(struct _ftp_transfer_stats *tr, off_t got,
				 int secs);



void
ftp_init(void)
{
    ftp_lcwd = ftp_pcwd = NULL;
    ftp_curmode = ' ';
    _ftp_anon = 0;
    ftp_last_resp = NULL;
    _ftp_host = _ftp_user = _ftp_port = _ftp_pass = NULL;
    ftp_history = NULL;
    _ftp_keptresp = -1;
    ftp_dosnames = -1;
    ftp_response = NULL;
    ftp_response_size = 0;
    conin = conout = NULL;
}



int
rftp_open(char *host, char *port, char *user, char *pass)
{
    int fd, resp;

    if (host) {
	free(_ftp_host);
	_ftp_host = strdup(host);
	free(_ftp_port);
	if (port && strcmp(port, "ftp") != 0)
	    _ftp_port = strdup(port);
	else
	    _ftp_port = NULL;
    }

    if ((fd=sopen(_ftp_host, _ftp_port ? _ftp_port : "ftp", AF_UNSPEC)) == -1)
	return -1;
    
    if (ftp_gethostaddr(fd) == -1) {
	close(fd);
	return -1;
    }
    
    conin = fdopen(fd, "r");
    conout = fdopen(fd, "w");
    
    if (!conin || !conout) {
	close(fd);
	return -1;
    }

    if ((resp=ftp_resp()) == 120)
	resp = ftp_resp();
	
    if (resp != 220) {
	close(fd);
	return -1;
    }

    ftp_remember_user(user, pass);

    return 0;
}



int
rftp_login(char *user, char *pass)
{
    int resp;
    char *b;
    int free_pass;

    status.host = mkhoststr(0, 0);
    
    ftp_put("user %s", _ftp_user);
    resp = ftp_resp();
	
    if (resp == 331) {
	free_pass = 0;
	if (_ftp_pass)
	    pass = _ftp_pass;
	else {
	    if (_ftp_anon) {
		free_pass = 1;
		pass = get_anon_passwd();
	    }
	    else {
		b = (char *)malloc(strlen(status.host)+14);
		sprintf(b, "Password (%s): ", status.host);
		if (disp_active) {
		    pass = read_string(b, 0);
		    free_pass = 1;
		}
		else
		    pass = getpass(b);
		free(b);
	    }
	}
	
	ftp_put("pass %s", pass);
	resp = ftp_resp();

	if (free_pass)
	    free(pass);
    }

    if (resp != 230)
	return -1;

    free(status.remote.path);
    status.remote.path = NULL;
    status_do(bs_none);
	
    return 0;
}



int
rftp_site(char *cmd)
{
    ftp_put("%s", cmd);
    return ftp_resp();
}



int
ftp_reconnect(void)
{
    if (_ftp_host == NULL)
	return -1;

    ftp_close();

    disp_status(DISP_INFO, "connecting. . .");

    if (ftp_open(NULL, NULL, NULL, NULL) == -1) {
	/* Error printed in ftp_open or sopen. */
	return -1;
    }

    if (ftp_login(NULL, NULL) == -1) {
	/* ftp response is error message */
	return -1;
    }

    status.remote.path = strdup(ftp_lcwd);
    status_do(bs_remote);
    ftp_pcwd = NULL;
    ftp_curmode = ' ';

    return 0;
}



int
rftp_close(void)
{
    int err = 0;
    
    if (conin == NULL)
	return 0;
    
    ftp_put("quit");
    if (ftp_resp() != 221)
	err = 1;

    if (conin)
	fclose(conin);

    if (conout)
	fclose(conout);
    
    conin = conout = NULL;
    
    return err;
}



directory *
rftp_list(char *path)
{
    directory *dir;
    int fd, resp;
    FILE *f;
    
    if (ftp_mode('a') == -1 || ftp_cwd(path) == -1)
	return NULL;
    
    if ((fd=ftp_port()) == -1)
	return NULL;
    
    ftp_put("list");
    if ((resp=ftp_resp()) != 150 && resp != 125) {
	close(fd);
	dir = (directory *)malloc(sizeof(directory));
	dir->line = (direntry *)malloc(sizeof(direntry));
	dir->path = strdup(path);
	dir->len = 0;
	dir->cur = dir->top = 0;
	dir->size = sizeof(struct direntry);
	dir->line->line = strdup("");
	dir->line->type = 'x';
	dir->line->name = strdup("");
	dir->line->link = NULL;
	return dir;
    }
    if ((f=ftp_accept(fd, "r")) == NULL)
	return NULL;
    
    dir = read_dir(f);
    if (dir)
	dir->path = strdup(path);
    
    fclose(f);
    
    ftp_resp();
    
    return dir;
}



directory *
ftp_cd(char *wd, int force)
{
    directory *dir;
    char *nwd;
    
    nwd = canonical(wd, NULL);
    
    dir = get_dir(nwd, force);
    if (dir != NULL) {
	free(ftp_lcwd);
	ftp_lcwd = nwd;
	
	free(status.remote.path);
	status.remote.path = strdup(ftp_lcwd);
	status_do(bs_remote);
    }
    else
	free(nwd);
    
    return dir;
}
	


void *
rftp_retr(char *file, int mode, off_t *startp, off_t *sizep)
{
    int fd, resp;
    char *dir, *name, *can, *p;
    FILE *fin;
    
    can = canonical(file, NULL);
    dir = xdirname(can);
    name = noalloc_basename(can);
    
    if (ftp_mode(mode) == -1 || ftp_cwd(dir) == -1)
	return NULL;
    
    if ((fd=ftp_port()) == -1)
	return NULL;
    
    if (startp && *startp > 0) {
	ftp_put("rest %lld", *startp);
	if (ftp_resp() != 350)
	    *startp = 0;
    }
    
    ftp_put("retr %s", name);
    if ((resp=ftp_resp()) != 150 && resp != 125) {
	close(fd);
	return NULL;
    }
    if (sizep) {
	/* XXX: check how other servers format 150s */
	if (strcmp(ftp_last_resp+strlen(ftp_last_resp)-8, " bytes).") == 0) {
	    if ((p=strrchr(ftp_last_resp, '(')) != NULL)
		*sizep = strtoll(p+1, NULL, 10);
	}
	else
	    *sizep = -1;
    }
    if ((fin=ftp_accept(fd, "r")) == NULL) {
	close(fd);
	return NULL;
    }
    
    return fin;
}



void *
rftp_stor(char *file, int mode)
{
    int fd, resp;
    char *dir, *name, *can;
    FILE *fin;
    
    can = canonical(file, NULL);
    dir = xdirname(can);
    name = noalloc_basename(can);
    
    if (ftp_mode(mode) == -1 || ftp_cwd(dir) == -1)
	return NULL;
    
    if ((fd=ftp_port()) == -1)
	return NULL;
    
    ftp_put("stor %s", name);
    if ((resp=ftp_resp()) != 150 && resp != 125) {
	close(fd);
	return NULL;
    }
    if ((fin=ftp_accept(fd, "w")) == NULL) {
	close(fd);
	return NULL;
    }
    
    return fin;
}



int
rftp_fclose(void *f)
{
    int err, resp;

    err = fclose(f);

    if (((resp=ftp_resp()) != 226 && resp != 250))
	return -1;
	
    return err;
}



int
rftp_mkdir(char *path)
{
    ftp_put("mkd %s", path);
    if (ftp_resp() != 257)
	return -1;
    
    return 0;
}



int
rftp_rmdir(char *path)
{
    ftp_put("rmd %s", path);
    if (ftp_resp() != 250)
	return -1;
    
    return 0;
}



int
rftp_deidle(void)
{
    ftp_put("noop");
    if (ftp_resp() != 200)
	return -1;
    
    return 0;
}



char *
rftp_pwd(void)
{
    char *s, *e, *dir;

    ftp_put("pwd");

    if (ftp_resp() != 257)
	return NULL;

    if ((s=strchr(ftp_last_resp, '"')) == NULL
	|| (e=strchr(s+1, '"')) == NULL)
	return NULL;
    
    if ((dir=(char *)malloc(e-s+1)) == NULL)
	return NULL;

    strncpy(dir, s+1, e-s-1);
    dir[e-s-1] = '\0';

    if (ftp_dosnames == -1) {
	if ((isalpha(dir[0]) && dir[1] == ':' && dir[2] == '\\'))
	    ftp_dosnames = 1;
	else if (dir[0] == '\\')
	    ftp_dosnames = 2;
	else
	    ftp_dosnames = 0;
    }

    if (ftp_dosnames == 1 || ftp_dosnames == 2) {
	if (ftp_dosnames == 1) {
	    strncpy(dir+1, s+1, e-s-1);
	    dir[e-s] = '\0';
	    dir[0] = '/';
	}
	for (s=dir; *s; s++)
	    if (*s == '\\')
		*s = '/';
    }

    free(ftp_pcwd);
    ftp_pcwd = strdup(dir);

    return dir;
}



char *
ftp_gets(FILE *f)
{
    char buf[8192], *line;
    int l, l2;
    
    if (fgets(buf, 8192, f) == NULL)
	return NULL;
    
    line = strdup(buf);
    l = strlen(line);
    
    while (line[l-1] != '\n') {
	if (fgets(buf, 8192, f) == NULL) {
	    free(line);
	    return NULL;
	}
	l2 = strlen(buf);
	if ((line=realloc(line, l+l2+1)) == NULL) {
	    disp_status(DISP_ERROR, "malloc failure");
	    return NULL;
	}
	strcpy(line+l, buf);
	l += l2;
    }
    if (line[l-2] == '\r')
	line[l-2] = '\0';
    else
	line[l-1] = '\0';
    
    return line;
}



static int
ftp_put(char *fmt, ...)
{
    char buf[8192];
    va_list argp;
    
    if (conout == NULL) {
	disp_status(DISP_ERROR, "not connected");
	return -1;
    }
    
    va_start(argp, fmt);
    vsprintf(buf, fmt, argp);
    va_end(argp);
    
    if (strncmp(buf, "pass ", 5) == 0 && _ftp_anon == 0)
	disp_status(DISP_PROTO, "-> pass ********");
    else
	disp_status(DISP_PROTO, "-> %s", buf);
    fprintf(conout, "%s\r\n", buf);
    
    if (fflush(conout) || ferror(conout)) {
	disp_status(DISP_ERROR, "error writing to server: %s",
		    strerror(errno));
	return -1;
    }
    
    return 0;
}	



static int
ftp_abort(FILE *fin)
{
    int resp;
    char buf[4096];
    fd_set ready;
    struct timeval poll;
    
    /* check wether abort is necessary (no data on control connection) */
    poll.tv_sec = poll.tv_usec = 0;
    FD_ZERO(&ready);
    FD_SET(fileno(conin), &ready);
    /* XXX: error ignored */
    if (select(fileno(conin)+1, &ready, NULL, NULL, &poll) == 1)
	return 0;
    
    /* do abort */
    disp_status(DISP_PROTO, "-> <attention>");
	     
    if (send(fileno(conout), "\377\364\377" /* IAC IP IAC */, 3,
	     MSG_OOB) != 3) {
	disp_status(DISP_ERROR, "cannot send attention: %s",
		    strerror(errno));
	return -1;
    }

    if (fputc('\362' /* DM */, conout) == EOF) {
	disp_status(DISP_ERROR, "cannot send attention: %s",
		    strerror(errno));
	return -1;
    }

    ftp_put("abor");

    /* XXX: what about uploads? */
    /* read remaining bytes from data connection */
    while (fread(buf, 4096, 1, fin) > 0)
	;
	
    /* hanlde server response */
    resp = ftp_resp();

    if (resp == 226 || resp == 250) {
	ftp_unresp(226);

	sleep(1);
	/* check wether 226 is for us */
	poll.tv_sec = poll.tv_usec = 0;
	FD_ZERO(&ready);
	FD_SET(fileno(conin), &ready);
	/* XXX: error ignored */
	if (select(fileno(conin)+1, &ready, NULL, NULL, &poll) == 1)
	    ftp_resp();

	return 0;
    }
    else if (resp == 426) {
	resp = ftp_resp();
    	ftp_unresp(426);
	disp_status(DISP_STATUS, "426 Transfer aborted.");
	    
	return (resp == 226 || resp == 250);
    }
    else
	return 1;
}




static int
ftp_resp(void)
{
    char *line;
    int resp;
    
    if (_ftp_keptresp != -1) {
	resp = _ftp_keptresp;
	_ftp_keptresp = -1;
	return resp;
    }

    if (conin == NULL) {
	disp_status(DISP_ERROR, "not connected");
	return -1;
    }

    clearerr(conin);
    if ((line=ftp_gets(conin)) == NULL) {
	if (ferror(conin))
	    disp_status(DISP_ERROR, "read error from server: %s",
			strerror(errno));
	else
	    disp_status(DISP_ERROR, "connection to server lost");
	return -1;
    }

    /* XXX: DISP_PROTO should be used for first line */
    resp = atoi(line);
    disp_status(DISP_STATUS, "%s", line);
    free(ftp_last_resp);
    ftp_last_resp = strdup(line);

    while (!(isdigit(line[0]) && isdigit(line[1]) &&
	     isdigit(line[2]) && line[3] == ' ')) {
	disp_status(DISP_HIST, "%s", line);
	
	if ((line=ftp_gets(conin)) == NULL) {
	    disp_status(DISP_ERROR, "read error from server: %s",
			strerror(errno));
	    return -1;
	}
    }

    disp_status(DISP_HIST, "%s", line);
    
    return resp;
}



static void
ftp_unresp(int resp)
{
    _ftp_keptresp = resp;
}



static int
ftp_port(void)
{
    int fd, port, val, i, delim;
    int len;
    unsigned int iaddr;
    unsigned char addr[4];
    char baddr[16], bport[6], *s, *e;
    char *saddr, *sport;
#ifdef HAVE_STRUCT_SOCKADDR_STORAGE
    struct sockaddr_storage sa;
#else
    struct sockaddr sa;
#endif
    struct sockaddr_in *sin;

    if (!opt_pasv) {
	sin = (struct sockaddr_in *)ftp_addr;

	len = sizeof(sa);
	if ((fd=spassive(ftp_addr->sa_family,
			 (struct sockaddr *)&sa, &len)) == -1)
	    return -1;

	port = htons(((struct sockaddr_in *)&sa)->sin_port);

	if (ftp_addr->sa_family == AF_INET) {
	    iaddr = htonl(sin->sin_addr.s_addr);
	    ftp_put("port %d,%d,%d,%d,%d,%d",
		    (iaddr>>24) & 0xff, (iaddr>>16) & 0xff,
		    (iaddr>>8) & 0xff, iaddr & 0xff,
		    (port>>8) & 0xff, port & 0xff);
	}
	else {
	    ftp_put("eprt |%d|%s|%d|",
		    /* proto no */ 2,
		    sockaddr_ntop(ftp_addr), port);
	}
	
	if (ftp_resp() != 200) {
	    close(fd);
	    return -1;
	}
    }
    else {
	if (ftp_addr->sa_family == AF_INET) {
	    ftp_put("pasv");
	    if (ftp_resp() != 227)
		return -1;

	    if ((s=strchr(ftp_last_resp, ',')) == NULL)
		return -1;
	    
	    while (isdigit(*(--s)))
		;
	    s++;
	    
	    for (i=0; i<4; i++) {
		val = strtol(s, &e, 10);
		if (val < 0 || val > 255 || *e != ',')
		    return -1;
		addr[i] = val;
		s = e+1;
	    }
	    port = strtol(s, &e, 10);
	    if (port < 0 || port > 255 || *e != ',')
		return -1;
	    s = e+1;
	    val = strtol(s, &e, 10);
	    if (val < 0 || val > 255)
		return -1;
	    port = port*256+val;
	    
	    sprintf(baddr, "%d.%d.%d.%d",
		    addr[0], addr[1], addr[2], addr[3]);
	    sprintf(bport, "%d", port);
	    saddr = baddr;
	    sport = bport;
	}
	else { /* inet 6 passive */
	    ftp_put("epsv");
	    if (ftp_resp() != 229)
		return -1;

	    if ((s=strchr(ftp_last_resp, '(')) == NULL)
		return -1;
	    delim = s[1];

	    if ((e=strchr(s+2, delim)) == NULL
		|| (e=strchr(e+1, delim)) == NULL)
		return -1;

	    s = e+1;
	    if ((e=strchr(s, delim)) == NULL)
		return -1;
	    if (e-s>5)
		return -1;
	    strncpy(bport, s, e-s);
	    bport[e-s] = '\0';
	    sport = bport;
	    saddr = _ftp_host;
	}
	fd = sopen(saddr, sport, ftp_addr->sa_family);
    }
    
    return fd;
}



static FILE *
ftp_accept(int fd, char *mode)
{
    int len, ns;
    struct sockaddr addr;

    if (!opt_pasv) {
	len = sizeof(addr);
	if ((ns=accept(fd, &addr, &len)) == -1)
	    return NULL;
	
	close(fd);
	fd = ns;
    }
	
    fcntl(fd, F_SETFD, 1); /* XXX: error check */
    
    return fdopen(fd, mode);
}



static int
ftp_mode(char m)
{
    if (m == ftp_curmode)
	return 0;
    
    ftp_put("type %c", toupper(m));
    if (ftp_resp() != 200)
	return -1;
    
    ftp_curmode = m;
    return 0;
}



int
rftp_cwd(char *path)
{
    char *s;
    int off;
    
    if (ftp_pcwd && strcmp(path, ftp_pcwd) == 0)
	return 0;

    if (ftp_dosnames == 1 || ftp_dosnames == 2) {
	off = (ftp_dosnames == 1) ? 1 : 0;
	for (s=path+off; *s; s++)
	    if (*s == '/')
		*s = '\\';
	ftp_put("cwd %s", path+off);
	for (s=path+off; *s; s++)
	    if (*s == '\\')
		*s = '/';
    }
    else
	ftp_put("cwd %s", path);
    if (ftp_resp() != 250)
	return -1;

    if (ftp_dosnames == -1) {
	if ((s=strchr(ftp_last_resp, '"')) != NULL
	    && strchr(s+1, '"') != NULL) {
	    if (isalpha(s[1]) && s[2] == ':' && s[3] == '\\')
		ftp_dosnames = 1;
	    else if (s[1] == '\\')
		ftp_dosnames = 2;
	    else
		ftp_dosnames = 0;
	}
	else
	    ftp_dosnames = 0;
    }

    free(ftp_pcwd);
    ftp_pcwd = strdup(path);
    
    return 0;
}



int
ftp_cat(void *fin, void *fout, off_t start, off_t size, int upload)
{
    char buf[4096], buf2[8192], *p;
    int nread, nwritten, err, trail_cr, errno_copy;
    enum { ERR_NONE, ERR_FIN, ERR_FOUT } error_cause;
    int old_alarm;
    off_t got;
    struct itimerval itv;
    struct _ftp_transfer_stats trstat;
    int do_read;
    int (*fn_read)(void *, size_t, void *);
    int (*fn_write)(void *, size_t, void *);
    int (*fn_eof)(void *);

    got = start;
    signal(SIGINT, sig_remember);

    _ftp_transfer_stats_init(&trstat, start, size, 3);
    old_alarm = sig_alarm = sig_intr = 0;
    itv.it_value.tv_sec = itv.it_interval.tv_sec = 1;
    itv.it_value.tv_usec = itv.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &itv, NULL);

    /* XXX: error check */
    if (upload) {
	/* fout is to server */
	if (ftp_xfer_start(fout) < 0)
	    return -1;

	fn_read = rftp_xfer_read;
	fn_eof = rftp_xfer_eof;
	fn_write = ftp_xfer_write;

	rftp_xfer_start(fin);
    }
    else {
	/* fin is from server */

	if (ftp_xfer_start(fin) < 0)
	    return -1;
	   
	fn_read = ftp_xfer_read;
	fn_eof = ftp_xfer_eof;
	fn_write = rftp_xfer_write;

	rftp_xfer_start(fout);
    }

    error_cause = ERR_NONE;
    trail_cr = 0;
    do_read = 1;
    for (;;) {
	if (do_read) {
	    if ((nread=fn_read(buf, 4096, fin)) > 0) {
		do_read = 0;
		nwritten = 0;
		if (ftp_curmode != 'a')
		    p = buf;
		else {
		    p = buf2;
		    if (upload)
			nread = _ftp_host2ascii(buf, buf2, nread, &trail_cr);
		    else
			nread = _ftp_ascii2host(buf, buf2, nread, &trail_cr);
		}
	    }
	    else if (nread < 0) {
		if (!fn_eof(fin)) {
		    errno_copy = errno;
		    error_cause = ERR_FIN;
		}
		break;
	    }
	}
	else {
	    if ((err=fn_write(p+nwritten, nread-nwritten, fout)) < 0) {
		errno_copy = errno;
		error_cause = ERR_FOUT;
		break;
	    }
	    nwritten += err;
	    got += err;
	    
	    if (nwritten == nread)
		do_read = 1;
	}
	
	if (old_alarm != sig_alarm) {
	    _ftp_update_transfer(&trstat, got, sig_alarm);
	    old_alarm = sig_alarm;
	}

	if (sig_intr)
	    break;
    }

    signal(SIGINT, sig_end);
    itv.it_value.tv_sec = itv.it_interval.tv_sec = 0;
    itv.it_value.tv_usec = itv.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &itv, NULL);

    _ftp_transfer_stats_cleanup(&trstat);

    if (error_cause || sig_intr) {
	sig_intr = 0;
	ftp_xfer_stop((upload ? fout : fin), 1);
	/* this is a hack, we should really use local file methods */
	rftp_xfer_stop((upload ? fin : fout), 0);
	switch (error_cause) {
	case ERR_FIN:
	    disp_status(DISP_ERROR, "read error: %s", strerror(errno_copy));
	    break;
	case ERR_FOUT:
	    disp_status(DISP_ERROR, "write error: %s", strerror(errno_copy));
	    break;
	default:
	    ;
	}
	return -1;
    }

    ftp_xfer_stop((upload ? fout : fin), 0);
    rftp_xfer_stop((upload ? fin : fout), 0);
    return 0;
}



static void
_ftp_update_transfer(struct _ftp_transfer_stats *tr, off_t got, int secs)
{
    off_t base, step;
    int nsec, i, stsec, eta;
    double rtot, rcur, rbps;
    char b[512];

    nsec = secs-tr->secs;

    if (nsec) {
	base = tr->cur[tr->ccur];
	step = (got-base) / nsec;
	for (i=0; i<nsec-1; i++) {
	    tr->ccur = (tr->ccur+1) % tr->ncur;
	    tr->cur[tr->ccur] = base + step*i;
	}
	tr->ccur = (tr->ccur+1) % tr->ncur;
	tr->cur[tr->ccur] = got;
    }
    if (tr->got != got) {
	tr->got = got;
	tr->stall_sec = secs;
	if (got > tr->size)
	    tr->size = -1;
    }
    tr->secs = secs;

    rtot = (got-tr->start)/(double)(secs*1024);
    rcur = ((tr->cur[tr->ccur] - tr->cur[(tr->ccur+1)%tr->ncur])
	    / ((double)(secs < tr->ncur-1 ? secs : tr->ncur-1)*1024));
    if (got == tr->start || tr->size == -1)
	eta = -1;
    else {
	rbps = (got-tr->start)/(double)secs;
	eta = (tr->size-got) / rbps;
    }

    stsec = secs - tr->stall_sec;
    if (stsec > opt_stall) {
	disp_status(DISP_ERROR,
		    "stalled for more than %d seconds, aborting.",
		    opt_stall);
	sig_intr = 1;
    }
    else {
	if (stsec > tr->ncur) {
	    if (stsec >= 60)
		sprintf(b, "stalled: %d:%02ds", stsec/60, stsec%60);
	    else
		sprintf(b, "stalled: %ds", stsec);
	}
	else
	    sprintf(b, "cur: %.2fkb/s", rcur);
	    
	if (eta != -1) {
	    if (eta > 60*60)
		sprintf(b+strlen(b), ", eta: %d:%02d:%02ds",
			eta/(60*60), (eta/60)%60, eta%60);
	    else if (eta > 60)
		sprintf(b+strlen(b), ", eta: %d:%02ds",
			eta/60, eta%60);
	    else
		sprintf(b+strlen(b), ", eta: %ds", eta);
	}

	if (tr->size != -1)
	    disp_status(DISP_STATUS,
			"transferred %lld/%lld (tot: %.2fkb/s, %s)",
			got, tr->size, rtot, b);
	else
	    disp_status(DISP_STATUS,
			"transferred %lld (tot: %.2fkb/s, %s)",
			got, rtot, b);
    }
}



static void
_ftp_transfer_stats_init(struct _ftp_transfer_stats *tr,
			 off_t start, off_t size, int cur_secs)
{
    int i;

    /* XXX: check return value */
    tr->ncur = cur_secs+1;
    tr->cur = malloc(tr->ncur*sizeof(off_t));
    tr->ccur = 0;
    for (i=0; i<tr->ncur; i++)
	tr->cur[i] = start;

    tr->got = tr->start = start;
    tr->size = size;
    tr->secs = tr->stall_sec = 0;
}



static void
_ftp_transfer_stats_cleanup(struct _ftp_transfer_stats *tr)
{
    free(tr->cur);
    tr->ncur = 0;
}



static int
ftp_gethostaddr(int fd)
{
    int len;
	
    len = sizeof(_ftp_addr);
    if (getsockname(fd, ftp_addr, &len) == -1) {
	disp_status(DISP_ERROR, "can't get host address: %s",
		    strerror(errno));
	return -1;
    }
    
    return 0;
}



void ftp_hist(char *line)
{
    struct ftp_hist *p;

    if (ftp_hist_size == 0)
	return;

    if (ftp_history == NULL) {
	if ((ftp_history=(struct ftp_hist *)
	     malloc(sizeof(struct ftp_hist))) == NULL) {
	    free(line);
	    return;
	}
	ftp_hist_last = ftp_history;
	ftp_history->next = NULL;
	ftp_history->line = line;
	ftp_hist_cursize = 1;

	return;
    }

    if ((ftp_hist_last->next=(struct ftp_hist *)
	 malloc(sizeof(struct ftp_hist))) == NULL) {
	free(line);
	return;
    }
    
    ftp_hist_last = ftp_hist_last->next;
    ftp_hist_last->next = NULL;
    ftp_hist_last->line = line;

    if (++ftp_hist_cursize > ftp_hist_size) {
	--ftp_hist_cursize;
	p = ftp_history;
	ftp_history = ftp_history->next;
	free(p->line);
	free(p);
    }
}



void
ftp_set_hist_size(int size, int *sizep)
{
    struct ftp_hist *p;

    if (size < 0)
	return;

    while (ftp_hist_cursize > size) {
	--ftp_hist_cursize;
	p = ftp_history;
	ftp_history = ftp_history->next;
	free(p->line);
	free(p);
    }

    ftp_hist_size = size;
}



void
opts_mode(int c, int *cp)
{
    switch (c) {
    case 'a':
	opt_mode = 'a';
	break;

    case 'i':
    case 'b':
	opt_mode = 'i';
    }
}



char *
ftp_host(void)
{
    return _ftp_host;
}



char *
ftp_prt(void)
{
    return _ftp_port;
}



char *
ftp_user(void)
{
    return _ftp_user;
}



char *
ftp_pass(void)
{
    return _ftp_pass;
}



int
ftp_anon(void)
{
    return _ftp_anon;
}



static int
_ftp_ascii2host(char *buf, char *buf2, int n, int *trail_cr)
{
    char *s, *t;
    int cr;

    if (n == 0)
	return 0;

    s = buf;
    t = buf2;
    cr = *trail_cr;

    for (; n; --n,s++) {
	if (cr) {
	    if (*s != '\n')
		*t++ = '\r';
	    cr = 0;
	}
	if (*s == '\r')
	    cr = 1;
	else
	    *(t++) = *s;
    }

    *trail_cr = cr;
    
    return t-buf2;
}



static int
_ftp_host2ascii(char *buf, char *buf2, int n, int *trail_cr)
{
    char *s, *t;
    int cr;

    if (n == 0)
	return 0;

    s = buf;
    t = buf2;
    cr = *trail_cr;

    for (; n; --n,s++) {
	if (*s == '\n' && !cr)
	    *t++ = '\r';
	cr = (*s == '\r');
	*t++ = *s;
    }

    *trail_cr = cr;
    
    return t-buf2;
}



void
ftp_remember_user(char *user, char *pass)
{
    if (user) {
	free(_ftp_user);
	_ftp_user = strdup(user);
	
	free(_ftp_pass);
	if (pass)
	    _ftp_pass = strdup(pass);
	else
	    _ftp_pass = NULL;

	if (strcmp(_ftp_user, "ftp") != 0
	    && strcmp(_ftp_user, "anonymous") != 0)
	    _ftp_anon = 0;
	else
	    _ftp_anon = 1;
    }
}



void
ftp_remember_host(char *host, char *port)
{
    if (host) {
	free(_ftp_host);
	_ftp_host = strdup(host);

	free(_ftp_port);
	if (port)
	    _ftp_port = strdup(port);
	else
	    _ftp_port = NULL;
    }
}



/* this method is also used for local files */
int
rftp_xfer_read(void *buf, size_t len, void *file)
{
    int n;
    FILE *f;
    fd_set fdset;

    f = (FILE *)file;
    
    for (;;) {
	n = fread(buf, 1, len, f);

	if (n == 0) {
	    if (ferror(f) && (errno == EINTR || errno == EAGAIN)) {
		clearerr(f);
		if (errno == EAGAIN) {
		    FD_ZERO(&fdset);
		    FD_SET(fileno(f), &fdset);
		    
		    if (select(fileno(f)+1, &fdset, NULL, NULL, NULL) == -1
			|| !FD_ISSET(fileno(f), &fdset))
			return 0;
		    continue;
		}
	    }
	    else
		return -1;
	}

	return n;
    }
}



int
rftp_xfer_start(void *file)
{
    FILE *f;

    f = (FILE *)file;

    set_file_blocking(fileno(f), 0);

    return 0;
}



int
rftp_xfer_stop(void *file, int aborting)
{
    if (aborting)
	return ftp_abort((FILE *)file);
    
    return 0;
}



/* this method is also used for local files */
int
rftp_xfer_write(void *buf, size_t len, void *file)
{
    int n;
    FILE *f;
    fd_set fdset;

    f = (FILE *)file;

    for (;;) {
	n = fwrite(buf, 1, len, f);

	if (n == 0) {
	    if (ferror(f) && (errno == EINTR || errno == EAGAIN)) {
		clearerr(f);
		if (errno == EAGAIN) {
		    FD_ZERO(&fdset);
		    FD_SET(fileno(f), &fdset);
		    
		    if (select(fileno(f)+1, NULL, &fdset, NULL, NULL) == -1
			|| !FD_ISSET(fileno(f), &fdset))
			return 0;
		    continue;
		}
	    }
	    else
		return -1;
	}
	return n;
    }
}



/* this method is also used for local files */
int
rftp_xfer_eof(void *file)
{
    return feof((FILE *)file);
}
