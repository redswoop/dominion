#ifndef _WFC_H_
#define _WFC_H_

int getcaller(void);
void gotcaller(unsigned int ms, unsigned int cs);
void topit(void);
void topit2(void);
char *curt(void);
void wfct(void);
void wfcs(void);
int ok_local();
void bargraph(int percent);

/* Supervisor mode: poll listen_fd for incoming TCP connections.
 * Returns accepted_fd on connection, -1 if nothing to accept. */
int wfc_poll_accept(void);

#endif /* _WFC_H_ */
