/*
  $NiH: sftp.c,v 1.6 2001/12/12 06:09:02 dillo Exp $

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



#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "directory.h"
#include "display.h"
#include "ftp.h"
#include "sftp.h"



#define SFTP_MAX_PACKET_SIZE	4102 /* max read size + SSH_FXP_DATA header */

int sftp_file_close(char *hnd);
char *sftp_get_handle(void);
int sftp_put_str(int type, char *str, int is_path);
int sftp_readdir(char *hnd, char *buf, int *lenp);
int sftp_status(void);

static int _sftp_get_packet(FILE *f, char *buf, int *lenp, int log_resp);
static char *_sftp_get_string(char *buf, char **endp);
static unsigned int _sftp_get_uint32(unsigned char *p);
static unsigned long long _sftp_get_uint64(unsigned char *p);
static void _sftp_log_packet(int dir, int type, char *data, int len);
static int _sftp_parse_name(unsigned char **pp, direntry *e);
static int _sftp_parse_status(int type, char *buf, int len);
static int _sftp_put_packet(FILE *f, int type, char *buf, int len,
			    int log_req);
static void _sftp_put_uint32(unsigned char *p, unsigned int i);
static directory *_sftp_read_dir(char *hnd);
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
    "No error",
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

/* XXX: shared with ftp.c */

extern FILE *conin, *conout;



int
sftp_mkdir(char *path)
{
    sftp_put_str(SSH_FXP_MKDIR, path, 1);
    if (sftp_status() != SSH_FX_OK)
	return -1;
    return 0;
}



int
sftp_rmdir(char *path)
{
    sftp_put_str(SSH_FXP_RMDIR, path, 1);
    if (sftp_status() != SSH_FX_OK)
	return -1;
    return 0;
}



directory *
sftp_list(char *path)
{
    directory *dir;
    char *hnd;

    sftp_put_str(SSH_FXP_OPENDIR, path, 1);

    if ((hnd=sftp_get_handle()) == NULL) {
	/* XXX: why return empty dir here and NULL below? */
	/* XXX: factor out */
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

    dir = _sftp_read_dir(hnd);
    if (dir)
	dir->path = strdup(path);
    
    sftp_file_close(hnd);
    
    return dir;
}



static directory *
_sftp_read_dir(char *hnd)
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
	    if (_sftp_parse_name(&p, &entry)) {
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
	/* XXX: error */
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
    int status;
    char *msg, buf[80];

    switch (type) {
    case SSH_FXP_STATUS:
	status = _sftp_get_uint32(data+4);
	if (status == SSH_FX_OK) {
	    status = 200;
	    msg = "Ok.";
	}
	else {
	    msg = _sftp_strerror(status);
	    status += 500;
	}
	sprintf(buf, "%d %s", status, msg);
	break;

    case SSH_FXP_VERSION:
	sprintf(buf, "202 protocol version %d",
		_sftp_get_uint32(data));
	break;
	
    /* XXX: other packet types */

    default:
	sprintf(buf, "500 Unknown packet type %d", type);
    }

    disp_status("%s", buf);
    ftp_hist(strdup(buf));
}



static int
_sftp_parse_name(unsigned char **pp, direntry *e)
{
    unsigned int flags;
    int n;
    char *p;
    mode_t mode;

    p = *pp;

    e->name = _sftp_get_string(p, &p);
    e->line = _sftp_get_string(p, &p);

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



char *
sftp_get_handle(void)
{
    int len;

    if (_sftp_get_packet(conin, _sftp_buffer, &len, 1) != SSH_FXP_HANDLE)
	return NULL;

    return _sftp_get_string(_sftp_buffer+4, NULL);
}



int
sftp_file_close(char *hnd)
{
    return sftp_put_str(SSH_FXP_CLOSE, hnd, 0);
}



int
sftp_readdir(char *hnd, char *buf, int *lenp)
{
    sftp_put_str(SSH_FXP_READDIR, hnd, 0);

    return _sftp_get_packet(conin, buf, lenp, 0);
}



int
sftp_put_str(int type, char *str, int is_path)
{
    int len;

    len = 8;
    if (is_path) {
	strcpy(_sftp_buffer+len, ftp_lcwd);
	len += strlen(ftp_lcwd);
	_sftp_buffer[len++] = '/';
    }
    strcpy(_sftp_buffer+len, str);
    len += strlen(str);

    _sftp_put_uint32(_sftp_buffer, _sftp_nextid++);
    _sftp_put_uint32(_sftp_buffer+4, len-8);
    
    return _sftp_put_packet(conout, type, _sftp_buffer, len, 1);
}



static void
_sftp_put_uint32(unsigned char *p, unsigned int i)
{
    p[0] = (i>>24) & 0xff;
    p[1] = (i>>16) & 0xff;
    p[2] = (i>>8) & 0xff;
    p[3] = i & 0xff;
}



static int
_sftp_put_packet(FILE *f, int type, char *buf, int len, int log_req)
{
    char b[9];

    _sftp_put_uint32(b, len+1);
    b[5] = type;

    if (fwrite(b, 1, 5, f) != 5)
	return -1;
    
    if (fwrite(buf, 1, len, f) != len)
	return -1;

    if (log_req)
	_sftp_log_packet(0, type, buf, len);

    return 0;
}
