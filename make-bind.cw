@ generate binding table.

@u
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "keys.h"
#include "rc.h"

#define FNAME	"bindings"
#define NAME	"binding"
#define TABLE	"fntable.c"

#define MAX_FN	8192

int *binding;
char *names[MAX_FN];
int num, maxkey;

char *prg;


@u
int
main(int argc, char **argv)
{
	FILE *fin, *fout;
	char line[4069], *p, *tok;
	int key, ind;

	prg = argv[0];
	maxkey = max_fnkey + 256;
	if ((binding=(int *)malloc(maxkey*sizeof(int))) == NULL) {
		fprintf(stderr, "%s: malloc failure.\n", prg);
		exit(1);
	}

	initnames();

	for (ind=0; ind<maxkey; ind++)
		binding[ind] = -1;

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
		binding[key] = ind;
	}

	if (ferror(fin)) {
		fprintf(stderr, "%s: read error (%s): %s.\n",
			prg, FNAME ".desc", strerror(errno));
		exit(1);
		fclose(fin);
	}
	fclose(fin);

	if ((fout=fopen(FNAME ".c", "w")) == NULL) {
		fprintf(stderr, "%s: can't open output file (%s): %s.\n",
			prg, FNAME ".c", strerror(errno));
		exit(1);
	}

	fprintf(fout, "int " NAME "[] = {\n\t");
	for (key=0; key<maxkey; key++) {
		fprintf(fout, "%d, ", binding[key]);
		if (key%16 == 15 && key < maxkey)
			fprintf(fout, "\n\t");
	}
	fprintf(fout, "\n};\n");

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
