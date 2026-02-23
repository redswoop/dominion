#ifndef _FCNS_IO_H_
#define _FCNS_IO_H_

/* File: bbs_output.c */

int strlenc(char *s);
void setfgc(int i);
void setbgc(int i);
void setc(unsigned char ch);
void ansic(int n);
void stream_emit_char(unsigned char c);
void outchr(unsigned char c);
void outstr(char *s);
void outstrm(char *s);
void nl();
void pl(char *s);
void npr(char *fmt, ...);
void lpr(char *fmt, ...);
void logpr(char *fmt, ...);
void prt(int i, char *s);
void savel(char *cl, char *atr, char *xl, char *cc);
void restorel(char *cl, char *atr, char *xl, char *cc);

/* File: bbs_input.c */

int kbhitb();
char getchd();
char getchd1();
void checkhangup();
int empty();
void skey1(char *ch);
char inkey();
unsigned char getkey();

/* File: bbs_ui.c */

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


#endif /* _FCNS_IO_H_ */
