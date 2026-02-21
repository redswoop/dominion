#ifndef _FCNS_SYS_H_
#define _FCNS_SYS_H_

/* File: bbs.c */

void main(int argc, char *argv[]);


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


/* File: xinit.c */

void bargraph1(int percent);
void dotopinit(char fn[40],int per);
void init(int show);
void end_bbs(int lev);


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
void acscfg(void);
void defcoled();
void config();


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


/* File: dv.c */

int get_dv_version(void);
void start_dv_crit(void);

#endif /* _FCNS_SYS_H_ */
