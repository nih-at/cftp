divert(-1)

changequote(<<,>>)

define(function,dnl name, synopsis, function, flags, help-string, description
<<divert(0)dnl
  <<$3>> , "<<$1>>", <<$4>>, "<<$5>>" ,
divert(-1)>>)

define(section,dnl file, name
<<divert(0)dnl
/* <<$2>>: <<$1>> */
divert(-1)>>)

define(endsec)

divert(0)dnl
<</*
   This file is automatically created from ``fntable.fn''; don't change
   this file, change ``fntable.fn'' instead.
*/

#include "directory.h"
#include "bindings.h"
#include "functions.h"
#include "fntable.h"

function functions[] = {
>>divert(-1)

define(endall,
<<divert(0)dnl
/* end marker */
  { 0, 0, 0, 0 }
};
divert(-1)>>)
