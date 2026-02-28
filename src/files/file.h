#ifndef _FILE_H_
#define _FILE_H_

#include "vardec_msgfile.h"
#include "vardec_types.h"

char *getfhead(int bot);
int ratio_ok();
int postr_ok();
int filer_ok();
void dliscan1(int dn);
void dliscan();
void closedl();
void align(char *s);
int compare(char s1[15], char s2[15]);
int printinfo(uploadsrec *u, int *abort,int number);
void printtitle();
int file_mask(char *s);
int pauseline(int line,int *abort);
int lfs(char spec[12],char ss[MAX_PATH_LEN],int *abort,long *bytes,int isnew);
int changefarea(void);
void listfiles(char ms[40]);
int nscandir(int d, int *abort, int title,int *next);
void nscanall();
int recno(char s[15]);
int nrecno(char s[15],int i1);
int checkdl(uploadsrec u,int dn);
void finddescription(char ms[41]);
void setformat();
int okfn(char *s);
void nnscan(char ms[41]);
int dcs(void);

#endif /* _FILE_H_ */
