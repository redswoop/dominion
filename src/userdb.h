#ifndef _USERDB_H_
#define _USERDB_H_

#include "vardec_user.h"

/* Lifecycle */
void userdb_init(const char *datadir, int maxusers);
void userdb_rebuild_index(void);

/* User record I/O — pure disk, no caching */
void userdb_load(unsigned int un, userrec *u);
int  userdb_save(unsigned int un, userrec *u);  /* 0=ok, -1=fail */
int  userdb_max_num(void);                       /* highest user# on disk */

/* Index mutation */
void userdb_index_add(int un, char *name);
int  userdb_index_remove(char *name);            /* 0=ok, -1=not found */

/* Index queries */
unsigned int userdb_find(char *name);
void userdb_get_entry(int index, smalrec *out);
int  userdb_user_count(void);

#endif /* _USERDB_H_ */
