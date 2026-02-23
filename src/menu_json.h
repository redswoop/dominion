#ifndef _MENU_JSON_H_
#define _MENU_JSON_H_

#include "menudb.h"
#include "cJSON.h"

/* Convert menu_data_t to/from cJSON tree */
cJSON *menu_data_to_json(const menu_data_t *data);
int    json_to_menu_data(cJSON *root, menu_data_t *out);

/* File-level load/save */
int menu_json_load(const char *path, menu_data_t *out);   /* 0=ok, -1=not found, -2=parse error */
int menu_json_save(const char *path, const menu_data_t *data);  /* 0=ok, -1=write error */

#endif /* _MENU_JSON_H_ */
