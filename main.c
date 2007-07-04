/*
  $NiH: main.c,v 1.49 2003/12/05 12:14:18 dillo Exp $

  main.c -- main function
  Copyright (C) 1996-2003 Dieter Baron

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
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <getopt.h>
#include <unistd.h>

#include "config.h"
#include "directory.h"
#include "display.h"
#include "loop.h"
#include "ftp.h"
#include "bindings.h"
#include "fntable.h"
#include "functions.h"
#include "list.h"
#include "options.h"
#include "signals.h"
#include "tag.h"
#include "status.h"
#include "tty.h"
#include "util.h"
#include "url.h"

/* in readrc.c */
int readrc(char **userp, char **passp, char **hostp, char **portp,
	   char **wdirp, int check_alias);



char *usage[] = {
	"{-h|-V}",
	"[-i tags] [-p port] [-u user] "
#ifdef USE_SFTP
	 "[-s] "
#endif
	 "{host|alias} [directory]",
	"[-i tags] url",
	NULL };

char help_head[] = "%s by Dieter Baron <dillo@giga.or.at>\n\n";

char help[] = "\
  -h, --help           display this help message\n\
  -V, --version        display version number\n\
  -i, --get-tags TAGS  download tags from file TAGS on startup\n\
  -p, --port PORT      specify port\n"
#ifdef USE_SFTP
"  -s, --sftp           use sftp\n"
#endif
"  -u, --user USER      specify user\n\
\n\
Report bugs to <dillo@giga.or.at>.\n";

char version_tail[] = "\
Copyright (C) 2003 Dieter Baron\n\
cftp comes with ABSOLUTELY NO WARRANTY, to the extent permitted by law.";

#ifdef USE_SFTP
#define OPTIONS	"hVi:p:u:s"
#else
#define OPTIONS	"hVi:p:u:"
#endif

struct option options[] = {
    { "help",      0, 0, 'h' },
    { "version",   0, 0, 'V' },
    { "get-tags",  1, 0, 'i' },
    { "port",      1, 0, 'p' },
#ifdef USE_SFTP
    { "sftp",      1, 0, 's' },
#endif
    { "user",      1, 0, 'u' },
    { NULL,        0, 0, 0   }
};

char *prg;
char version[] = PACKAGE " " VERSION;



void print_usage(FILE *f);
void read_netrc(char *host, char **user, char **pass, char **wdir);
void sig_end(int i);
void sig_escape(int i);
void sig_reenter(int i);



int
main(int argc, char **argv)
{
    extern int opterr, optind;
    extern char *optarg;

    struct ftp_hist *h;
    directory *dir;
    char *host, *user = NULL, *port = NULL, *pass = NULL, *wdir = NULL;
    char *poss_fn, *poss_dir;
    char *tag_file, *args[2];
    int c;
    char *b, *b2;
    int check_alias;

    prg = argv[0];
	
    signal(SIGPIPE, SIG_IGN);

    if ((opt_pager=getenv("PAGER")) == NULL)
	opt_pager = strdup("more");
    else
	opt_pager = strdup(opt_pager);
    if (opt_pager == NULL) {
	fprintf(stderr, "%s: malloc failure\n", prg);
	exit(1);
    }
	
    if (tag_init() < 0)
	exit(1);
    status_init(); /* can't fail */

    tag_file = NULL;
    ftp_proto = 0;

    opterr = 0;
    while ((c=getopt_long(argc, argv, OPTIONS, options, 0)) != EOF) {
	switch (c) {
	case 'i':
	    tag_file = optarg;
	    break;
	case 'p':
	    port = strdup(optarg);
	    break;
	case 'u':
	    user = strdup(optarg);
	    break;
#ifdef USE_SFTP
	case 's':
	    ftp_proto = 1;
	    break;
#endif
	case 'V':
	    printf("%s\n", version);
	    fputs(version_tail, stdout);
	    exit(0);
	case 'h':
	    printf(help_head, version);
	    print_usage(stdout);
	    printf("\n%s", help);
	    exit(0);
	case '?':
	    print_usage(stderr);
	    exit(1);
	}
    }

    if (optind == argc) {
	print_usage(stderr);
	exit(1);
    }

    if (is_url(argv[optind])) {
	if (argc > optind+1) {
	    print_usage(stderr);
	    exit(1);
	}
	if (parse_url(argv[optind], &ftp_proto, &user, &pass,
		      &host, &port, &wdir) < 0)
	    exit(1);
	
	check_alias = 0;
    }
    else {
	if (argc > optind+2) {
	    print_usage(stderr);
	    exit(1);
	}
	host = strdup(argv[optind]);
	if (argc > optind+1)
	    wdir = strdup(argv[optind+1]);
	
	check_alias = 1;
    }

    curdir = NULL;

    /* XXX */ readrc(&user, &pass, &host, &port, &wdir, check_alias);

    if (ftp_proto == 0) {
	if (user == NULL) {
	    read_netrc(host, &user, &pass, &wdir);
	}
	
	if (user == NULL ||
	    (strcmp(user, "ftp") == 0 || strcmp(user, "anonymous") == 0)) {
	    if (user == NULL)
		user = "ftp";
	    if (pass == NULL)
		pass = get_anon_passwd();
	}
    }
    
    if (tty_init() < 0)
	exit(1);

    ftp_init();

    if (ftp_open(host, port, user, pass) == -1) {
	for (h=ftp_history; h; h=h->next)
	    printf("%s\n", h->line);

	exit(1);
    }

    if (init_disp() < 0)
	exit(1);
    list_init();
	    
    if (signals_init() < 0)
	exit(1);

    binding_state = bs_remote;

    if (ftp_login(user, pass) == -1) {
	exit_disp();
	exit(1);
    }
	
    if (wdir == NULL) {
	if ((wdir=ftp_pwd()) == NULL)
	    wdir = strdup("/");
    }
    else if (wdir[0] != '/') {
	if ((b=ftp_pwd()) == NULL)
	    wdir = strdup("/");
	else {
	    if (b[strlen(b)-1] == '/')
		b[strlen(b)-1] = '\0';
	    b2 = wdir;
	    if ((wdir=(char *)malloc(strlen(b2)+strlen(b)+2)) == NULL) {
		exit_disp();
		fprintf(stderr, "%s: malloc failure\n", prg);
		exit(1);
	    }
	    sprintf(wdir, "%s/%s", b, b2);
	}
    }
	
    if ((dir=ftp_cd(wdir, 0)) == NULL) {
	if ((poss_fn=strrchr(wdir, '/')) != NULL) {
	    *(poss_fn++)=0;
	    poss_dir = strlen(wdir) ? wdir : "/";
	    if (ftp_cwd(poss_dir) == 0) {
		ftp_lcwd = strdup(poss_dir);
		free(status.remote.path);
		status.remote.path = strdup(ftp_lcwd);
		status_do(bs_remote);
		aux_download(poss_fn, -1, 0);
	    }
	}
	if (dir == NULL) {
	    if ((wdir=ftp_pwd()) == NULL)
		wdir = strdup("/");
	    if ((dir=ftp_cd(wdir, 0)) == NULL) {
		escape_disp(0);
		ftp_close();
		exit_disp();
		exit(1);
	    }
	}
    }

    curdir = dir;
    list = (struct list *)curdir;

    if (tag_file) {
	/* XXX: factor out */
	list_do(1);
	tty_lowleft();
	fflush(stdout);

	args[0] = tag_file;
	args[1] = NULL;
	fn_loadtag(args);
	
	/* XXX: don't hardcode, use separate option */
	args[0] = "-c";
	fn_gettags(args);
    }

    loop();
	
    ftp_close();
    exit_disp();

    exit(0);
}



void
print_usage(FILE *f)
{
    int i;

    for (i=0; usage[i]; i++)
	fprintf(f, "%s %s %s\n", i ? "      " : "Usage:",
		prg, usage[i]);
}



void
read_netrc(char *host, char **user, char **pass, char **wdir)
{
    FILE *f;
    char b[1024], *home, *p, *q;
    int match, init, end, userp;
    struct stat stat;

    if ((home=getenv("HOME")) == NULL)
	home = "";
    sprintf(b, "%s/.netrc", home);

    if ((f=fopen(b, "r")) == NULL)
	return;
		
    match = init = end = userp = 0;

    while (!end && fscanf(f, "%s", b) != EOF) {
	if (strcmp(b, "machine") == 0) {
	    if (match)
		end = 1;
	    else if (fscanf(f, "%s", b) == EOF)
		end = 1;
	    else if (strcmp(b, host) == 0)
		match = 1;
	}
	else if (strcmp(b, "default") == 0) {
	    if (match)
		end = 1;
	    else
		match = 1;
	}
	else if (strcmp(b, "login") == 0) {
	    if (fscanf(f, "%s", b) == EOF)
		end = 1;
	    else if (match && *user == NULL) {
		*user = strdup(b);
		userp = 1;
	    }
	}
	else if (strcmp(b, "password") == 0) {
	    if (fscanf(f, "%s", b) == EOF)
		end = 1;
	    else if (match) {
		*pass = strdup(b);
	    }
	}
	else if (strcmp(b, "account") == 0) {
	    if (fscanf(f, "%s", b) == EOF)
		end = 1;
	}
	else if (strcmp(b, "macdef") == 0) {
	    if (fscanf(f, "%s", b) == EOF)
		end = 1;
	    else {
		if (strcmp(b, "init") == 0)
		    init = 1;
		fgets(b, 1024, f);
		while (fgets(b, 1024, f) != NULL
		       && *b != '\n') {
		    if (match && init
			&& *wdir == NULL
			&& strncmp(b, "cd ", 3) == 0) {
			p = b+3+strspn(b+3, " \t");
			if ((q=strchr(p, ' ')))
			    *q='\0';
			if ((q=strchr(p, '\t')))
			    *q='\0';
			if ((q=strchr(p, '\n')))
			    *q='\0';
			*wdir = strdup(p);
		    }
		}
		init = 0;
	    }
	}
    }
    if (!userp) {
	free(*pass);
	*pass = NULL;
    }
    if (*pass && userp && strcmp(*user, "anonymous")!=0) {
	fstat(fileno(f), &stat);
	if (stat.st_mode&022) {
	    fprintf(stderr, "%s: .netrc contains password "
		    "and is readable by others!\n",
		    prg);
	    exit(1);
	}
    }
    fclose(f);
}
