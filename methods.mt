dnl -*- text -*-
dnl
dnl  $NiH: methods.mt,v 1.4 2002/09/16 12:42:37 dillo Exp $
dnl
dnl  methods.mt -- definition and documentation for protocol methods
dnl  Copyright (C) 2001, 2002 Dieter Baron
dnl
dnl  This file is part of cftp, a fullscreen ftp client
dnl  The author can be contacted at <dillo@giga.or.at>
dnl
dnl  This program is free software; you can redistribute it and/or modify
dnl  it under the terms of the GNU General Public License as published by
dnl  the Free Software Foundation; either version 2 of the License, or
dnl  (at your option) any later version.
dnl
dnl  This program is distributed in the hope that it will be useful,
dnl  but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl  GNU General Public License for more details.
dnl
dnl  You should have received a copy of the GNU General Public License
dnl  along with this program; if not, write to the Free Software
dnl  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

rcsid(<<$NiH: methods.mt,v 1.4 2002/09/16 12:42:37 dillo Exp $>>)

method(int, close, void,
  <<Close connection to server.>>)

method(int, cwd, <<char *path>>,
  <<Change directory to PATH.>>)

method(int, deidle, void,
  <<Deidle connection to server.>>)

method(int, fclose, <<void *f>>,
  <<Close the file F previously opend with retr or stor.>>)

method(<<directory *>>, list, <<char *dir>>,
  <<Return directory listing of DIR>>)

method(int, open, <<char *host, char *port, char *user, char *pass>>,
  <<Open connection to server.  If HOST and PORT are NULL, reopen
connection to last server.  If separate actions for login are
neccessary, don't do them now (just remember USER and PASS).>>)

method(int, mkdir, <<char *dir>>,
  <<Make directory DIR on server.>>)

method(int, login, <<char *user, char *pass>>,
  <Login as USER (with password PASS).   If USER is NULL, use previous
user.>>)

method(<<char *>>, pwd, void,
  <<Return current directory.>>)

method(<<void *>>, retr, <<char *file, int mode, off_t *startp, off_t *sizep>>,
  <<Start retrieval of FILE in mode MODE, starting at offset *STARTP,
if possible.  The real starting offset is returned in *STARTP, the
file length, if known, is returned in *SIZEP.>>)

method(int, rmdir, <<char *dir>>,
  <<Remove directory DIR on server.>>)

method(int, site, <<char *cmd>>,
  <<Execute site specific command.>>)

method(<<void *>>, stor, <<char *file, int mode>>,
  <<Start storing FILE in mode MODE.>>)

method(int, xfer_eof, <<void *file>>,
  <<Return true if end of file is reached on FILE.>>)

method(int, xfer_read, <<void *buf, size_t n, void *file>>,
  <<Read N bytes into BUF from FILE.  Returns the number of bytes
read, or -1 on error or end of file.  This method must be
interruptible by signals.>>)

method(int, xfer_start, <<void *file>>,
  <<Start transfer on file.>>)

method(int, xfer_stop, <<void *file, int aborting>>,
  <<Stop transfer on file.  If ABORTING, transfer might not have been
completed.>>)

method(int, xfer_write, <<void *buf, size_t n, void *file>>,
  <<Write N bytes from BUF to FILE.  Return the number of bytes
written, -1 if an error occured before writing the first byte.  This
method must be interruptible by signals.>>)

endall
