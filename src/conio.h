#ifndef _CONIO_H_
#define _CONIO_H_

#include "vardec_ui.h"

void movecsr(int x,int y);
/* wherex/wherey declared in platform.h with extern "C" linkage */
void lf();
void cr();
void clrscrb();
void bs();
void reset_attr_cache(void);
void out1chx(unsigned char ch);
void out1ch(unsigned char ch);
void outs(const char *s);
void copy_line(char *s, char *b, long *ptr, long len);
void set_protect(int l);
void savescreen(screentype *s);
void restorescreen(screentype *s);
void temp_cmd(char *s, int ccc);
char scan_to_char(unsigned char ch,char *s);
int alt_key(unsigned char ch);
void skey(unsigned char ch);
void tleft(int dot);
void topscreen(void);

#endif /* _CONIO_H_ */
