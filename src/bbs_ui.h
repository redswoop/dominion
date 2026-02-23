#ifndef _BBS_UI_H_
#define _BBS_UI_H_

#include "vardec_types.h"

void backblue(void);
void backspace();
void pausescr();
void mpl(int i);
void mpl1(int i);
int input1(char *s, int maxlen, int lc, int crend);
void inputdate(char *s,int time);
int inputfone(char *s);
void input(char *s, int len);
void inputl(char *s, int len);
void inputdat(char msg[MAX_PATH_LEN],char *s, int len,int lc);
int ynn(int pos);
int ny();
int yn();
char nek(char *s, int f);
char onek(char *s);

#endif /* _BBS_UI_H_ */
