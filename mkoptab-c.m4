divert(-1)

diverts:
	0: declarations, definitions and initializations
	1: options array
	2: closing line of array

changequote(<<,>>)

divert(1)dnl
struct uoption option[] = {
divert(2)dnl
};
divert(-1)


dnl struct uoption {
dnl     char *name, *short;
dnl     char *help;
dnl     int type;
dnl     union optvar var;
dnl     void (*func)();
dnl };

define(type,dnl t, (i)nt, (c)har, (s)tring, (b)oolean, (e)num
<<ifelse(<<$1>>, i, <<$2>>, <<$1>>, c, <<$3>>, <<$1>>, s, <<$4>>, <<$1>>, b, <<$5>>, <<$6>>)>>)

define(option,dnl name, short, variable, function, type, default, values, help, doku
<<divert(1)dnl
  { "<<$1>>" , "<<$2>>", "<<$8>>",
    type(<<$5>>, OPT_INT, OPT_CHR, OPT_STR, OPT_BOOL, OPT_ENUM), dnl
(void *)&<<$3>>, <<$4>>, ifelse(<<$5>>, e, openum_<<$7>>, NULL) dnl
},
divert(0)dnl
type(<<$5>>, <<int >>, <<int >>, <<char *>>, <<int >>, <<int >>)<<$3>> dnl
= <<$6>>;
ifelse(<<$4>>, NULL, , extern void <<$4>><<()>>;
)
divert(-1)>>)

define(values,dnl name, values
<<divert(0)dnl
char *openum_<<$1>>[] = { <<$2>>, NULL };
divert(-1)>>)

divert(0)dnl
/*
   This file is automatically created from ``options.op''; don't make
   changes to this file, change ``options.op'' instead.
*/

#include <stddef.h>
#include "options.h"

divert(-1)

define(endall,
<<divert(0)
undivert(1)dnl
  NULL, NULL, NULL, 0, NULL, NULL
undivert(2)dnl
divert(-1)>>)
