@ Command loop

@(loop.h@)
@<prototypes@>

@u
#include "directory.h"
#include "functions.h"
#include "display.h"
#include "tty.h"

extern int binding[];
extern char **binding_args[];

@ the loop itself.

@d<prototypes@>
void loop(void);

@u
void
loop()
{
	int c, ret = 0;
	function *f;

	disp_dir(curdir, curtop, cursel, 1);
	
	while ((c=tty_readkey()) != -1)
		if (binding[c] != -1) {
			f = functions+binding[c];

			if (f->type == FN_EXIT)
				return;

			if (f->type != FN_PRE)
				disp_status("");

			f->fn(binding_args[c]);

			if (f->type != FN_PRE)
				void_prefix();
		}
		else {
			disp_status("");
			void_prefix();
		}
}
