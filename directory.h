#ifndef HAD_DIRECTORY_Y
#define HAD_DIRECTORY_Y

/*
  $NiH: directory.h,v 1.13 2002/09/16 12:42:29 dillo Exp $

  directory.h -- handle directory cache
  Copyright (C) 1996-2007 Dieter Baron

  This file is part of cftp, a fullscreen ftp client
  The author can be contacted at <dillo@giga.or.at>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.
  3. The name of the author may not be used to endorse or promote
     products derived from this software without specific prior
     written permission.
 
  THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/



#include <sys/types.h>
#include <time.h>

struct direntry {
    char *line, *name;
    char *link;
    off_t size;
    time_t mtime;
    int pos;
    char type;
};

typedef struct direntry direntry;

struct directory {
    int len, top, cur;
    int size;
    struct direntry *line;
    char *path;
    int alloc_len;
    int sorted;
};

typedef struct directory directory;



int dir_add(directory *dir, direntry *entry);
int dir_find(directory *dir, char *entry);
void dir_free(directory *d);
directory *dir_new(void);
void dir_sort(directory *dir, int sort_type);
directory *get_dir(char *path, int force);



extern directory *curdir;

#endif /* directory.h */
