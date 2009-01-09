#ifndef HAD_TTY_H
#define HAD_TTY_H

/*
  $NiH: tty.h,v 1.16 2001/12/11 14:37:44 dillo Exp $

  tty.h -- lowlevel tty handling
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



enum {
    _TTY_cl, _TTY_ho, _TTY_cd, _TTY_ce, _TTY_so, _TTY_se, _TTY_vi,
    _TTY_ve, _TTY_cs, _TTY_sf, _TTY_sr, _TTY_SF, _TTY_SR,
    _TTY_ll,
    _TTY_al, _TTY_dl, _TTY_AL, _TTY_DL   
};
extern char *_tty_caps[];

#define TTY_CAP(cap)	(_tty_caps[(_TTY_##cap)])

#define tty_clear()	(tty_put0(TTY_CAP(cl), tty_lines))
#define tty_home()	(tty_put0(TTY_CAP(ho), 1))
#define tty_clreos(l)	(tty_put0(TTY_CAP(cd), (l)))
#define tty_clreol()	(tty_put0(TTY_CAP(ce), 1))
#define tty_standout()	(tty_put0(TTY_CAP(so), 1))
#define tty_standend()	(tty_put0(TTY_CAP(se), 1))
#define tty_hidecrsr()	(tty_put0(TTY_CAP(vi), 1))
#define tty_showcrsr()	(tty_put0(TTY_CAP(ve), 1))

void tty_lowleft(void);

#define tty_scregion(s, e)	(tty_putp(TTY_CAP(cs), 1, s, e, 0, 0))

#define tty_scrollup(n, l)	(tty_parp(TTY_CAP(sf), TTY_CAP(SF), (n), (l)))
#define tty_scrolldown(n, l)	(tty_parp(TTY_CAP(sr), TTY_CAP(SR), (n), (l)))
#define tty_inslines(n, l)	(tty_parp(TTY_CAP(al), TTY_CAP(AL), (n), (l)))
#define tty_dellines(n, l)	(tty_parp(TTY_CAP(dl), TTY_CAP(DL), (n), (l)))



extern volatile int tty_cols, tty_lines;

enum tty_am { TTY_AMNONE, TTY_AMXN, TTY_AM };

extern enum tty_am tty_am;

extern int tty_metap, tty_noLP,
	   tty_verase, tty_vwerase, tty_vkill;

extern void (*tty_redraw)(void);



int tty_init(void);
int tty_setup(void);
int tty_restore(void);
void tty_put(char *name, int lines);
void tty_put0(char *cap, int lines);
void tty_goto(int x, int y);
int tty_noecho(void);
int tty_echo(void);
int tty_cbreak(void);
int tty_readkey(void);
int tty_vmin(int min, int tim);
char *tty_getcap(char *name);
int tty_iscap(char *name);
void tty_putp(char *cap, int line, int arg0, int arg1, int arg2, int arg3);
void tty_parp(char *cap, char *pcap, int n, int pad);

#endif /* tty.h */
