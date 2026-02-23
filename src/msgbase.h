#ifndef _MSGBASE_H_
#define _MSGBASE_H_

#include "vardec_msgfile.h"
#include "vardec_user.h"
#include "jammb.h"

int external_edit(char *fn1, char *direc, int numlines);
int okfsed();
void addline(char *b, char *s, long *ll);
void showmsgheader(char a,char title[MAX_PATH_LEN],char name[41],char date[41],char to[41],int reading, int msg_total,char comment[51],char subnum,int *abort);
void osan(char *s, int *abort, int *next);
void getorigin(int origin, originrec *orig);
void upload_post();
void extract_out(char *b, long len, hdrinfo *hdr);
void load_workspace(char *fnx, int no_edit);
void yourinfomsg();
int sublist(char type);
void get_quote();
char *ini(char name[MAX_PATH_LEN]);
void parseadd(char s[MAX_PATH_LEN],addressrec *a);
void parse_email_info(char *s, unsigned short *un1);
int inscan(int sb,userrec *u);
void togglenws(int sb,userrec *u,int scan);

#endif /* _MSGBASE_H_ */
