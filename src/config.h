#ifndef _CONFIG_H_
#define _CONFIG_H_

void bitset(char *msg,int byte,int bit);
void togglebit(long *byte,int bit);
void bits(char *msg,int byte,int bit);
void getselect(char *s,int row,int col,int len,int lc);
void getselectt(unsigned short *i,int row,int col,int len);
int getselectd(int row,int col,int len);
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
void acscfg(void);
void defcoled();
void config();

#endif /* _CONFIG_H_ */
