/*
  $NiH: keys.c,v 1.10 2001/12/11 14:37:34 dillo Exp $

  keys.c -- parse and print key specifications
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
#include <string.h>
#include <ctype.h>
#include "keys.h"

struct keyname key_name[] = {
    { 8, "bs", "backspace" },
    { 9, "tab", NULL },
    { 13, "ret", "return" },
    { 27, "esc", "escape" },
    { 32, "spc", "space" },
    { 127, "del", "delete" },
    { 256,  NULL, NULL }
};

struct fnkey fnkey[] = {
    { "left", "cursor left", "kl", NULL },
    { "right", "cursor right", "kr", NULL },
    { "up", "cursor up", "ku", NULL },
    { "down", "cursor down", "kd", NULL },
    { "home", "home", "kh", NULL },
    { "num7", "number block 7", "K1", NULL },
    { "num9", "number block 9", "K2", NULL },
    { "num5", "number block 5", "K3", NULL },
    { "num1", "number block 1", "K4", NULL },
    { "num3", "number block 3", "K5", NULL },
    { "f10", "function key 10 (or 0)", "k0", NULL },
    { "f1", "function key 1", "k1", NULL },
    { "f2", "function key 2", "k2", NULL },
    { "f3", "function key 3", "k3", NULL },
    { "f4", "function key 4", "k4", NULL },
    { "f5", "function key 5", "k5", NULL },
    { "f6", "function key 6", "k6", NULL },
    { "f7", "function key 7", "k7", NULL },
    { "f8", "function key 8", "k8", NULL },
    { "f9", "function key 9", "k9", NULL },
    { "clratab", "clear all tabs", "ka", NULL },
    { "clrtab", "clear tab this column", "kt", NULL },
    { "clr", "clear screen", "kC", NULL },
    { "delline", "delete line", "kL", NULL },
    { "exins", "exit insert mode", "kM", NULL },
    { "clreol", "clear to end of line", "kE", NULL },
    { "clreos", "clear to end of screen", "kS", NULL },
    { "ins", "insert", "kI", NULL },
    { "insline", "insert line", "kA", NULL },
    { "pgdown", "next page", "kN", NULL },
    { "pgup", "previous page", "kP", NULL },
    { "scrup", "scroll forward", "kF", NULL },
    { "scrdown", "scroll reverse", "kR", NULL },
    { "settab", "set tab stop this column", "kT", NULL }
};

int max_fnkey = sizeof(fnkey)/sizeof(struct fnkey);



char *
print_key(int key, int longp)
{
    int i;
    static char buf[64];
	
    if (key < 256) {
	for (i=0; (key&127)>key_name[i].key; i++)
	    ;
	if ((key&127) == key_name[i].key) {
	    if (key > 128) {
		sprintf(buf, "%s%s",
			(longp ? "meta " : "M-"),
			((longp && key_name[i].longname) ? 
			 key_name[i].longname
			 : key_name[i].name));
		return buf;
	    }
	    else
		return ((longp && key_name[i].longname) ? 
			key_name[i].longname
			: key_name[i].name);
	}
	if ((key&127) < 32) {
	    sprintf(buf, "%s%s%c",
		    (key > 128 ? (longp ?
				  "meta " : "M-") :
		     ""),
		    (longp ? "control " : "C-"),
		    tolower((key&127)+96));
	    return buf;
	}
	else {
	    sprintf(buf, "%s%c",
		    (key > 128 ? (longp ?
				  "meta " : "M-") :
		     ""),
		    key&127);
	    return buf;
	}
    }
    else
	return ((longp && fnkey[key-256].longname) ?
		fnkey[key-256].longname : fnkey[key-256].name);
}



int
parse_key(char *kn)
{
    int k, i;
	
    if (strlen(kn) == 1)
	return kn[0];

    i = 0;
    if (tolower(kn[0]) == 'c' && kn[1] == '-')
	i = 2;
    if (kn[0] == '^')
	i = 1;
    if (strncasecmp(kn, "control ", 8) == 0)
	i = 8;
    if (i) {
	if ((k=parse_key(kn+i)) > 256)
	    return -1;
	return (k & 0x9f);
    }

    i = 0;
    if (tolower(kn[0]) == 'm' && kn[1] == '-')
	i = 2;
    if (strncasecmp(kn, "meta ", 5) == 0)
	i = 5;
    if (i) {
	if ((k=parse_key(kn+i)) > 256)
	    return -1;
	return (k | 0x80);
    }

    for (i=0; key_name[i].key < 255; i++) {
	if (strcasecmp(kn, key_name[i].name) == 0
	    || (key_name[i].longname
		&& strcasecmp(kn, key_name[i].longname) == 0))
	    return key_name[i].key;
    }

    for (i=0; i<max_fnkey; i++) {
	if (strcasecmp(kn, fnkey[i].name) == 0
	    || (fnkey[i].longname
		&& strcasecmp(kn, fnkey[i].longname) == 0))
	    return i+256;
    }

    return -1;
}
