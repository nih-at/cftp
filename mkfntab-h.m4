divert(-1)

changequote(<<,>>)

define(function,dnl name, synopsis, function, flags, help-string, description
<<divert(0)dnl
ifelse(0,<<$3>>,,<<void <<$3>>(char **args);
>>)dnl
divert(-1)>>)

define(section,dnl file, name
<<divert(0)dnl
/* <<$2>>: <<$1>> */
divert(-1)>>)

define(endsec)

divert(0)dnl
<<#ifndef HAD_FNTABLE_H
#define HAD_FNTABLE_H

/*
   This file is automatically created from ``fntable.fn''; don't change
   this file, change ``fntable.fn'' instead.
*/

>>divert(-1)

define(endall,
<<divert(0)
#endif /* fntable.h */
divert(-1)>>)
