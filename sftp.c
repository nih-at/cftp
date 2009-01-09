/*
  $NiH: sftp.c,v 1.28 2005/06/03 11:56:16 wiz Exp $

  sftp.c -- sftp protocol functions
  Copyright (C) 2001, 2002 Dieter Baron

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



#include "config.h"
#ifdef USE_SFTP

#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
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
#define SFTP_HEADER_LEN		9	/* length + type + id */

#define SFTP_DATA_HEADER_LEN	SFTP_HEADER_LEN+4
					/* no of bytes before data in a
					   SSH_FXP_DATA packet */
#define SFTP_DATA_LEN		8192	/* data length used for read/write */

#define SFTP_FL_LOG		0x1	/* log packet */
#define SFTP_FL_IS_PATH		0x2	/* string is path (prepend cwd) */
#define SFTP_FL_NONBLOCK	0x4	/* don't block in i/o */
#define SFTP_FL_CONT		0x8	/* continue with packet */

struct packet {
    int type;		/* packet type */
    int id;		/* packet id */
    char *dat;		/* packet data (starting after id) */
    int plen;		/* length of packet data */
    int pn;		/* number of bytes read/write */
};

struct handle {
    int len;
    char *str;
};

enum sftp_file_state {
    SFTP_FS_EOF, SFTP_FS_ERROR, SFTP_FS_SEND, SFTP_FS_RECEIVE
};

struct sftp_file {
    int flags;
    struct handle *hnd;	/* sftp handle */
    off_t off;		/* current offset within file */
    enum sftp_file_state state;
    			/* current state */
    int doff;		/* offset to unreturned data */
    int dend;		/* end of data within current packet */
    int woff;		/* offset to length in write packet */
};



int sftp_file_close(struct handle *hnd);
struct handle *sftp_file_open(char *file, int flags);
struct handle *sftp_get_handle(void);
int sftp_put_handle(int type, struct handle *hnd, int flags);
int sftp_put_str(int type, char *str, int len, int flags);
int sftp_readdir(struct handle *hnd, struct packet *pkt, int flags);
int sftp_send_init(int proto_version);
int sftp_start_read(struct sftp_file *f, int nbytes);
int sftp_status(void);

static int _sftp_close(int st_valid, int status);
static int _sftp_get_packet(struct packet *p, int flags);
static char *_sftp_get_string(char *buf, char **endp);
static unsigned int _sftp_get_uint32(char *p, char **endp);
static unsigned long long _sftp_get_uint64(char *p, char **endp);
static void _sftp_log_handle(char *buf, char *data, char **endp);
static void _sftp_log_packet(int dir, struct packet *pkt);
static void _sftp_log_pflags(char *buf, char *data, char **endp);
static void _sftp_log_str(char *buf, char *data, char **endp);
static void _sftp_make_packet(struct packet *pkt, int type, char *end);
static int _sftp_parse_name(char *p, char **endp, direntry *e);
static int _sftp_parse_status(struct packet *pkt);
static void _sftp_put_handle(char *buf, char **endp, struct handle *hnd);
static int _sftp_put_packet(struct packet *pkt, int flags);
static void _sftp_put_string(char *buf, char **endp,
			     char *str, int slen, int flags);
static void _sftp_put_uint32(char *p, char **endp, unsigned int i);
static void _sftp_put_uint64(char *p, char **endp, unsigned long long i);
static int _sftp_read(int fd, void *buf, size_t nbytes, int flags);
static directory *_sftp_read_dir(struct handle *hnd);
static void _sftp_send_error_packet(char *fmt, ...);
static void _sftp_sig_child(int sig);
static void _sftp_start_ssh(int fdin, int fdout);
static int _sftp_start_write(struct sftp_file *f, int len);
static char *_sftp_strerror(int error);
static int _sftp_writev(int fd, struct iovec *iov, int niov, int flags);



#define SFTP_PROTO_VERSION	3

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



static char _sftp_buffer[SFTP_MAX_PACKET_SIZE];
static struct packet _sftp_packet_s;
#define _sftp_packet (&_sftp_packet_s)

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

static int _sftp_nextid;	/* id to use in next packet */
static pid_t _sftp_ssh_pid;	/* PID of ssh subprocess */
static int _conin, _conout;	/* file descriptors to ssh subprocess */



/*
  Implements mkdir method.
*/

int
sftp_mkdir(char *path)
{
    sftp_put_str(SSH_FXP_MKDIR, path, 0, SFTP_FL_LOG|SFTP_FL_IS_PATH);
    if (sftp_status() != SSH_FX_OK)
	return -1;
    return 0;
}



/*
  Implements rmdir method.
*/

int
sftp_rmdir(char *path)
{
    sftp_put_str(SSH_FXP_RMDIR, path, 0, SFTP_FL_LOG|SFTP_FL_IS_PATH);
    if (sftp_status() != SSH_FX_OK)
	return -1;
    return 0;
}



/*
  Implements list method.
*/

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
    char *p;
    time_t oldt, newt;
    int n, pn;

    dir = dir_new();
    oldt = 0;

    n = 0;
    while (sftp_readdir(hnd, _sftp_packet, 0) == 0
	   && _sftp_packet->type == SSH_FXP_NAME) {
	pn = _sftp_get_uint32(_sftp_packet->dat, &p);
	while (pn--) {
	    if (_sftp_parse_name(p, &p, &entry) == 0) {
		if (strcmp(entry.name, ".") == 0
		    || strcmp(entry.name, "..") == 0) {
		    free(entry.name);
		    free(entry.line);
		    continue;
		}
		dir_add(dir, &entry);
		n++;
	    }

	    if ((newt=time(NULL)) != oldt) {
		disp_status(DISP_STATUS, "listed %d", n);
		oldt = newt;
	    }
	}
    }

    if (_sftp_parse_status(_sftp_packet) != SSH_FX_EOF)
	_sftp_log_packet(1, _sftp_packet);

    return dir;
}    



/*
  Return string corresponding to error number ERROR.  This is a static
  string that remains valid until next call to _sftp_strerror.
*/

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



/*
  Read packet into P.  Return -1 in case of error, 0 if packet is
  complete, 1 if not completely read (non-blocking case).
*/

static int
_sftp_get_packet(struct packet *p, int flags)
{
    int n, off;
    char *cp;

    if (_conin < 0)
	return -1;

    if (!(flags & SFTP_FL_CONT)) {
	p->plen = 0;
	p->pn = 0;
    }

    if (p->pn < SFTP_HEADER_LEN) {
	/* read length and type */
	if ((n=_sftp_read(_conin, p->dat+p->pn, SFTP_HEADER_LEN-p->pn,
			  flags)) < 0)
	    return -1;

	p->pn += n;

	if (p->pn < SFTP_HEADER_LEN)
	    return 1;

	p->plen = _sftp_get_uint32(p->dat, &cp);
	p->type = *(cp++);
	p->id = _sftp_get_uint32(cp, &cp);
	p->plen -= SFTP_HEADER_LEN - 4;
    }

    off = p->pn - SFTP_HEADER_LEN;
    if ((n=_sftp_read(_conin, p->dat+off, p->plen-off, flags)) < 0)
	return -1;

    p->pn += n;

    if (off+n < p->plen)
	return 1;

    if (flags & SFTP_FL_LOG)
	_sftp_log_packet(1, p);

    return 0;
}



static void
_sftp_log_packet(int dir, struct packet *pkt)
{
    static char *req[] = {
	"init %I",
	"version %I",
	"open %s %p %a",
	"close %h",
	"read %h @ %l + %i",
	"write %h @ %l + %i",
	"lstat %s",
	"fstat %h",
	"setstat, %s %a",
	"fsetstat %h %a",
	"opendir %s",
	"readdir %h",
	"remove %s",
	"mkdir %s",
	"rmdir %s",
	"realpath %s",
	"stat %s",
	"rename %s %s"
    };
    static char *rsp[] = {
	"status %m",
	"handle %h",
	"data %i",
	"name %i %s",
	"attrs %a"
    };
    static char *ext[] = {
	"extended %s",
	"extended-reply"
    };
    static char *dir_pre[] = { "-> ", "<= " };

    char buf[8192], *fmt, *bp, *fp, *fq, *pp;
    int n;

    if (pkt->type >= SSH_FXP_INIT && pkt->type <= SSH_FXP_RENAME)
	fmt = req[pkt->type-SSH_FXP_INIT];
    else if (pkt->type >= SSH_FXP_STATUS && pkt->type <= SSH_FXP_ATTRS)
	fmt = rsp[pkt->type-SSH_FXP_STATUS];
    else if (pkt->type >= SSH_FXP_EXTENDED
	     && pkt->type <= SSH_FXP_EXTENDED_REPLY) {
	fmt = ext[pkt->type-SSH_FXP_EXTENDED];
    }
    else
	fmt = "unknown packet type %t";

    strcpy(buf, dir_pre[dir!=0]);
    bp = buf+strlen(buf);

    if (pkt->type != SSH_FXP_INIT && pkt->type != SSH_FXP_VERSION) {
	sprintf(bp, "[%d] ", pkt->id);
	bp += strlen(bp);
    }

    /* XXX: check for buf overrun */

    fp=fmt;
    pp = pkt->dat;
    while ((fq=strchr(fp, '%'))) {
	n = fq-fp;
	if (n) {
	    strncpy(bp, fp, n);
	    bp += n;
	}

	switch (fq[1]) {
	case 'a':
	    *bp = '\0';
	    break;
	case 'h':
	    _sftp_log_handle(bp, pp, &pp);
	    break;
	case 'I':
	    sprintf(bp, "%i", pkt->id);
	    break;
	case 'i':
	    sprintf(bp, "%u", _sftp_get_uint32(pp, &pp));
	    break;
	case 'l':
	    pp += 8;
	    break;
	case 'm':
	    strcpy(bp, _sftp_strerror(_sftp_get_uint32(pp, &pp)));
	    break;
	case 'p':
	    _sftp_log_pflags(bp, pp, &pp);
	    break;
	case 's':
	    _sftp_log_str(bp, pp, &pp);
	    break;
	case 'T':
	    sprintf(bp, "%u", pkt->type);
	    break;
	default:
	    bp[0] = '%';
	    bp[1] = fq[1];
	    break;
	}

	fp = fq+2;
	bp += strlen(bp);
    }
    strcpy(bp, fp);

    disp_status(DISP_PROTO, "%s", buf);

    if (pkt->type == SSH_FXP_INIT || pkt->type == SSH_FXP_VERSION) {
	bp = buf+3;
	while (pp < pkt->dat+pkt->plen) {
	    _sftp_log_str(bp, pp, &pp);
	    disp_status(DISP_HIST, "%s", buf);
	    pp += _sftp_get_uint32(pp, NULL) + 4;
	}
    }

}



static int
_sftp_parse_name(char *p, char **endp, direntry *e)
{
    unsigned int flags;
    int n, len;
    mode_t mode;

    e->name = _sftp_get_string(p, &p);

    len = _sftp_get_uint32(p, &p);
    e->line = malloc(len+2);
    e->line[0] = ' ';
    strncpy(e->line+1, p, len);
    e->line[len+1] = '\0';
    p += len;
    
    e->link = NULL;

    flags = _sftp_get_uint32(p, &p);

    if (flags & SSH_FILEXFER_ATTR_SIZE) {
	e->size = _sftp_get_uint64(p, &p);
    }
    else
	e->size = -1;

    if (flags & SSH_FILEXFER_ATTR_UIDGID)
	p += 8;

    if (flags & SSH_FILEXFER_ATTR_PERMISSIONS) {
	mode = _sftp_get_uint32(p, &p);

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
	e->mtime = _sftp_get_uint32(p, &p);
    }
    else
	e->mtime = 0;

    if (flags & SSH_FILEXFER_ATTR_EXTENDED) {
	n = _sftp_get_uint32(p, &p);

	while (n--) {
	    p += _sftp_get_uint32(p, NULL) + 4;
	    p += _sftp_get_uint32(p, NULL) + 4;
	}
    }

    if (endp)
	*endp = p;

    return 0;
}



/*
  Return uint32 at P.  If END is non-NULL, point *END to next byte to
  be processed.
*/

static unsigned int
_sftp_get_uint32(char *p, char **end)
{
    unsigned char *up;
    
    if (end)
	*end = p+4;

    up = (unsigned char *)p;

    return (up[0]<<24) | (up[1]<<16) | (up[2]<<8) | up[3];
}



/*
  Return uint64 at P.  If END is non-NULL, point *END to next byte to
  be processed.
*/

static unsigned long long
_sftp_get_uint64(char *p, char **end)
{
    unsigned char *up;
    
    if (end)
	*end = p+8;
    
    up = (unsigned char *)p;

    return ((unsigned long long)up[0]<<56) | ((unsigned long long)up[1]<<48)
	| ((unsigned long long)up[2]<<40) | ((unsigned long long)up[3]<<32)
	| (up[4]<<24) | (up[5]<<16) | (up[6]<<8) | up[7];
}



/*
  Return string at P (malloc()ed, NUL-terminated.  If END is non-NULL,
  point *END to next byte to be processed.
*/

static char *
_sftp_get_string(char *buf, char **endp)
{
    char *s, *p;
    int len;

    len = _sftp_get_uint32(buf, &p);
    if (!len)
	return NULL;

    if ((s=malloc(len+1)) == NULL)
	return NULL;

    memcpy(s, p, len);
    s[len] = '\0';

    if (endp)
	*endp = p+len;

    return s;
}



/*
  Read status packet and return status code.  If packet read is not a
  status packet, or if an error occurs, return -1.
*/

int
sftp_status(void)
{
    if (_sftp_get_packet(_sftp_packet, SFTP_FL_LOG) < 0)
	return -1;

    return _sftp_parse_status(_sftp_packet);
}



/*
  Return status code from status packet.  If packet is not a status
  packet, return -1.
*/

static int
_sftp_parse_status(struct packet *pkt)
{
    if (pkt->type != SSH_FXP_STATUS || pkt->plen < 4)
	return -1;

    return _sftp_get_uint32(pkt->dat, NULL);
}



/*
  Read handle packet and return handle.  If packet read is not a
  handle packet or if an error occurs, return NULL.
*/

struct handle *
sftp_get_handle(void)
{
    struct handle *hnd;
    char *p;

    if (_sftp_get_packet(_sftp_packet, SFTP_FL_LOG) < 0
	|| _sftp_packet->type != SSH_FXP_HANDLE)
	return NULL;

    hnd = malloc(sizeof(*hnd));

    hnd->len = _sftp_get_uint32(_sftp_packet->dat, &p);
    hnd->str = malloc(hnd->len);
    memcpy(hnd->str, p, hnd->len);
    
    return hnd;
}



/*
  Close file associated with HND on server.  Return status returned
  from server.
*/

int
sftp_file_close(struct handle *hnd)
{
    sftp_put_handle(SSH_FXP_CLOSE, hnd, SFTP_FL_LOG);

    free(hnd->str);
    free(hnd);

    return sftp_status();
}



/*
  Send a readdir request on HND, and read response into PKT.  Return
  value returned by _sftp_get_packet.
*/

int
sftp_readdir(struct handle *hnd, struct packet *pkt, int flags)
{
    sftp_put_handle(SSH_FXP_READDIR, hnd, flags);

    return _sftp_get_packet(pkt, flags);
}



/*
  Send a packet of type TYPE, containing only string STR.
*/

int
sftp_put_str(int type, char *str, int slen, int flags)
{
    char *p;

    p = _sftp_packet->dat;
    _sftp_put_string(p, &p, str, slen, flags);
    _sftp_make_packet(_sftp_packet, type, p);
    
    return _sftp_put_packet(_sftp_packet, flags&~SFTP_FL_CONT);
}



/*
  Put unit32 I at P.  If END is non-NULL, point *END to next byte to
  be filled.  */

static void
_sftp_put_uint32(char *p, char **end, unsigned int i)
{
    unsigned char *q;
    
    if (end)
	*end = p+4;

    q = (unsigned char *)p;
    
    q[0] = (i>>24) & 0xff;
    q[1] = (i>>16) & 0xff;
    q[2] = (i>>8) & 0xff;
    q[3] = i & 0xff;
}



/*
  Put unit64 I at P.  If END is non-NULL, point *END to next byte to
  be filled.
*/

static void
_sftp_put_uint64(char *p, char **end, unsigned long long i)
{
    unsigned char *q;
    
    if (end)
	*end = p+8;

    q = (unsigned char *)p;

    q[0] = (i>>56) & 0xff;
    q[1] = (i>>48) & 0xff;
    q[2] = (i>>40) & 0xff;
    q[3] = (i>>32) & 0xff;
    q[4] = (i>>24) & 0xff;
    q[5] = (i>>16) & 0xff;
    q[6] = (i>>8) & 0xff;
    q[7] = i & 0xff;
}



/*
  Write packet PKG.  If PKG->id is -1, use next unused id.  Return -1
  in case of error, 0 if packet was completely written, 1 else.
*/

static int
_sftp_put_packet(struct packet *pkt, int flags)
{
    char b[SFTP_HEADER_LEN], *p;
    struct iovec iov[2];
    int niov, n, off;
    
    niov = 0;

    if (!(flags & SFTP_FL_CONT)) {
	if (pkt->id == -1)
	    pkt->id = _sftp_nextid++;
	pkt->pn = 0;
    }

    if (pkt->pn < SFTP_HEADER_LEN) {
	_sftp_put_uint32(b, &p, pkt->plen + SFTP_HEADER_LEN-4);
	*(p++) = pkt->type;
	_sftp_put_uint32(p, &p, pkt->id);

	iov[niov].iov_base = b+pkt->pn;
	iov[niov].iov_len = SFTP_HEADER_LEN-pkt->pn;
	niov++;
    }

    off = pkt->pn-SFTP_HEADER_LEN;
    if (off < 0)
	off = 0;
	
    if (pkt->plen) {
	iov[niov].iov_base = pkt->dat + off;
	iov[niov].iov_len = pkt->plen - off;
	niov++;
    }
    
    if ((n=_sftp_writev(_conout, iov, niov, flags)) < 0)
	return -1;

    pkt->pn += n;

    if (pkt->pn < pkt->plen+SFTP_HEADER_LEN)
	return 1;

    if (flags & SFTP_FL_LOG)
	_sftp_log_packet(0, pkt);

    return 0;
}



/*
  Implements login method.
*/

int
sftp_login(char *user, char *pass)
{
    /* XXX: handle user != NULL */

    status.host = mkhoststr(0, 0);
    
    return 0;
}



/*
  Implements open method.
*/

int
sftp_open(char *host, char *port, char *user, char *pass)
{
    int pin[2], pout[2];
    pid_t pid;

    ftp_remember_host(host, port);
    ftp_remember_user(user, pass);

    _sftp_packet->dat = _sftp_buffer;

    if (pipe(pin) != 0 || pipe(pout) != 0) {
	disp_status(DISP_ERROR, "cannot create pipes: %s",
		    strerror(errno));
	return -1;
    }

    switch ((pid=fork())) {
    case -1:
	disp_status(DISP_ERROR, "cannot fork: %s", strerror(errno));
	return -1;

    case 0:
	close(pin[0]);
	close(pout[1]);
	_sftp_start_ssh(pout[0], pin[1]);
	/* NOTREACHED */

    default:
	_sftp_ssh_pid = pid;
	signal(SIGCHLD, _sftp_sig_child);
	
	close(pin[1]);
	close(pout[0]);

	_conin = pin[0];
	_conout = pout[1];

	fcntl(_conin, F_SETFD, 1);
	fcntl(_conout, F_SETFD, 1);
	
	/* XXX: check for minmal server version */
	if (sftp_send_init(SFTP_PROTO_VERSION) < 0) {
	    sftp_close();
	    return -1;
	}
	return 0;
    }
}



/*
  Set up environment and exec ssh.  In case of error, write status
  packet to pipe and exit(0).
*/

static void
_sftp_start_ssh(int fdin, int fdout)
{
    char *args[20];
    int n;

    signal(SIGPIPE, SIG_DFL);

    close(0);
    close(1);

    /* for use in _sftp_send_error_packet() */
    _conout = fdout;

    if (dup(fdin) != 0 || dup(fdout) != 1) {
	_sftp_send_error_packet("cannot dup: %s", strerror(errno));
	exit(0);
    }

    close(fdin);
    close(fdout);

    n = 0;
    args[n++] = "ssh";
    args[n++] = "-2";
    args[n++] = "-s";
    if (ftp_prt()) {
	args[n++] = "-p";
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
    _sftp_send_error_packet("exec failed: %s", strerror(errno));
    exit(0);
}



/*
  Implements close method.
*/

int
sftp_close(void)
{
    return _sftp_close(0, 0);
}



/*
  Send init packet and read reply.  Return protocol version from
  reply, or -1 in case of error or if response packet is not of type
  version.
*/

int
sftp_send_init(int proto_version)
{
    _sftp_make_packet(_sftp_packet, SSH_FXP_INIT, _sftp_packet->dat);
    _sftp_packet->id = proto_version;

    if (_sftp_put_packet(_sftp_packet, SFTP_FL_LOG) < 0)
	return -1;
    
    if (_sftp_get_packet(_sftp_packet, SFTP_FL_LOG) < 0
	|| _sftp_packet->type != SSH_FXP_VERSION)
	return -1;

    return _sftp_packet->id;
}



/*
  Implements pwd method.
*/

char *
sftp_pwd(void)
{
    char *dir;
    
    if (sftp_put_str(SSH_FXP_REALPATH, ftp_lcwd ? ftp_lcwd : "", 0,
		     SFTP_FL_LOG) < 0)
	return NULL;
    
    if (_sftp_get_packet(_sftp_packet, SFTP_FL_LOG) <0
	|| _sftp_packet->type != SSH_FXP_NAME)
	return NULL;

    /* XXX: magic number */
    dir = _sftp_get_string(_sftp_buffer+4, NULL);

    if (dir) {
	free(ftp_lcwd);
	ftp_lcwd = strdup(dir);
    }

    return dir;
}



static int
_sftp_close(int st_valid, int status)
{
    int ret;
    
    signal(SIGCHLD, SIG_IGN);

    if (_conin >= 0) {
	close(_conin);
	_conin = -1;
    }
    if (_conout >= 0) {
	close(_conout);
	_conout = -1;
    }

    ret = 1;

    if (_sftp_ssh_pid) {
	if (!st_valid) {
	    if (waitpid(_sftp_ssh_pid, &status, 0) == _sftp_ssh_pid)
		st_valid = 1;
	    else {
		disp_status(DISP_ERROR, "cannot wait on ssh (pid %d): %s",
			    (int)_sftp_ssh_pid, strerror(errno));
		ret = -1;
	    }
	}
	
	if (st_valid) {
	    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
		disp_status(DISP_ERROR, "ssh exited with %d",
			    WEXITSTATUS(status));
		ret = -1;
	    }
	    else if (WIFSIGNALED(status)) {
		disp_status(DISP_ERROR, "ssh exited due to signal %d",
			    WTERMSIG(status));
		ret = -1;
	    }
	}
    }

    _sftp_ssh_pid = 0;

    return ret;
}



static void
_sftp_log_str(char *buf, char *data, char **endp)
{
    int len;
    char *p;

    len = _sftp_get_uint32(data, &p);
    sprintf(buf, "\"%.*s\"", len, p);

    if (endp)
	*endp = p+len;
}



static void
_sftp_log_handle(char *buf, char *data, char **endp)
{
    int i, len;
    char *p;

    len = _sftp_get_uint32(data, &p);
    *(buf++) = '<';

    for (i=0; i<len; i++) {
#define HEX_DIGIT(x) ((x) < 10 ? (x)+'0' : (x)+'a')
	*(buf++) = HEX_DIGIT(p[i]>>4);
	*(buf++) = HEX_DIGIT(p[i]&0xf);
#undef HEX_DIGIT
    }
    strcpy(buf, ">");

    if (endp)
	*endp = p+len;
}



/*
  Put string representation of open pflags at DATA into BUF.  If ENDP
  is non-NULL, point it at next unprocessed byte.
*/

static void
_sftp_log_pflags(char *buf, char *data, char **endp)
{
    static char *fl[] = {
	"read", "write", "append", "creat", "trunc", "excl"
    };
    
    int flags, i, n;

    flags = _sftp_get_uint32(data, endp);

    n = 0;
    *(buf++) = '(';
    for (i=0; i<sizeof(fl)/sizeof(fl[0]); i++) {
	if (flags & (1<<i)) {
	    sprintf(buf, "%s%s", (n ? ", " : ""), fl[i]);
	    buf += strlen(buf);
	    n = 1;
	}
    }
    strcpy(buf, ")");
}



/*
  Send a packet of type TYPE, containing only string HND.
*/

int
sftp_put_handle(int type, struct handle *hnd, int flags)
{
    return sftp_put_str(type, hnd->str, hnd->len, flags & ~SFTP_FL_IS_PATH);
}



/*
  Implements deidle method.
*/

int
sftp_deidle(void)
{
    /* XXX: deidle connection */
    return 0;
}



/*
  Implements retr method.
*/

void *
sftp_retr(char *file, int mode, off_t *startp, off_t *sizep)
{
    struct handle *hnd;
    struct sftp_file *f;

    if ((hnd=sftp_file_open(file, SSH_FXF_READ)) == NULL)
	return NULL;
    if ((f=malloc(sizeof(*f))) == NULL)
	return NULL;

    f->hnd = hnd;
    f->flags = SSH_FXF_READ;
    if (startp && *startp > 0)
	f->off = *startp;
    else
	f->off = 0;

    if (sizep)
	*sizep = -1;

    return f;
}



/*
  Implements stor method.
*/

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
    f->flags = SSH_FXF_WRITE;
    f->off = 0;

    return f;
}



/*
  Implements fclose method.
*/

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



/*
  Implements site method.
*/

int
sftp_site(char *cmd)
{
    /* map to extended somehow? */
    /* what return value? */
    return 0;
}



/*
  Implements cwd method.
*/

int
sftp_cwd(char *path)
{
    int flags, off;
    mode_t mode;
    
    if (sftp_put_str(SSH_FXP_STAT, path, 0, SFTP_FL_LOG|SFTP_FL_IS_PATH) < 0)
	return -1;
    if (_sftp_get_packet(_sftp_packet, SFTP_FL_LOG) < 0
	|| _sftp_packet->type != SSH_FXP_ATTRS)
	return -1;

    flags = _sftp_get_uint32(_sftp_packet->dat, NULL);

    /* XXX: magic number */
    if (flags & SSH_FILEXFER_ATTR_PERMISSIONS) {
	off = 4 + (flags & SSH_FILEXFER_ATTR_SIZE ? 8 : 0)
	    + (flags & SSH_FILEXFER_ATTR_UIDGID ? 8 : 0);
	mode = _sftp_get_uint32(_sftp_packet->dat+off, NULL);

	if ((mode & S_IFMT) != S_IFDIR)
	    return -1;
    }

    free(ftp_pcwd);
    ftp_pcwd = strdup(path);
    
    return 0;
}



/*
  Open file FILE on server with FLAGS.  Return handle to file, NULL in
  case of error.
*/

struct handle *
sftp_file_open(char *file, int flags)
{
    char *p;

    p = _sftp_packet->dat;
    _sftp_put_string(p, &p, file, 0, SFTP_FL_IS_PATH);
    _sftp_put_uint32(p, &p, flags);
    _sftp_put_uint32(p, &p, 0);
    _sftp_make_packet(_sftp_packet, SSH_FXP_OPEN, p);

    if (_sftp_put_packet(_sftp_packet, SFTP_FL_LOG) < 0)
	return NULL;

    return sftp_get_handle();
}



/*
  Put string STR of length SLEN at BUF.  If SLEN is 0, STR is assumed
  to be NUL-terminated.  If SFTP_FL_IS_PATH is set in FLAGS, prepend
  cwd.  If ENDP is non-NULL, point it at next byte to be filled.
*/
  
static void
_sftp_put_string(char *buf, char **endp, char *str, int slen, int flags)
{
    int len;
    char *p;

    p = buf+4;
    len = 0;
    if ((flags & SFTP_FL_IS_PATH) && str[0] != '/') {
	strcpy(p+len, ftp_lcwd);
	len += strlen(ftp_lcwd);
	p[len++] = '/';
    }
    if (!slen)
	slen = strlen(str);
    memcpy(p+len, str, slen);
    len += slen;

    _sftp_put_uint32(buf, NULL, len);

    if (endp)
	*endp = p+len;
}



/*
  Implements xfer_read method.
*/

int
sftp_xfer_read(void *buf, size_t nbytes, void *file)
{
    struct sftp_file *f;
    int n, nret, ret;

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
	    ret = _sftp_put_packet(_sftp_packet,
				   SFTP_FL_NONBLOCK|SFTP_FL_CONT);

	    if (ret < 0) {
		f->state = SFTP_FS_ERROR;
		break;
	    }
	    else if (ret == 1)
		return nret;

	    /* reset packet */
	    _sftp_make_packet(_sftp_packet, 0, _sftp_packet->dat);
	    f->doff = f->dend = 0;
	    f->state = SFTP_FS_RECEIVE;
	    break;
	    
	case SFTP_FS_RECEIVE:
	    ret = _sftp_get_packet(_sftp_packet,
				   SFTP_FL_NONBLOCK|SFTP_FL_CONT);

	    if (ret < 0) {
		f->state = SFTP_FS_ERROR;
		break;
	    }
	    else if (_sftp_packet->pn < SFTP_DATA_HEADER_LEN)
		return nret;
	    
	    if (_sftp_packet->type == SSH_FXP_DATA) {
		if (f->doff == 0) {
		    f->doff = SFTP_DATA_HEADER_LEN - SFTP_HEADER_LEN;
		    f->dend = f->doff
			+ _sftp_get_uint32(_sftp_packet->dat, NULL);
		}
	    }
	    else {
		if (_sftp_packet->type == SSH_FXP_STATUS
		    && _sftp_get_uint32(_sftp_packet->dat, NULL) == SSH_FX_EOF)
		    f->state = SFTP_FS_EOF;
		else {
		    _sftp_log_packet(1, _sftp_packet);
		    f->state = SFTP_FS_ERROR;
		}
		break;
	    }

	    n = _sftp_packet->pn - SFTP_HEADER_LEN;
	    if (n > f->dend)
		n = f->dend;
	    n -= f->doff;
	    if (n > nbytes-nret)
		n = nbytes-nret;
	    
	    memcpy(((char *)buf)+nret, _sftp_packet->dat+f->doff, n);
	    f->doff += n;
	    nret += n;

	    if (ret == 0 && f->doff >= f->dend)
		sftp_start_read(f, SFTP_DATA_LEN);
	}
    }

    return nret;
}



/*
  Implements xfer_start method.
*/

int
sftp_xfer_start(void *file)
{
    struct sftp_file *f;

    f = file;

    if (f->flags & SSH_FXF_WRITE)
	_sftp_start_write(f, SFTP_DATA_LEN);
    else {
	if (sftp_start_read(f, SFTP_DATA_LEN) < 0)
	    return -1;
    }
    
    set_file_blocking(_conin, 0);
    set_file_blocking(_conout, 0);

    return 0;
}



/*
  Implements xfer_stop method.
*/

int
sftp_xfer_stop(void *file, int aborting)
{
    struct sftp_file *f;
    int n;

    set_file_blocking(_conin, 1);
    set_file_blocking(_conout, 1);

    f = file;

    if (aborting) {
	/* don't send packet unless already begun */
	if (f->state == SFTP_FS_SEND && _sftp_packet->pn == 0)
	    f->state = SFTP_FS_EOF;
    }
    
    if ((f->flags & SSH_FXF_WRITE
	 && f->state == SFTP_FS_SEND
	 && f->doff < f->dend)) {
	/* fix up incomplete write packet */
	n = f->doff - (f->woff+4);
	if (n == 0)
	    f->state = SFTP_FS_EOF;
	else {
	    _sftp_packet->plen = f->doff;
	    _sftp_put_uint32(_sftp_buffer+f->woff, NULL, n);
	}
    }

    while (f->state == SFTP_FS_SEND) {
	switch (_sftp_put_packet(_sftp_packet, SFTP_FL_CONT)) {
	case -1:
	    return -1;
	case 0:
	    f->state = SFTP_FS_RECEIVE;
	    _sftp_make_packet(_sftp_packet, 0, _sftp_packet->dat);
	}
    }
    while (f->state == SFTP_FS_RECEIVE) {
	switch (_sftp_get_packet(_sftp_packet, SFTP_FL_CONT)) {
	case -1:
	    return -1;

	case 0:
	    f->state = SFTP_FS_EOF;
	    if (_sftp_parse_status(_sftp_packet) != SSH_FX_OK) {
		_sftp_log_packet(1, _sftp_packet);
		return -1;
	    }
	}
    }
    
    return 0;
}



/*
  Implements xfer_write method.
*/

int
sftp_xfer_write(void *buf, size_t nbytes, void *file)
{
    struct sftp_file *f;
    int n, nret, ret;

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
	    n = f->dend - f->doff;
	    if (n > nbytes-nret)
		n = nbytes-nret;

	    if (n) {
		memcpy(_sftp_packet->dat+f->doff, buf, n);
		f->doff += n;
		nret += n;
	    }

	    if (f->doff >= f->dend) {
		
		ret = _sftp_put_packet(_sftp_packet,
				     SFTP_FL_NONBLOCK|SFTP_FL_CONT);

		if (ret < 0) {
		    f->state = SFTP_FS_ERROR;
		    break;
		}
		else if (ret == 1)
		    return nret;

		f->off += _sftp_get_uint32(_sftp_buffer+f->woff, NULL);
		_sftp_make_packet(_sftp_packet, 0, _sftp_packet->dat);
		f->state = SFTP_FS_RECEIVE;
	    }

	    break;
	    
	case SFTP_FS_RECEIVE:
	    ret = _sftp_get_packet(_sftp_packet,
				   SFTP_FL_NONBLOCK|SFTP_FL_CONT);
	    
	    if (ret < 0) {
		f->state = SFTP_FS_ERROR;
		break;
	    }
	    else if (ret == 1)
		return nret;
	    
	    if (_sftp_parse_status(_sftp_packet) != SSH_FX_OK) {
		_sftp_log_packet(1, _sftp_packet);
		f->state = SFTP_FS_ERROR;
	    }

	    _sftp_start_write(f, SFTP_DATA_LEN);
	    break;
	}
    }

    return nret;
}



/*
  Implements xfer_eof method.
*/

int
sftp_xfer_eof(void *file)
{
    struct sftp_file *f;

    f = file;

    return f->state == SFTP_FS_EOF;
}



/*
  Start read cycle: prepare read packet for NBYTES bytes, step offest
  of F, and put F in state receive.
*/

int
sftp_start_read(struct sftp_file *f, int nbytes)
{
    char *p;

    p = _sftp_packet->dat;
    _sftp_put_handle(p, &p, f->hnd);
    _sftp_put_uint64(p, &p, f->off);
    _sftp_put_uint32(p, &p, nbytes);
    _sftp_make_packet(_sftp_packet, SSH_FXP_READ, p);
    _sftp_packet->id = _sftp_nextid++;

    f->state = SFTP_FS_SEND;
    f->off += nbytes;

    return 0;
}



/*
  Put handle HND at BUF.  If ENDP is non-NULL, point it at next byte
  to be filled.
*/
  
static void
_sftp_put_handle(char *buf, char **endp, struct handle *hnd)
{
    _sftp_put_string(buf, endp, hnd->str, hnd->len, 0);
}



static int
_sftp_read(int fd, void *buf, size_t nbytes, int flags)
{
    int n, nret;
    fd_set fdset;

    nret = 0;
    while (nbytes) {
	n = read(fd, ((char *)buf)+nret, nbytes);

	if (n < 0) {
	    if (errno == EINTR) {
		if (flags & SFTP_FL_NONBLOCK)
		    return nret;
		else
		    continue;
	    }
	    else if (errno == EAGAIN) {
		FD_ZERO(&fdset);
		FD_SET(fd, &fdset);
		
		if (select(fd+1, &fdset, NULL, NULL, NULL) == -1
		    || !FD_ISSET(fd, &fdset)) {
		    if (errno == EINTR) {
			if (flags & SFTP_FL_NONBLOCK)
			    return nret;
			else
			    continue;
		    }
		    
		    if (nret)
			return nret;
		    else
			return -1;
		}
		continue;
	    }
	    return -1;
	}
	
	nbytes -= n;
	nret += n;
	if (flags & SFTP_FL_NONBLOCK)
	    return nret;
    }

    return nret;
}



static int
_sftp_writev(int fd, struct iovec *iov, int niov, int flags)
{
    int n, nret;
    fd_set fdset;

    nret = 0;
    while (niov) {
	n = writev(fd, iov, niov);

	if (n < 0) {
	    if (errno == EINTR) {
		if (flags & SFTP_FL_NONBLOCK)
		    return nret;
		else
		    continue;
	    }
	    else if (errno == EAGAIN) {
		FD_ZERO(&fdset);
		FD_SET(fd, &fdset);
		
		if (select(fd+1, NULL, &fdset, NULL, NULL) == -1
		    || !FD_ISSET(fd, &fdset)) {
		    if (errno == EINTR) {
			if (flags & SFTP_FL_NONBLOCK)
			    return nret;
			else
			    continue;
		    }
		    if (nret)
			return nret;
		    else
			return -1;
		}
		continue;
	    }
	    return -1;
	}

	nret += n;
	while (n) {
	    if (n >= iov[0].iov_len) {
		n -= iov[0].iov_len;
		iov++;
		--niov;
	    }
	    else {
		iov[0].iov_len -= n;
		iov[0].iov_base = ((char *)iov[0].iov_base) + n;
		n = 0;
	    }
	}
	if (flags & SFTP_FL_NONBLOCK)
	    return nret;
    }

    return nret;
}



/*
  Start write cycle: prepare write packet for LEN bytes, initialize members of F, and put F in state send.
*/

static int
_sftp_start_write(struct sftp_file *f, int len)
{
    char *p;
    
    p = _sftp_packet->dat;
    _sftp_put_handle(p, &p, f->hnd);
    _sftp_put_uint64(p, &p, f->off);
    f->woff = p-_sftp_packet->dat;
    _sftp_put_uint32(p, &p, len);

    f->state = SFTP_FS_SEND;
    f->doff = p-_sftp_buffer;
    p += len;
    f->dend = p-_sftp_buffer;

    _sftp_make_packet(_sftp_packet, SSH_FXP_WRITE, p);
    _sftp_packet->id = _sftp_nextid++;

    return 0;
}



static void
_sftp_make_packet(struct packet *pkt, int type, char *end)
{
    pkt->type = type;
    pkt->id = -1;
    pkt->pn = 0;
    pkt->plen = end - pkt->dat;
}



static void
_sftp_send_error_packet(char *fmt, ...)
{
    char *p, *q;
    va_list argp;

    p = _sftp_packet->dat;
    _sftp_put_uint32(p, &p, SSH_FX_NO_CONNECTION);
    q = p+4;
    va_start(argp, fmt);
    vsprintf(q, fmt, argp);
    va_end(argp);
    q += strlen(q);
    _sftp_put_uint32(p, NULL, q-(p+4));
    _sftp_make_packet(_sftp_packet, SSH_FXP_STATUS, q);
    _sftp_put_packet(_sftp_packet, 0);
}



static void
_sftp_sig_child(int sig)
{
    int status;

    if (_sftp_ssh_pid != 0
	&& waitpid(_sftp_ssh_pid, &status, WNOHANG) == _sftp_ssh_pid) {
	_sftp_close(1, status);
    }
}

#endif /* USE_SFTP */
