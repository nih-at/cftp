@ structures and routines to handle tagging of files for later
download.

@(tag.h@)
@<types@>
@<prototypes@>
@<globals@>

@u
#include <stdio.h>
#include <string.h>
#include "tag.h"

@<local prototypes@>
@<local globals@>


@ tagging information is stored in a sorted list (one entry for each
directory) containing sorted lists (one entry for each file).

@d<types@>
struct dirtags {
    char *name;
    struct dirtags *next;
    struct filetags *tags;
};

struct filetags {
    char *name;
    struct filetags *next;
    long size;
    char type;
};

typedef struct dirtags dirtags;
typedef struct filetags filetags;


@ all taged files are kept in the global variable tags, all tags in
the current directory are kept in the variable curtags.

@d<globals@>
extern dirtags tags;
extern dirtags *curtags;

@d<local globals@>
dirtags tags;
dirtags *curtags;


@ .

@d<prototypes@>
void tag_changecurrent(char *dir);

@u
void
tag_changecurrent(char *dir)
{
    curtags = tag_getdir(dir);
}


@ functions to manipulate tags lists.

@d<prototypes@>
int tag_file(char *dir, char *file, long size, char type, int flag);

@u
int
tag_file(char *dir, char *file, long size, char type, int flag)
{
    extern char *ftp_lcwd;
    
    int filep;
    filetags *tl, *n;
 
    if (dir == NULL) {
	if (curtags == NULL) {
	    curtags = tag_insdir(ftp_lcwd);
	    if (curtags == NULL)
		return -1;
	}
	tl = curtags->tags;
    }
    else {
	dirtags *d;

	if ((d=tag_insdir(dir)) == NULL)
	    return -1;
	tl = d->tags;
    }

    filep = tag_filep(tl, file);
    
    if (filep && flag <= 0) {
	tag_delfile(tl, file);
	return 0;
    }
    else if (!filep && flag >= 0) {
	if (n=tag_insfile(tl, file)) {
	    n->size = size;
	    n->type = type;
	    return 1;
	}
	else
	    return 0;
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
filetags *tag_do(filetags *root, char *name, int flag, filetags *(*mem)());

@u
filetags *
tag_do(filetags *root, char *name, int flag, filetags *(*mem)())
{
    filetags *n;
    int c = 1;

    while (root->next) {
	if ((c=strcmp(name, root->next->name)) <= 0)
	    break;
	root = root->next;
    }
	
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

    if (flag <= 0)
	return NULL;

    n = mem(NULL);

    if (n != NULL) {
	n->name = strdup(name);
	n->next = root->next;
	root->next = n;
    }
    return n;
}


@ alloc/dealloc functions.

@d<local prototypes@>
filetags *tag_newdir(filetags *dummy);
filetags *tag_freedir(filetags *d);
filetags *tag_newfile(filetags *dummy);
filetags *tag_freefile(filetags *f);

@u
filetags *
tag_newdir(filetags *dummy)
{
    dirtags *d = (dirtags *)malloc(sizeof(dirtags));

    if (d) {
	d->next = NULL;
	d->tags = tag_newfile(NULL);
	d->name = NULL;
    }
    
    return (filetags *)d;
}

filetags *
tag_freedir(filetags *f)
{
    dirtags *d = (dirtags *)f;
    filetags *g;

    f=d->tags;
    while (f) {
	g=f;
	f=f->next;
	free(g->name);
	free(g);
    }
    free(d->name);
    free(d);

    return NULL;
}

filetags *
tag_newfile(filetags *dummy)
{
    filetags *f = (filetags *)malloc(sizeof(filetags));

    if (f) {
	f->name = NULL;
	f->next = NULL;
    }

    return f;
}

filetags *
tag_freefile(filetags *f)
{
    if (f) {
	free(f->name);
	free(f);
    }

    return NULL;
}


@ checking if any tags exist.

@d<prototypes@>
int tag_anytags(void);

@u
int
tag_anytags(void)
{
    dirtags *d;

    for (d=tags.next; d; d=d->next)
	if (d->tags && d->tags->next)
	    return 1;

    return 0;
}
