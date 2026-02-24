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

void outsat(const char *s, int x, int y);
void editdata(char *str,int len,int xcoord,int ycoord);
int editdig(char *str,int len,int xcoord,int ycoord);
void editline(char *s, int len, int status, int *returncode, char *ss);
void pr_wait(int i1);
void clickat(long byte,long bit,int x, int y);
int click(long *byt,long bit,int x, int y);

/* Interactive user search with Y/N/Q confirmation prompts */
int finduser1(char *sx);

#endif /* _BBS_UI_H_ */
