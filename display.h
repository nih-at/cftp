#ifndef HAD_DISPLAY_H
#define HAD_DISPLAY_H

/*
  $NiH: display.h,v 1.17 2002/09/16 12:42:30 dillo Exp $

  display.h -- display functions
  Copyright (C) 1996-2002 Dieter Baron

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

extern volatile int tty_lines;
/* first line of scrolling region */
#define win_top		(0)
/* last line of scrolling region */
#define win_bottom	(tty_lines-3)
/* number of lines in scrolling region */
#define win_lines	(tty_lines-2)

extern int disp_quiet;
extern int disp_active;



#define DISP_STATUS	0x01	/* display in status line */
#define DISP_HIST	0x02	/* add to history */
#define DISP_STDOUT	0x04	/* print to stdout if display not started */
#define DISP_STDERR	0x08	/* print to stderr if display not started */

#define DISP_INFO	DISP_STATUS|DISP_STDOUT
				/* for (startup) info messages */
#define DISP_ERROR	DISP_STATUS|DISP_HIST|DISP_STDERR
				/* for error messages */
#define DISP_PROTO	DISP_STATUS|DISP_HIST
				/* for responses to / requests from server */



void disp_beep(void);
int disp_close(FILE *f, int quietp);
void disp_head(char *fmt, ...);
FILE *disp_open(char *cmd, int quietp);
int disp_prompt_char(void);
void disp_redraw(void);
void disp_reline(int line);
void disp_restat(void);
void disp_status(int flags, char *fmt, ...);
void escape_disp(int clearp);
void exit_disp(void);
int init_disp(void);
int read_char(char *prompt);
char *read_string(char *prompt, int echop);
void reenter_disp(void);

#endif /* display.h */
