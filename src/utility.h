#ifndef _UTILITY_H_
#define _UTILITY_H_

#include "vardec_user.h"

unsigned char upcase(unsigned char ch);
void reset_act_sl();
int sysop1();
int okansi();
int okavt();
void frequent_init();
void *mallocx(unsigned long l);
double ratio();
double post_ratio();
char *pnam(userrec *u1);
char *nam(userrec *u1, unsigned int un);
unsigned int finduser(char *s);
void changedsl();
int checkacs(int w);

#endif /* _UTILITY_H_ */
