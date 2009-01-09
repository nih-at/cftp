/*
  $NiH: signals.c,v 1.11 2002/09/16 12:42:42 dillo Exp $

  signals.c -- signal handling functions
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
#include <stdlib.h>

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
	

