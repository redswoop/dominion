#ifndef _FCNS_IO_H_
#define _FCNS_IO_H_

/* File: conio.c */

void movecsr(int x,int y);
int wherex();
int wherey();
void lf();
void cr();
void clrscrb();
void bs();
void reset_attr_cache(void);
void out1chx(unsigned char ch);
void out1ch(unsigned char ch);
void outs(char *s);
void copy_line(char *s, char *b, long *ptr, long len);
void set_protect(int l);
void savescreen(screentype *s);
void restorescreen(screentype far *s);
void temp_cmd(char *s, int ccc);
char scan_to_char(unsigned char ch,char *s);
int alt_key(unsigned char ch);
void skey(unsigned char ch);
void tleft(int dot);              /* time-left display */
void topscreen(void);
void initpointer(int init);
void executemouse(int x,int y);


/* File: com.c */

unsigned char upcase(unsigned char ch);
int strlenc(char *s);
void savel(char *cl, char *atr, char *xl, char *cc);
void restorel(char *cl, char *atr, char *xl, char *cc);
void checkhangup();
void addto(char *s, int i);
void makeavt(unsigned char attr, char *s, int forceit);
void makeansi(unsigned char attr, char *s, int forceit);
void setfgc(int i);
void setbgc(int i);
void execute_ansi();
void outchr(unsigned char c);
void outstr(char *s);
void outstrm(char *s);
void nl();
void backblue(void);
void backspace();
void setc(unsigned char ch);
void pausescr();
void npr(char *fmt, ...);
void lpr(char *fmt, ...);
void logpr(char *fmt, ...);
void pl(char *s);
int kbhitb();
int empty();
void skey1(char *ch);
char getchd();
char getchd1();
char inkey();
void mpl(int i);              /* moveable prompt line */
void mpl1(int i);
unsigned char getkey();
int input1(char *s, int maxlen, int lc, int crend);
void inputdate(char *s,int time);
int inputfone(char *s);
void input(char *s, int len);
void inputl(char *s, int len);
int ynn(int pos);
int ny();
int yn();
void ansic(int n);
char nek(char *s, int f);     /* next-key with echo */
char onek(char *s);
void prt(int i, char *s);
void inputdat(char msg[MAX_PATH_LEN],char *s, int len,int lc);


/* File: x00com.c */

void dtr(int i);
void outcomch(char ch);
char peek1c();
char get1c();
int comhit();
void dump();
void set_baud(unsigned int rate);
void initport(int port_num);
void closeport();
int cdet();
void send_telnet_negotiation(int fd);
void send_terminal_init(int fd);
void send_terminal_restore(int fd);


/* File: wfc.c */

int getcaller(void);
void gotcaller(unsigned int ms, unsigned int cs);
void topit(void);
void topit2(void);
char *curt(void);
void wfct(void);
void wfcs(void);
int ok_local();
void bargraph(int percent);

#endif /* _FCNS_IO_H_ */
