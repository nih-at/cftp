#ifndef HAD_OPTIONS_H
#define HAD_OPTIONS_H

#define OPT_INT 0
#define OPT_CHR 1
#define OPT_STR 2

union optvar {
    int *i;
    char **s;
};

struct option {
    char *name, *shrt;
    char *help;
    int type;
    union optvar var;
    void (*func)();
};

extern struct option option[];

extern int opt_mode;
extern int opt_tagchar;
extern int ftp_hist_size;

#endif /* options.h */
