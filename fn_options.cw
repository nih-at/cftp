@ manipulating and viewing options.

@(fn_options.fn@)
; fn_options
@<functions@>

@u
#include "directory.h"
#include "functions.h"
#include "display.h"
#include "options.h"

@<local prototypes@>


@ transfer mode.

@d<local prototypes@>
void aux_show_mode(void);
void aux_set_mode(int mode);

@u
void
aux_show_mode(void)
{
	disp_status("using %s mode", opt_mode == 'a' ? "ascii": "image");
}


@d<functions@>
  { fn_ascii, "ascii", 0, "use ascii mode for downloads" }
  { fn_image, "image", 0, "use image (binary) mode for downloads" }

@u
void
fn_ascii(void)
{
	opt_mode = 'a';
	aux_show_mode();
}

void
fn_image(void)
{
	opt_mode = 'i';
	aux_show_mode();
}
