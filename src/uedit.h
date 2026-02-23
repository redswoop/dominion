#ifndef _UEDIT_H_
#define _UEDIT_H_

#include "vardec_user.h"

void deluser(int un);
void addtrash(userrec u);
void print_data(int un, userrec *u,int lng);
int usearch(int un,char val[41]);
void uedit(int usern);

#endif /* _UEDIT_H_ */
