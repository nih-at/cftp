@ ftp routines

@(ftp.h@)
#include <stdio.h>
@<prototypes@>

@<globals@>


@u
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "directory.h"
#include "display.h"
#include "sockets.h"
#include "ftp.h"
#include "readdir.h"
#include "util.h"

@<local prototypes@>
@<local globals@>


@ keeping state.

@d<globals@>
struct ftp_hist {
    char *line;
    struct ftp_hist *next;
};

extern struct ftp_hist *ftp_history;

extern char ftp_anon;

@d<local globals@>
char *ftp_head, *ftp_lcwd, *ftp_pcwd = NULL;
char ftp_curmode = ' ', ftp_anon = 0;
char *ftp_last_resp = NULL;

struct ftp_hist *ftp_history = NULL, *ftp_hist_last;
int ftp_hist_size = 200, ftp_hist_cursize;


@ last response.

@d<globals@>
extern char **ftp_response;

@d<local globals@>
char **ftp_response = NULL;
long ftp_response_size = 0;


@ opening a connection, and logging in.

@d<local globals@>
FILE *conin=NULL, *conout=NULL;
unsigned char ftp_addr[4];

@d<prototypes@>
int ftp_open(char *host);
int ftp_login(char *host, char *user, char *pass);

@u
int
ftp_open(char *host)
{
	int fd;

	if ((fd=sopen(host, "ftp")) == -1)
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
		ftp_lcwd = strdup("~/");
		ftp_head = (char *)malloc(strlen(user)+strlen(host)+2);
		sprintf(ftp_head, "%s@@%s", user, host);
		ftp_anon = 0;
	}
	else {
		ftp_lcwd = strdup("/");
		ftp_head = strdup(host);
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

	disp_head("%s", ftp_head);
	
	return 0;
}


@ closing the connection.

@d<prototypes@>
int ftp_close(void);

@u
int
ftp_close(void)
{
	int err = 0;
	
	ftp_put("quit");
	if (ftp_resp() != 221)
		err = 1;

	fclose(conin);
	fclose(conout);
	conin = conout = NULL;

	return err;
}


@ getting current dir listing.

@d<prototypes@>
directory *ftp_list(char *path);

@u
directory *
ftp_list(char *path)
{
	directory *dir;
	int fd;
	FILE *f;

	if (ftp_mode('a') == -1 || ftp_cwd(path) == -1)
		return NULL;

	disp_head("%s: %s", ftp_head, path);

	if ((fd=ftp_port()) == -1)
		return NULL;
	
	ftp_put("list");
	if (ftp_resp() != 150) {
		close(fd);
		dir = (directory *)malloc(sizeof(directory));
		dir->list = (direntry *)malloc(sizeof(direntry));
		dir->path = path;
		dir->num = 1;
		dir->list->line = strdup("");
		dir->list->type = 'x';
		dir->list->name = dir->list->link = NULL;
		return dir;
	}
	if ((f=ftp_accept(fd, "r")) == NULL)
	    return NULL;
	
	dir = read_dir(f);

	fclose(f);

	ftp_resp();

	return dir;
}


@ changing directory.

@d<prototypes@>
directory *ftp_cd(char *wd);

@u
directory *
ftp_cd(char *wd)
{
	directory *dir;
	char *nwd, *d, *w, *p;
	
	nwd = canonical(wd, NULL);

	dir = get_dir(nwd);
	if (dir != NULL) {
		free(ftp_lcwd);
		ftp_lcwd = nwd;
		
		disp_head("%s: %s", ftp_head, ftp_lcwd);
	}
	else
		free(nwd);

	return dir;
}
	

@ downloading a file.

@d<prototypes@>
int ftp_retr(char *file, FILE *fout, long size, int mode);

@u
int
ftp_retr(char *file, FILE *fout, long size, int mode)
{
	int fd, err;
	char *dir, *name, *can;
	FILE *fin;

	can = canonical(file, NULL);
	dir = dirname(can);
	name = basename(can);
	
	if (ftp_mode(mode) == -1 || ftp_cwd(dir) == -1)
		return -1;

	if ((fd=ftp_port()) == -1)
		return -1;
	
	ftp_put("retr %s", name);
	if (ftp_resp() != 150) {
		close(fd);
		return -1;
	}
	if ((fin=ftp_accept(fd, "r")) == NULL) {
		close(fd);
		return -1;
	}

	err = ftp_cat(fin, fout, size);

	fclose(fin);

	if ((ftp_resp() != 226))
		return -1;
	
	return err;
}


@ noop

@d<prototypes@>
int ftp_noop(void);

@u
int
ftp_noop(void)
{
    ftp_put("noop");
    if (ftp_resp() != 200)
	return -1;
    
    return 0;
}


@ pwd

@d<prototypes@>
char *ftp_pwd(void);

@u
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
    
    if ((dir=(char *)malloc(e-s)) == NULL)
	return NULL;

    strncpy(dir, s+1, e-s-1);
    dir[e-s-1] = '\0';

    return dir;
}

@ low level interface.

@d<prototypes@>
char *ftp_gets(FILE *f);

@u
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
		line = realloc(line, l+1);
		strcpy(line+l, buf);
	}
	if (line[l-2] == '\r')
		line[l-2] = '\0';
	else
		line[l-1] = '\0';

	return line;
}


@d<local prototypes@>
int ftp_put(char *fmt, ...);

@u
int
ftp_put(char *fmt, ...)
{
	char buf[8192];
	va_list argp;

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
	fflush(conout);
}	


@d<local prototypes@>
int ftp_resp(void);

@u
int
ftp_resp(void)
{
    char *line, **l;
    int resp;
    long i;
    
    if ((line=ftp_gets(conin)) == NULL)
	return -1;

    resp = atoi(line);
    disp_status("%s", line);
    free(ftp_last_resp);
    ftp_last_resp = strdup(line);

    while (!(isdigit(line[0]) && isdigit(line[1]) &&
	     isdigit(line[2]) && line[3] == ' ')) {
	ftp_hist(line);
	
	if ((line=ftp_gets(conin)) == NULL)
	    return -1;
    }

    ftp_hist(line);
    
    return resp;
}


@ opening data connection.

@d<local prototypes@>
int ftp_port(void);

@u
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


@d<local prototypes@>
FILE *ftp_accept(int fd, char *mode);

@u
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


@ setting transfer mode.

@d<local prototypes@>
int ftp_mode(char m);

@u
int
ftp_mode(char m)
{
	if (m == ftp_curmode)
		return 0;

	ftp_put("type %c", m);
	if (ftp_resp() != 200)
		return -1;

	ftp_curmode = m;
	return 0;
}


@ changing directory at the server.

@d<local prototypes@>
int ftp_cwd(char *path);

@u
int
ftp_cwd(char *path)
{
	if (ftp_pcwd && strcmp(path, ftp_pcwd) == 0)
		return 0;

	ftp_put("cwd %s", path);
	if (ftp_resp() != 250)
		return -1;

	free(ftp_pcwd);
	ftp_pcwd = strdup(path);

	return 0;
}


@d<local prototypes@>
int ftp_cat(FILE *fin, FILE *fout, long size);

@u
int
ftp_cat(FILE *fin, FILE *fout, long size)
{
    time_t oldt, newt;
	char buf[4096], fmt[4096], *l;
	int n, err = 0;
	long got = 0;

	if (size >= 0)
		sprintf(fmt, "transferred %%ld/%ld", size);
	else
		strcpy(fmt, "transferred %ld");
	
	if (ftp_curmode == 'i')
		while ((n=fread(buf, 1, 4096, fin)) > 0) {
			if (fwrite(buf, 1, n, fout) != n)
				err = 1;
			got += n;
			if ((newt=time(NULL)) != oldt) {
			    disp_status(fmt, got);
			    oldt = newt;
			}
		}
	else
		while ((l=ftp_gets(fin)) != NULL) {
			n = strlen(l)+1;
			if (fprintf(fout, "%s\n", l) != n)
				err = 1;
			got += n;
			if ((newt=time(NULL)) != oldt) {
			    disp_status(fmt, got);
			    oldt = newt;
			}
		}

	if (err || ferror(fin) || ferror(fout))
		return -1;

	return 0;
}


@ getting local host address.

@d<local prototypes@>
int ftp_gethostaddr(int fd);

@u
int
ftp_gethostaddr(int fd)
{
	struct sockaddr_in addr;
	int len;
	
	len = sizeof(addr);
	if (getsockname(fd, (struct sockaddr* )&addr, &len) == -1)
		return -1;

	memcpy(ftp_addr, (char *)&addr.sin_addr.s_addr, 4);
}


@ ftp exchange history

@d<local prototypes@>
void ftp_histf(char *fmt, ...);
void ftp_hist(char *line);

@u
void
ftp_histf(char *fmt, ...)
{
    char buf[1024];
    
    va_list argp;

    va_start(argp, fmt);
    vsprintf(buf, fmt, argp);
    va_end(argp);

    ftp_hist(strdup(buf));
}


void ftp_hist(char *line)
{
    struct ftp_hist *p;

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
