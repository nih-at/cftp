@ structures and routines to handle tagging of files for later
download.

@(directory.h@)
@<types@>
@<prototypes@>
@<globals@>

@u
#include <stdio.h>
#include <string.h>
@<local types@>
@<local prototypes@>
@<local globals@>


@ tagging information is stored in a sorted list (one entry for each
directory) containing sorted lists (one entry for each file).

@<types@>
struct dirtags {
	char *dir;
	struct filetags *tags;
	struct dirtags *next;
};

struct filetags {
	char *name;
	struct filetags *next;
};

typedef struct dirtags dirtags;
typedef struct filetags filetags;


@ all taged files are kept in the global variable tags, all tags in
the current directory are kept in the variable curtags.

@<globals@>
extern dirtags tags;
extern filetags curtags;

@<local globals@>
dirtags tags;
filetags curtags;

