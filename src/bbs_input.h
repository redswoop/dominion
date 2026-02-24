#ifndef _BBS_INPUT_H_
#define _BBS_INPUT_H_

int kbhitb();
char getchd();
char getchd1();
void checkhangup();
int empty();
void skey1(char *ch);
char inkey();
unsigned char getkey();

void inli(char *s, char *rollover, int maxlen, int crend);
int ainli(char *s, char *rollover, int maxlen, int crend,int slash,int back);
int binli(char *s, char *rollover, int maxlen, int crend,int slash,int back,int roll);
char *smkey(char *avail,int num, int slash, int crend,int full);

#endif /* _BBS_INPUT_H_ */
