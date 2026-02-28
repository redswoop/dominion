#ifndef _BBS_OUTPUT_H_
#define _BBS_OUTPUT_H_

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
void checka(int *abort, int *next, int act);
void pla(char *s, int *abort);
void mla(char *s, int *abort);
void reprint();

/* Display functions (relocated from disk.cpp) */
void showfile(char *fn);
void printmenu(int which);
int printfile(char *fn);

/* Output capture (relocated from disk.cpp) */
void set_global_handle(int i);
void global_char(char ch);

#endif /* _BBS_OUTPUT_H_ */
