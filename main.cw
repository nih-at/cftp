@ main

@d<usage strings@>
char *usage[] = {
	"{-h|-V}",
	"[-p port] [-u user] host [directory]",
	"url",
	NULL };

#define OPTIONS	"hVp:u:"

char help[] = "\
  -h: display this help message\n\
  -p: specify port\n\
  -u: specify user\n\
  -V: display version number\n";

@u
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include "directory.h"
#include "display.h"
#include "loop.h"
#include "ftp.h"
#include "functions.h"

@<usage strings@>

char *prg;
extern char version[];

@<prototypes@>


@ main

@u
int
main(int argc, char **argv)
{
	directory *dir;
	char *host, *user = NULL, *port = "ftp", *pass = NULL, *wdir = NULL;

	prg = argv[0];
	
	signal(SIGPIPE, SIG_IGN);
	
	@<process command line arguments@>
	
	if (init_disp() < 0)
	    exit(1);
	    
	signal(SIGINT, sig_end);
	signal(SIGHUP, sig_end);
	signal(SIGTERM, sig_end);
	signal(SIGTSTP, sig_escape);
	signal(SIGCONT, sig_reenter);

	if (ftp_open(host, user, pass) == -1) {
		exit_disp();
		exit(1);
	}
	
	if ((dir=ftp_cd(wdir)) == NULL) {
		escape_disp(0);
		ftp_close();
		exit_disp();
		exit(1);
	}
	

	curdir = dir;
	curtop = 0;
	cursel = curtop;

	loop();
	
	ftp_close();
	exit_disp();

	exit(0);
}


@ command line arguments

@d<process command line arguments@>
{
	extern int opterr, optind;
	extern char *optarg;

	int c, err = 0;
	char *b;
	
	opterr = 0;

	while ((c = getopt(argc, argv, OPTIONS)) != EOF)
		switch (c) {
		case 'p':
			port = strdup(optarg);
			break;
		case 'u':
			user = strdup(optarg);
			break;
		case 'V':
			printf("%s\n", version);
			exit(0);
		case 'h':
			printf("%s\n\n", version);
			print_usage(1);
			printf("\n%s", help);
			exit(0);
		case '?':
			print_usage(0);
			exit(1);
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
		parse_url(argv[optind], &user, &host, &port, &wdir);
	}
	else {
		if (argc > optind+2) {
			print_usage(0);
			exit(1);
		}
		host = argv[optind];
		if (argc > optind+1)
			wdir = argv[optind+1];
	}

	if (user == NULL) {
		read_netrc(host, &user, &pass, &wdir);
	}

	if (user == NULL ||
	    (strcmp(user, "ftp") == 0 || strcmp(user, "anonymous") == 0)) {
		if (user == NULL)
			user = "ftp";
		if (wdir == NULL)
			wdir = "/";
		if (pass == NULL)
			pass = get_annon_passwd();
	}
	else {
		if (wdir == NULL)
			wdir = "~";
		if (pass == NULL) {
			b = (char *)malloc(strlen(user)+strlen(host)+16);
			sprintf(b, "Password (%s@@%s): ", user, host);
			pass = getpass(b);
			free(b);
		}
	}
}


@d<prototypes@>
int parse_url(char *url, char **user, char **host, char **port, char **dir);

@u
int
parse_url(char *url, char **user, char **host, char **port, char **dir)
{
	char *p, *q;
	int userp = 0;

	if (strncmp(url, "ftp://", 6) != 0)
		return -1;
	url+=6;

	if ((p=strchr(url, '/')) == NULL)
		return -1;
	*(p++) = '\0';

	if ((q=strchr(url, '@@')) != NULL) {
		*q = '\0';
		*user = url;
		url = q+1;
		userp = 1;
	}

	if ((q=strchr(url, ':')) != NULL) {
		*q = '\0';
		*port = q+1;
	}

	*host = url;

	if (*p != '\0') {
		*dir = (char *)malloc(strlen(p)+3);
		sprintf(*dir, "%s%s", userp ? "~/" : "/", p);
	}

	return 0;
}


@ printing multiline usage message.

@d<prototypes@>
void print_usage(int flag);

@u
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


@ getting anonymous password (ident).

@d<prototypes@>
char *get_annon_passwd(void);

@u
char *
get_annon_passwd(void)
{
	char b[1024];
	int l;
	uid_t uid;
	struct passwd *pwd;

	uid = getuid();
	pwd = getpwuid(uid);
	
	strcpy(b, pwd->pw_name);
	l = strlen(b);
	b[l++] = '@@';
	gethostname(b+l, 1023-l);
	l += strlen(b+l);
	getdomainname(b+l, 1023-l);

	return strdup(b);
}


@ read in .netrc file.

@d<prototypes@>
void read_netrc(char *host, char **user, char **pass, char **wdir);

@u
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
						if (q=strchr(p, ' '))
							*q='\0';
						if (q=strchr(p, '\t'))
							*q='\0';
						if (q=strchr(p, '\n'))
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


@ signal handler: reset & restore term

@d<prototypes@>
void sig_end(int i);
void sig_escape(int i);
void sig_reenter(int i);

@u
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
