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


/* File: lilo.c */

int getmuser();
void getmatrixpw(void);
void checkmatrixpw(void);
int matrix(void);
void getuser();
void logon();
void logoff();
void scrollfile(void);
void oneliner();
void fastscreen(char fn[13]);
void lastfewcall(void);
int check_ansi();
int checkpw();


/* File: newuser.c */

void go(int x,int y);
void goin(int x,int y);
int check_name(char *nn);
void input_comment(void);
void input_name(char *namer);
void input_realname(char *namer);
void input_city();
void input_phone();
void input_sex(userrec *u);
void input_age(userrec *u);
void input_comptype();
void input_screensize();
void input_pw();
void newuser();
void infoform(char fn[8],int once);
void readform(char fn[8],char i[31]);


/* File: personal.c */

void change_colors(userrec *u1);
void print_cur_stat();
char *cn(char c);
char *describe(char col);
void color_list();
void config_qscan(int dl);
void list_macro(unsigned char *s);
void make_macros();
void input_pw1();
void getfileformat();
void setcolors(userrec *uu);
void input_ansistat();
void selecthelplevel();
void getmsgformat();


/* File: nuv.c */

int num_nuv(char *fn);
void read_nuv(unsigned int user, char *fn, nuvdata *newuser);
void write_nuv(unsigned int user, char *fn, nuvdata *newuser);
void del_nuv(unsigned int user);
int enter_nuv(userrec tu,int un,int form);
int avoted(unsigned int user);
void print_nuv(nuvdata v);
int vote_nuv(unsigned int user, nuvdata *resn,int *done1);
void val_nuv(unsigned int user);
void nuv(void);


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


/* File: utility1.c */

int finduser1(char *sx);
void ssm(unsigned int un, unsigned int sy, char *s);
void rsm(int un, userrec *u);


/* File: uedit.c */

void deluser(int un);
void addtrash(userrec u);
void print_data(int un, userrec *u,int lng);
int usearch(int un,char val[41]);
void uedit(int usern);


/* File: regis.c */

void checkreg(void);

#endif /* _FCNS_USER_H_ */
