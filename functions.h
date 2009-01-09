#ifndef HAD_FUNCTIONS_H
#define HAD_FUNCTIONS_H

/*
  $NiH: functions.h,v 1.14 2005/06/03 10:46:19 dillo Exp $

  functions.h -- auxiliary functions for bindable function handling
  Copyright (C) 1996-2002, 2005 Dieter Baron

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



#include "config.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

struct function {
	void (*fn)(char **args);
	char *name;
	int type;
	char *help;
};

typedef struct function function;

extern function functions[];

#define FN_PRE	1
#define FN_RC   2
#define FN_EXIT	-1




void void_prefix(void);
int get_prefix(int deflt);
int get_prefix_args(char **args, int deflt);
void add_prefix(int n);
void set_prefix(int p);
void negate_prefix(void);
void show_prefix(void);
int find_function(char *f);
struct binding *get_function(int key, enum state state);

void aux_scroll(int top, int sel, int force);
int aux_enter(char *name);
int aux_download(char *name, off_t size, int restart);
int aux_pipe(char *name, off_t size, int mode, char *cmd, int quietp);
#define aux_view(name)	(aux_pipe((name), -1, 'a', opt_pager, 1))

#endif /* functions.h */
