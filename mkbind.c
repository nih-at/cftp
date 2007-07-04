/*
  $NiH: mkbind.c,v 1.24 2003/04/16 00:54:22 dillo Exp $

  mkbind.c -- make binding table
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



#include <sys/types.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bindings.h"
#include "display.h"
#include "fntable.h"
#include "functions.h"
#include "keys.h"
#include "rc.h"

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

struct binding binding[MAX_FN];

char *binding_argpool[1];
int binding_nargpool = 0;

char *prg, *srcdir;

void print_args(FILE *fout, char **args);
FILE *vpath_open(char *name);

void initnames(void);




int
main(int argc, char **argv)
{
    FILE *fin, *fout;
    char line[4096], *p, *tok, **args;
    char tmp[128];
    struct binding *b;
    int i, j, off, argoff;
    int maxkey;

    prg = argv[0];
    rc_inrc = 1;
    maxkey = max_fnkey + 256;

    if (argc == 2 && strcmp(argv[1], ".") != 0)
	srcdir = argv[1];
    else
	srcdir = NULL;

    initnames();

    for (i=0; i<maxkey; i++) {
	binding[i].next = NULL;
	binding[i].state = bs_none;
	binding[i].fn = -1;
	binding[i].args = NULL;
    }
    off = 0;


    /* processing ``bindings.desc'' */

    if ((fin=vpath_open(FNAME ".desc")) == NULL)
	exit(1);

    rc_lineno = 0;
    while (fgets(line, 4096, fin) != NULL) {
	rc_lineno++;
	p = line;
	if ((tok=rc_token(&p)) == NULL || tok[0] == '#')
	    continue;
	if (strcasecmp(tok, "bind") != 0) {
	    rc_error("non-bind command ignored: %s", tok);
	    continue;
	}
	
	args = rc_list(p);

	fn_bind(args);
    }
    
    if (ferror(fin)) {
	fprintf(stderr, "%s: read error in `%s': %s.\n",
		prg, rc_filename, strerror(errno));
	fclose(fin);
	exit(1);
    }
    fclose(fin);
    free(rc_filename);
    rc_filename = NULL;


    /* writing ``bindings.c'' in temp file */

    sprintf(tmp, FNAME ".c.%d", getpid());

    if ((fout=fopen(tmp, "w")) == NULL) {
	fprintf(stderr, "%s: can't open output file `%s': %s.\n",
		prg, tmp, strerror(errno));
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
	fprintf(stderr, "%s: write error on `%s': %s.\n",
		prg, FNAME ".c", strerror(errno));
	exit(1);
	fclose(fout);
    }

    /* output: bindings_pool */

    off = 1;
    fprintf(fout, "struct binding " NAME "_" POOL "[] = {\n");
    for (i=0; i<maxkey; i++)
	if (binding[i].next) {
	    for (b=binding[i].next; b; b=b->next) {
		fprintf(fout, "    { ");
		if (b->next)
		    fprintf(fout, "binding_pool+%3d, ", off);
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

		off++;
	    }
	}
    fprintf(fout, "};\n\n");
    fprintf(fout, "int " NAME "_n" POOL " = sizeof(" NAME "_" POOL ")"
	    " / sizeof(" NAME "_" POOL "[0]);\n\n");
    
    if (ferror(fout)) {
	fprintf(stderr, "%s: write error on `%s': %s.\n",
		prg, FNAME ".c", strerror(errno));
	exit(1);
	fclose(fout);
    }

    
    /* output: binding_argpool */

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
	fprintf(stderr, "%s: write error on `%s': %s.\n",
		prg, FNAME ".c", strerror(errno));
	fclose(fout);
	exit(1);
    }

    fclose(fout);

    /* rename tmp file to ``bindings.c'' */

    if (rename(tmp, FNAME ".c") < 0) {
	fprintf(stderr, "%s: cannot rename `%s' to `%s': %s\n",
		prg, tmp, FNAME ".c", strerror(errno));
	exit(1);
    }

    exit(0);
}



void
initnames(void)
{
    char line[8192], *p, *q;
    FILE *f;

    num = 0;
	
    if ((f=vpath_open(TABLE)) == NULL) {
	exit(1);
    }

    rc_lineno = 1;
    while (fgets(line, 8192, f) && line[strlen(line)-2] != '{')
	rc_lineno++;

    if (ferror(f)) {
	fprintf(stderr, "%s: read error in `%s': %s.\n",
		prg, rc_filename, strerror(errno));
	exit(1);
    }
    if (feof(f)) {
	rc_error("beggining of functions array not found");
	exit(1);
    }

    while (fgets(line, 8192, f) && line[0] != '}') {
	if (strncmp(line, "  { 0, 0, ", 10) == 0)
	    break;
	if (strncmp(line, "/*", 2) == 0)
	    continue;
	p = strchr(line, '\"');
	if (p == NULL) {
	    rc_error("function name missing; line skipped");
	    continue;
	}
	q = strchr(p+1, '\"');
	if (q == NULL)
	    rc_error("end of function name missing; line skipped");
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
	fprintf(stderr, "%s: read error in `%s': %s.\n",
		prg, rc_filename, strerror(errno));
	exit(1);
    }

    fclose(f);
    free(rc_filename);
    rc_filename = NULL;
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



FILE *
vpath_open(char *name)
{
    FILE *f;
    char *vname;

    if ((f=fopen(name, "r")) != NULL) {
	rc_filename = strdup(name);
	return f;
    }

    if (srcdir == NULL) {
	fprintf(stderr, "%s: can't open file `%s': %s\n",
		prg, name, strerror(errno));
	return NULL;
    }

    if ((vname=(char *)malloc(strlen(name)+strlen(srcdir)+2)) == NULL) {
	fprintf(stderr, "%s: malloc failure\n", prg);
	exit(1);
    }

    sprintf(vname, "%s/%s", srcdir, name);

    if ((f=fopen(vname, "r")) != NULL) {
	rc_filename = vname;
	return f;
    }
    
    fprintf(stderr, "%s: can't find file `%s': %s\n",
	    prg, name, strerror(errno));
    free(vname);

    return NULL;
}



/* dummy functions needed by fn_bind */

void
disp_status(int flags, char *fmt, ...)
{
    va_list argp;
    
    if (flags & DISP_STDERR) {
	va_start(argp, fmt);
	vfprintf(stderr, fmt, argp);
	va_end(argp);
    }
    
    return;
}

char *
read_string(char *prompt, int echop)
{
    return NULL;
}
