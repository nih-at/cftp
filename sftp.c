/*
  $NiH: sftp.c,v 1.11 2001/12/17 05:46:28 dillo Exp $

  sftp.c -- sftp protocol functions
  Copyright (C) 2001 Dieter Baron

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
#ifdef USE_SFTP

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "directory.h"
#include "display.h"
#include "ftp.h"
#include "methods.h"
#include "status.h"
#include "util.h"



#define SFTP_MAX_PACKET_SIZE	32*1024
#define SFTP_HEADER_LEN		13	/* no of bytes before data in a
					   SSH_FXP_DATA packet */
#define SFTP_DATA_LEN		4096	/* data length used for read/write */

#define SFTP_FL_LOG		0x1
#define SFTP_FL_IS_PATH		0x2

struct handle {
    int len;
    char *str;
};

enum sftp_file_state {
    SFTP_FS_EOF, SFTP_FS_ERROR, SFTP_FS_SEND, SFTP_FS_RECEIVE
};

#define SFTP_FFL_READ	0x1
#define SFTP_FFL_WRITE	0x2

struct sftp_file {
    int flags;
    struct handle *hnd;	/* sftp handle */
    size_t off;		/* current offset within file */
    enum sftp_file_state state;
    			/* current state */
    int pend;		/* packet length */
    int poff;		/* offset within current packet */
    int doff;		/* offset to unreturned data */
    int dend;		/* end of data within current packet */
    int woff;		/* offset to length in write packet */
};



int sftp_file_close(struct handle *hnd);
struct handle *sftp_file_open(char *file, int flags);
struct handle *sftp_get_handle(void);
int sftp_put_handle(int type, struct handle *hnd, int flags);
int sftp_put_str(int type, char *str, int len, int flags);
int sftp_readdir(struct handle *hnd, char *buf, int *lenp);
int sftp_send_init(int proto_version);
int sftp_send_read(struct sftp_file *f, int nbytes);
int sftp_status(void);

static int _sftp_fread(struct sftp_file *f);
static int _sftp_fwrite(struct sftp_file *f);
static int _sftp_get_packet(FILE *f, char *buf, int *lenp, int flags);
static char *_sftp_get_string(char *buf, char **endp);
static unsigned int _sftp_get_uint32(unsigned char *p);
static unsigned long long _sftp_get_uint64(unsigned char *p);
static void _sftp_log_handle(char *buf, char *pre, char *cmd,
			     unsigned char *data);
static void _sftp_log_packet(int dir, int type, char *data, int len);
static void _sftp_log_str(char *buf, char *pre, char *cmd,
			  unsigned char *data);
static int _sftp_parse_name(unsigned char **pp, direntry *e);
static int _sftp_parse_status(int type, char *buf, int len);
static void _sftp_put_handle(unsigned char *buf, unsigned char **endp,
			     struct handle *hnd);
static int _sftp_put_packet(FILE *f, int type, char *buf, int len,
			    int log_req);
static void _sftp_put_string(unsigned char *buf, unsigned char **endp,
			     char *str, int slen, int flags);
static void _sftp_put_uint32(unsigned char *p, unsigned int i);
static void _sftp_put_uint64(unsigned char *p, unsigned long long i);
static directory *_sftp_read_dir(struct handle *hnd);
static void _sftp_start_ssh(int fdin, int fdout);
int _sftp_start_write(struct sftp_file *f, int len);
static char *_sftp_strerror(int error);



#define SSH_FX_PROTO_VERSION	3

/* packet types */

#define SSH_FXP_INIT		1
#define SSH_FXP_VERSION		2
#define SSH_FXP_OPEN		3
#define SSH_FXP_CLOSE		4
#define SSH_FXP_READ		5
#define SSH_FXP_WRITE		6
#define SSH_FXP_LSTAT		7
#define SSH_FXP_FSTAT		8
#define SSH_FXP_SETSTAT		9
#define SSH_FXP_FSETSTAT	10
#define SSH_FXP_OPENDIR		11
#define SSH_FXP_READDIR		12
#define SSH_FXP_REMOVE		13
#define SSH_FXP_MKDIR		14
#define SSH_FXP_RMDIR		15
#define SSH_FXP_REALPATH	16
#define SSH_FXP_STAT		17
#define SSH_FXP_RENAME		18
#define SSH_FXP_STATUS		101
#define SSH_FXP_HANDLE		102
#define SSH_FXP_DATA		103
#define SSH_FXP_NAME		104
#define SSH_FXP_ATTRS		105
#define SSH_FXP_EXTENDED	200
#define SSH_FXP_EXTENDED_REPLY	201

/* attribute flag bits */

#define SSH_FILEXFER_ATTR_SIZE		0x00000001
#define SSH_FILEXFER_ATTR_UIDGID	0x00000002
#define SSH_FILEXFER_ATTR_PERMISSIONS	0x00000004
#define SSH_FILEXFER_ATTR_ACMODTIME	0x00000008
#define SSH_FILEXFER_ATTR_EXTENDED	0x80000000

/* open pflags bits */

#define SSH_FXF_READ		0x00000001
#define SSH_FXF_WRITE		0x00000002
#define SSH_FXF_APPEND		0x00000004
#define SSH_FXF_CREAT		0x00000008
#define SSH_FXF_TRUNC		0x00000010
#define SSH_FXF_EXCL		0x00000020

/* error/status code definitions */

#define SSH_FX_OK			0
#define SSH_FX_EOF			1
#define SSH_FX_NO_SUCH_FILE		2
#define SSH_FX_PERMISSION_DENIED	3
#define SSH_FX_FAILURE			4
#define SSH_FX_BAD_MESSAGE		5
#define SSH_FX_NO_CONNECTION		6
#define SSH_FX_CONNECTION_LOST		7
#define SSH_FX_OP_UNSUPPORTED		8



static unsigned char _sftp_buffer[SFTP_MAX_PACKET_SIZE];

static char *_sftp_errlist[] = {
    "OK",
    "End of file",
    "No such file or directory",
    "Permission denied",
    "Failure",
    "Bad message",
    "No connection",
    "Connection lost",
    "Operation unsupported"
};

static int _sftp_nerr = sizeof(_sftp_errlist) / sizeof(_sftp_errlist[0]);

static int _sftp_nextid;
static pid_t _sftp_ssh_pid;

extern char *prg;

/* XXX: shared with ftp.c */

extern FILE *conin, *conout;

#ifdef DEBUG_XFER
static FILE *flog;
#endif



int
sftp_mkdir(char *path)
{
    sftp_put_str(SSH_FXP_MKDIR, path, 0, SFTP_FL_LOG|SFTP_FL_IS_PATH);
    if (sftp_status() != SSH_FX_OK)
	return -1;
    return 0;
}



int
sftp_rmdir(char *path)
{
    sftp_put_str(SSH_FXP_RMDIR, path, 0, SFTP_FL_LOG|SFTP_FL_IS_PATH);
    if (sftp_status() != SSH_FX_OK)
	return -1;
    return 0;
}



directory *
sftp_list(char *path)
{
    directory *dir;
    struct handle *hnd;

    sftp_put_str(SSH_FXP_OPENDIR, path, 0, SFTP_FL_LOG|SFTP_FL_IS_PATH);

    if ((hnd=sftp_get_handle()) == NULL)
	return NULL;

    dir = _sftp_read_dir(hnd);
    if (dir)
	dir->path = strdup(path);
    
    sftp_file_close(hnd);
    
    return dir;
}



static directory *
_sftp_read_dir(struct handle *hnd)
{
    directory *dir;
    direntry entry;
    unsigned char *p;
    time_t oldt, newt;
    int n, pn;
    int type, len;

    dir = dir_new();
    oldt = 0;

    n = 0;
    while ((type=sftp_readdir(hnd, _sftp_buffer, &len))
	   == SSH_FXP_NAME) {
	pn = _sftp_get_uint32(_sftp_buffer+4);
	for (p = _sftp_buffer+8; pn--;) {
	    if (_sftp_parse_name(&p, &entry) == 0) {
		if (strcmp(entry.name, ".") == 0
		    || strcmp(entry.name, "..") == 0)
		    continue;
		dir_add(dir, &entry);
		n++;
	    }

	    if ((newt=time(NULL)) != oldt) {
		disp_status("listed %d", n);
		oldt = newt;
	    }
	}
    }

    if (_sftp_parse_status(type, _sftp_buffer, len) != SSH_FX_EOF) {
	_sftp_log_packet(1, type, _sftp_buffer, len);
    }

    return dir;
}    



static char *
_sftp_strerror(int error)
{
    static char buf[128];

    if (error < 0 || error >= _sftp_nerr) {
	sprintf(buf, "Unknown error %d", error);
	return buf;
    }

    return _sftp_errlist[error];
}



static int
_sftp_get_packet(FILE *f, char *buf, int *lenp, int log_resp)
{
    int c, i, len, type;

    if (f == NULL)
	return -1;

    for (i=len=0; i<4; i++) {
	if ((c=fgetc(f)) == EOF)
	    return -1;
	len = (len<<8)|c;
    }

    /* XXX: check for buffer overflow */

    if ((type=fgetc(f)) == EOF)
	return -1;
    --len;

    if (fread(buf, 1, len, f) != len)
	return -1;

    if (lenp)
	*lenp = len;

    if (log_resp)
	_sftp_log_packet(1, type, buf, len);

    return type;
}



static void
_sftp_log_packet(int dir, int type, char *data, int len)
{
    static char *req[] = {
	"init", "version", "open", "close", "read", "write",
	"lstat", "fstat", "setstat", "fsetstat", "opendir",
	"readdir", "remove", "mkdir", "rmdir", "realpath", "stat",
	"rename"
    };
    static char *rsp[] = {
	"status", "handle", "data", "name", "attrs"
    };
    static char *dir_pre[] = { "->", "<=" };

    char buf[80], *pre, *cmd;

    pre = dir_pre[dir!=0];

    if (type >= SSH_FXP_INIT && type <= SSH_FXP_RENAME)
	cmd = req[type-SSH_FXP_INIT];
    else if (type >= SSH_FXP_STATUS && type <= SSH_FXP_ATTRS)
	cmd = rsp[type-SSH_FXP_STATUS];
    else
	cmd = NULL;

    switch (type) {
    case SSH_FXP_INIT:
    case SSH_FXP_VERSION:
	sprintf(buf, "%s %s %d", pre, cmd, _sftp_get_uint32(data));
	break;

    case SSH_FXP_OPEN:
	/* XXX: include flags & attrs */
    case SSH_FXP_RENAME:
	/* XXX: include new name */
    case SSH_FXP_SETSTAT:
	/* XXX: include attrs */
    case SSH_FXP_OPENDIR:
    case SSH_FXP_REALPATH:
    case SSH_FXP_REMOVE:
    case SSH_FXP_MKDIR:
    case SSH_FXP_RMDIR:
    case SSH_FXP_LSTAT:
	_sftp_log_str(buf, pre, cmd, data);
	break;

    case SSH_FXP_CLOSE:
    case SSH_FXP_READDIR:
    case SSH_FXP_HANDLE:
    case SSH_FXP_FSETSTAT:
	/* XXX: include attrs */
    case SSH_FXP_FSTAT:
	_sftp_log_handle(buf, pre, cmd, data);
	break;
	
    case SSH_FXP_READ:
    case SSH_FXP_WRITE:
	/* XXX: parse and log data */
    case SSH_FXP_ATTRS:
	sprintf(buf, "%s %s", pre, cmd);
	break;

    case SSH_FXP_STATUS:
	sprintf(buf, "%s %s %s", pre, cmd,
		_sftp_strerror(_sftp_get_uint32(data+4)));
	break;
	    
    case SSH_FXP_DATA:
	sprintf(buf, "%s %s %d", pre, cmd, len-4);
	break;

    case SSH_FXP_NAME:
	/* XXX: if count == 1, include name */
	sprintf(buf, "%s %s %d", pre, cmd, _sftp_get_uint32(data+4));
	break;

    default:
	sprintf(buf, "%s unknown packet type %d", pre, type);

    }
    
    disp_status("%s", buf);
    ftp_hist(strdup(buf));
}



static int
_sftp_parse_name(unsigned char **pp, direntry *e)
{
    unsigned int flags;
    int n, len;
    char *p;
    mode_t mode;

    p = *pp;

    e->name = _sftp_get_string(p, &p);

    len = _sftp_get_uint32(p);
    e->line = malloc(len+2);
    e->line[0] = ' ';
    strncpy(e->line+1, p+4, len);
    e->line[len+1] = '\0';
    p += len+4;
    
    e->link = NULL;

    flags = _sftp_get_uint32(p);
    p += 4;

    if (flags & SSH_FILEXFER_ATTR_SIZE) {
	e->size = _sftp_get_uint64(p);
	p += 8;
    }
    else
	e->size = -1;

    if (flags & SSH_FILEXFER_ATTR_UIDGID)
	p += 8;

    if (flags & SSH_FILEXFER_ATTR_PERMISSIONS) {
	mode = _sftp_get_uint32(p);
	p += 4;
	switch (mode & S_IFMT) {
	case S_IFDIR:
	    e->type = 'd';
	    break;
	case S_IFREG:
	case S_IFIFO:
	    e->type = 'f';
	    break;
	case S_IFLNK:
	    e->type = 'l';
	    break;
	default:
	    e->type = 'x';
	}
    }
    else
	e->type = 'x';

    if (flags & SSH_FILEXFER_ATTR_ACMODTIME) {
	p += 4; /* skip atime */
	e->mtime = _sftp_get_uint32(p);
	p += 4;
    }
    else
	e->mtime = 0;

    if (flags & SSH_FILEXFER_ATTR_EXTENDED) {
	n = _sftp_get_uint32(p);
	p += 4;

	while (n--) {
	    p += _sftp_get_uint32(p) + 4;
	    p += _sftp_get_uint32(p) + 4;
	}
    }

    *pp = p;

    return 0;
}



static unsigned int
_sftp_get_uint32(unsigned char *p)
{
    return (p[0]<<24) | (p[1]<<16) | (p[2]<<8) | p[3];
}



static unsigned long long
_sftp_get_uint64(unsigned char *p)
{
    return ((unsigned long long)p[0]<<56) | ((unsigned long long)p[1]<<48)
	| ((unsigned long long)p[2]<<40) | ((unsigned long long)p[3]<<32)
	| (p[4]<<24) | (p[5]<<16) | (p[6]<<8) | p[7];
}



static char *
_sftp_get_string(char *buf, char **endp)
{
    char *s;
    int len;

    len = _sftp_get_uint32(buf);
    if (!len)
	return NULL;

    if ((s=malloc(len+1)) == NULL)
	return NULL;

    memcpy(s, buf+4, len);
    s[len] = '\0';

    if (endp)
	*endp = buf+4+len;

    return s;
}



int
sftp_status(void)
{
    int type;
    int len;

    type = _sftp_get_packet(conin, _sftp_buffer, &len, 1);

    return _sftp_parse_status(type, _sftp_buffer, len);
}



static int
_sftp_parse_status(int type, char *buf, int len)
{
    if (type != SSH_FXP_STATUS || len < 8)
	return -1;

    return _sftp_get_uint32(buf+4);
}



struct handle *
sftp_get_handle(void)
{
    int len;
    struct handle *hnd;

    if (_sftp_get_packet(conin, _sftp_buffer, &len, 1) != SSH_FXP_HANDLE)
	return NULL;

    hnd = malloc(sizeof(*hnd));

    hnd->len = _sftp_get_uint32(_sftp_buffer+4);
    hnd->str = malloc(len);
    memcpy(hnd->str, _sftp_buffer+8, len);
    
    return hnd;
}



int
sftp_file_close(struct handle *hnd)
{
    sftp_put_handle(SSH_FXP_CLOSE, hnd, SFTP_FL_LOG);

    free(hnd->str);
    free(hnd);

    return sftp_status();
}



int
sftp_readdir(struct handle *hnd, char *buf, int *lenp)
{
    sftp_put_handle(SSH_FXP_READDIR, hnd, 0);

    return _sftp_get_packet(conin, buf, lenp, 0);
}



int
sftp_put_str(int type, char *str, int slen, int flags)
{
    unsigned char *p;

    p = _sftp_buffer;

    _sftp_put_uint32(p, _sftp_nextid++);
    p += 4;
    _sftp_put_string(p, &p, str, slen, flags);
    
    return _sftp_put_packet(conout, type, _sftp_buffer, p-_sftp_buffer, flags);
}



static void
_sftp_put_uint32(unsigned char *p, unsigned int i)
{
    p[0] = (i>>24) & 0xff;
    p[1] = (i>>16) & 0xff;
    p[2] = (i>>8) & 0xff;
    p[3] = i & 0xff;
}



static void
_sftp_put_uint64(unsigned char *p, unsigned long long i)
{
    p[0] = (i>>56) & 0xff;
    p[1] = (i>>48) & 0xff;
    p[2] = (i>>40) & 0xff;
    p[3] = (i>>32) & 0xff;
    p[4] = (i>>24) & 0xff;
    p[5] = (i>>16) & 0xff;
    p[6] = (i>>8) & 0xff;
    p[7] = i & 0xff;
}



static int
_sftp_put_packet(FILE *f, int type, char *buf, int len, int flags)
{
    char b[9];

    _sftp_put_uint32(b, len+1);
    b[4] = type;

    if (fwrite(b, 1, 5, f) != 5)
	return -1;
    
    if (fwrite(buf, 1, len, f) != len)
	return -1;

    fflush(f);

    if (flags & SFTP_FL_LOG)
	_sftp_log_packet(0, type, buf, len);

    return 0;
}



int
sftp_login(char *user, char *pass)
{
    /* XXX: handle user != NULL */

    status.host = mkhoststr(0, 0);
    
    return 0;
}



int
sftp_open(char *host, char *port, char *user, char *pass)
{
    int pin[2], pout[2];
    pid_t pid;

    ftp_remember_host(host, port);
    ftp_remember_user(user, pass);

    if (pipe(pin) != 0 || pipe(pout) != 0) {
	if (disp_active)
	    disp_status("cannot create pipes: %s", strerror(errno));
	else
	    fprintf(stderr, "%s: cannot create pipes: %s",
		    prg, strerror(errno));
	return -1;
    }

    switch ((pid=fork())) {
    case -1:
	if (disp_active)
	    disp_status("cannot fork: %s", strerror(errno));
	else
	    fprintf(stderr, "%s: cannot fork: %s", prg, strerror(errno));
	return -1;

    case 0:
	close(pin[0]);
	close(pout[1]);
	_sftp_start_ssh(pout[0], pin[1]);
	/* NOTREACHED */

    default:
	_sftp_ssh_pid = pid;
	
	close(pin[1]);
	close(pout[0]);

	conin = conout = NULL;
	if ((conin=fdopen(pin[0], "rb")) == NULL) {
	    if (disp_active)
		disp_status("cannot fdopen input pipe: %s", strerror(errno));
	    else
		fprintf(stderr, "cannot fdopen input pipe: %s",
			prg, strerror(errno));
	    close(pout[1]);
	    sftp_close();
	    return -1;
	}
	if ((conout=fdopen(pout[1], "wb")) == NULL) {
	    if (disp_active)
		disp_status("cannot fdopen output pipe: %s", strerror(errno));
	    else
		fprintf(stderr, "cannot fdopen output pipe: %s",
			prg, strerror(errno));
	    sftp_close();
	    return -1;
	}
	/* XXX: check for minmal server version */
	if (sftp_send_init(SSH_FX_PROTO_VERSION) < 0) {
	    sftp_close();
	    return -1;
	}
	return 0;
    }
}



static void
_sftp_start_ssh(int fdin, int fdout)
{
    char *args[20];
    int n;
    
    close(0);
    close(1);

    if (dup(fdin) == -1 || dup(fdout) == -1) {
	fprintf(stderr, "%s<child>: cannot dup: %s\n",
		prg, strerror(errno));
	exit(1);
    }
    close(fdin);
    close(fdout);

    n = 0;
    args[n++] = "ssh";
    args[n++] = "-2";
    args[n++] = "-s";
    if (ftp_prt()) {
	args[n++] = "-P";
	args[n++] = ftp_prt();
    }
    if (ftp_user()) {
	args[n++] = "-l";
	args[n++] = ftp_user();
    }
    args[n++] = ftp_host();
    args[n++] = "sftp";
    args[n++] = NULL;

    execvp("ssh", args);
    fprintf(stderr, "%s<child>: exec failed: %s\n",
	    prg, strerror(errno));
    exit(1);
}



int
sftp_close(void)
{
    int status;
    
    if (conin) {
	fclose(conin);
	conin = NULL;
    }
    if (conout) {
	fclose(conout);
	conout = NULL;
    }
    if (_sftp_ssh_pid != 0) {
	if (waitpid(_sftp_ssh_pid, &status, 0) == -1) {
	    if (disp_active)
		disp_status("cannot wait on ssh (pid %d): %s",
			    (int)_sftp_ssh_pid, strerror(errno));
	    else
		fprintf(stderr, "%s: cannot wait on ssh (pid %d): %s\n",
			prg, (int)_sftp_ssh_pid, strerror(errno));
	    return -1;
	}
	if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
	    if (disp_active)
		disp_status("ssh exited with %d", WEXITSTATUS(status));
	    else
		fprintf(stderr, "%s: ssh exited with %d\n",
			prg, WEXITSTATUS(status));
	    return -1;
	}
	else if (WIFSIGNALED(status)) {
	    if (disp_active)
		disp_status("ssh exited due to signal %d",
			    WTERMSIG(status));
	    else
		fprintf(stderr, "%s: ssh exited due to signal %d\n",
			prg, WTERMSIG(status));
	    return -1;
	}
    }

    return 0;
}



int
sftp_send_init(int proto_version)
{
    int len;
    
#ifdef DEBUG_XFER
    flog = fopen("cftp.log", "w");
    setvbuf(flog, NULL, _IOLBF, 0);
#endif

    _sftp_put_uint32(_sftp_buffer, proto_version);

    if (_sftp_put_packet(conout, SSH_FXP_INIT, _sftp_buffer, 4,
			 SFTP_FL_LOG) < 0)
	return -1;
    if (_sftp_get_packet(conin, _sftp_buffer, &len, 1) != SSH_FXP_VERSION)
	return -1;

    return _sftp_get_uint32(_sftp_buffer);
}



char *
sftp_pwd(void)
{
    int len;
    char *dir;
    
    if (sftp_put_str(SSH_FXP_REALPATH, ftp_lcwd ? ftp_lcwd : "", 0,
		     SFTP_FL_LOG) < 0)
	return NULL;
    if (_sftp_get_packet(conin, _sftp_buffer, &len, 1) != SSH_FXP_NAME)
	return NULL;

    dir = _sftp_get_string(_sftp_buffer+8, NULL);

    if (dir) {
	free(ftp_lcwd);
	ftp_lcwd = strdup(dir);
    }

    return dir;
}



static void
_sftp_log_str(char *buf, char *pre, char *cmd, unsigned char *data)
{
    int len;

    len = _sftp_get_uint32(data+4);
    sprintf(buf, "%s %s \"%.*s\"", pre, cmd, len, data+8);
}



static void
_sftp_log_handle(char *buf, char *pre, char *cmd, unsigned char *data)
{
    int i, len;
    char *p;

    len = _sftp_get_uint32(data+4);
    sprintf(buf, "%s %s <", pre, cmd);
    p = buf+strlen(buf);

    for (i=0; i<len; i++) {
#define HEX_DIGIT(x) ((x) < 10 ? (x)+'0' : (x)+'a')
	*(p++) = HEX_DIGIT(data[i+8]>>4);
	*(p++) = HEX_DIGIT(data[i+8]&0xf);
#undef HEX_DIGIT
    }
    strcpy(p, ">");
}



int
sftp_put_handle(int type, struct handle *hnd, int flags)
{
    return sftp_put_str(type, hnd->str, hnd->len, flags & ~SFTP_FL_IS_PATH);
}



int
sftp_deidle(void)
{
    /* XXX: deidle connection */
    return 0;
}



void *
sftp_retr(char *file, int mode, long *startp, long *sizep)
{
    struct handle *hnd;
    struct sftp_file *f;

    if ((hnd=sftp_file_open(file, SSH_FXF_READ)) == NULL)
	return NULL;
    if ((f=malloc(sizeof(*f))) == NULL)
	return NULL;

    f->hnd = hnd;
    f->flags = SFTP_FFL_READ;
    if (startp)
	f->off = *startp;
    else
	f->off = 0;

    return f;
}



void *
sftp_stor(char *file, int mode)
{
    struct handle *hnd;
    struct sftp_file *f;

    if ((f=malloc(sizeof(*f))) == NULL)
	return NULL;

    if ((hnd=sftp_file_open(file, SSH_FXF_WRITE|SSH_FXF_CREAT|SSH_FXF_TRUNC))
	== NULL) {
	free(f);
	return NULL;
    }

    f->hnd = hnd;
    f->flags = SFTP_FFL_WRITE;
    f->off = 0;

    return f;
}



int
sftp_fclose(void *f)
{
    int ret;
    struct sftp_file *file;

    file = (struct sftp_file *)f;

    ret = sftp_file_close(file->hnd);
    free(file);
    
    return ret;
}



int
sftp_site(char *cmd)
{
    /* map to extended somehow? */
    /* what return value? */
    return 0;
}



int
sftp_cwd(char *path)
{
    int flags, len, off;
    mode_t mode;
    
    if (sftp_put_str(SSH_FXP_STAT, path, 0, SFTP_FL_LOG|SFTP_FL_IS_PATH) < 0)
	return -1;
    if (_sftp_get_packet(conin, _sftp_buffer, &len, 1) != SSH_FXP_ATTRS)
	return -1;

    flags = _sftp_get_uint32(_sftp_buffer+4);

    if (flags & SSH_FILEXFER_ATTR_PERMISSIONS) {
	off = 8 + (flags & SSH_FILEXFER_ATTR_SIZE ? 8 : 0)
	    + (flags & SSH_FILEXFER_ATTR_UIDGID ? 8 : 0);
	mode = _sftp_get_uint32(_sftp_buffer+off);

	if ((mode & S_IFMT) != S_IFDIR)
	    return -1;
    }

    free(ftp_pcwd);
    ftp_pcwd = strdup(path);
    
    return 0;
}



struct handle *
sftp_file_open(char *file, int flags)
{
    struct handle *hnd;
    unsigned char *p;

    p = _sftp_buffer;
    
    _sftp_put_uint32(p, _sftp_nextid++);
    p += 4;
    _sftp_put_string(p, &p, file, 0, SFTP_FL_IS_PATH);
    _sftp_put_uint32(p, flags);
    p += 4;
    _sftp_put_uint32(p, 0);
    p += 4;

    if (_sftp_put_packet(conout, SSH_FXP_OPEN,
			 _sftp_buffer, p-_sftp_buffer, flags) < 0)
	return NULL;

    return sftp_get_handle();
}



static void
_sftp_put_string(unsigned char *buf, unsigned char **endp,
		 char *str, int slen, int flags)
{
    int len;

    len = 4;
    if ((flags & SFTP_FL_IS_PATH) && str[0] != '/') {
	strcpy(buf+len, ftp_lcwd);
	len += strlen(ftp_lcwd);
	buf[len++] = '/';
    }
    if (!slen)
	slen = strlen(str);
    memcpy(buf+len, str, slen);
    len += slen;

    _sftp_put_uint32(buf, len-4);

    if (endp)
	*endp = buf+len;
}



int
sftp_xfer_read(void *buf, size_t nbytes, void *file)
{
    struct sftp_file *f;
    int n, type, nret;

    f = file;
    nret = 0;

    while (nret < nbytes) {
	switch (f->state) {
	case SFTP_FS_EOF:
	case SFTP_FS_ERROR:
	    if (nret)
		return nret;
	    else
		return -1;
	    
	case SFTP_FS_SEND:
	    n = _sftp_fwrite(f);

	    if (n < 0) {
		f->state == SFTP_FS_ERROR;
		break;
	    }
	    else if (f->state == SFTP_FS_SEND)
		return nret;

	    break;
	    
	case SFTP_FS_RECEIVE:
	    n = _sftp_fread(f);
	    
	    if (n < 0) {
		f->state = SFTP_FS_ERROR;
		break;
	    }
	    else if (n == 0 || (f->poff < SFTP_HEADER_LEN))
		return nret;
	    
	    type = _sftp_buffer[4];
	    
	    if (type == SSH_FXP_DATA) {
		if (f->doff == 0) {
		    f->doff = SFTP_HEADER_LEN;
		    f->dend = _sftp_get_uint32(_sftp_buffer+9)
			+ SFTP_HEADER_LEN;
		}
	    }
	    else {
		if (type == SSH_FXP_STATUS
		    && _sftp_get_uint32(_sftp_buffer+9) == SSH_FX_EOF)
		    f->state = SFTP_FS_EOF;
		else {
		    _sftp_log_packet(1, type, _sftp_buffer+5, f->pend-5);
		    f->state = SFTP_FS_ERROR;
		}
		break;
	    }

	    n = (f->poff < f->dend ? f->poff : f->dend) - f->doff;
	    if (n > nbytes-nret)
		n = nbytes-nret;
	    
	    memcpy(buf+nret, _sftp_buffer+f->doff, n);
	    f->doff += n;
	    nret += n;

	    if (f->doff >= f->dend)
		sftp_send_read(f, SFTP_DATA_LEN);
	}
    }

    return nret;
}



int
sftp_xfer_start(void *file)
{
    struct sftp_file *f;

    f = file;

    if (f->flags & SFTP_FFL_WRITE)
	_sftp_start_write(f, SFTP_DATA_LEN);
    else {
	if (sftp_send_read(f, SFTP_DATA_LEN) < 0)
	    return -1;
    }
    
    set_file_blocking(fileno(conin), 0);
    set_file_blocking(fileno(conout), 0);

    return 0;
}



int
sftp_xfer_stop(void *file, int aborting)
{
    struct sftp_file *f;
    int n, type;

    set_file_blocking(fileno(conin), 1);
    set_file_blocking(fileno(conout), 1);

    f = file;

    if (aborting) {
	/* don't send packet unless already begun */
	if (f->state == SFTP_FS_SEND && f->poff == 0)
	    f->state == SFTP_FS_EOF;
    }
    
    if ((f->flags & SFTP_FFL_WRITE
	 && f->state == SFTP_FS_SEND
	 && f->doff < f->dend)) {
	/* fix up incomplete write packet */
	n = f->doff - (f->woff+4);
	if (n == 0)
	    f->state = SFTP_FS_EOF;
	else {
	    f->pend = f->doff;
	    _sftp_put_uint32(_sftp_buffer, f->pend-4);
	    _sftp_put_uint32(_sftp_buffer+f->woff, n);
	}
    }

    while (f->state == SFTP_FS_SEND) {
	n = _sftp_fwrite(f);

	if (n < 0)
	    return -1;
    }
    while (f->state == SFTP_FS_RECEIVE) {
	n = _sftp_fread(f);

	if (n < 0)
	    return -1;

	if (f->pend != 0 && f->poff >= f->pend) {
	    f->state = SFTP_FS_EOF;
	    type = _sftp_buffer[4];
	    if (type == SSH_FXP_STATUS
		&& _sftp_get_uint32(_sftp_buffer+9) != SSH_FX_OK) {
		_sftp_log_packet(1, type, _sftp_buffer+5, f->pend-5);
		return -1;
	    }
	}
    }
    
    return 0;
}



int
sftp_xfer_write(void *buf, size_t nbytes, void *file)
{
    struct sftp_file *f;
    int n, type, nret;

    f = file;
    nret = 0;

#ifdef DEBUG_XFER
    fprintf(flog, "xfer_write entered\n");
#endif

    while (nret < nbytes) {
#ifdef DEBUG_XFER
    fprintf(flog, "xfer_write top: state: %d, nret %d\n",
		f->state, nret);
#endif
	switch (f->state) {
	case SFTP_FS_EOF:
	case SFTP_FS_ERROR:
	    if (nret) {
#ifdef DEBUG_XFER
		fprintf(flog, "xfer_write returned %d: state: %d\n",
			nret, f->state);
		return nret;
#endif
	    }
	    else {
#ifdef DEBUG_XFER
		fprintf(flog, "xfer_write returned -1: state: %d\n",
			f->state);
#endif
		return -1;
	    }
	    
	case SFTP_FS_SEND:
	    n = f->dend - f->doff;
	    if (n > nbytes-nret)
		n = nbytes-nret;

	    memcpy(_sftp_buffer+f->doff, buf, n);
	    f->doff += n;
	    nret += n;

#ifdef DEBUG_XFER
	    fprintf(flog, "xfer_write: copied %d bytes, nret: %d\n", n, nret);
#endif

	    if (f->doff >= f->dend) {
#ifdef DEBUG_XFER
		fprintf(flog, "xfer_write: sending packet: "
			"poff: %d, pend: %d\n",
			f->poff, f->pend);
#endif
		
		n = _sftp_fwrite(f);

#ifdef DEBUG_XFER
		fprintf(flog, "xfer_write: %d bytes sent\n", n);
#endif

		if (n < 0) {
		    f->state = SFTP_FS_ERROR;
		    break;
		}
		else if (f->state == SFTP_FS_SEND) {
#ifdef DEBUG_XFER
		    fprintf(flog, "xfer_write: returned %d\n", nret);
#endif
		    return nret;
		}

		f->off += _sftp_get_uint32(_sftp_buffer+f->woff);
	    }

	    break;
	    
	case SFTP_FS_RECEIVE:
#ifdef DEBUG_XFER
	    fprintf(flog, "xfer_write: receiving packet\n");
#endif
	    n = _sftp_fread(f);
#ifdef DEBUG_XFER
	    fprintf(flog, "xfer_write: %d bytes received\n", n);
#endif
	    
	    if (n < 0) {
		f->state = SFTP_FS_ERROR;
		break;
	    }
	    else if (f->pend == 0 || f->poff < f->pend) {
#ifdef DEBUG_XFER
		fprintf(flog, "xfer_write: returned %d\n", nret);
#endif
		return nret;
	    }
	    
	    type = _sftp_buffer[4];
	    
	    if (type != SSH_FXP_STATUS
		|| _sftp_get_uint32(_sftp_buffer+9) != SSH_FX_OK) {
		_sftp_log_packet(1, type, _sftp_buffer+5, f->pend-5);
		f->state = SFTP_FS_ERROR;
	    }

#ifdef DEBUG_XFER
	    fprintf(flog, "xfer_write: starting write\n");
#endif
	    _sftp_start_write(f, SFTP_DATA_LEN);
	    break;
	}
    }

#ifdef DEBUG_XFER
    fprintf(flog, "xfer_write: returned %d\n", nret);
#endif
    return nret;
}



int
sftp_xfer_eof(void *file)
{
    struct sftp_file *f;

    f = file;

    return f->state == SFTP_FS_EOF;
}



int
sftp_send_read(struct sftp_file *f, int nbytes)
{
    unsigned char *p;

    p = _sftp_buffer + 4;
    *(p++) = SSH_FXP_READ;
    _sftp_put_uint32(p, _sftp_nextid++);
    p += 4;
    _sftp_put_handle(p, &p, f->hnd);
    _sftp_put_uint64(p, f->off);
    p += 8;
    _sftp_put_uint32(p, nbytes);
    p += 4;

    _sftp_put_uint32(_sftp_buffer, p-_sftp_buffer-4);

    f->poff = 0;
    f->pend = p-_sftp_buffer;
    f->state = SFTP_FS_SEND;
    f->off += nbytes;

    return 0;
}



static void
_sftp_put_handle(unsigned char *buf, unsigned char **endp,
		 struct handle *hnd)
{
    _sftp_put_string(buf, endp, hnd->str, hnd->len, 0);
}



static int
_sftp_fread(struct sftp_file *f)
{
    int early, n, nret;
    fd_set fdset;

#ifdef DEBUG_XFER
    fprintf(flog, "fread entered\n");
#endif

    nret = 0;
    for (;;) {
#ifdef DEBUG_XFER
	fprintf(flog, "fread top: nret %d, poff: %d, pend %d\n",
		nret, f->poff, f->pend);
#endif
	
	early = (f->poff < 4);
	if (early)
	    n = 4 - f->poff;
	else
	    n = f->pend - f->poff;

	n = fread(_sftp_buffer+f->poff, 1, n, conin);

#ifdef DEBUG_XFER
	fprintf(flog, "%d bytes read\n", n);
#endif

	if (n == 0) {
	    if (ferror(conin) && (errno == EINTR || errno == EAGAIN)) {
		clearerr(conin);
		if (errno == EAGAIN) {
#ifdef DEBUG_XFER
		    fprintf(flog, "fread: selecting\n");
#endif
		    FD_ZERO(&fdset);
		    FD_SET(fileno(conin), &fdset);
		    
		    if (select(fileno(conin)+1, &fdset, NULL,
			       NULL, NULL) == -1
			|| !FD_ISSET(fileno(conin), &fdset)) {
			/* XXX: ignore nret? */
#ifdef DEBUG_XFER
			fprintf(flog, "fread: select timeout\n");
#endif
			return 0;
		    }
		    continue;
		}
		/* XXX: ignore nret? */
		return 0;
	    }
	    else {
		/* XXX: ignore nret? */
		return -1;
	    }
	}
	f->poff += n;
	nret += n;

	if (early && f->poff >= 4) {
	    f->pend = _sftp_get_uint32(_sftp_buffer) + 4;
#ifdef DEBUG_XFER
	    fprintf(flog, "fread: packet length: %d\n", f->pend);
#endif
	    continue;
	}

	return nret;
    }
}



static int
_sftp_fwrite(struct sftp_file *f)
{
    int n, do_select;
    fd_set fdset;

#ifdef DEBUG_XFER
    fprintf(flog, "fwrite entered\n");
#endif

    n = do_select = 0;
    for (;;) {
#ifdef DEBUG_XFER
	fprintf(flog, "fwrite top: do_select: %d, poff %d, pend: %d\n",
		do_select, f->poff, f->pend);
#endif

	if (!do_select) {
	    n = fwrite(_sftp_buffer+f->poff, 1, f->pend-f->poff, conout);
	
#ifdef DEBUG_XFER
	    fprintf(flog, "fwrite: %d bytes written\n", n);
#endif

	    if (n == 0 && f->poff < f->pend) {
		if (ferror(conout) && (errno == EINTR || errno == EAGAIN)) {
		    clearerr(conout);
		    if (errno == EAGAIN || f->poff >= f->pend) {
			do_select = 1;
			continue;
		    }
		    return 0;
		}
		else
		    return -1;
	    }
	    f->poff += n;

	    if (f->poff < f->pend)
		return n;
	}

	if (!do_select && f->poff >= f->pend) {
#ifdef DEBUG_XFER
	    fprintf(flog, "fwrite: flusing packet\n");
#endif

	    /* XXX: this may block.  however, fflush is too braindead
               to deal with nonblocking i/o */
	    set_file_blocking(fileno(conout), 1);
	    if (fflush(conout) == EOF) {
		if (errno == EAGAIN) {
		    clearerr(conout);
		    do_select = 1;
		    continue;
		}
		else if (errno == EINTR)
		    return 0;
		else {
#ifdef DEBUG_XFER
		    fprintf(flog, "fwrite: fflush error: %s", strerror(errno));
#endif
		    return -1;
		}
	    }
	    set_file_blocking(fileno(conout), 0);

#ifdef DEBUG_XFER
	    fprintf(flog, "fwrite: packet flushed, switching to receive\n");
#endif
	    f->state = SFTP_FS_RECEIVE;
	    f->poff = f->pend = f->doff = f->dend = 0;
	}

	if (do_select) {
#ifdef DEBUG_XFER
	    fprintf(flog, "fwrite: selecting\n");
#endif
	    FD_ZERO(&fdset);
	    FD_SET(fileno(conout), &fdset);
	    
	    if (select(fileno(conout)+1, NULL, &fdset,
		       NULL, NULL) == -1
		|| !FD_ISSET(fileno(conout), &fdset)) {
#ifdef DEBUG_XFER
		fprintf(flog, "fwrite: select timeout\n");
#endif
		return 0;
	    }
	    do_select = 0;
	    continue;
	}
	
#ifdef DEBUG_XFER
	fprintf(flog, "fwrite returned: %d\n", n);
#endif
	return n;
    }
}



int
_sftp_start_write(struct sftp_file *f, int len)
{
    unsigned char *p;
    
    f->state = SFTP_FS_SEND;

    p = _sftp_buffer + 4;

    *(p++) = SSH_FXP_WRITE;
    _sftp_put_uint32(p, _sftp_nextid++);
    p += 4;
    _sftp_put_handle(p, &p, f->hnd);
    _sftp_put_uint64(p, f->off);
    p += 8;
    f->woff = p-_sftp_buffer;
    _sftp_put_uint32(p, len);
    p += 4;

    f->doff = p-_sftp_buffer;
    p += len;
    f->dend = p-_sftp_buffer;

    _sftp_put_uint32(_sftp_buffer, p-_sftp_buffer-4);
    
    f->poff = 0;
    f->pend = f->dend;

    return 0;
}

#endif /* USE_SFTP */
