#ifndef _EXTRN_H_
#define _EXTRN_H_

#include "vardec_types.h"

void cd_to(char *s);
void get_dir(char *s, int be);
int do_it(char cl[MAX_PATH_LEN]);
int runprog(char *s, int swp);
void alf(int f, const char *s);
char *create_chain_file(char *fn);
int restore_data(char *s);
void save_state(char *s, int state);
void dorinfo_def(void);
void write_door_sys(int rname);
void rundoor(char type,char ms[MAX_PATH_LEN]);

#endif /* _EXTRN_H_ */
