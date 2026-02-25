#ifndef _NEWUSER_H_
#define _NEWUSER_H_

#include "user.h"

/* Cursor helpers — also used by cmd_registry.cpp and config.cpp */
void go(int x, int y);
void goin(int x, int y);

int  check_name(const char *nn);

/* Input helpers — some still called from lilo.cpp and uedit.cpp */
void input_sex(User& u);
void input_age(User& u);
void input_city(void);
void input_comptype(void);
void input_screensize(void);

void newuser(void);
void infoform(char fn[8], int once);
void readform(char fn[8], char i[31]);

#endif /* _NEWUSER_H_ */
