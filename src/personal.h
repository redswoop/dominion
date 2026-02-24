#ifndef _PERSONAL_H_
#define _PERSONAL_H_

#include "user.h"

void change_colors(User& u1);
void print_cur_stat();
char *cn(char c);
char *describe(char col);
void color_list();
void config_qscan(int dl);
void list_macro(char *s);
void make_macros();
void input_pw1();
void getfileformat();
void setcolors(User& uu);
void input_ansistat();
void selecthelplevel();
void getmsgformat();
void modify_mailbox(void);

#endif /* _PERSONAL_H_ */
