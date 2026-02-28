#ifndef _FILE1_H_
#define _FILE1_H_

#include "vardec_msgfile.h"

void batrec(int rw, int bnum);
void delbatch(int i);
void downloaded(char *fn);
void upload_batch_file(int blind);
void uploaded(char *fn);
void handle_dszline(char *l);
void process_dszlog();
void batchul(int t);
void batchdl(int t);
void newdl(int dn);
void mark(int dn);
int findfile(int dn,char s[15]);
void printbatchstat(int dl);
void send_batch(void);
void download(int dn,int mark);
void newul(int dn);
void addtobatch(uploadsrec u,int dn,int sending);
void upload(char ms[41]);

#endif /* _FILE1_H_ */
