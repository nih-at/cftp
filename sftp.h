#ifndef HAD_FTP_H
#define HAD_FTP_H

/*
  $NiH: sftp.h,v 1.1 2001/12/11 20:23:21 dillo Exp $

  sftp.h -- sftp protocol functions
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



directory *sftp_list(char *path);
int sftp_mkdir(char *path);
int sftp_rmdir(char *path);

#endif /* ftp.h */
