#ifndef _FCNS_H_
#define _FCNS_H_

#include "platform.h"
#include "vardec.h"

#include "jammb.h"



/* File: bbs.c */

void main(int argc, char *argv[]);


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
void tleft(int dot);
void topscreen(void);
void initpointer(int init);
void executemouse(int x,int y);


/* File: bbsutl.c */

void far *malloca(unsigned long nbytes);
void stuff_in(char *s, char *s1, char *f1, char *f2, char *f3, char *f4, char *f5);
void inli(char *s, char *rollover, int maxlen, int crend);
int ainli(char *s, char *rollover, int maxlen, int crend,int slash,int back);
int binli(char *s, char *rollover, int maxlen, int crend,int slash,int back,int roll);
void checka(int *abort, int *next, int act);
void pla(char *s, int *abort);
void mla(char *s, int *abort);
void sl1(int cmd,char *s);
void sysoplog(char s[161]);
void isr1(int un, char *name);
char *smkey(char *avail,int num, int slash, int crend,int full);
int sysop2();
void setmci(char ch);


/* File: file.c */

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
int lfs(char spec[12],char ss[MAX_PATH_LEN],int *abort,long *bytes,int new);
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


/* File: file1.c */

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


/* File: mm.c */

int slok(char val[31],char menu);
void msgcommand(char type,char ms[40]);
void othercmd(char type,char ms[40]);
int ex(char type[2],char ms[MAX_PATH_LEN]);
void menuman(void);
void handleinput(char *s,int begx);


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
void mpl(int i);
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
char nek(char *s, int f);
char onek(char *s);
void prt(int i, char *s);
void inputdat(char msg[MAX_PATH_LEN],char *s, int len,int lc);


/* File: utility.c */

void reset_act_sl();
int sysop1();
int okansi();
int okavt();
void frequent_init();
void far *mallocx(unsigned long l);
double ratio();
double post_ratio();
char *pnam(userrec *u1);
char *nam(userrec *u1, unsigned int un);
unsigned int finduser(char *s);
void changedsl();
int checkacs(int w);


/* File: extrn.c */

void cd_to(char *s);
void get_dir(char *s, int be);
int do_it(char cl[MAX_PATH_LEN]);
int runprog(char *s, int swp);
void alf(int f, char *s);
char *create_chain_file(char *fn);
int restore_data(char *s);
void save_state(char *s, int state);
void dorinfo_def(void);
void write_door_sys(int rname);
void rundoor(char type,char ms[MAX_PATH_LEN]);


/* File: mm1.c */

char *getfmt(char *fn, int which);
void parsemmfmt(char line[MAX_PATH_LEN]);
void readmnufmt(mmrec pf);
unsigned char bo(unsigned char c);
char *noc2(char s1[100]);
int read_menu(char fn[15],int doauto);
int readmenu(char fn[15]);
int ccount(char s[MAX_PATH_LEN]);
char *aligncmd(char in[MAX_PATH_LEN]);
void drawhead(int top);
void showmenu();
void plfmt(char *s);
void plfmta(char *s,int *abort);
//int mslok(char val[MAX_PATH_LEN],char inp[MAX_PATH_LEN],int qyn,varrec *vars,int numvars);
void addpop(char param[MAX_PATH_LEN]);
void batchpop(char param[MAX_PATH_LEN]);
void menubatch(char fn[12]);
void showmenucol();


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


/* File: jam.c */

void errorjam(void);
void getjamhdr(hdrinfo *hdr1);
void show_message(int *next,int abort,char *buf,UINT32 len);
void read_msg(long recnr,int *next);
int DisplayMsgSubFld( void );
void scanj(int msgnum,int *nextsub,int sb, int private);
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


/* File: msgbase.c */

int external_edit(char *fn1, char *direc, int numlines);
int okfsed();
void addline(char *b, char *s, long *ll);
void showmsgheader(char a,char title[MAX_PATH_LEN],char name[41],char date[41],char to[41],int reading, int nummsgs,char comment[51],char subnum,int *abort);
void osan(char *s, int *abort, int *next);
void getorigin(int origin, originrec *or);
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


/* File: disk.c */

void read_in_file(char *fn, messagerec *m, int maxary);
double freek(int dr);
void fix_user_rec(userrec *u);
void close_user();
void open_user();
int number_userrecs();
void read_user(unsigned int un, userrec *u);
void write_user(unsigned int un, userrec *u);
void save_status();
void isr(int un, char *name);
void dsr(char *name);
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


/* File: timest.c */

void check_event();
void run_event();
unsigned char years_old(unsigned char m, unsigned char d, unsigned char y);
void itimer();
double timer();
long timer1();
double nsl();
char *date();
char *times();
void wait(double d);
void wait1(long l);
char *ctim(double d);
void ptime();
char *curtime(void);


/* File: utility1.c */

int finduser1(char *sx);
void ssm(unsigned int un, unsigned int sy, char *s);
void rsm(int un, userrec *u);


/* File: file2.c */

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
int get_protocol(int batch);
int extern_prot(int pn, char *fn1, int sending);
void stuff_in2(char *s, char *s1, char f1[63],int l1,char f2[63],int l2, char f3[63],int l3,char f4[63],int l4, char f5[63],int l5);
int dirlist(char type);
void genner(char *fn,char *spec, int type);
void gofer(void);
void listgen(void);


/* File: file3.c */

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
void fdnupload_files(char *fn, int dn, int type);
int fdnfilenet(void);


/* File: archive.c */

void selectarc(void);
int list_arc_out(char *fn, char *dir);
void add_arc(char *arc, char *fn);
void arcex(char *fn);
void arc_cl(int type);
int testarc(char *fn, char *dir);
int comment_arc(char *fn, char *dir,char *cmntfn);
int get_arc_cmd(char *out, char *arcfn, int cmd, char *ofn);
int adddiz(char *fn,uploadsrec *u);
void unarc(char *arc, char *fn);


/* File: filesys.c */

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


/* File: menued.c */

void menuinfoed(char fn[15]);
void edmenu (int x);
void top(char fn[15]);
void menu(char fn[15]);
void addmenu(char fn[15]);
void extractheader();
void readheader();
void listmform(void);
void menued(char fn[15]);


/* File: uedit.c */

void deluser(int un);
void addtrash(userrec u);
void print_data(int un, userrec *u,int lng);
int usearch(int un,char val[41]);
void uedit(int usern);


/* File: diredit.c */

void dirdata(int n, char *s);
void showdirs();
void swap_dirs(int dir1, int dir2);
void insert_dir(int n,char path[60], int temp,int config);
void delete_dir(int n);
void diredit();
void protdata(int n, char *s);
void showprots();
void insert_prot(int n);
void delete_prot(int n);
void protedit();


/* File: subedit.c */

void noc(char *s, char *s1);
void boarddata(int n, char *s);
void showsubs();
void swap_subs(int sub1, int sub2);
void insert_sub(int n,int config);
void delete_sub(int n);
void boardedit();
void confdata(int n, char *s);
void showconf();
void insert_conf(int n);
void delete_conf(int n);
void confedit();


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


/* File: bbsutl2.c */

void bbsCRC(void);
int so();
int cs();
int lcs();
void makewindow(int x, int y, int xlen, int ylen);
void outsat(char *s, int x, int y);
void editdata(char *str,int len,int xcoord,int ycoord);
int editdig(char *str,int len,int xcoord,int ycoord);
void editline(char *s, int len, int status, int *returncode, char *ss);
void reprint();
void setbeep(int i);
void pr_wait(int i1);
void set_autoval(int n);
void clickat(long byte,long bit,int x, int y);
int click(long *byt,long bit,int x, int y);
void val_cur_user(int wait);


/* File: sysopf.c */

void reset_files(int show);
void get_status();
void read_new_stuff();
void chuser();
void zlog();
void beginday();
void print_local_file(char ss[MAX_PATH_LEN]);
void text_edit();
void viewlog();
void glocolor(void);


/* File: modem.c */

void pr1(unsigned char *s);
void get_modem_line(char *s, double d, int allowa);
void holdphone(int d,int force);
void proresult(resultrec *ri);
int switchresult(char *s);
void imodem(int x);
void answer_phone();
int getcaller(void);
void gotcaller(unsigned int ms, unsigned int cs);
void topit(void);
void topit2(void);
char *curt(void);
void wfct(void);
void wfcs(void);
int ok_local();
void bargraph(int percent);


/* File: xinit.c */

void bargraph1(int percent);
void dotopinit(char fn[40],int per);
void init(int show);
void end_bbs(int lev);


/* File: personal.c */

void change_colors(userrec *u1);
void print_cur_stat();
char *cn(char c);
char *describe(char col);
void color_list();
void config_qscan(int dl);
void list_macro(unsigned char *s);
void make_macros();
void input_pw1();
void getfileformat();
void setcolors(userrec *uu);
void input_ansistat();
void selecthelplevel();
void getmsgformat();


/* File: misccmd.c */

void read_automessage();
void write_automessage();
void addbbs(char *fn);
void searchbbs(char *fn);
void list_users();
void yourinfo();
void jumpconf(char ms[41]);
void add_time(int limit);
void remove_time();
void bank2(int limit);
char *stl(long l);
char *sti(int i);
void updtopten(void);
char *topten(int type);
char *get_date_filename();
void today_history();
void dtitle(char msg[MAX_PATH_LEN]);
void selfval(char *param);
char *ctype(int which);
int numctype(void);


/* File: config.c */

unsigned char getkey();
void lpr(char *fmt, ...);
char *ctim(double d);
char *get_string2(int f);
void bitset(char *msg,int byte,int bit);
void togglebit(long *byte,int bit);
void bits(char *msg,int byte,int bit);
void getselect(char *s,int row,int col,int len,int lc);
void getselectt(unsigned short *i,int row,int col,int len);
int getselectd(int *i,int row,int col,int len);
void setbit(int row, int col,char bits[16],int *byte);
void getlist(int *val,char **list,int num);
void filecfg();
void namepath();
void flagged();
void varible();
void events();
void modeminfo();
void autoval();
void archive();
void secleved();
void nued();
void showfdr();
void insert_fdr(int n);
void delete_fdr(int n);
void fdredit();
void cfgorigin();
void fidocfg();
void fidocfg();
void acscfg(void);
void defcoled();
void config();
void main(int argc, char *argv[]);


/* File: lilo.c */

int getmuser();
void getmatrixpw(void);
void checkmatrixpw(void);
int matrix(void);
void getuser();
void logon();
void logoff();
void scrollfile(void);
void oneliner();
void fastscreen(char fn[13]);
void lastfewcall(void);
int check_ansi();
int checkpw();


/* File: error.c */

void err(int err,char *fn,char msg[80]);


/* File: chat.c */

void two_way_chat(char *s, char *rollover, int maxlen, int crend);
void chat1(char *chatline, int two_way);
void reqchat1(char reason[MAX_PATH_LEN]);
void chatsound();
void reqchat(char reason[MAX_PATH_LEN]);
void readfilter(char fn[15],char fn2[15]);
void viewfile();
void playmod(void);


/* File: nuv.c */

int num_nuv(char *fn);
void read_nuv(unsigned int user, char *fn, nuvdata *newuser);
void write_nuv(unsigned int user, char *fn, nuvdata *newuser);
void del_nuv(unsigned int user);
int enter_nuv(userrec tu,int un,int form);
int avoted(unsigned int user);
void print_nuv(nuvdata v);
int vote_nuv(unsigned int user, nuvdata *resn,int *done1);
void val_nuv(unsigned int user);
void nuv(void);


/* File: dv.c */

int get_dv_version(void);
void start_dv_crit(void);


/* File: newuser.c */

void go(int x,int y);
void goin(int x,int y);
int check_name(char *nn);
void input_comment(void);
void input_name(char *namer);
void input_realname(char *namer);
void input_city();
void input_phone();
void input_sex(userrec *u);
void input_age(userrec *u);
void input_comptype();
void input_screensize();
void input_pw();
void newuser();
void infoform(char fn[8],int once);
void readform(char fn[8],char i[31]);


/* File: regis.c */

void checkreg(void);

#endif
