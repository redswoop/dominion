#ifndef _ARCHIVE_H_
#define _ARCHIVE_H_

#include "vardec_msgfile.h"

void selectarc(void);
int list_arc_out(char *fn, char *dir);
void add_arc(char *arc, char *fn);
void arcex(char *fn);
void arc_cl(int type);
int testarc(char *fn, char *dir);
int comment_arc(char *fn, char *dir,char *cmntfn);
int get_arc_cmd(char *out, char *arcfn, int cmd, char *ofn);
int adddiz(char *fn,uploadsrec *u);
void unarc(char *arc, char *fn);

#endif /* _ARCHIVE_H_ */
