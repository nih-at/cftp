/*
  ftp -- ftp protocol functions
  Copyright (C) 1996, 1997 Dieter Baron

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



#include "config.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
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
char ftp_anon;
char *ftp_last_resp;
char *ftp_host, *ftp_prt, *ftp_user, *ftp_pass;
				/* host, port, user, passwd (for reconnect) */

struct ftp_hist *ftp_history;	/* command/response history */
struct ftp_hist *ftp_hist_last; /* tail of history */
int ftp_hist_cursize;		/* size of history (for dynamic growth) */

static int _ftp_keptresp;
static int ftp_dosnames;	/* server sends directory names in
				   DOS format */

char **ftp_response;
long ftp_response_size;

FILE *conin, *conout;		/* control connection to server */
unsigned char ftp_addr[4];	/* local ip address (for port commands) */



int ftp_put(char *fmt, ...);
int ftp_abort(FILE *fin);
int ftp_resp(void);
void ftp_unresp(int resp);
int ftp_port(void);
FILE *ftp_accept(int fd, char *mode);
int ftp_mode(char m);
int ftp_cwd(char *path);
int ftp_cat(FILE *fin, FILE *fout, long size);
int ftp_gethostaddr(int fd);
void ftp_histf(char *fmt, ...);
void ftp_hist(char *line);



void
ftp_init(void)
{
    ftp_pcwd = NULL;
    ftp_curmode = ' ';
    ftp_anon = 0;
    ftp_last_resp = NULL;
    ftp_host = ftp_prt = ftp_pass = NULL;
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

	if ((fd=sopen(host, port)) == -1)
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

	return 0;
}



int
ftp_login(char *host, char *user, char *pass)
{
    int resp;

    if (ftp_resp() != 220)
	return -1;

    if (strcmp(user, "ftp") != 0 && strcmp(user, "anonymous") != 0) {
	status.host = (char *)malloc(strlen(user)+strlen(host)+2);
	sprintf(status.host, "%s@%s", user, host);
	ftp_anon = 0;
    }
    else {
	status.host = strdup(host);
	ftp_anon = 1;
    }

    ftp_put("user %s", user);
    resp = ftp_resp();
	
    if (resp == 331) {
	ftp_put("pass %s", pass);
	resp = ftp_resp();
    }

    if (resp != 230)
	return -1;

    free(status.remote.path);
    status.remote.path = NULL;
    status_do(bs_none);
	
    return 0;
}



int
ftp_reconnect(void)
{
    char *pass;

    pass = ftp_pass;

    if (ftp_host == NULL || ftp_prt == NULL || ftp_user == NULL)
	return -1;

    if (conin) {
	fclose(conin);
	conin = NULL;
    }
    if (conout) {
	fclose(conout);
	conout = NULL;
    }

    if (pass == NULL) {
	char *b;
	
	b = (char *)malloc(strlen(ftp_user)+strlen(ftp_host)+16);
	sprintf(b, "Password (%s@%s): ", ftp_user, ftp_host);
	pass = read_string(b, 0);
	free(b);
    }

    disp_status("connecting. . .");

    if (ftp_open(ftp_host, ftp_prt) == -1) {
	/* Error printed in ftp_open or sopen. */
	return -1;
    }

    if (ftp_login(ftp_host, ftp_user, pass) == -1) {
	/* ftp response is error message */
	return -1;
    }

    status.remote.path = strdup(ftp_lcwd);
    status_do(bs_remote);
    ftp_pcwd = NULL;

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
		dir->path = path;
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
ftp_retr(char *file, int mode)
{
	int fd;
	char *dir, *name, *can;
	FILE *fin;

	can = canonical(file, NULL);
	dir = dirname(can);
	name = (char *)basename(can);
	
	if (ftp_mode(mode) == -1 || ftp_cwd(dir) == -1)
		return NULL;

	if ((fd=ftp_port()) == -1)
		return NULL;
	
	ftp_put("retr %s", name);
	if (ftp_resp() != 150) {
		close(fd);
		return NULL;
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
	int fd;
	char *dir, *name, *can;
	FILE *fin;

	can = canonical(file, NULL);
	dir = dirname(can);
	name = (char *)basename(can);
	
	if (ftp_mode(mode) == -1 || ftp_cwd(dir) == -1)
		return NULL;

	if ((fd=ftp_port()) == -1)
		return NULL;
	
	ftp_put("stor %s", name);
	if (ftp_resp() != 150) {
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
	if (isalpha(dir[0]) && dir[1] == ':' && dir[2] == '\\')
	    ftp_dosnames = 1;
	else
	    ftp_dosnames = 0;
    }

    if (ftp_dosnames == 1) {
	    strncpy(dir+1, s+1, e-s-1);
	    dir[e-s] = '\0';
	    dir[0] = '/';
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

	if (strncmp(buf, "pass ", 5) == 0 && ftp_anon == 0) {
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
	unsigned long host;
	int fd, port;

	if ((fd=spassive(&host, &port)) == -1)
		return -1;

	ftp_put("port %d,%d,%d,%d,%d,%d", (int)ftp_addr[0],
		(int)ftp_addr[1], (int)ftp_addr[2], (int)ftp_addr[3], 
		port>>8, port&0xff);
	if (ftp_resp() != 200) {
		close(fd);
		return -1;
	}

	return fd;
}



FILE *
ftp_accept(int fd, char *mode)
{
	int len, ns;
	struct sockaddr addr;

	len = sizeof(addr);
	if ((ns=accept(fd, &addr, &len)) == -1)
		return NULL;

	close(fd);
	
	return fdopen(ns, mode);
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
    
    if (ftp_pcwd && strcmp(path, ftp_pcwd) == 0)
	return 0;

    if (ftp_dosnames == 1) {
	for (s=path+1; *s; s++)
	    if (*s == '/')
		*s = '\\';
	ftp_put("cwd %s", path+1);
	for (s=path+1; *s; s++)
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
ftp_cat(FILE *fin, FILE *fout, long size)
{
    time_t oldt, newt;
    char buf[4096], fmt[4096];
    int c, n, err;
    long got = 0;

    if (size >= 0)
	sprintf(fmt, "transferred %%ld/%ld", size);
    else
	strcpy(fmt, "transferred %ld");
	
    oldt = 0;

    signal(SIGINT, sig_remember);

    if (ftp_curmode == 'i')
	while ((n=fread(buf, 1, 4096, fin)) > 0) {
	    if (sig_intr)
		break;
	    if (fwrite(buf, 1, n, fout) != n) {
		break;
	    }
	    got += n;
	    if ((newt=time(NULL)) != oldt) {
		disp_status(fmt, got);
		oldt = newt;
	    }
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

	    if (got%512 == 0 && (newt=time(NULL)) != oldt) {
		disp_status(fmt, got);
		oldt = newt;
	    }

	}

    signal(SIGINT, sig_end);

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



int
ftp_gethostaddr(int fd)
{
	struct sockaddr_in addr;
	int len;
	
	len = sizeof(addr);
	if (getsockname(fd, (struct sockaddr* )&addr, &len) == -1) {
	    if (disp_active)
		disp_status("can't get host address: %s",
			    strerror(errno));
	    else
		fprintf(stderr, "%s: can't get host addres: %s\n",
			prg, strerror(errno));
	    
	    return -1;
	}

	memcpy(ftp_addr, (char *)&addr.sin_addr.s_addr, 4);

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
