@ user setable options.

@(options.h@)
@<extern declarations@>

@u
@<globals@>


@ transfer mode. default is 'i', image.

@d<extern declarations@>
extern int opt_mode;

@d<globals@>
int opt_mode = 'i';


@ character to mark taged files.

@d<extern declarations@>
extern int opt_tagchar;

@d<globals@>
int opt_tagchar = '>';
