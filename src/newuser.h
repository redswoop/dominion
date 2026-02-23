#ifndef _NEWUSER_H_
#define _NEWUSER_H_

#include "vardec_user.h"

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

#endif /* _NEWUSER_H_ */
