#ifndef _JAM_BBS_H_
#define _JAM_BBS_H_

#include "jam/jammb.h"
#include "vardec_msgfile.h"
#include "user/user.h"
#include "vardec_types.h"

void errorjam(void);
void getjamhdr(hdrinfo *hdr1);
void show_message(int *next,int abort,char *buf,UINT32 len);
void read_msg(long recnr,int *next);
int DisplayMsgSubFld( void );
void scanj(int msgnum,int *nextsub,int sb, int is_private);
void rscanj(void);
char *ninmsg(hdrinfo *hdr1,long *len,int *save,int sb);
void replyj(int sb,int msgnum);
void post(int sb);
int okpost(void);
void SaveJamMsg(hdrinfo *hdr,long len, char *b,int sb);
int get_receiver(hdrinfo *hdr);
int inputhdr(hdrinfo *hdr,int usehdr,int sb);
void postjam(int sb,hdrinfo *hdr1,int usehdr);
CHAR8 * GetSubFldStr( JAMSUBFIELD * pSubFld );
int JamMsgInit( JAMAPIREC * pJam );
int JamMsgDeinit( JAMAPIREC * pJam );
int JamMsgWrite( JAMAPIREC * pJam, CHAR8 * pMsgTxt );
int JamMsgAddSFldStr( JAMAPIREC * pJam, UINT16 SubFld, CHAR8 * Str, UINT32 * pSubFldPos );
void JAMOpen(char *fn);
void JAMClose(void);
void readmailj(int msgnum,int sb);
void addLastRead(int num);
void saveLastRead(void);
void nscan(int sb,int *next);
void gnscan(void);
void email(int u,char subject[MAX_PATH_LEN],int ask);
void smail(char ms[MAX_PATH_LEN]);
int findnextwaiting(int msgnum, int old, const User& u);
int findwaiting(void);
int numwaiting(const User& u);
int getnextword(char *buf,long len, long *pos,char *s);
void quote_jam(char *buf,long len,hdrinfo *hdr);
int findnextthread(int msgnum);
void editpost(UINT32 *attr);

#endif /* _JAM_BBS_H_ */
