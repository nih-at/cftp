@ ftp routines

@(ftp.h@)
#include <stdio.h>
@<prototypes@>

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

@<local prototypes@>
@<local globals@>


@ keeping state.

@d<local globals@>
char *ftp_head, *ftp_lcwd, *ftp_pcwd = NULL;
char ftp_curmode = ' ', ftp_anon = 0;


@ opening a connection and logging in.

@d<local globals@>
FILE *conin=NULL, *conout=NULL;
unsigned char ftp_addr[4];

@d<prototypes@>
int ftp_open(char *host, char *user, char *pass);

@u
int
ftp_open(char *host, char *user, char *pass)
{
	int fd, resp;

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
	f = ftp_accept(fd, "r");
	
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
	
	if (wd[0] == '/' || wd[0] == '~') {
		nwd = strdup(wd);
		for (p=nwd+strlen(nwd)-1; p>nwd && *p=='/'; --p)
			*p='\0';
	}
	else {
		nwd = (char *)malloc(strlen(ftp_lcwd)+strlen(wd)+2);
		strcpy(nwd, ftp_lcwd);
		w = strdup(wd);
		for (p=w+strlen(w); *p=='/'; --p)
			*p='\0';

		for (d=strtok(w, "/"); d; d=strtok(NULL, "/")) {
			if (strcmp(d, "..") == 0) {
				p = strrchr(nwd, '/');
				if (p != NULL) {
					if (p != nwd)
						*p = '\0';
					else
						strcpy(nwd, "/");
				}
			}
			else if (strcmp(d, ".") != 0) {
				if (nwd[strlen(nwd)-1] != '/')
					strcat(nwd, "/");
				strcat(nwd, d);
			}
		}
		
		free(w);
	}

	dir = get_dir(nwd);
	if (dir != NULL) {
		free(ftp_lcwd);
		ftp_lcwd = nwd;
	}
	else
		free(nwd);

	disp_head("%s: %s", ftp_head, ftp_lcwd);
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
	FILE *fin;
	
	if (ftp_mode(mode) == -1 || ftp_cwd(ftp_lcwd) == -1)
		return -1;

	if ((fd=ftp_port()) == -1)
		return -1;
	
	ftp_put("retr %s", file);
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

	if (strncmp(buf, "pass ", 5) == 0 && ftp_anon == 0)
		disp_status("-> pass ********");
	else
		disp_status("-> %s", buf);
	fprintf(conout, "%s\r\n", buf);
	fflush(conout);
}	


@d<local prototypes@>
int ftp_resp(void);

@u
int
ftp_resp(void)
{
	char *line;
	int resp;

	if ((line=ftp_gets(conin)) == NULL)
		return -1;

	resp = atoi(line);
	disp_status("%s", line);

	while (!(isdigit(line[0]) && isdigit(line[1]) &&
	       isdigit(line[2]) && line[3] == ' ')) {
		free(line);
		if ((line=ftp_gets(conin)) == NULL)
			return -1;
	}
	free(line);

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
	char buf[4096], fmt[4096], *l;
	int n, err = 0;
	long got = 0;

	if (size)
		sprintf(fmt, "transferred %%ld/%ld", size);
	else
		strcpy(fmt, "transferred %ld");
	
	if (ftp_curmode == 'i')
		while ((n=fread(buf, 1, 4096, fin)) > 0) {
			if (fwrite(buf, 1, n, fout) != n)
				err = 1;
			got += n;
			disp_status(fmt, got);
		}
	else
		while ((l=ftp_gets(fin)) != NULL) {
			n = strlen(l)+1;
			if (fprintf(fout, "%s\n", l) != n)
				err = 1;
			got += n;
			disp_status(fmt, got);
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
