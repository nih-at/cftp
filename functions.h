#ifndef HAD_FUNCTIONS_H
#define HAD_FUNCTIONS_H

typedef struct function {
	void (*fn)();
	char *name;
	int type;
	char *help;
} function;

extern function functions[];

#define FN_PRE	1
#define FN_EXIT	-1



extern directory *curdir;
extern int curtop, cursel;



void void_prefix(void);
int get_prefix(int deflt);
int get_prefix_args(char **args, int deflt);
void add_prefix(int n);
void set_prefix(int p);
void negate_prefix(void);
void show_prefix(void);
void change_curdir(directory *dir);
int find_function(char *f);
void aux_scroll(int top, int sel, int force);

#endif /* functions.h */
