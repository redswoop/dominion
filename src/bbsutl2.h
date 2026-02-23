#ifndef _BBSUTL2_H_
#define _BBSUTL2_H_

void bbsCRC(void);
int so();
int cs();
int lcs();
void makewindow(int x, int y, int xlen, int ylen);
void outsat(char *s, int x, int y);
void editdata(char *str,int len,int xcoord,int ycoord);
int editdig(char *str,int len,int xcoord,int ycoord);
void editline(char *s, int len, int status, int *returncode, char *ss);
void reprint();
void setbeep(int i);
void pr_wait(int i1);
void set_autoval(int n);
void clickat(long byte,long bit,int x, int y);
int click(long *byt,long bit,int x, int y);
void val_cur_user(int wait);

#endif /* _BBSUTL2_H_ */
