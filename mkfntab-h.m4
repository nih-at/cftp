divert(-1)

changequote({,})

define(function,dnl name, synopsis, function, flags, help-string, description
{divert(0)dnl
ifelse(0,{$3},,{void {$3}(void);
})dnl
divert(-1)})

define(section,dnl file, name
{divert(0)dnl
/* {$2}: {$1} */
divert(-1)})

define(endsec)

define(endall)
