/*
  $NiH$

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



#define SFTP_MAX_PACKET_SIZE	4102 /* max read size + SSH_FXP_DATA header */

static int _sftp_get_packet(FILE *f, char *buf, int *lenp)
static char *_sftp_strerror(int error);



/* packet types */

#define SSH_FXP_INIT                1
#define SSH_FXP_VERSION             2
#define SSH_FXP_OPEN                3
#define SSH_FXP_CLOSE               4
#define SSH_FXP_READ                5
#define SSH_FXP_WRITE               6
#define SSH_FXP_LSTAT               7
#define SSH_FXP_FSTAT               8
#define SSH_FXP_SETSTAT             9
#define SSH_FXP_FSETSTAT           10
#define SSH_FXP_OPENDIR            11
#define SSH_FXP_READDIR            12
#define SSH_FXP_REMOVE             13
#define SSH_FXP_MKDIR              14
#define SSH_FXP_RMDIR              15
#define SSH_FXP_REALPATH           16
#define SSH_FXP_STAT               17
#define SSH_FXP_RENAME             18
#define SSH_FXP_STATUS            101
#define SSH_FXP_HANDLE            102
#define SSH_FXP_DATA              103
#define SSH_FXP_NAME              104
#define SSH_FXP_ATTRS             105
#define SSH_FXP_EXTENDED          200
#define SSH_FXP_EXTENDED_REPLY    201

/* attribute flag bits */

#define SSH_FILEXFER_ATTR_SIZE          0x00000001
#define SSH_FILEXFER_ATTR_UIDGID        0x00000002
#define SSH_FILEXFER_ATTR_PERMISSIONS   0x00000004
#define SSH_FILEXFER_ATTR_ACMODTIME     0x00000008
#define SSH_FILEXFER_ATTR_EXTENDED      0x80000000

/* open pflags bits */

#define SSH_FXF_READ            0x00000001
#define SSH_FXF_WRITE           0x00000002
#define SSH_FXF_APPEND          0x00000004
#define SSH_FXF_CREAT           0x00000008
#define SSH_FXF_TRUNC           0x00000010
#define SSH_FXF_EXCL            0x00000020

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

static const char _sftp_errlist[] = {
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

static const int _sftp_nerr = sizeof(_sftp_errlist) / sizeof(_sftp_errlist[0]);



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
_sftp_get_packet(FILE *f, char *buf, int *lenp)
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

    if (fread(buf, len-1, 1, f) != len-1)
	return -1;

    if (lenp)
	*lenp = len;

    return type;
}
