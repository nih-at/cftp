#ifndef HAD_DISPLAY_H
#define HAD_DISPLAY_H

/*
  display.h -- display functions
  Copyright (C) 1996, 1997, 1998 Dieter Baron

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



#ifndef SWIG

#include <stdio.h>

extern volatile int tty_lines;
/* first line of scrolling region */
#define win_top		(0)
/* last line of scrolling region */
#define win_bottom	(tty_lines-3)
/* number of lines in scrolling region */
#define win_lines	(tty_lines-2)

extern int disp_quiet;
extern int disp_active;

#endif



#ifndef SWIG
int init_disp(void);
void exit_disp();

void disp_status(char *fmt, ...);
void disp_head(char *fmt, ...);
void disp_restat(void);
FILE *disp_open(char *cmd, int quietp);
int disp_close(FILE *f, int quietp);
#endif

void escape_disp(int clearp);
void reenter_disp(void);
void disp_redraw(void);
#ifdef SWIG
char *read_string(char *prompt, int echop=1);
#else
char *read_string(char *prompt, int echop);
#endif
int read_char(char *prompt);
int disp_prompt_char(void);
void disp_beep(void);

#endif /* display.h */
