#ifndef _NUV_H_
#define _NUV_H_

#include "vardec_types.h"
#include "vardec_user.h"

typedef struct {
        int vote,
            counts,
            sl;

        char say[41],
             name[41];
} vcrec;

typedef struct {
        int num,
            age;
        char firston[8],
            name[41],
             snote[MAX_PATH_LEN];
        int vote_yes,
            vote_no,
            vcmt_num;
         vcrec vote_comment[20];
} nuvdata;

int num_nuv(char *fn);
void read_nuv(unsigned int user, char *fn, nuvdata *newuser);
void write_nuv(unsigned int user, char *fn, nuvdata *newuser);
void del_nuv(unsigned int user);
int enter_nuv(userrec tu,int un,int form);
int avoted(unsigned int user);
void print_nuv(nuvdata v);
int vote_nuv(unsigned int user, nuvdata *resn,int *done1);
void val_nuv(unsigned int user);
void nuv(void);

#endif /* _NUV_H_ */
