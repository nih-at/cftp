/*
  mkbind -- make binding table
  Copyright (C) 1996 Dieter Baron

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

#define FNAME	"bindings"
#define NAME	"binding"
#define ARGS	"_args"
#define POOL	"_pool"
#define TABLE	"fntable.c"

#define MAX_FN	8192

int *binding, *binding_off;
char **binding_pool;
char *names[MAX_FN];
int num, maxkey;

char *prg;



int
main(int argc, char **argv)
{
    FILE *fin, *fout;
    char line[4069], *p, *tok;
    int key, ind, off, i, size = 8192;

    prg = argv[0];
    maxkey = max_fnkey + 256;
    binding = (int *)malloc(maxkey*sizeof(int));
    binding_off = (int *)malloc(maxkey*sizeof(int));
    binding_pool = (char **)malloc(size*sizeof(char *));
    if (binding == NULL || binding_off == NULL
	|| binding_pool == NULL) {
	fprintf(stderr, "%s: malloc failure.\n", prg);
	exit(1);
    }
    
    initnames();

    for (ind=0; ind<maxkey; ind++) {
	binding[ind] = -1;
	binding_off[ind] = -1;
    }
    off = 0;

    if ((fin=fopen(FNAME ".desc", "r")) == NULL) {
	fprintf(stderr, "%s: can't open input file (%s): %s.\n",
		prg, FNAME ".desc", strerror(errno));
	exit(1);
    }

    while (fgets(line, 4096, fin) != NULL) {
	p = line;
	if ((tok=rc_token(&p)) == NULL)
	    continue;
	if (strcasecmp(tok, "bind") != 0) {
	    fprintf(stderr, "%s: non-bind command ignored: `%s'\n",
		    prg, line);
	    continue;
	}
	
	if ((tok=rc_token(&p)) == NULL) {
	    fprintf(stderr, "%s: key name missing\n",
		    prg);
	    continue;
	}
	if ((key=parse_key(tok)) == -1) {
	    fprintf(stderr, "%s: unknown key: `%s'\n",
		    prg, tok);
	    continue;
	}

	if ((tok=rc_token(&p)) == NULL) {
	    fprintf(stderr, "%s: function name missing\n",
		    prg);
	    continue;
	}
	if ((ind=getindex(tok)) == -1) {
	    fprintf(stderr, "%s: unknown function: %s\n",
		    prg, tok);
	    continue;
	}
	
	i = 0;
	while ((tok=rc_token(&p)) != NULL)
	    binding_pool[off+i++] = strdup(tok);
		    
	binding[key] = ind;
	if (i) {
	    binding_pool[off+i++] = NULL;
	    binding_off[key] = off;
	    off += i;
	}
    }

    if (ferror(fin)) {
	fprintf(stderr, "%s: read error (%s): %s.\n",
		prg, FNAME ".desc", strerror(errno));
	fclose(fin);
	exit(1);
    }
    fclose(fin);

    if ((fout=fopen(FNAME ".c", "w")) == NULL) {
	fprintf(stderr, "%s: can't open output file (%s): %s.\n",
		prg, FNAME ".c", strerror(errno));
	exit(1);
    }

    fprintf(fout, "#include <stdio.h>\n\n");

    fprintf(fout, "int " NAME "[] = {\n  ");
    for (key=0; key<maxkey; key++) {
	fprintf(fout, "%d, ", binding[key]);
	if (key%16 == 15 && key < maxkey)
	    fprintf(fout, "\n  ");
    }
    fprintf(fout, "\n};\n\n");

    if (ferror(fout)) {
	fprintf(stderr, "%s: write error (%s): %s.\n",
		prg, FNAME ".c", strerror(errno));
	exit(1);
	fclose(fout);
    }

    fprintf(fout, "char *" NAME POOL "[] = {\n  ");
    for (i=0; i<off; i++) {
	if (binding_pool[i])
	    fprintf(fout, "\"%s\", ", binding_pool[i]);
	else
	    fprintf(fout, "NULL, ");
	if (i%8 == 7 && i < off)
	    fprintf(fout, "\n  ");
    }

    fprintf(fout, "\n};\n\n");
    fprintf(fout,
	    "int binding_pool_len = sizeof(binding_pool);\n\n");
    
    if (ferror(fout)) {
	fprintf(stderr, "%s: write error (%s): %s.\n",
		prg, FNAME ".c", strerror(errno));
	exit(1);
	fclose(fout);
    }

        fprintf(fout, "char **" NAME ARGS "[] = {\n  ");
    for (key=0; key<maxkey; key++) {
	if (binding_off[key] != -1)
	    fprintf(fout, NAME POOL "+%d, ", binding_off[key]);
	else
	    fprintf(fout, "NULL, ");
	if (key%8 == 7 && key < maxkey)
	    fprintf(fout, "\n  ");
    }

    fprintf(fout, "\n};\n\n");

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
		names[num++] = strdup(p);
		if (num>MAX_FN) {
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



int
getindex(char *name)
{
	int i;

	for (i=0; i<num; i++)
		if (strcmp(name, names[i]) == 0)
			return i;

	return -1;
}
