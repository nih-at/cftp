#ifndef HAD_TTY_H
#define HAD_TTY_H

/*
  tty.h -- lowlevel tty handling
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



#define tty_clear()	(tty_put("cl", tty_lines))
#define tty_home()	(tty_put("ho", 1))
#define tty_clreos(l)	(tty_put("cd", (l)))
#define tty_clreol()	(tty_put("ce", 1))
#define tty_insline(l)	(tty_put("al", (l)))
#define tty_delline(l)	(tty_put("dl", (l)))
#define tty_standout()	(tty_put("so", 1))
#define tty_standend()	(tty_put("se", 1))
#define tty_hidecrsr()	(tty_put("vi", 1))
#define tty_showcrsr()	(tty_put("ve", 1))



extern int tty_cols, tty_lines, tty_metap, tty_noLP, tty_am,
	   tty_verase, tty_vwerase, tty_vkill;

extern void (*tty_redraw)(void);



int tty_init(void);
int tty_setup(void);
int tty_restore(void);
void tty_put(char *name, int lines);
void tty_goto(int x, int y);
int tty_noecho(void);
int tty_echo(void);
int tty_cbreak(void);
int tty_readkey(void);
int tty_vmin(int min, int tim);
char *tty_getcap(char *name);
int tty_iscap(char *name);

#endif /* tty.h */
