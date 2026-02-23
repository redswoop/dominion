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

#endif /* _BBS_OUTPUT_H_ */
