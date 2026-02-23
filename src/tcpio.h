#ifndef _TCPIO_H_
#define _TCPIO_H_

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

#endif /* _TCPIO_H_ */
