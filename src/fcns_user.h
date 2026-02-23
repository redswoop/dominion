#ifndef _FCNS_USER_H_
#define _FCNS_USER_H_

#include "userdb.h"

/* File: disk.c */

void read_in_file(char *fn, messagerec *m, int maxary);
double freek(int dr);
void save_status();
double freek1(char *s);
char *get_file(char *fn, long *len);
void set_global_handle(int i);
void global_char(char ch);
int exist(char *s);
FILE *flopen(char *fn,char *mode,long *len);
void filter(char *s,unsigned char c);
void remove_from_temp(char *fn, char *dir, int po);
void showfile(char *fn);
void printmenu(int which);
int printfile(char *fn);


/* File: utility.c */

unsigned char upcase(unsigned char ch);
void reset_act_sl();
int sysop1();
int okansi();
int okavt();
void frequent_init();
void far *mallocx(unsigned long l);
double ratio();
double post_ratio();
char *pnam(userrec *u1);
char *nam(userrec *u1, unsigned int un);
unsigned int finduser(char *s);
void changedsl();
int checkacs(int w);


#endif /* _FCNS_USER_H_ */
