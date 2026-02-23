/* topten.cpp — Rankings/statistics (top ten lists). */

#include "topten.h"
#include "platform.h"
#include "bbs_output.h"
#include "bbs_input.h"
#include "bbs_ui.h"
#include "utility.h"
#include "session.h"
#include "userdb.h"
#include "system.h"

#include "mci.h"


char *stl(long l)
{
    static char s[10];

    ltoa(l,s,10);
    return(s);
}

char *sti(int i)
{
    static char s[10];

    itoa(i,s,10);
    return(s);
}


void updtopten(void)
{
    auto& sys = System::instance();
    userrec u;
    int i,i1,stop,num_users;
    char s1[161],s2[161],s3[10],s4[10],sname[31],s[5][10][31],ch;


    int low_posts_on_list,
    low_logons_on_list,
    posts_per_user[10],
    logons_per_user[10];

    unsigned long low_upk_on_list,
    low_dnk_on_list,
    up_k_per_user[10],
    dn_k_per_user[10];

    float low_time_on_list,
    timeon_per_user[10];


    low_posts_on_list  = 0;
    low_logons_on_list = 0;
    low_upk_on_list    = 0;
    low_dnk_on_list    = 0;
    low_time_on_list   = 0;

    for (i = 0; i < 10; i++) {
        posts_per_user[i]  = 0;
        logons_per_user[i] = 0;
        up_k_per_user[i]   = 0;
        dn_k_per_user[i]   = 0;
        timeon_per_user[i] = 0;
    }

    num_users = userdb_max_num();

    for (i = 0; i < 5; i++)
        for (i1 = 0; i1 < 10; i1++)
            strcpy(s[i][i1], "None");

    for (i = 1; i <= num_users; i++) {
        userdb_load(i, &u);
        if(u.inact & inact_deleted)
            continue;

        if (u.msgpost >= low_posts_on_list) {
            low_posts_on_list = posts_per_user[9];
            for (i1 = 0; i1 < 10; i1++)
                if (u.msgpost >= posts_per_user[i1]) {
                    stop = i1;
                    i1 = 10;
                }
            for (i1 = 9; i1 > stop; i1--) {
                posts_per_user[i1] =
                    posts_per_user[i1-1];
                strcpy(s[0][i1], s[0][i1-1]);
            }
            posts_per_user[stop] = u.msgpost;
            strcpy(s[0][stop], nam(&u,i));
        }

        if (u.logons >= low_logons_on_list) {
            low_logons_on_list = logons_per_user[9];
            for (i1 = 0; i1 < 10; i1++)
                if (u.logons >= logons_per_user[i1]) {
                    stop = i1;
                    i1 = 10;
                }
            for (i1 = 9; i1 > stop; i1--) {
                logons_per_user[i1] =
                    logons_per_user[i1-1];
                strcpy(s[1][i1], s[1][i1-1]);
            }
            logons_per_user[stop] = u.logons;
            strcpy(s[1][stop], nam(&u,i));
        }

        if (u.uk >= low_upk_on_list) {
            low_upk_on_list = up_k_per_user[9];
            for (i1 = 0; i1 < 10; i1++)
                if (u.uk >= up_k_per_user[i1]&&u.uk>0) {
                    stop = i1;
                    i1 = 10;
                }
            for (i1 = 9; i1 > stop; i1--) {
                up_k_per_user[i1] =
                    up_k_per_user[i1-1];
                strcpy(s[2][i1], s[2][i1-1]);
            }
            up_k_per_user[stop] = u.uk;
            strcpy(s[2][stop], nam(&u,i));
        }


        if (u.dk >= low_dnk_on_list) {
            low_dnk_on_list = dn_k_per_user[9];
            for (i1 = 0; i1 < 10; i1++)
                if (u.dk >= dn_k_per_user[i1]&&u.dk>0) {
                    stop = i1;
                    i1 = 10;
                }
            for (i1 = 9; i1 > stop; i1--) {
                dn_k_per_user[i1] =
                    dn_k_per_user[i1-1];
                strcpy(s[3][i1], s[3][i1-1]);
            }
            dn_k_per_user[stop] = u.dk;
            strcpy(s[3][stop], nam(&u,i));
        }

        if (u.timeon >= low_time_on_list) {
            low_time_on_list = timeon_per_user[9];
            for (i1 = 0; i1 < 10; i1++)
                if (u.timeon >= timeon_per_user[i1]) {
                    stop = i1;
                    i1 = 10;
                }
            for (i1 = 9; i1 > stop; i1--) {
                timeon_per_user[i1] =
                    timeon_per_user[i1-1];
                strcpy(s[4][i1], s[4][i1-1]);
            }
            timeon_per_user[stop] = u.timeon;
            strcpy(s[4][stop], nam(&u,i));
        }
    }

    sprintf(s1,"%stopten.dat",sys.cfg.datadir);
    i=open(s1,O_BINARY|O_RDWR|O_TRUNC|O_CREAT,S_IREAD|S_IWRITE);
    write(i,&s[0],5*10*31);
    write(i,&posts_per_user[0],10*sizeof(posts_per_user[0]));
    write(i,&logons_per_user[0],10*sizeof(logons_per_user[0]));
    write(i,&up_k_per_user[0],10*sizeof(up_k_per_user[0]));
    write(i,&dn_k_per_user[0],10*sizeof(dn_k_per_user[0]));
    write(i,&timeon_per_user[0],10*sizeof(timeon_per_user[0]));
    close(i);

}

char *topten(int type)
{
    auto& sys = System::instance();
    userrec u;
    int i,i1,stop,num_users,ok;
    char s1[161],s2[161],s3[10],s4[10],sname[31],s[5][10][31],ch;
    static char retstr[41];


    int low_posts_on_list,
    low_logons_on_list,
    posts_per_user[10],
    logons_per_user[10];

    unsigned long low_upk_on_list,
    low_dnk_on_list,
    up_k_per_user[10],
    dn_k_per_user[10];

    float low_time_on_list,
    timeon_per_user[10];


    sprintf(s1,"%stopten.dat",sys.cfg.datadir);
    i=open(s1,O_BINARY|O_RDWR);
    if(i<0) {
        updtopten();
        i=open(s1,O_BINARY|O_RDWR);
        if(i<0) return("Ouch");
    }

    read(i,&s[0],5*10*31);
    read(i,&posts_per_user[0],10*sizeof(posts_per_user[0]));
    read(i,&logons_per_user[0],10*sizeof(logons_per_user[0]));
    read(i,&up_k_per_user[0],10*sizeof(up_k_per_user[0]));
    read(i,&dn_k_per_user[0],10*sizeof(dn_k_per_user[0]));
    read(i,&timeon_per_user[0],10*sizeof(timeon_per_user[0]));
    close(i);


    i=mci_topten_which();

    switch(mci_topten_type()) {
    case 0:
        strcpy(sname,s[0][i]);
        sprintf(s3,"%d",posts_per_user[i]);
        break;
    case 1:
        strcpy(sname,s[1][i]);
        sprintf(s3,"%d",logons_per_user[i]);
        break;
    case 2:
        strcpy(sname,s[2][i]);
        sprintf(s3,"%ld",up_k_per_user[i]);
        break;
    case 3:
        strcpy(sname,s[3][i]);
        sprintf(s3,"%ld",dn_k_per_user[i]);
        break;
    case 4:
        strcpy(sname,s[4][i]);
        sprintf(s3,"%-6.0f",timeon_per_user[i]);
        break;
    }

    if(type)
        strcpy(s1,s3);
    else
        strcpy(s1,sname);

    if(s1[0])
        strcpy(retstr,s1);
    return retstr;

}
