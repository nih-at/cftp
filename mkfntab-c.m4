divert(-1)

changequote({,})

define(function,dnl name, synopsis, function, flags, help-string, description
{divert(0)dnl
  {$3} , "{$1}", {$4}, "{$5}" ,
divert(-1)})

define(section,dnl file, name
{divert(0)dnl
/* {$2}: {$1} */
divert(-1)})

define(endsec)

define(endall)
