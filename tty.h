#ifndef HAD_TTY_H
#define HAD_TTY_H

/*
  tty.h -- lowlevel tty handling
  Copyright (C) 1996, 1997 Dieter Baron

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



enum {
    _TTY_cl, _TTY_ho, _TTY_cd, _TTY_ce, _TTY_so, _TTY_se, _TTY_vi,
    _TTY_ve, _TTY_cs, _TTY_sf, _TTY_sr, _TTY_SF, _TTY_SR,
    _TTY_ll,
    _TTY_al, _TTY_dl, _TTY_AL, _TTY_DL   
};
extern char *_tty_caps[];
extern int fputchar();

#define TTY_CAP(cap)	(_tty_caps[(_TTY_##cap)])
#define tty_put0(cap, lines)  \
		(tputs(TTY_CAP(cap), (lines), fputchar))

#define tty_clear()	(tty_put0(cl, tty_lines))
#define tty_home()	(tty_put0(ho, 1))
#define tty_lowleft()	(tty_put0(ll, 1))
#define tty_clreos(l)	(tty_put0(cd, (l)))
#define tty_clreol()	(tty_put0(ce, 1))
#define tty_standout()	(tty_put0(so, 1))
#define tty_standend()	(tty_put0(se, 1))
#define tty_hidecrsr()	(tty_put0(vi, 1))
#define tty_showcrsr()	(tty_put0(ve, 1))

#define tty_scregion(s, e)	(tty_putp(TTY_CAP(cs), 1, s, e, 0, 0))

#define tty_scrollup(n, l)	(tty_parp(TTY_CAP(sf), TTY_CAP(SF), (n), (l)))
#define tty_scrolldown(n, l)	(tty_parp(TTY_CAP(sr), TTY_CAP(SR), (n), (l)))
#define tty_inslines(n, l)	(tty_parp(TTY_CAP(al), TTY_CAP(AL), (n), (l)))
#define tty_dellines(n, l)	(tty_parp(TTY_CAP(dl), TTY_CAP(DL), (n), (l)))



extern volatile int tty_cols, tty_lines;

enum tty_am { TTY_AMNONE, TTY_AMXN, TTY_AM };

enum tty_am tty_am;

extern int tty_metap, tty_noLP,
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
void tty_putp(char *cap, int line, int arg0, int arg1, int arg2, int arg3);

#endif /* tty.h */
