/*
  mkbind -- make binding table
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



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "keys.h"
#include "rc.h"
#include "bindings.h"
#include "functions.h"

#define FNAME	"bindings"
#define NAME	"binding"
#define POOL	"pool"
#define ARGS	"argpool"
#define TABLE	"fntable.c"

#define MAX_FN	8192
#define LINELEN 72

const char header[] = "/*\n\
   This file is automatically created from ``bindings.desc'' by mkbind;\n\
   don't make changes to this file, change ``bindings.desc'' instead.\n\
*/\n\
\n\
#include <stddef.h>\n\
#include \"bindings.h\"\n\n";

char *states[] = {
    "bs_quit", "bs_none", "bs_remote", "bs_local", "bs_tag"
};

int nstates = sizeof(states)/sizeof(states[0]);



function functions[MAX_FN];
int num;

enum state binding_state;
struct binding binding[MAX_FN];

char *binding_argpool[1];
int binding_nargpool = 0;

char *prg;

void print_args(FILE *fout, char **args);



int
main(int argc, char **argv)
{
    FILE *fin, *fout;
    char line[4069], *p, *tok, **args;
    struct binding *b;
    int i, j, off, argoff, len, wlen;
    int maxkey;

    prg = argv[0];
    rc_inrc = 1;
    rc_lineno = 0;
    maxkey = max_fnkey + 256;

    initnames();

    for (i=0; i<maxkey; i++) {
	binding[i].next = NULL;
	binding[i].state = bs_none;
	binding[i].fn = -1;
	binding[i].args = NULL;
    }
    off = 0;


    /* processing ``bindings.desc'' */

    if ((fin=fopen(FNAME ".desc", "r")) == NULL) {
	fprintf(stderr, "%s: can't open input file (%s): %s.\n",
		prg, FNAME ".desc", strerror(errno));
	exit(1);
    }

    while (fgets(line, 4096, fin) != NULL) {
	rc_lineno++;
	p = line;
	if ((tok=rc_token(&p)) == NULL)
	    continue;
	if (strcasecmp(tok, "bind") != 0) {
	    fprintf(stderr, "%s: non-bind command ignored: `%s'\n",
		    prg, line);
	    continue;
	}
	
	args = rc_list(p);

	fn_bind(args);
    }
    
    if (ferror(fin)) {
	fprintf(stderr, "%s: read error (%s): %s.\n",
		prg, FNAME ".desc", strerror(errno));
	fclose(fin);
	exit(1);
    }
    fclose(fin);



    /* writing ``bindings.c'' */

    if ((fout=fopen(FNAME ".c", "w")) == NULL) {
	fprintf(stderr, "%s: can't open output file (%s): %s.\n",
		prg, FNAME ".c", strerror(errno));
	exit(1);
    }

    fprintf(fout, "%s", header);


    /* output: bindings */

    off = argoff = 0;
    fprintf(fout, "struct binding " NAME "[] = {\n");
    for (i=0; i<maxkey; i++) {
	fprintf(fout, "    { ");
	if (binding[i].next) {
	    fprintf(fout, "binding_pool+%3d, ", off);
	    for (b=binding[i].next; b; b=b->next)
		off++;
	}
	else
	    fprintf(fout, "NULL            , ");
	if (binding[i].state+1 < nstates)
	    fprintf(fout, "%s, ", states[binding[i].state+1]);
	else
	    fprintf(fout, "%d, ", binding[i].state);
	fprintf(fout, "%3d, ", binding[i].fn);
	if (binding[i].args) {
	    fprintf(fout, "binding_argpool+%d },\n", argoff);
	    for (j=0; binding[i].args[j]; j++)
		;
		argoff += j+1;
	}
	else
	    fprintf(fout, "NULL },\n");
    }
    fprintf(fout, "};\n\n");

    if (ferror(fout)) {
	fprintf(stderr, "%s: write error (%s): %s.\n",
		prg, FNAME ".c", strerror(errno));
	exit(1);
	fclose(fout);
    }

    /* output: bindings_pool */

    off = 0;
    fprintf(fout, "struct binding " NAME "_" POOL "[] = {\n");
    for (i=0; i<maxkey; i++)
	if (binding[i].next) {
	    for (b=binding[i].next; b; b=b->next) {
		fprintf(fout, "    { ");
		if (b->next) {
		    fprintf(fout, "binding_pool+%3d, ", off++);
		}
		else
		    fprintf(fout, "NULL            , ");
		if (b->state+1 < nstates)
		    fprintf(fout, "%s, ", states[b->state+1]);
		else
		    fprintf(fout, "%d, ", b->state);
		fprintf(fout, "%3d, ", b->fn);
		if (b->args) {
		    fprintf(fout, "binding_argpool+%d },\n", argoff);
		    for (j=0; b->args[j]; j++)
			;
		    argoff += j+1;
		}
		else
		    fprintf(fout, "NULL },\n");
	    }
	}
    fprintf(fout, "};\n\n");
    fprintf(fout, "int " NAME "_n" POOL " = sizeof(" NAME "_" POOL ")"
	    " / sizeof(" NAME "_" POOL "[0]);\n\n");
    
    if (ferror(fout)) {
	fprintf(stderr, "%s: write error (%s): %s.\n",
		prg, FNAME ".c", strerror(errno));
	exit(1);
	fclose(fout);
    }

    /* output: binding_argpool */

    len = 3;
    fprintf(fout, "char *" NAME "_" ARGS "[] = {\n   ");
    for (i=0; i<maxkey; i++)
	if (binding[i].args)
	    print_args(fout, binding[i].args);
    for (i=0; i<maxkey; i++)
	if (binding[i].next)
	    for (b=binding[i].next; b; b=b->next)
		if (b->args)
		    print_args(fout, b->args);
    fprintf(fout, "\n};\n\n");

    fprintf(fout, "int " NAME "_n" ARGS " = sizeof(" NAME "_" ARGS ")"
	    " / sizeof(" NAME "_" ARGS "[0]);\n");

    if (ferror(fout)) {
	fprintf(stderr, "%s: write error (%s): %s.\n",
		prg, FNAME ".c", strerror(errno));
	exit(1);
	fclose(fout);
    }

    fclose(fout);
    exit(0);
}



int
initnames()
{
    char line[8192], *p, *q;
    FILE *f;

    num = 0;
	
    if ((f=fopen(TABLE, "r")) == NULL) {
	fprintf(stderr, "%s: can't open input file (%s): %s.\n",
		prg, TABLE, strerror(errno));
	exit(1);
    }

    while (fgets(line, 8192, f) && line[strlen(line)-2] != '{')
	;

    if (ferror(f)) {
	fprintf(stderr, "%s: read error (%s): %s.\n",
		prg, TABLE, strerror(errno));
	exit(1);
    }
    if (feof(f)) {
	fprintf(stderr, "%s: beginning of names in table not found.\n",
		prg);
	exit(1);
    }

    while (fgets(line, 8192, f) && line[0] != '}') {
	if (strncmp(line, "  { 0, 0, ", 10) == 0)
	    break;
	if (strncmp(line, "/*", 2) == 0)
	    continue;
	p = strchr(line, '\"');
	if (p == NULL) {
	    fprintf(stderr, "%s: table syntax error: %s\n",
		    prg, line);
	    continue;
	}
	q = strchr(p+1, '\"');
	if (q == NULL)
	    fprintf(stderr, "%s: table syntax error: %s\n",
		    prg, line);
	else {
	    p++;
	    *q = '\0';
	    functions[num].name = strdup(p);
	    functions[num].fn = NULL;
	    functions[num].type = 0;
	    functions[num].help = NULL;
	    
	    if (++num >= MAX_FN) {
		fprintf(stderr, "%s: internal function "
			"table too small\n", prg);
		exit(1);
	    }
	}
    }

    if (ferror(f)) {
	fprintf(stderr, "%s: read error (%s): %s.\n",
		prg, TABLE, strerror(errno));
	exit(1);
    }

    fclose(f);
}



void
print_args(FILE *fout, char **args)
{
    static int len = 0;

    int wlen, j;
    
    for (j=0; args[j]; j++) {
	wlen = 4+strlen(args[j]);
	if (len+wlen > LINELEN) {
	    fprintf(fout, "\n   ");
	    len = 3;
	}
	else
	    len += wlen;

	fprintf(fout, " \"%s\",", args[j]);
    }
    
    if (len+6 > LINELEN) {
	fprintf(fout, "\n   ");
	len = 3;
    }
    else
	len += 6;
    
    fprintf(fout, " NULL,");
}



void
disp_status(char *fmt, ...)
{
    return;
}

char *
read_string(char *prompt, int echop)
{
    return NULL;
}

