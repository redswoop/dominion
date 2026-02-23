#ifndef _TIMEST_H_
#define _TIMEST_H_

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

#endif /* _TIMEST_H_ */
