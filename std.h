

/* File: std.c */

void addto(char *s, int i);
void makeansi(unsigned char attr, char *s);
void outchr(unsigned char c);
void SetAttribute(unsigned char c);
void outstr(char *s);
void nl(void);
void npr(char *fmt, ...);
void pl(char *s);
void pr(char *fmt, ...);
void pla(char *s);
void input1(char *s, int maxlen, int lc, int crend);
void input(char *s,int len);
int strlenc(char *s);
char onek(char *s);
void pausescr();
void cd_to(char *s);
void InitText();
void EndText();
void Gotoxy(int x,int y);
void Cls();
