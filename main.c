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

char *prg;
extern char version[];



int parse_url(char *url, char **user, char **host, char **port, char **dir);
void print_usage(int flag);
char *get_annon_passwd(void);
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
	char *host, *user = NULL, *port = "ftp", *pass = NULL, *wdir = NULL;
	int keep_pass = 1;
	int c, err = 0;
	char *b;

	prg = argv[0];
	
	signal(SIGPIPE, SIG_IGN);
	
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
		if (parse_url(argv[optind], &user, &host, &port, &wdir) < 0)
		    exit(1);
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
		if (pass == NULL)
			pass = get_annon_passwd();
	}
	else {
	    if (pass == NULL) {
		b = (char *)malloc(strlen(user)+strlen(host)+16);
		sprintf(b, "Password (%s@%s): ", user, host);
		pass = getpass(b);
		keep_pass = 0;
		free(b);
	    }
	}
	
	if (tty_init() < 0)
	    exit(1);

	if (ftp_open(host, port) == -1)
		exit(1);

	if (init_disp() < 0)
	    exit(1);
	    
	signal(SIGINT, sig_end);
	signal(SIGHUP, sig_end);
	signal(SIGTERM, sig_end);
	signal(SIGTSTP, sig_escape);
	signal(SIGCONT, sig_reenter);

	if (ftp_login(host, user, pass) == -1) {
		exit_disp();
		exit(1);
	}
	
	if (wdir == NULL)
	    wdir = ftp_pwd();
	
	if ((dir=ftp_cd(wdir)) == NULL) {
	    if ((wdir=ftp_pwd()) == NULL)
		wdir="/";
	    if ((dir=ftp_cd(wdir)) == NULL) {
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
	curtop = 0;
	cursel = curtop;

	loop();
	
	ftp_close();
	exit_disp();

	exit(0);
}



int
parse_url(char *url, char **user, char **host, char **port, char **dir)
{
    char *p, *q;
    int userp = 0;

    if (strncmp(url, "ftp://", 6) != 0)
	return -1;
    url+=6;

    if ((p=strchr(url, '/')) != NULL)
	*(p++) = '\0';
    else
	p = url+strlen(url);
    
    if ((q=strchr(url, '@')) != NULL) {
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

    if (p && *p != '\0') {
	if ((*dir=(char *)malloc(strlen(p)+3)) == NULL) {
	    fprintf(stderr, "%s: malloc failure\n", prg);
	    return -1;
	}
	    sprintf(*dir, "%s%s", userp ? "~/" : "/", p);
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
	b[l++] = '@';
	gethostname(b+l, 1023-l);
	l += strlen(b+l);
	getdomainname(b+l, 1023-l);

	return strdup(b);
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
