#ifndef _STRINGED_H_
#define _STRINGED_H_

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

#endif /* _STRINGED_H_ */
