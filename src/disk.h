#ifndef _DISK_H_
#define _DISK_H_

#include <stdio.h>
#include "vardec_msgfile.h"

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

#endif /* _DISK_H_ */
