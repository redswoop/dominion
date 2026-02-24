#ifndef _LILO_H_
#define _LILO_H_

int getmuser();
void getmatrixpw(void);
void checkmatrixpw(void);
int matrix(void);
void getuser();
void logon();
void logoff();
void set_autoval(int n);
void scrollfile(void);
void oneliner();
void fastscreen(char fn[13]);
void lastfewcall(void);
int check_ansi();
int checkpw();

#endif /* _LILO_H_ */
