#include <stdio.h>
#include "directory.h"
#include "display.h"
#include "functions.h"
#include "tag.h"
#include "options.h"

directory *curdir;
int curtop, cursel;

int prefix_arg, prefix_valid = 0;



void
void_prefix(void)
{
	prefix_valid = 0;
}



int
get_prefix(int deflt)
{
	return (prefix_valid ? prefix_arg : deflt);
}



int
get_prefix_args(char **args, int deflt)
{
    if (args)
	return atoi(args[0]);
    else
	return (prefix_valid ? prefix_arg : deflt);
}



void
add_prefix(int n)
{
	if (prefix_valid)
		prefix_arg = prefix_arg*10 + n;
	else {
		prefix_valid = 1;
		prefix_arg = n;
	}
	show_prefix();
}



void
set_prefix(int p)
{
    prefix_valid = 1;
    prefix_arg = p;
    show_prefix();
}



void
negate_prefix(void)
{
    return;
}



void
show_prefix(void)
{

	disp_status(": %d", prefix_arg);
}



void
change_curdir(directory *dir)
{
    extern dirtags *curtags;

    filetags *tags;
    int i;

    for (i=0; i<curdir->num; i++)
	curdir->list[i].line[0] = ' ';

    tag_changecurrent(dir->path);

    if (curtags) {
	for (tags=curtags->tags->next; tags; tags=tags->next)
	    if ((i=dir_find(dir, tags->name)) >= 0)
		dir->list[i].line[0] = opt_tagchar;
    }

    curdir = dir;
}



int
find_function(char *f)
{
    int i;
    
    for (i=0; functions[i].name && strcmp(f, functions[i].name); i++)
	    ;

    return (functions[i].name ? i : -1);
}
