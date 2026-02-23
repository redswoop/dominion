#include "vars.h"
#pragma hdrstop


#include <math.h>

int finduser1(char *sx)
{
    int i,i1,i2;
    char s[MAX_PATH_LEN],s1[MAX_PATH_LEN],ch;
    userrec u;

    if (sx[0]==0)
        return(0);
    i=finduser(sx);
    if (i>0)
        return(i);
    if(strcmpi(sx,"SYSOP")==0) {
        return 1;
    }
    strcpy(s,sx);
    for (i=0; s[i]!=0; i++)
        s[i]=toupper(s[i]);
    i2=0;
    for (i=0; (i<userdb_user_count()) && (i2==0); i++) {
        smalrec sr;
        userdb_get_entry(i, &sr);
        if (strstr(sr.name,s)!=NULL) {
            i1=sr.number;
            userdb_load(i1,&u);
            sprintf(s1,"Do you mean %s (Y/N/Q) ? ",nam(&u,i1));
            prt(5,s1);
            ch=onek("QYN");
            if (ch=='Y')
                i2=i1;
            if (ch=='Q')
                i=sys.status.users;
        }
    }
    return(i2);
}


void ssm(unsigned int un, unsigned int sy, char *s)
{
    int  f,i,i1;
    userrec u;
    char s1[161];
    shortmsgrec sm;

    if (un==65535)
        return;
    if (sy==0) {
        userdb_load(un,&u);
        if (!(u.inact & inact_deleted)) {
            sprintf(s1,"%ssmw.dat",syscfg.datadir);
            f=open(s1,O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
            i=(int) (filelength(f) / sizeof(shortmsgrec));
            i1=i-1;
            if (i1>=0) {
                lseek(f,((long) (i1)) * sizeof(shortmsgrec), SEEK_SET);
                read(f,(void *)&sm,sizeof(shortmsgrec));
                while ((sm.tosys==0) && (sm.touser==0) && (i1>0)) {
                    --i1;
                    lseek(f,((long) (i1)) * sizeof(shortmsgrec), SEEK_SET);
                    read(f,(void *)&sm,sizeof(shortmsgrec));
                }
                if ((sm.tosys) || (sm.touser))
                    ++i1;
            } 
            else
                i1=0;
            sm.tosys=sy;
            sm.touser=un;
            strncpy(sm.message,s,80);
            sm.message[80]=0;
            lseek(f,((long) (i1)) * sizeof(shortmsgrec), SEEK_SET);
            write(f,(void *)&sm,sizeof(shortmsgrec));
            close(f);
            u.sysstatus |= sysstatus_smw;
            userdb_save(un,&u);
        }
    } 
}

void rsm(int un, userrec *u)
{
    shortmsgrec sm;
    int i,i1,f,any;
    char s1[MAX_PATH_LEN];

    any=0;
    if ((u->sysstatus) & sysstatus_smw) {
        sprintf(s1,"%ssmw.dat",syscfg.datadir);
        f=open(s1,O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
        i=(int) (filelength(f) / sizeof(shortmsgrec));
        for (i1=0; i1<i; i1++) {
            lseek(f,((long) (i1)) * sizeof(shortmsgrec),SEEK_SET);
            read(f,(void *)&sm,sizeof(shortmsgrec));
            if ((sm.touser==un) && (sm.tosys==0)) {
                pl(sm.message);
                sm.touser=0;
                sm.tosys=0;
                sm.message[0]=0;
                lseek(f,((long) (i1)) * sizeof(shortmsgrec),SEEK_SET);
                write(f,(void *)&sm,sizeof(shortmsgrec));
                any=1;
            }
        }
        close(f);
        u->sysstatus ^= sysstatus_smw;
        smwcheck=1;
    }
    if (any)
        nl();
}

/* insensitive strstr() */

char * stristr (char *t, char *s) {

    char *t1;
    char *s1;


    while(*t) {
        t1 = t;
        s1 = s;
        while(*s1) {
            if (toupper(*s1) != toupper(*t)) break;
            else {
                s1++;
                t++;
            }
        }
        if (!*s1) return t1;
        t = t1 + 1;
    }
    return NULL;
}



/* case insensitive strnstr() */

char * strnistr (char *t, char *s, size_t n) {

    char *t1;
    char *s1;


    while(*t) {
        t1 = t;
        s1 = s;
        while(*s1) {
            if (toupper(*s1) != toupper(*t)) break;
            else {
                n--;
                s1++;
                t++;
                if(!n) break;
            }
        }
        if (!*s1) return t1;
        t = t1 + 1;
    }
    return NULL;
}





/* strnstr() */

char * strnstr (char *t, char *s, size_t n) {

    char *t1;
    char *s1;


    while(*t) {
        t1 = t;
        s1 = s;
        while(*s1) {
            if (*s1 != *t) break;
            else {
                n--;
                s1++;
                t++;
                if(!n) break;
            }
        }
        if (!*s1) return t1;
        t = t1 + 1;
    }
    return NULL;
}


