/*
  ftp -- ftp protocol functions
  Copyright (C) 1996, 1997, 1998, 1999, 2000 Dieter Baron

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

#ifdef HAVE_BASENAME
# ifdef HAVE_LIBGEN_H
#  include <libgen.h>
# endif
#else
char *basename(char *);
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

extern char *prg;



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



int ftp_put(char *fmt, ...);
int ftp_abort(FILE *fin);
int ftp_resp(void);
void ftp_unresp(int resp);
int ftp_port(void);
FILE *ftp_accept(int fd, char *mode);
int ftp_mode(char m);
int ftp_cwd(char *path);
int ftp_gethostaddr(int fd);
void ftp_histf(char *fmt, ...);
void ftp_hist(char *line);

#ifdef ENABLE_TRANSFER_RATE
static void _ftp_update_transfer(char *fmt, long got, long *cur,
				 int old_sec, int new_sec);
#endif



void
ftp_init(void)
{
    ftp_pcwd = NULL;
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
ftp_open(char *host, char *port)
{
    int fd;

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
    
    if (ftp_resp() != 220) {
	close(fd);
	return -1;
    }

    return 0;
}



int
ftp_login(char *user, char *pass)
{
    int resp;
    char *b;
    int free_pass;

    if (user) {
	free(_ftp_user);
	_ftp_user = strdup(user);

	free(_ftp_pass);
	if (pass)
	    _ftp_pass = strdup(pass);
	else
	    _ftp_pass = NULL;
    }

    if (strcmp(_ftp_user, "ftp") != 0 && strcmp(_ftp_user, "anonymous") != 0)
	_ftp_anon = 0;
    else
	_ftp_anon = 1;

    status.host = mkhoststr(0, 0);
    
    ftp_put("user %s", user);
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
ftp_site(char *cmd)
{
    ftp_put("%s", cmd);
    return ftp_resp();
}



int
ftp_reconnect(void)
{
    char *pass;

    pass = _ftp_pass;

    if (_ftp_host == NULL || _ftp_port == NULL || _ftp_user == NULL)
	return -1;

    if (conin) {
	fclose(conin);
	conin = NULL;
    }
    if (conout) {
	fclose(conout);
	conout = NULL;
    }

    disp_status("connecting. . .");

    if (ftp_open(NULL, NULL) == -1) {
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
ftp_close(void)
{
    int err = 0;
    
    if (conin == NULL)
	return 0;
    
    ftp_put("quit");
    if (ftp_resp() != 221)
	err = 1;
    
    fclose(conin);
    fclose(conout);
    conin = conout = NULL;
    
    return err;
}



directory *
ftp_list(char *path)
{
    directory *dir;
    int fd;
    FILE *f;
    
    if (ftp_mode('a') == -1 || ftp_cwd(path) == -1)
	return NULL;
    
    free(status.remote.path);
    status.remote.path = strdup(path);
    status_do(bs_remote);
    
    if ((fd=ftp_port()) == -1)
	return NULL;
    
    ftp_put("list");
    if (ftp_resp() != 150) {
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
	


FILE *
ftp_retr(char *file, int mode, long *startp, long *sizep)
{
    int fd;
    char *dir, *name, *can, *p;
    FILE *fin;
    
    can = canonical(file, NULL);
    dir = dirname(can);
    name = basename(can);
    
    if (ftp_mode(mode) == -1 || ftp_cwd(dir) == -1)
	return NULL;
    
    if ((fd=ftp_port()) == -1)
	return NULL;
    
    if (startp && *startp > 0) {
	ftp_put("rest %ld", *startp);
	if (ftp_resp() != 350)
	    *startp = 0;
    }
    
    ftp_put("retr %s", name);
    if (ftp_resp() != 150) {
	close(fd);
	return NULL;
    }
    if (sizep != NULL) {
	/* XXX: check how other servers format 150s */
	if (strcmp(ftp_last_resp+strlen(ftp_last_resp)-8, " bytes).") == 0) {
	    if ((p=strrchr(ftp_last_resp, '(')) != NULL)
		*sizep = strtol(p+1, NULL, 10);
	}
    }
    if ((fin=ftp_accept(fd, "r")) == NULL) {
	close(fd);
	return NULL;
    }
    
    return fin;
}



FILE *
ftp_stor(char *file, int mode)
{
    int fd, resp;
    char *dir, *name, *can;
    FILE *fin;
    
    can = canonical(file, NULL);
    dir = dirname(can);
    name = basename(can);
    
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
ftp_fclose(FILE *f)
{
    int err;

    err = fclose(f);

    if ((ftp_resp() != 226))
	return -1;
	
    return err;
}



int
ftp_mkdir(char *path)
{
    ftp_put("mkd %s", path);
    if (ftp_resp() != 257)
	return -1;
    
    return 0;
}



int
ftp_noop(void)
{
    ftp_put("noop");
    if (ftp_resp() != 200)
	return -1;
    
    return 0;
}



char *
ftp_pwd(void)
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
    int l;
    
    if (fgets(buf, 8192, f) == NULL)
	return NULL;
    
    line = strdup(buf);
    l = strlen(line);
    
    while (line[l-1] != '\n') {
	if (fgets(buf, 8192, f) == NULL) {
	    free(line);
	    return NULL;
	}
	l += strlen(buf);
	if ((line=realloc(line, l+1)) == NULL) {
	    disp_status("malloc failure");
	    return NULL;
	}
	strcpy(line+l, buf);
    }
    if (line[l-2] == '\r')
	line[l-2] = '\0';
    else
	line[l-1] = '\0';
    
    return line;
}



int
ftp_put(char *fmt, ...)
{
    char buf[8192];
    va_list argp;
    
    if (conout == NULL) {
	disp_status("not connected");
	return -1;
    }
    
    va_start(argp, fmt);
    vsprintf(buf, fmt, argp);
    va_end(argp);
    
    if (strncmp(buf, "pass ", 5) == 0 && _ftp_anon == 0) {
	disp_status("-> pass ********");
	ftp_hist(strdup("-> pass ********"));
    }
    else {
	disp_status("-> %s", buf);
	ftp_histf("-> %s", buf);
    }
    fprintf(conout, "%s\r\n", buf);
    
    if (fflush(conout) || ferror(conout)) {
	disp_status("error writing to server: %s", strerror(errno));
	return -1;
    }
    
    return 0;
}	



int
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
    disp_status("-> <attention>");
    ftp_hist(strdup("-> <attention>"));
	     
    if (send(fileno(conout), "\377\364\377" /* IAC IP IAC */, 3,
	     MSG_OOB) != 3)
	return -1;

    if (fputc('\362' /* DM */, conout) == EOF)
	return -1;

    ftp_put("abor");

    /* read remaining bytes from data connection */
    while (fread(buf, 4096, 1, fin) > 0)
	;
	
    /* hanlde server response */
    resp = ftp_resp();

    if (resp == 226) {
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
	disp_status("426 Transfer aborted.");
	    
	return resp == 226;
    }
    else
	return 1;
}




int
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
	disp_status("not connectecd");
	return -1;
    }

    clearerr(conin);
    if ((line=ftp_gets(conin)) == NULL) {
	if (ferror(conin))
	    disp_status("read error from server: %s", strerror(errno));
	else
	    disp_status("connection to server lost");
	return -1;
    }

    resp = atoi(line);
    disp_status("%s", line);
    free(ftp_last_resp);
    ftp_last_resp = strdup(line);

    while (!(isdigit(line[0]) && isdigit(line[1]) &&
	     isdigit(line[2]) && line[3] == ' ')) {
	ftp_hist(line);
	
	if ((line=ftp_gets(conin)) == NULL) {
	    disp_status("read error from server: %s", strerror(errno));
	    return -1;
	}
    }

    ftp_hist(line);
    
    return resp;
}



void ftp_unresp(int resp)
{
    _ftp_keptresp = resp;
}



int
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



FILE *
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



int
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
ftp_cwd(char *path)
{
    char *s, *e;
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
	    && (e=strchr(s+1, '"')) != NULL) {
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
ftp_cat(FILE *fin, FILE *fout, long start, long size)
{
    char buf[4096], fmt[4096];
    int c, n, err;
    long got;
#ifdef ENABLE_TRANSFER_RATE
    struct itimerval itv;
    long cur[4];
    int old_alarm;
#else
    time_t oldt, newt;
#endif

    if (size >= 0) {
#ifdef ENABLE_TRANSFER_RATE
	sprintf(fmt, "transferred %%ld/%ld "
		"(total: %%.2fkb/s, current: %%.2fkb/s)",
		size);
#else
	sprintf(fmt, "transferred %%ld/%ld", size);
#endif
    }
    else {
#ifdef ENABLE_TRANSFER_RATE
	strcpy(fmt, "transferred %ld (total: %%.2fkb/s, current: %%.2fkb/s)");
#else
	strcpy(fmt, "transferred %ld");
#endif
    }

    got = start;
    signal(SIGINT, sig_remember);

#ifdef ENABLE_TRANSFER_RATE
    cur[0] = cur[1] = cur[2] = cur[3] = 0;
    old_alarm = sig_alarm = sig_intr = 0;
    itv.it_value.tv_sec = itv.it_interval.tv_sec = 1;
    itv.it_value.tv_usec = itv.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &itv, NULL);
#if 0
    if ((flags=fcntl(fileno(fin), F_GETFL, 0)) != -1) {
	flags |= O_NONBLOCK;
	fcntl(fileno(fin), F_SETFL, flags);
    }
#endif
#else
    oldt = 0;
#endif

    if (ftp_curmode == 'i')
	while ((n=fread(buf, 1, 4096, fin)) > 0) {
	    if (sig_intr)
		break;
	    if (fwrite(buf, 1, n, fout) != n) {
		break;
	    }
	    got += n;
#ifdef ENABLE_TRANSFER_RATE
	    if (old_alarm != sig_alarm) {
		_ftp_update_transfer(fmt, got-start, cur,
				     old_alarm, sig_alarm);
		old_alarm = sig_alarm;
	    }
#else
	    if ((newt=time(NULL)) != oldt) {
		disp_status(fmt, got);
		oldt = newt;
	    }
#endif
	}
    else
	while ((c=getc(fin)) != EOF) {
	    if (sig_intr)
		break;
	    got++;
	    if (c == '\r') {
		if ((c=getc(fin)) != '\n')
		    putc('\r', fout);
		ungetc(c, fin);
	    }
	    else {
		putc(c, fout);
	    }
	    if (ferror(fout))
		break;

#ifdef ENABLE_TRANSFER_RATE
	    if (old_alarm != sig_alarm) {
		_ftp_update_transfer(fmt, got-start, cur,
				     old_alarm, sig_alarm);
		old_alarm = sig_alarm;
	    }
#else
	    if (got%512 == 0 && (newt=time(NULL)) != oldt) {
		disp_status(fmt, got);
		oldt = newt;
	    } 
#endif
	}

    signal(SIGINT, sig_end);
#ifdef ENABLE_TRANSFER_RATE
    itv.it_value.tv_sec = itv.it_interval.tv_sec = 0;
    itv.it_value.tv_usec = itv.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &itv, NULL);
#endif

    if (ferror(fin) || sig_intr) {
	errno = 0;
	sig_intr = 0;
	ftp_abort(fin);
	return -1;
    }
    else if (ferror(fout)) {
	err = errno;
	errno = 0;
	ftp_abort(fin);
	disp_status("write error: %s", strerror(err));
	return -1;
    }

    return 0;
}



#ifdef ENABLE_TRANSFER_RATE
static void
_ftp_update_transfer(char *fmt, long got, long *cur, int old_sec, int new_sec)
{
    long step;
    int nsec;

    nsec = new_sec-old_sec;

    switch (nsec) {
    case 0:
	break;

    case 1:
	cur[0] = cur[1];
	cur[1] = cur[2];
	cur[2] = cur[3];
	break;

    case 2:
	cur[0] = cur[2];
	cur[1] = cur[3];
	cur[2] = cur[1] + (got-cur[1])/2;
	break;

    default:
	step = (got-cur[3]) / nsec;
	cur[0] = cur[3]+step*(nsec-3);
	cur[1] = cur[3]+step*(nsec-2);
	cur[2] = cur[3]+step*(nsec-1);
	break;
    }
    cur[3] = got;

    disp_status(fmt, got, got/(float)(new_sec*1024),
		(cur[3]-cur[0])/((float)3*1024));
}
#endif /* ENABLE_TRANSFER_RATE */



int
ftp_gethostaddr(int fd)
{
    int len;
	
    len = sizeof(_ftp_addr);
    if (getsockname(fd, ftp_addr, &len) == -1) {
	if (disp_active)
	    disp_status("can't get host address: %s",
			strerror(errno));
	else
	    fprintf(stderr, "%s: can't get host addres: %s\n",
		    prg, strerror(errno));
	
	return -1;
    }
    
    return 0;
}



void
ftp_histf(char *fmt, ...)
{
    char buf[1024];
    
    va_list argp;

    if (ftp_hist_size == 0)
	return;

    va_start(argp, fmt);
    vsprintf(buf, fmt, argp);
    va_end(argp);

    ftp_hist(strdup(buf));
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
