#ifndef _FILE3_H_
#define _FILE3_H_

#include "vardec_msgfile.h"
#include "vardec_types.h"

void add_extended_description(char *fn, char *desc);
void delete_extended_description(char *fn);
char *read_extended_description(char *fn);
int print_extended(char *fn, int *abort, unsigned char numlist, int indent);
int count_extended(char *fn);
int finddup(char *fn,int quiet);
void listbatch();
void batchdled(int stay);
void copyupfile(char fn[12],char todir[MAX_PATH_LEN],char fdir[MAX_PATH_LEN]);
char *stripfn(char *fn);
void stripfn1(char *fn);
int upload_file2(char *fn, int dn, char *desc);
int maybe_upload(char *fn, int dn, char *desc);
void modify_extended_description(char **sss);
void verify_hangup(void);
void addgif(uploadsrec *u, char *path);

#endif /* _FILE3_H_ */
