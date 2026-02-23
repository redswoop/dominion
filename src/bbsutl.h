#ifndef _BBSUTL_H_
#define _BBSUTL_H_

void *malloca(unsigned long nbytes);
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

#endif /* _BBSUTL_H_ */
