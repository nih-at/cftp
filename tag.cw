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
	struct dirtags *next;
	struct filetags *tags;
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
extern dirtags *curtags;

@<local globals@>
dirtags tags;
dirtags *curtags;


@ functions to manipulate tags lists.

@d<prototypes@>
int tag_file(char *dir, char *file, int flag);

int
tag_file(char *dir, char *file, int flag)
{
    int filep;
    filetags *tl;

    if (dir == NULL)
	tl = curtags->tags;
    else {
	dirtags *d = tag_insdir(dir);
	if (d == NULL)
	    return -1;
	tl = d->tags;
    }

    filep = tag_filep(tl, file);
    
    if (filep && flag <= 0) {
	tag_delfile(tl, file);
	return 0;
    }
    else if (!filep && flag >= 0) {
	tag_insfile(tl, file);
	return 1;
    }
    else
	return filep;
}


@ utility functions.

@d<local prototypes@>
#define tag_getdir(dir)	\
	((dirtags *)tag_do((filetags *)&tags, (dir), 0, NULL))
#define tag_insdir(dir)	\
	((dirtags *)tag_do((filetags *)&tags, (dir), 1, tag_newdir))

#define tag_filep(t, file)	\
	(tag_do((t), (file), 0, NULL) != NULL)
#define tag_insfile(t, file)	\
	(tag_do((t), (file), 1, tag_newfile))
#define tag_delfile(t, file)	\
	(tag_do((t), (file), -1, tag_freefile))


@ do the actual work:
	root is the root of the list to be worked on.
	name is the name of the item to be worked on.
	flag says what to do: -1 for deletion, 0 for lookup,
			      1 for insertion.
	mem is a function called to alloc/dealoc an entry.

@d<local prototypes@>
filetags *tag_do(filetags *root, char *name, int flag, filetags (*mem)());

@u
filetags *
tag_do(filetags *root, char *name, int flag, filetags (*mem)())
{
    int c = -1;

    while (c < 0 && root->next)
	c = strcmp(dir, root->next->dir);
	
    if (c == 0) {
	if (flag >= 0)
	    return root->next;
	else {
	    filetags *d = root->next;

	    if (d != NULL) {
		root->next = d->next;
		mem(d);
	    }
	    return NULL;
	}
    }

    if (flag > 0)
	return NULL;

    filetags *n = mem(NULL);
    if (n != NULL) {
	n->next = root->next;
	root->next = n;
    }
    return n;
}
