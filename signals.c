/*
  $NiH$

  signals -- signal handling functions
  Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001 Dieter Baron

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



#include <signal.h>
#include "signals.h"
#include "display.h"

volatile int sig_intr, sig_pipe, sig_alarm;



int
signals_init(void)
{
    sig_intr = sig_pipe = sig_alarm = 0;

    signal(SIGINT, sig_end);
    signal(SIGHUP, sig_end);
    signal(SIGTERM, sig_end);
    signal(SIGTSTP, sig_escape);
    signal(SIGCONT, sig_reenter);
    signal(SIGALRM, sig_remember);

    return 0;
}



void
sig_end(int i)
{
    exit_disp();
    exit(1);
}

void
sig_escape(int i)
{
    escape_disp(0);
    kill(0, SIGSTOP);
}

void
sig_reenter(int i)
{
    reenter_disp();
}



void
sig_remember(int i)
{
    switch (i) {
    case SIGPIPE:
	sig_pipe++;
	break;
    case SIGINT:
	sig_intr++;
	break;
    case SIGALRM:
	sig_alarm++;
    }
    signal(i, sig_remember);
}
	

