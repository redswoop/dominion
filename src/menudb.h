#ifndef _MENUDB_H_
#define _MENUDB_H_

#include "vardec_ui.h"

#define MAX_MENU_COMMANDS 50

typedef struct {
    char    name[20];
    mmrec   header;
    menurec commands[MAX_MENU_COMMANDS];
    int     count;
} menu_data_t;

/* Lifecycle */
void menudb_init(const char *menudir);

/* CRUD */
int  menudb_create(const char *key, const menu_data_t *data);  /* 0=ok, -1=already exists */
int  menudb_load(const char *key, menu_data_t *out);           /* 0=ok, -1=not found, -2=read error */
int  menudb_save(const char *key, const menu_data_t *data);    /* 0=ok, -1=write error */
int  menudb_delete(const char *key);                           /* 0=ok, -1=not found */
int  menudb_exists(const char *key);                           /* 1=yes, 0=no */
int  menudb_list(char names[][20], int max_names);             /* returns count */

#endif /* _MENUDB_H_ */
