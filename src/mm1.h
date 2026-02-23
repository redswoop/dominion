#ifndef _MM1_H_
#define _MM1_H_

#include "vardec_ui.h"
#include "vardec_types.h"

char *getfmt(char *fn, int which);
void parsemmfmt(char line[MAX_PATH_LEN]);
void readmnufmt(mmrec pf);
unsigned char bo(unsigned char c);
char *noc2(char s1[100]);
int read_menu(char fn[15],int doauto);
int readmenu(char fn[15]);
int ccount(char s[MAX_PATH_LEN]);
char *aligncmd(char in[MAX_PATH_LEN]);
void drawhead(int top);
void showmenu();
void plfmt(char *s);
void plfmta(char *s,int *abort);
void menubatch(char fn[12]);
void showmenucol();

#endif /* _MM1_H_ */
