#ifndef _SYSOPF_H_
#define _SYSOPF_H_

#include "vardec_types.h"

void reset_files(int show);
void get_status();
void read_new_stuff();
void chuser();
void zlog();
void beginday();
void print_local_file(char ss[MAX_PATH_LEN]);
void text_edit();
void viewlog();
void glocolor(void);
void logtypes(char type,char *fmt, ...);

#endif /* _SYSOPF_H_ */
