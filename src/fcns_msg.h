#ifndef _FCNS_MSG_H_
#define _FCNS_MSG_H_

#include "jammb.h"

/* File: jam.c */

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
int findnextwaiting(int msgnum,int old,userrec *u);
int findwaiting(void);
int numwaiting(userrec *u);
int getnextword(char *buf,long len, long *pos,char *s);
void quote_jam(char *buf,long len,hdrinfo *hdr);
int findnextthread(int msgnum);
void editpost(UINT32 *attr);


/* File: msgbase.c */

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


/* File: mm.c */

int slok(char val[31],char menu);
void msgcommand(char type,char ms[40]);
void othercmd(char type,char ms[40]);
int ex(char type[2],char ms[MAX_PATH_LEN]);
void menuman(void);
void handleinput(char *s,int begx);


/* File: mm1.c */

char *getfmt(char *fn, int which);
void parsemmfmt(char line[MAX_PATH_LEN]);
void readmnufmt(mmrec pf);
unsigned char bo(unsigned char c);  /* bold attribute */
char *noc2(char s1[100]);
int read_menu(char fn[15],int doauto);
int readmenu(char fn[15]);
int ccount(char s[MAX_PATH_LEN]);
char *aligncmd(char in[MAX_PATH_LEN]);
void drawhead(int top);
void showmenu();
void plfmt(char *s);
void plfmta(char *s,int *abort);
void addpop(char param[MAX_PATH_LEN]);
void batchpop(char param[MAX_PATH_LEN]);
void menubatch(char fn[12]);
void showmenucol();


/* File: mm2.c */

void getcmdtype(void);
void logtypes(char type,char *fmt, ...);
void badcommand(char onf,char tw);
void matrixcmd(char type);
void amsgcommand(char type);
void hangupcmd(char type,char ms[40]);
void sysopcmd(char type,char ms[41]);
int pmmkey(char *s);
void readmenup();
void bar(int where);
void drawheader(void);
void pldn(void);
void configpldn(int config);
void makerembox(int x,int y,int ylen,char *fn);
void aligncmd1(char in[MAX_PATH_LEN],char *cmd,char *desc);
char *makelen(char *in ,int len);
void popup(char *fn);


/* File: stringed.c */

int opp(int i);
char *disk_string(int whichstring,char *fn);
char *get_string(int whichstring);
char *get_string2(int whichstring);
char *getdesc(int whichstring);
char *get_say(int which);
void addsay(void);
void readstring(int which);
void extractstring(int which);
void liststring(int type,int where);
void edstring(int type);
void searchrum(void);

#endif /* _FCNS_MSG_H_ */
