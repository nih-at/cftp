divert(-1)

diverts:
	0  chapter menu
	1  chapter menu, all functions
	2  section menu + finished sections
	3  subsection nodes

changequote(<<,>>)

define(menuentry, dnl node, description
<<* <<$1>>::dnl
ifelse(eval(len(<<$1>>)<4), 1, <<	>>)dnl
ifelse(eval(len(<<$1>>)<12), 1, <<	>>)dnl
	<<$2>>>>)

define(function,dnl name, synopsis, function, flags, help-string, description
<<divert(1)dnl
menuentry(<<$1>>, <<$5>>)
divert(2)dnl
menuentry(<<$1>>, <<$5>>)
divert(3)dnl
@c ---------------------------------------------------------------------
@node <<$1>>
@subsection <<$1>> -- <<$5>>
@cindex <<$1>>

@itemize @bullet
@item
Synopsis: <<$1>> <<$2>>
dnl @item
dnl Default Binding: 
@end itemize

<<$6>>

divert(-1)>>)

define(section,dnl file, name
<<divert(0)dnl
menuentry(<<$2>>,)
divert(2)dnl

@c =====================================================================
@node <<$2>>
<<@section>> <<$2>>
@cindex <<$2>>

@menu
divert(-1)>>)

define(endsec, dnl
<<divert(2)dnl
@end menu

undivert(3)dnl
divert(-1)>>)

divert(0)dnl
@c This file is automatically created from ``fntable.fn''; don't change
@c this file, change ``fntable.fn'' instead.

@c *********************************************************************
@node	 Functions
@chapter Functions

@menu
divert(1)dnl
All Functions Alphabetically
<-------
divert(-1)

define(endall,
<<divert(1)dnl
------->
@end menu

divert(0)
undivert>>)
