#ifndef HAD_DISPLAY_H
#define HAD_DISPLAY_H

/*
  display.h -- display functions
  Copyright (C) 1996 Dieter Baron

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



#include <stdio.h>
#include "directory.h"

extern int tty_lines;
#define win_lines	(tty_lines-4)



int init_disp(void);
void exit_disp();
void escape_disp(int clearp);
void reenter_disp(void);
void disp_redraw(void);
void disp_dir(directory *d, int top, int sel, int newdir);
void disp_reline(int line);
char *read_string(char *prompt, int echop);
int read_char(char *prompt);
int disp_prompt_char(void);
void disp_status(char *fmt, ...);
void disp_head(char *fmt, ...);
FILE *disp_open(int lines);
int disp_close(FILE *f);

#endif /* display.h */
