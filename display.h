#ifndef HAD_DISPLAY_H
#define HAD_DISPLAY_H

#include <stdio.h>

extern int tty_lines;
#define win_lines	(tty_lines-4)



int init_disp(void);
void exit_disp();
void escape_disp(int clearp);
void reenter_disp(void);
void disp_redraw(void);
void disp_dir(directory *d, int top, int sel, int newdir);
void disp_reline(int line);
char *read_string(char *prompt, int echop);
int read_char(char *prompt);
int disp_prompt_char(void);
void disp_status(char *fmt, ...);
void disp_head(char *fmt, ...);
FILE *disp_open(int lines);
int disp_close(FILE *f);

#endif /* display.h */
