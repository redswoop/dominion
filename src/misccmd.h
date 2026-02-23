#ifndef _MISCCMD_H_
#define _MISCCMD_H_

#include "vardec_types.h"

void list_users();
void yourinfo();
void jumpconf(char ms[41]);
char *get_date_filename();
void today_history();
void dtitle(char msg[MAX_PATH_LEN]);
void selfValidationCheck(char *param);
char *getComputerType(int which);
int numctype(void);
int numComputerTypes(void);

#endif /* _MISCCMD_H_ */
