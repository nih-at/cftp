@ low level tty handling stuff.

@u
#include <stdio.h>
#include <errno.h>
#include <termios.h>
#include "keys.h"
#include "tty.h"

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
extern char *cap_cm, *cap_cl, *cap_cd, *cap_al, *cap_dl, *cap_ho,
	    *cap_ce, *cap_so, *cap_se;

@d<local globals@>
int tty_cols, tty_lines, tty_metap, tty_noLP, tty_am,
    tty_verase, tty_vwerase, tty_vkill;

@ initializing termcap.

@d<local globals@>
char termcap_entry[4196];
char termcap_area[4196], *termcap_str;
struct termios tty_tio, tty_tio_ext;
speed_t tty_baud;
char *cap_ke, *cap_ks, *cap_mm, *cap_mo;
char PC, *BC, *UP;
short ospeed;

char *cap_cm, *cap_cl, *cap_cd, *cap_al, *cap_dl, *cap_ho,
     *cap_ce, *cap_so, *cap_se;

@d<prototypes@>
int tty_init(void);

@u
int
tty_init(void)
{
	char *term, *pc;

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
	if (tty_metap) {
		cap_mm = tty_getcap("mm");
		cap_mo = tty_getcap("mo");
	}
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
	cap_cm = tty_getcap("cm");
	cap_cl = tty_getcap("cl");
	cap_cd = tty_getcap("cd");
	cap_al = tty_getcap("al");
	cap_dl = tty_getcap("dl");
	cap_ho = tty_getcap("ho");
	cap_ce = tty_getcap("ce");
	cap_so = tty_getcap("so");
	cap_se = tty_getcap("se");

	/* screen size */
	tty_cols = tgetnum("co");
	tty_lines = tgetnum("li");

	/* erase, werase, kill */
	tty_verase = tty_tio.c_cc[VERASE];
	tty_vwerase = tty_tio.c_cc[VWERASE];
	tty_vkill = tty_tio.c_cc[VKILL];
	
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

	tty_clear();
	fflush(stdout);

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
	
	cap_ks = tty_getcap("ks");
	cap_ke = tty_getcap("ke");

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
	if (cap_ks)
		tty_puts(cap_ks, 1);
	if (cap_mm)
		tty_puts(cap_mm, 1);

	return tcsetattr(0, TCSANOW, &tty_tio);
}

int
tty_restore(void)
{
	if (cap_ke)
		tty_puts(cap_ke, 1);
	if (cap_mo)
		tty_puts(cap_mo, 1);

	return tcsetattr(0, TCSANOW, &tty_tio_ext);
}


@ outputing stuff.

@d<prototypes@>
int fputchar();

#define tty_puts(s, l)	(tputs((s), (l), fputchar))
#define tty_goto(x, y)	(tty_puts(tgoto(cap_cm, (x), (y)), 1))
#define tty_clear()	(tty_puts(cap_cl, tty_lines))
#define tty_clreos(l)	(tty_puts(cap_cd, (l)))
#define tty_insline(l)	(tty_puts(cap_al, (l)))
#define tty_delline(l)	(tty_puts(cap_dl, (l)))
#define tty_home()	(tty_puts(cap_ho, 1))
#define tty_clreol()	(tty_puts(cap_ce, 1))
#define tty_standout()	(tty_puts(cap_so, 1))
#define tty_standend()	(tty_puts(cap_se, 1))

@d<local prototypes@>
#define tty_getcap(s)	(tgetstr((s), &termcap_str))


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
	char c;
	char s[128];
	int i, j;

	c = getchar();

	if (keyflag[c] == KEY_PREF) {
		tty_vmin(0, 5);
		s[0] = c;
		i = 1;
		while (1) {
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
