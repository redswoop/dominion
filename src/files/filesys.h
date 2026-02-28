#ifndef _FILESYS_H_
#define _FILESYS_H_

#include "vardec_msgfile.h"
#include "vardec_types.h"

int getrec(char *spec,int *type);
void getnextrec(char *spec,int *cp,int type);
int comparedl(uploadsrec *x, uploadsrec *y, int type);
void quicksort(int l,int r,int type);
void sortdir(int dn, int type);
void sort_all(char ms[MAX_PATH_LEN]);
void valfiles();
int upload_file(char *fn, int dn,int *ato);
int uploadall(int dn, char s[20]);
void removefile(void);
void editfile();
void localupload();
void create_file();
void del_entry(int which);
void move_file(void);

#endif /* _FILESYS_H_ */
