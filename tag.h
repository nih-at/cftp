#ifndef HAD_TAG_H
#define HAD_TAG_H

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

extern dirtags tags;
extern dirtags *curtags;



void tag_changecurrent(char *dir);
int tag_file(char *dir, char *file, long size, char type, int flag);
int tag_anytags(void);

#endif /* tag.h */
