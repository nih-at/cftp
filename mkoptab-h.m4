divert(-1)

changequote({,})

define(type,dnl t, (i)nt, (c)har, (s)tring, (b)oolean
{ifelse({$1}, i, {$2}, {$1}, c, {$3}, {$1}, s, {$4}, {$5})})

define(option,dnl name, short, variable, function, type, default, help, doku
{divert(0)dnl
extern type({$5}, {int }, {int }, {char *}, {int }){$3};
divert(-1)})


divert(0)dnl
{#ifndef HAD_OPTIONS_H
#define HAD_OPTIONS_H
/*
   This file is automatically created from ``options.op''; don't make
   changes to this file, change ``options.op'' instead.
*/

#define OPT_INT 0
#define OPT_CHR 1
#define OPT_STR 2
#define OPT_BOOL 3

union optvar {
    int *i;
    char **s;
};

struct uoption {
    char *name, *shrt;
    char *help;
    int type;
    union optvar var;
    void (*func)();
};

extern struct uoption option[];}

divert(-1)

define(endall,
{divert(0)
#endif /* options.h */
divert(-1)})

