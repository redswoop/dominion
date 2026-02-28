#ifndef _FILE2_H_
#define _FILE2_H_

#include "vardec_msgfile.h"
#include "vardec_types.h"

void yourinfodl();
void setldate();
void unlisteddl(char ms[MAX_PATH_LEN]);
void getfileinfo();
int printfileinfo(uploadsrec *u, int dn);
void displayformat();
void ascii_send(char *fn, int *sent, double *percent);
void send_file(char *fn, int *sent, int *abort, char *ft);
void receive_file(char *fn, int *received, char *ft, int okbatch);
int get_batchprotocol(int dl,int *hang);
int get_protocol(int is_batch);
int extern_prot(int pn, char *fn1, int sending);
void stuff_in2(char *s, char *s1, char f1[63],int l1,char f2[63],int l2, char f3[63],int l3,char f4[63],int l4, char f5[63],int l5);
int dirlist(char type);
void genner(char *fn,char *spec, int type);
void listgen(void);

#endif /* _FILE2_H_ */
