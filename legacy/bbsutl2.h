#ifndef _BBSUTL2_H_
#define _BBSUTL2_H_

void outsat(char *s, int x, int y);
void editdata(char *str,int len,int xcoord,int ycoord);
int editdig(char *str,int len,int xcoord,int ycoord);
void editline(char *s, int len, int status, int *returncode, char *ss);
void pr_wait(int i1);
void clickat(long byte,long bit,int x, int y);
int click(long *byt,long bit,int x, int y);

#endif /* _BBSUTL2_H_ */
