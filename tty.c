/*
  $NiH: tty.c,v 1.23 2002/09/15 13:06:05 dillo Exp $

  tty.c -- lowlevel tty handling
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



#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <termios.h>
#include <sys/ioctl.h>

#include "config.h"
#ifdef HAVE_TERMCAP_H
#include <termcap.h>
#endif
#ifndef HAVE_DECL_OSPEED
char PC, *BC, *UP;
short ospeed;
#endif

#ifndef HAVE_FPUTCHAR
int fputchar(int c);
#endif

#if !defined(HAVE_TPARAM) && !defined(HAVE_TPARM)
char *tparam (char *string, char *outstring, int len,
	      int arg0, int arg1, int arg2, int arg3);
#endif

#include "keys.h"
#include "tty.h"

#ifndef VWERASE
#ifdef VWERSE
#define VWERASE	VWERSE
#endif
#endif
#ifndef _POSIX_VDISABLED
#define _POSIX_VDISABLED -1
#endif

extern char *prg;

void tty_keypad_init(void);
void _tty_capinit(void);
int tty_ispref(char *s, int l);

#ifdef SIGWINCH
void tty_winch(int s);
#endif

enum tty_am tty_am;
volatile int tty_cols, tty_lines;
int tty_metap, tty_noLP,
    tty_verase, tty_vwerase, tty_vkill;

char termcap_entry[4196];
char termcap_area[4196], *termcap_str;
struct termios tty_tio, tty_tio_ext;
speed_t tty_baud;

#define KEY_PREF	1
#define KEY_SLPREF	2
char keyflag[256];

struct cap {
    char *name;
    char *cap;
};

#define CAP_SIZE	253
#define CAP_G		17
struct cap cap[253];

void (*tty_redraw)(void) = NULL;



int
tty_init(void)
{
	char *term, *pc, *env;
#ifdef SIGWINCH
	struct winsize ws;
#endif
	
	/* get termcap entry */
	if ((term=getenv("TERM")) == NULL) {
		fprintf(stderr, "%s: no terminal type specified\n",
			prg);
		return -1;
	}
	switch (tgetent(termcap_entry, term)) {
	case -1:
		fprintf(stderr, "%s: can't access termcap: %s\n",
			prg, strerror(errno));
		return -1;
	case 0:
		fprintf(stderr, "%s: unknown terminal: %s\n",
			prg, term);
		return -1;
	}
	termcap_str = termcap_area;

	/* meta and function keys */
	tty_metap = tgetflag("km");
	tty_keypad_init();

	if (tcgetattr(0, &tty_tio_ext) < 0) {
		fprintf(stderr, "%s: can't get terminal attributes: %s\n",
			prg, strerror(errno));
		return -1;
	}
	tty_tio = tty_tio_ext;
	tty_baud = cfgetospeed(&tty_tio);

	/* globals for tputs(3) and tgoto */
	ospeed = tty_baud;
	pc = tty_getcap("pc");
	if (pc)
		PC = *pc;
	else
		PC = 0;
	BC = tty_getcap("le");
	UP = tty_getcap("up");

	/* other capabilities we need (mostly in the output macros) */
	if (tgetflag("am")) {
	    if (tgetflag("xn"))
		tty_am = TTY_AMXN;
	    else
		tty_am = TTY_AM;
	}
	else
	    tty_am = TTY_AMNONE;

	tty_noLP = !tgetflag("LP");

	/* screen size */
	tty_cols = tgetnum("co");
	tty_lines = tgetnum("li");
#ifdef SIGWINCH
	if (ioctl(0, TIOCGWINSZ, &ws) == 0) {
	    if (ws.ws_col && ws.ws_row) {
		tty_cols = ws.ws_col;
		tty_lines = ws.ws_row;
	    }
	}
	signal(SIGWINCH, tty_winch);
#endif
	if ((env=getenv("LINES")))
	    tty_lines = atoi(env);
	if ((env=getenv("COLUMNS")))
	    tty_cols = atoi(env);

	if (tty_lines == 0 && tty_cols == 0)
	    return -1;

	/* erase, werase, kill */
	if ((tty_verase=tty_tio.c_cc[VERASE]) == _POSIX_VDISABLED)
	    tty_verase = -1;
#ifdef VWERASE
	if ((tty_vwerase=tty_tio.c_cc[VWERASE]) == _POSIX_VDISABLED)
	    tty_vwerase = -1;
#else
	tty_vwerase = -1;
#endif
	if ((tty_vkill=tty_tio.c_cc[VKILL]) == _POSIX_VDISABLED)
	    tty_vkill = -1;
	
	/* input mode (cbreak, noecho) */
	tty_tio.c_cc[VMIN] = 1;
	tty_tio.c_cc[VTIME] = 0;
	tty_tio.c_lflag &= ~(ECHO|ICANON|ECHONL);
	tty_tio.c_iflag &= ~(IXON|IXOFF);
	tty_tio.c_lflag |= ISIG;
	
	if (tcsetattr(0, TCSANOW, &tty_tio_ext) < 0) {
		fprintf(stderr, "%s: can't set terminal attribues: %s\n",
			prg, strerror(errno));
		return -1;
	}

	_tty_capinit();

	return 0;
}



void
tty_keypad_init(void)
{
	int i;
	char *seq;
	
	for (i=0; i<256; i++)
		keyflag[0] = 0;
	for (i=0; i<max_fnkey; i++) {
		seq = tty_getcap(fnkey[i].cap);
		if (seq && strlen(seq) > 1) {
			fnkey[i].seq = seq;
			keyflag[(int)seq[0]] = KEY_PREF;
		}
	}
}



int
tty_setup(void)
{
    tty_put("ks", 1);
    tty_put("mm", 1);

    return tcsetattr(0, TCSANOW, &tty_tio);
}



int
tty_restore(void)
{
    tty_put("ke", 1);
    tty_put("mo", 1);
	
    return tcsetattr(0, TCSANOW, &tty_tio_ext);
}



void
tty_put(char *name, int lines)
{
    tputs(tty_getcap(name), lines, fputchar);
}



void
tty_put0(char *cap, int lines)
{
    tputs(cap, lines, fputchar);
}



void
tty_goto(int x, int y)
{
    tputs(tgoto(tty_getcap("cm"), x, y), 1, fputchar);
}



int
tty_echo(void)
{
	if ((tty_tio.c_lflag & ECHO) == 0) {
		tty_tio.c_lflag |= ECHO;
		
		if (tcsetattr(0, TCSANOW, &tty_tio) < 0) {
			fprintf(stderr, "%s: can't set terminal "
				"attribues: %s\n",
				prg, strerror(errno));
			return -1;
		}
	}

	return 0;
}



int
tty_noecho(void)
{
	if (tty_tio.c_lflag & ECHO) {
		tty_tio.c_lflag &= ~ECHO;
		
		if (tcsetattr(0, TCSANOW, &tty_tio) < 0) {
			fprintf(stderr, "%s: can't set terminal "
				"attribues: %s\n",
				prg, strerror(errno));
			return -1;
		}
	}

	return 0;
}



int tty_cbreak(void)
{
	tty_tio.c_lflag &= ~ECHO;
	
	if (tcsetattr(0, TCSANOW, &tty_tio_ext) < 0) {
		fprintf(stderr, "%s: can't set terminal "
			"attribues: %s\n",
			prg, strerror(errno));
		return -1;
	}

	return 0;
}



int
tty_readkey(void)
{
    static unsigned char s[128];
    static int l = 0;
    int c, len;
    int vmin = 0;

    if (l == 0) {
	while ((c=getchar())==EOF && errno == EINTR)
	    errno = 0;
	if (c == EOF)
	    return -1;
	s[l++] = c;
    }

    len = 0;
    
    if (keyflag[(int)s[0]] == KEY_PREF) {
	while ((c=tty_ispref((char *)s, l)) == EOF) {
 	    if (!vmin) {
		tty_vmin(0, 5);
		vmin = 1;
	    }

	    while ((c=getchar())==EOF && errno == EINTR)
			errno = 0;
	    if (c == EOF) {
		clearerr(stdin);
		c = s[0];
		break;
	    }
	    s[l++] = c;
	}
	if (c == -2) {
	    c = s[0];
	}

    }

    else {
	c = s[0];
	l = 1;
    }

    if (c > 256)
	len =(fnkey[c-256].seq ? strlen(fnkey[c-256].seq) : 1);
    else
	len = 1;
    
    if (vmin)
	tty_vmin(1, 0);
	
    l -= len;
    if (l)
	memmove(s, s+len, l);
    
    return c;
}
			


int tty_ispref(char *s, int l)
{
    int j;
    
    for (j=0; j<max_fnkey; j++)
	if (fnkey[j].seq) {
	    if (!strncmp(s, fnkey[j].seq, l) && fnkey[j].seq[l] == '\0')
		return 256+j;
	    else if (!strncmp(fnkey[j].seq, s, l))
		return EOF;
	}

    return -2;
}



int
tty_vmin(int min, int tim)
{
	if (tty_tio.c_cc[VMIN] != min || tty_tio.c_cc[VTIME] != tim) {
		tty_tio.c_cc[VMIN] = min;
		tty_tio.c_cc[VTIME] = tim;
		
		if (tcsetattr(0, TCSANOW, &tty_tio) < 0) {
			fprintf(stderr, "%s: can't set terminal "
				"attribues: %s\n",
				prg, strerror(errno));
			return -1;
		}
	}

	return 0;
}



char *
tty_getcap(char *name)
{
    int i, j;

    i = j = (name[0]*name[1]) % CAP_SIZE;

    while (cap[i].name) {
	if (strcmp(cap[i].name, name) == 0)
	    return cap[i].cap;
	i = (i+CAP_G) % CAP_SIZE;
	if (i == j)	/* cap array full! */
	    return NULL;
    }
    if ((cap[i].name=strdup(name)) == NULL)
	return NULL;
    cap[i].cap = (char *)tgetstr(name, &termcap_str);

    return cap[i].cap;
}



int
tty_iscap(char *name)
{
    return (tgetflag(name) == 1);
}



#ifdef SIGWINCH
void
tty_winch(int s)
{
    int change = 0;
    struct winsize ws;

    if (ioctl(0, TIOCGWINSZ, &ws) == 0) {
	if (tty_cols != ws.ws_col
	    || tty_lines != ws.ws_row)
	    change = 1;
	tty_cols = ws.ws_col;
	tty_lines = ws.ws_row;
    }
    if (change && tty_redraw)
	tty_redraw();
}
#endif



void
tty_parp(char *cap, char *pcap, int n, int pad)
{
    int i;
    
    if (*pcap)
	tty_putp(pcap, pad, n, 0, 0, 0);
    else
	for (i=0; i<n; i++)
	    tputs(cap, pad, fputchar);
}



char *_tty_capnames[] = {
    "cl", "ho", "cd", "ce", "so", "se", "vi", "ve", "cs",
    "sf", "sr", "SF", "SR", "ll", "al", "dl", "AL", "DL"
};

char *_tty_caps[sizeof(_tty_capnames)/sizeof(_tty_capnames[0])];

void
_tty_capinit()
{
    int i;

    for (i=0; i<sizeof(_tty_capnames)/sizeof(_tty_capnames[0]); i++) {
	_tty_caps[i] = tty_getcap(_tty_capnames[i]);
	if (_tty_caps[i] == NULL)
	    _tty_caps[i] = "";
    }

    /* screen removes `sf' capability, so we compensate */
    if (!*TTY_CAP(sf)) {
	TTY_CAP(sf) = tty_getcap("do");
	if (TTY_CAP(sf) == NULL)
	    TTY_CAP(sf) = "";
    }
}



void
tty_putp(char *cap, int lines, int arg0, int arg1, int arg2, int arg3)
{
#ifndef HAVE_TPARM
    static char buf[40];
#endif
    char *s;

#ifndef HAVE_TPARM
    s = (char *)tparam(cap, buf, sizeof(buf), arg0, arg1, arg2, arg3);
#else
    s = (char *)tparm(cap, arg0, arg1, arg2, arg3);
#endif
    
    tputs(s, lines, fputchar);

#ifndef HAVE_TPARM
    if (s != buf)
	free(s);
#endif
}



void
tty_lowleft(void)
{
    if (*TTY_CAP(ll))
	tty_put0(TTY_CAP(ll), 1);
    else
	tty_goto(0, tty_lines-1);
}
