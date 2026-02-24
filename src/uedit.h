#ifndef _UEDIT_H_
#define _UEDIT_H_

#include "user.h"

void deluser(int un);
void addtrash(const User& u);
void print_data(int un, const User& u, int lng);
int usearch(int un,char val[41]);
void uedit(int usern);
void val_cur_user(int wait);

#endif /* _UEDIT_H_ */
