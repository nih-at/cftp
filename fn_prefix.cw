@ bindable functions for prefix argument.

@(fn_prefix.fn@)
; fn_prefix
@<functions@>

@u
#include <stdio.h>
#include "directory.h"
#include "functions.h"


@ functions for entering digits

@d<functions@>
  { fn_prefix, "prefix", FN_PRE, "prefix digit" }

@u
int
fn_prefix(char **args)
{
    if (args == NULL) {
	/* emacs C-u */
	return;
    }
    else if (args[0][1] == '\0') {
	if (isdigit(args[0][0]))
	    add_prefix(args[0][0] - '0');
	else if (args[0][0] == '-')
	    negate_prefix();
    }
    else {
	int p = atoi(args[0]);

	if (p || args[0][0] == '0')
	    set_prefix(p);
    }
}


