#ifndef _FCNS_SYS_H_
#define _FCNS_SYS_H_

/* File: bbsutl.c */

void far *malloca(unsigned long nbytes);
void stuff_in(char *s, char *s1, char *f1, char *f2, char *f3, char *f4, char *f5);
void stuff_in1(char *s, char *s1, char *f1, char *f2, char *f3, char *f4, char *f5, char *f6, char *f7, char *f8, char *f9, char *f0);
void inli(char *s, char *rollover, int maxlen, int crend);
int ainli(char *s, char *rollover, int maxlen, int crend,int slash,int back);
int binli(char *s, char *rollover, int maxlen, int crend,int slash,int back,int roll);
void checka(int *abort, int *next, int act);
void pla(char *s, int *abort);
void mla(char *s, int *abort);
void sl1(int cmd,char *s);
void sysoplog(char s[161]);
char *smkey(char *avail,int num, int slash, int crend,int full);
int sysop2();
void setmci(char ch);


/* File: bbsutl2.c */

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


/* File: timest.c */

void check_event();
void run_event();
unsigned char years_old(unsigned char m, unsigned char d, unsigned char y);
void itimer();
double timer();
long timer1();
double nsl();
char *date();
char *times();
void wait(double d);
void wait1(long l);
char *ctim(double d);
void ptime();
char *curtime(void);


#endif /* _FCNS_SYS_H_ */
