@ low level tty handling stuff.

@u
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <termios.h>
#include "keys.h"
#include "tty.h"

#ifndef VWERASE
#define VWERASE	VWERSE
#endif
#ifndef _POSIX_VDISABLED
#define _POSIX_VDISABLED -1
#endif

extern char *prg;

@<local prototypes@>
@<local globals@>

@(tty.h@)
@<prototypes@>
@<globals@>


@ terminal characteristics needed by other parts of the program.

@d<globals@>
extern int tty_cols, tty_lines, tty_metap, tty_noLP, tty_am,
	   tty_verase, tty_vwerase, tty_vkill;

@d<local globals@>
int tty_cols, tty_lines, tty_metap, tty_noLP, tty_am,
    tty_verase, tty_vwerase, tty_vkill;

@ initializing termcap.

@d<local globals@>
char termcap_entry[4196];
char termcap_area[4196], *termcap_str;
struct termios tty_tio, tty_tio_ext;
speed_t tty_baud;
char PC, *BC, *UP;
short ospeed;


@d<prototypes@>
int tty_init(void);

@u
int
tty_init(void)
{
	char *term, *pc;
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
		fprintf(stderr, "%s: can't get terminal attribues: %s\n",
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
	tty_am = tgetflag("am");
	tty_noLP = !tgetflag("LP");

	/* screen size */
	tty_cols = tgetnum("co");
	tty_lines = tgetnum("li");
#ifdef SIGWINCH
	if (ioctl(0, TIOCGWINSZ, &ws) == 0) {
	    tty_cols = ws.ws_col;
	    tty_lines = ws.ws_row;
	}
	signal(SIGWINCH, tty_winch);
#endif

	/* erase, werase, kill */
	if ((tty_verase=tty_tio.c_cc[VERASE]) == _POSIX_VDISABLED)
	    tty_verase = -1;
	if ((tty_vwerase=tty_tio.c_cc[VWERASE]) == _POSIX_VDISABLED)
	    tty_vwerase = -1;
	if ((tty_vkill=tty_tio.c_cc[VKILL]) == _POSIX_VDISABLED)
	    tty_vkill = -1;
	
	/* input mode (cbreak, noecho) */
	tty_tio.c_cc[VMIN] = 1;
	tty_tio.c_cc[VTIME] = 0;
	tty_tio.c_lflag &= ~(ECHO|ICANON|ECHONL);
	tty_tio.c_lflag |= ISIG;
	
	if (tcsetattr(0, TCSANOW, &tty_tio_ext) < 0) {
		fprintf(stderr, "%s: can't set terminal attribues: %s\n",
			prg, strerror(errno));
		return -1;
	}

	return 0;
}


@ handling function keys.

@d<local globals@>
#define KEY_PREF	1
#define KEY_SLPREF	2
char keyflag[256];

@d<local prototypes@>
void tty_keypad_init(void);

@u
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
			keyflag[seq[0]] = KEY_PREF;
		}
	}
}


@ setting up and restoring tty.

@d<prototypes@>
int tty_setup(void);
int tty_restore(void);

@u
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


@ outputing stuff.

@d<prototypes@>
void tty_put(char *name, int lines);
void tty_goto(int x, int y);

@d<local prototypes@>
int fputchar();

@u
void
tty_put(char *name, int lines)
{
    tputs(tty_getcap(name), lines, fputchar);
}

void
tty_goto(int x, int y)
{
    tputs(tgoto(tty_getcap("cm"), x, y), 1, fputchar);
}

@d<prototypes@>
#define tty_clear()	(tty_put("cl", tty_lines))
#define tty_home()	(tty_put("ho", 1))
#define tty_clreos(l)	(tty_put("cd", (l)))
#define tty_clreol()	(tty_put("ce", 1))
#define tty_insline(l)	(tty_put("al", (l)))
#define tty_delline(l)	(tty_put("dl", (l)))
#define tty_standout()	(tty_put("so", 1))
#define tty_standend()	(tty_put("se", 1))
#define tty_hidecrsr()	(tty_put("vi", 1))
#define tty_showcrsr()	(tty_put("vo", 1))


@ turning echoing on and off

@d<prototypes@>
int tty_noecho(void);
int tty_echo(void);

@u
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


@ put tty in cbreak

@d<prototypes@>
int tty_cbreak(void);

@u
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


@ read a key (handling multi byte keys ala cursor and function keys)

@d<prototypes@>
int tty_readkey(void);

@u
int
tty_readkey(void)
{
	int c;
	char s[128];
	int i, j;

	while ((c=getchar())==EOF && errno == EINTR)
	    ;
	if (c == EOF)
	    return -1;

	if (keyflag[c] == KEY_PREF) {
		tty_vmin(0, 5);
		s[0] = c;
		i = 1;
		while (1) {
		    while ((c=getchar())==EOF && errno == EINTR)
			;
			if ((c=getchar()) == EOF) {
				tty_vmin(1, 0);
				clearerr(stdin);
				return s[0];
			}
			s[i++] = c;
			s[i] = '\0';
			for (j=0; j<max_fnkey; j++)
				if (fnkey[j].seq &&
				    strcmp(s, fnkey[j].seq) == 0) {
					tty_vmin(1, 0);
					return 256+j;
				}
		}
	}

	return c;
}
			

@ setting VMIN and VTIME.

@d<prototypes@>
int tty_vmin(int min, int tim);

@u
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


@ managing capability strings.

@d<local globals@>
struct cap {
    char *name;
    char *cap;
};

#define CAP_SIZE	253
#define CAP_G		17
struct cap cap[253];

@d<prototypes@>
char *tty_getcap(char *name);

@u
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


@d<prototypes@>
int tty_iscap(char *name);

@u
int
tty_iscap(char *name)
{
    return (tgetflag(name) == 1);
}


@ handling screen size changes

@d<local prototypes@>
#ifdef SIGWINCH
void tty_winch(int s);
#endif

@d<globals@>
extern void (*tty_redraw)(void);

@d<local globals@>
void (*tty_redraw)(void) = NULL;

@u
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
