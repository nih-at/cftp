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

@<local prototypes@>

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

	    if (f->type == FN_EXIT
		&& fnexit() == 0)
		return;

	    if (f->type != FN_PRE)
		disp_status("");

	    if (f->type != FN_EXIT)
		f->fn(binding_args[c]);

	    if (f->type != FN_PRE)
		void_prefix();
	}
	else {
	    disp_status("");
	    void_prefix();
	}
}


@ check if we should really exit.

@d<local prototypes@>
int fnexit(void);

@u
int
fnexit(void)
{
    int c, ret = 0;
    
    if (tag_anytags()) {
	c = read_char("Tags remaining; really quit? ");
	if (c != 'y' && c != 'Y')
	    ret = 1;
    }

    return ret;
}
