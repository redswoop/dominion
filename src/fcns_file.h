#ifndef _FCNS_FILE_H_
#define _FCNS_FILE_H_

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

/* File: file3.c (additional) */

void modify_extended_description(char **sss);
void verify_hangup(void);
void addgif(uploadsrec *u, char *path);

/* File: file.c (additional) */

int dcs(void);

#endif /* _FCNS_FILE_H_ */
