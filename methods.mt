dnl -*- text -*-
dnl
dnl  $NiH$
dnl
dnl  methods.mt -- definition and documentation for protocol methods
dnl  Copyright (C) 2001 Dieter Baron
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

rcsid(<<$NiH$>>)

method(int, cat, <<void *fin, void *fout, long start, long size, int upload>>,
  <<transfer file from FIN to FOUT.  If UPLOAD, FIN is a FILE * and
FOUT has been obtained from stor, otherwise FOUT is a FILE * and FIN
has been obtained from retr.  START is the starting offset and SIZE is
the expecded file size if known, -1 otherwise.>>)

method(int, close, void,
  <<close connection to server.>>)

method(int, cwd, <<char *path>>,
  <<change directory to PATH.>>)

method(int, deidle, void,
  <<deidle connection to server.>>)

method(int, fclose, <<void *f>>,
  <<close the file F previously opend with retr or stor.>>)

method(<<directory *>>, list, <<char *dir>>,
  <<return directory listing of DIR>>)

method(int, open, <<char *host, char *port, char *user, char *pass>>,
  <<open connection to server.  If HOST and PORT are NULL, reopen
connection to last server.  If separate actions for login are
neccessary, don't do them now (just remember USER and PASS).>>)

method(int, mkdir, <<char *dir>>,
  <<make directory DIR on server>>)

method(int, login, <<char *user, char *pass>>,
  <login as USER (with password PASS).   If USER is NULL, use previous
user.>>)

method(<<char *>>, pwd, void,
  <<return current directory>>)

method(<<void *>>, retr, <<char *file, int mode, long *startp, long *sizep>>,
  <<start retrieval of FILE in mode MODE, starting at offset *STARTP,
if possible.  The real starting offset is returned in *STARTP.  The
file length, if known, is returned in *SIZEP.>>)

method(int, rmdir, <<char *dir>>,
  <<remove directory DIR on server>>)

method(int, site, <<char *cmd>>,
  <<execute site specific command>>)

method(<<void *>>, stor, <<char *file, int mode>>,
  <<start storing FILE in mode MODE.>>)

endall
