#ifndef HAD_KEYS_H
#define HAD_KEYS_H

struct keyname {
	int key;
	char *name, *longname;
};

struct fnkey {
	char *name, *longname;
	char *cap, *seq;
};

extern struct keyname keyname[];
extern struct fnkey fnkey[];
extern int max_fnkey;



char *print_key(int key, int longp);
int parse_key(char *kn);

#endif /* keys.h */


