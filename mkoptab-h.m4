divert(-1)

changequote({,})

define(type,dnl t, (i)nt, (c)har, (s)tring, (b)oolean (e)num
{ifelse({$1}, i, {$2}, {$1}, c, {$3}, {$1}, s, {$4}, {$1}, b, {$5}, {$6})})

define(option,dnl name, short, variable, function, type, default, values, help, doku
{divert(0)dnl
extern type({$5}, {int }, {int }, {char *}, {int }, {int }){$3};
divert(-1)})

define(values,dnl name, values
)


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
#define OPT_ENUM 4

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
    char **values;
};

extern struct uoption option[];}

divert(-1)

define(endall,
{divert(0)
#endif /* options.h */
divert(-1)})

