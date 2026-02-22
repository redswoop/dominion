#ifndef _JSON_IO_H_
#define _JSON_IO_H_

#include "cJSON.h"
#include "vardec.h"

/* File read/write helpers */
cJSON *read_json_file(const char *path);
int write_json_file(const char *path, cJSON *root);

/* userrec serialization */
cJSON *userrec_to_json(userrec *u);
void json_to_userrec(cJSON *j, userrec *u);

/* configrec + niftyrec serialization */
cJSON *configrec_to_json(configrec *c, niftyrec *n);
void json_to_configrec(cJSON *j, configrec *c, niftyrec *n);

/* statusrec serialization */
cJSON *statusrec_to_json(statusrec *s);
void json_to_statusrec(cJSON *j, statusrec *s);

#endif /* _JSON_IO_H_ */
