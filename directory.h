#ifndef HAD_DIRECTORY_Y
#define HAD_DIRECTORY_Y

typedef struct direntry {
	char *line;
	char *name, *link;
	long size;
	char type;
} direntry;

typedef struct directory {
	char *path;
	int num;
	struct direntry *list;
} directory;



void dir_free(directory *d);
directory *get_dir(char *path);
int dir_find(directory *dir, char *entry);

#endif /* directory.h */
