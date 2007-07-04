#ifndef HAD_DISPLAY_H
#define HAD_DISPLAY_H

/*
  $NiH: display.h,v 1.17 2002/09/16 12:42:30 dillo Exp $

  display.h -- display functions
  Copyright (C) 1996-2002 Dieter Baron

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
