/*
  main -- main function
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
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <getopt.h>
#include <unistd.h>


#include "directory.h"
#include "display.h"
#include "loop.h"
#include "ftp.h"
#include "bindings.h"
#include "functions.h"
#include "list.h"
#include "options.h"
#include "signals.h"
#include "tag.h"
#include "status.h"
#include "tty.h"

/* in readrc.c */
int readrc(char **userp, char **passp, char **hostp, char **portp,
	   char **wdirp, int check_alias);



char *usage[] = {
	"{-h|-V}",
	"[-p port] [-u user] {host|alias} [directory]",
	"url",
	NULL };

char help_head[] = "%s by Dieter Baron <dillo@giga.or.at>\n\n";

char help[] = "\
  -h, --help        display this help message\n\
  -V, --version     display version number\n\
  -p, --port PORT   specify port\n\
  -u, --user USER   specify user\n\
\n\
Report bugs to <dillo@giga.or.at>.\n";

char version_tail[] = "\
Copyright (C) 1997 Dieter Baron\n\
cftp comes with ABSOLUTELY NO WARRANTY, to the extent permitted by law.\n\
You may redistribute copies of\n\
cftp under the terms of the GNU General Public License.\n\
For more information about these matters, see the files named COPYING.\n";

#define OPTIONS	"hVp:u:"

struct option options[] = {
    { "help",      0, 0, 'h' },
    { "version",   0, 0, 'V' },
    { "port",      1, 0, 'p' },
    { "user",      1, 0, 'u' },
    { NULL,        0, 0, 0   }
};

char *prg;
extern char version[];



int parse_url(char *url, char **user, char **pass,
	      char **host, char **port, char **dir);
char *deurl(char *u);
void print_usage(int flag);
char *get_anon_passwd(void);
void read_netrc(char *host, char **user, char **pass, char **wdir);
void sig_end(int i);
void sig_escape(int i);
void sig_reenter(int i);



int
main(int argc, char **argv)
{
    extern int opterr, optind;
    extern char *optarg;

    directory *dir;
    char *host, *user = NULL, *port = NULL, *pass = NULL, *wdir = NULL;
    int keep_pass = 1;
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

    opterr = 0;
    while ((c=getopt_long(argc, argv, OPTIONS, options, 0)) != EOF) {
	switch (c) {
	case 'p':
	    port = strdup(optarg);
	    break;
	case 'u':
	    user = strdup(optarg);
	    break;
	case 'V':
	    printf("%s\n", version);
	    fputs(version_tail, stdout);
	    exit(0);
	case 'h':
	    printf(help_head, version);
	    print_usage(1);
	    printf("\n%s", help);
	    exit(0);
	case '?':
	    print_usage(0);
	    exit(1);
	}
    }

    if (optind == argc) {
	print_usage(0);
	exit(1);
    }
    
    if (strncmp(argv[optind], "ftp://", 6) == 0) {
	if (argc > optind+1) {
	    print_usage(0);
	    exit(1);
	}
	if (parse_url(argv[optind], &user, &pass, &host, &port, &wdir) < 0)
	    exit(1);
	
	check_alias = 0;
    }
    else {
	if (argc > optind+2) {
	    print_usage(0);
	    exit(1);
	}
	host = strdup(argv[optind]);
	if (argc > optind+1)
	    wdir = strdup(argv[optind+1]);
	
	check_alias = 1;
    }

    curdir == NULL;

    /* XXX */ readrc(&user, &pass, &host, &port, &wdir, check_alias);

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
    else {
	if (pass == NULL) {
	    b = (char *)malloc(strlen(user)+strlen(host)+16);
	    sprintf(b, "Password (%s@%s): ", user, host);
	    pass = (char *)getpass(b);
	    keep_pass = 0;
	    free(b);
	}
    }
    if (port == NULL)
	port = "ftp";
    
    if (tty_init() < 0)
	exit(1);

    ftp_init();

    if (ftp_open(host, port) == -1)
	exit(1);

    if (init_disp() < 0)
	exit(1);
    list_init();
	    
    if (signals_init() < 0)
	exit(1);

    binding_state = bs_remote;

    if (ftp_login(host, user, pass) == -1) {
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
	if ((wdir=ftp_pwd()) == NULL)
	    wdir = strdup("/");
	if ((dir=ftp_cd(wdir, 0)) == NULL) {
	    escape_disp(0);
	    ftp_close();
	    exit_disp();
	    exit(1);
	}
    }

    ftp_host = host;
    ftp_prt = port;
    ftp_user = user;
    if (keep_pass)
	ftp_pass = pass;

    curdir = dir;
    list = (struct list *)curdir;

    loop();
	
    ftp_close();
    exit_disp();

    exit(0);
}



int
parse_url(char *url, char **user, char **pass,
	  char **host, char **port, char **dir)
{
    char *p, *q, *r;
    int userp = 0;

    if (strncmp(url, "ftp://", 6) != 0)
	return -1;
    url+=6;

    if ((p=strchr(url, '/')) != NULL)
	*(p++) = '\0';
    else
	p = url+strlen(url);
    
    if ((q=strrchr(url, '@')) != NULL) {
	*q = '\0';
	if ((r=strchr(url, ':')) != NULL) {
	    *(r++) = '\0';
	    *pass = deurl(r);
	}
	*user = deurl(url);
	url = q+1;
	userp = 1;
    }

    if ((q=strchr(url, ':')) != NULL) {
	*q = '\0';
	*port = deurl(q+1);
    }
	
    *host = deurl(url);

    if (p && *p != '\0') {
	if ((*dir=(char *)malloc(strlen(p)+3)) == NULL) {
	    fprintf(stderr, "%s: malloc failure\n", prg);
	    return -1;
	}
	/* sprintf(*dir, "%s%s", userp ? "" : "/", p); */
	*dir = deurl(p);
    }

    return 0;
}



void
print_usage(int flag)
{
    int i;
    FILE *f;

    f = (flag ? stdout : stderr);

    for (i=0; usage[i]; i++)
	fprintf(f, "%s %s %s\n", i ? "      " : "Usage:",
		prg, usage[i]);
}



char *
get_anon_passwd(void)
{
    char pass[8192], host[1024], domain[1024];
    struct passwd *pwd;

    pwd = getpwuid(getuid());

    if (pwd)
	sprintf(pass, "%s@", pwd->pw_name);
    else
	strcpy(pass, "unknown@");

    gethostname(host, 1023);
    getdomainname(domain, 1023);

    if (strcmp(domain, "(none)") != 0 && domain[0] != '\0') {
	if (domain[0] != '.')
	    sprintf(pass+strlen(pass), "%s.%s", host, domain);
	else
	    sprintf(pass+strlen(pass), "%s%s", host, domain);
    }
    else if (strchr(host, '.'))
	strcat(pass, host);
    
    return strdup(pass);
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



int
hexdigit(int c)
{
    if (c >= '0' && c <= '9')
	return c-'0';
    if (c >= 'a' && c <= 'f')
	return c-'a';
    if (c >= 'A' && c <= 'F')
	return c-'F';
    
    return 0;
}

char *
deurl(char *s)
{
    char *t, *p;
    int c;

    if ((t=(char *)malloc(strlen(s)+1)) != NULL) {
	for (p=t; *s; s++) {
	    if (*s == '%') {
		c = hexdigit(*(++s))*16;
		c += hexdigit(*(++s));

		if (c != 0)
		    *(p++) = c;
	    }
	    else
		    *(p++) = *s;
	}
	*p = '\0';
    }

    return t;
}
