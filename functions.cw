@ functions bindable to keys.

this description is obsolete.

all these are functions of no arguments returning an int.
 usual functions return 0.
 functions that quit the program (fn_exit) return -1.
 functions that preserve the prefix argument (fn_prefixN) return 1;

prefix argument handling is in here, bindable manipulation functions
are in fn_prefix.cw.

use get_prefix(default_value) to retrieve the prefix argument.

@(functions.h@)
@<types@>
@<prototypes@>
@<globals@>

@u
#include <stdio.h>
#include "directory.h"
#include "display.h"
#include "functions.h"

@<local globals@>


@ the function table.

@d<types@>
typedef struct function {
	void (*fn)();
	char *name;
	int type;
	char *help;
} function;

#define FN_PRE	1
#define FN_EXIT	-1


@d<globals@>
extern function functions[];


@ currently selected item.

@d<globals@>
extern directory *curdir;
extern int curtop, cursel;

@d<local globals@>
directory *curdir;
int curtop, cursel;


@ prefix.

@d<local globals@>
int prefix_arg, prefix_valid = 0;

@d<prototypes@>
void void_prefix(void);

@u
void void_prefix(void)
{
	prefix_valid = 0;
}

@d<prototypes@>
int get_prefix(int deflt);

@u
int get_prefix(int deflt)
{
	return (prefix_valid ? prefix_arg : deflt);
}

@d<prototypes@>
void add_prefix(int n);
void show_prefix(void);

@u
void add_prefix(int n)
{
	if (prefix_valid)
		prefix_arg = prefix_arg*10 + n;
	else {
		prefix_valid = 1;
		prefix_arg = n;
	}
	show_prefix();
}

void show_prefix(void)
{

	disp_status(": %d", prefix_arg);
}


@ shared functions from other function files.

@d<prototypes@>
void aux_scroll(int top, int sel, int force);
