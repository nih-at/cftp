divert(-1)

diverts:
	0: declarations, definitions and initializations
	1: options array
	2: closing line of array

changequote(,)

divert(1)dnl
struct uoption option[] = {
divert(2)dnl
};
divert(-1)

changequote({,})

struct uoption {
    char *name, *short;
    char *help;
    int type;
    union optvar var;
    void (*func)();
};

define(type,dnl t, (i)nt, (c)har, (s)tring, (b)oolean
{ifelse({$1}, i, {$2}, {$1}, c, {$3}, {$1}, s, {$4}, {$5})})

define(option,dnl name, short, variable, function, type, default, help, doku
{divert(1)dnl
  "{$1}" , "{$2}", "{$7}",
    type({$5}, OPT_INT, OPT_CHR, OPT_STR, OPT_BOOL), (void *)&{$3}, {$4},
divert(0)dnl
type({$5}, {int }, {int }, {char *}, {int }){$3} = {$6};
ifelse({$4}, NULL, , extern void {$4}{()};
)
divert(-1)})


divert(0)dnl
/*
   This file is automatically created from ``options.op''; don't make
   changes to this file, change ``options.op'' instead.
*/

#include <stddef.h>
#include "options.h"

divert(-1)

define(endall,
{divert(0)
undivert(1)dnl
  NULL, NULL, NULL, 0, NULL, NULL
undivert(2)dnl
divert(-1)})
