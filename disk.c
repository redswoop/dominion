#include "vars.h"
#pragma hdrstop


#include <dir.h>
#include <math.h>
#include <share.h>


void read_in_file(char *fn, messagerec *m, int maxary)
{
    int i,which;
    char *buf,s[161];
    long l,l1;

    for (i=0; i<maxary; i++) {
        m[i].stored_as=0L;
        m[i].storage_type=0;
    }

    sprintf(s,"%s%s",syscfg.gfilesdir,fn);
    i=open(s,O_RDWR | O_BINARY);
    if (i<0) {
        err(1,s,"In Read_in_file");
    }
    l=filelength(i);
    buf=(char *) farmalloc(l);
    lseek(i,0L,SEEK_SET);
    if (buf==NULL) {
        err(3,"","In Read_in_file");
    }
    read(i,(void *) buf,l);
    close(i);
    l1=0;

    while(l1<l) {
        if(buf[l1]=='~') {
            i=0;
            l1++;
            while(buf[l1]!=10&&buf[l1]!=13) {
                s[i++]=buf[l1++];
            }

            s[i]=0;
            which=atoi(s);
            m[which].storage_type=l1;
            m[which].stored_as=0;
        } 
        else {
            m[which].stored_as++;
            l1++;
        }
    }

    farfree((void *) buf);
}

double freek(int dr)
{
    float d;
    struct dfree df;

    getdfree(dr,&df);
    d=(float) df.df_avail;
    d*=((float) df.df_bsec);
    d*=((float) df.df_sclus);
    d/=1024.0;
    if (df.df_sclus==0xffff)
        d=-1.0;
    return(d);
}


void fix_user_rec(userrec *u)
{
    u->name[30]=0;
    u->realname[20]=0;
    u->callsign[6]=0;
    u->phone[12]=0;
    u->pw[19]=0;
    //  u->laston[8]=0;
    u->note[40]=0;
    u->macros[0][80]=0;
    u->macros[1][80]=0;
    u->macros[2][80]=0;
}


void close_user()
{
    if (userfile!=-1) {
        close(userfile);
        userfile=-1;
    }
}

void open_user()
{
    char s[81];

    if (userfile==-1) {
        sprintf(s,"%sUSER.LST",syscfg.datadir);
        /*        if(lock) {
                    userfile=open(s,O_RDWR | O_BINARY | SH_DENYRW);
                }
                else*/
            userfile=open(s,O_RDWR | O_BINARY | SH_DENYNONE);
        if (userfile<0) {
            userfile=-1;
        }
    }
}


int number_userrecs()
{
    open_user();
    return((int) (filelength(userfile)/syscfg.userreclen)-1);
}

void read_user(unsigned int un, userrec *u)
{
    long pos;
    char s[80];
    int i;

    open_user();
    if(userfile==-2) sysoplog(">*< File Locked!!! >*<");
    if ((userfile<0) || (un>number_userrecs())) {
        u->inact=inact_deleted;
        fix_user_rec(u);
        return;
    }

    if (((useron) && (un==usernum)) || ((wfc) && (un==1))) {
        *u=thisuser;
        fix_user_rec(u);
        return;
    }

    pos=((long) syscfg.userreclen) * ((long) un);
    lseek(userfile,pos,SEEK_SET);
    i=read(userfile, (void *)u, syscfg.userreclen);
    if (i==-1) {
        open_user();
        if ((userfile<0) || (un>number_userrecs())) {
            u->inact=inact_deleted;
            fix_user_rec(u);
            return;
        }
        pos=((long) syscfg.userreclen) * ((long) un);
        lseek(userfile,pos,SEEK_SET);
        i=read(userfile, (void *)u, syscfg.userreclen);
        if (i==-1) {
            pl("COULDN'T READ USER.");
        }
        close_user();
    }
    fix_user_rec(u);
}


void write_user(unsigned int un, userrec *u)
{
    long pos;
    char s[80];
    unsigned char oldsl;
    int i;

    if(usernum==0) return;

    if (userfile==-1) {
        sprintf(s,"%sUSER.LST",syscfg.datadir);
        userfile=open(s,O_RDWR | O_BINARY | O_CREAT | O_DENYALL, S_IREAD | S_IWRITE);
    }

    if (((useron) && (un==usernum)) || ((wfc) && (un==1))) {
        thisuser=*u;
    }
    pos=((long) syscfg.userreclen) * ((long) un);
    lseek(userfile,pos,SEEK_SET);
    i=write(userfile, (void *)u, syscfg.userreclen);
    if (i==-1) {
        sprintf(s,"%sUSER.LST",syscfg.datadir);
        userfile=open(s,O_RDWR | O_BINARY | O_CREAT|O_DENYALL, S_IREAD | S_IWRITE);
        pos=((long) syscfg.userreclen) * ((long) un);
        lseek(userfile,pos,SEEK_SET);
        i=write(userfile, (void *)u, syscfg.userreclen);
        if (i==-1) {
            pl("COULDN'T WRITE USER.");
        }
        close_user();
    }
    close_user();
}


void save_status()
{
    char s[80];

    sprintf(s,"%sSTATUS.DAT",syscfg.datadir);
    statusfile=open(s,O_RDWR | O_BINARY | O_DENYALL);
    write(statusfile, (void *)(&status), sizeof(statusrec));
    close(statusfile);
    statusfile=-1;
}


void isr(int un, char *name)
{
    int cp,i;
    char s[81];
    smalrec sr;

    cp=0;
    while ((cp<status.users) && (strcmp(name,(smallist[cp].name))>0))
        ++cp;
    memmove(&(smallist[cp+1]),&(smallist[cp]),sizeof(smalrec)*(status.users-cp));
    strcpy(sr.name,name);
    sr.number=un;
    smallist[cp]=sr;
    sprintf(s,"%suser.idx",syscfg.datadir);
    i=open(s,O_RDWR | O_BINARY | O_TRUNC);
    if (i<0) {
        err(1,s,"In ISR");

    }
    ++status.users;
    save_status();
    write(i,(void *) (smallist), (sizeof(smalrec) * status.users));
    close(i);
}


void dsr(char *name)
{
    int cp,i;
    char s[81];
    smalrec sr;

    cp=0;
    while ((cp<status.users) && (strcmp(name,(smallist[cp].name))!=0))
        ++cp;
    if (strcmp(name,(smallist[cp].name))) {
        sprintf(s,"6%s NOT ABLE TO BE DELETED",name);
        sl1(0,s);
        sl1(0,"9### 7Reset User Index to fix it");
        return;
    }
    memmove(&(smallist[cp]),&(smallist[cp+1]),sizeof(smalrec)*(status.users-cp));
    sprintf(s,"%suser.idx",syscfg.datadir);
    i=open(s,O_RDWR | O_BINARY | O_TRUNC | O_CREAT, S_IREAD | S_IWRITE);
    if (i<0) {
        err(4,s,"In Dsr");
    }
    --status.users;
    save_status();
    write(i,(void *) (smallist), (sizeof(smalrec) * status.users));
    close(i);
}

double freek1(char *s)
{
    int d;

    d=cdir[0];
    if (s[1]==':')
        d=s[0];
    d=d-'A'+1;
    return(freek(d));
}

char *get_file(char *fn, long *len)
{
    int i;
    char *s;

    i=open(fn,O_RDWR | O_BINARY);
    if (i<0) {
        *len=0L;
        return(NULL);
    }
    if ((s=malloca(filelength(i)+50))==NULL) {
        *len=0L;
        close(i);
        return(NULL);
    }
    *len=(long) read(i,(void *)s, filelength(i));
    close(i);
    return(s);
}


#define GLOBAL_SIZE 4096
static char *global_buf;
static int global_ptr;

void set_global_handle(int i)
{
    char s[81];

    if (x_only)
        return;

    if (i) {
        if (!global_handle) {
            sprintf(s,"%sGLOBAL.TXT",syscfg.gfilesdir);
            global_handle=open(s,O_RDWR | O_APPEND | O_BINARY | O_CREAT,
            S_IREAD | S_IWRITE);
            global_ptr=0;
            global_buf=malloca(GLOBAL_SIZE);
            if ((global_handle<0) || (!global_buf)) {
                global_handle=0;
                if (global_buf) {
                    farfree(global_buf);
                    global_buf=NULL;
                }
            }

        }
    } 
    else {
        if (global_handle) {
            write(global_handle,global_buf,global_ptr);
            close(global_handle);
            global_handle=0;
            if (global_buf) {
                farfree(global_buf);
                global_buf=NULL;
            }
        }
    }
}


void global_char(char ch)
{

    if (global_buf && global_handle) {
        global_buf[global_ptr++]=ch;
        if (global_ptr==GLOBAL_SIZE) {
            write(global_handle,global_buf,global_ptr);
            global_ptr=0;
        }
    }
}

int exist(char *s)
{
    int i;
    struct ffblk ff;

    i=findfirst(s,&ff,0);
    if (i)
        return(0);
    else
        return(1);
}


FILE *flopen(char *fn,char *mode,long *len)
{
    int i;
    FILE *f;

    i=open(fn,O_RDWR|O_BINARY);
    if(i>=0) {
        *len=filelength(i);
    }
    f=fdopen(i,mode);

    return f;
}

void filter(char *s,unsigned char c)
{
    int x=0;

    while(s[x++]!=c);
    s[x-1]=0;

}


void remove_from_temp(char *fn, char *dir, int po)
{
    int i,i1,f1,ok;
    char s[81],s1[81];
    struct ffblk ff;
    uploadsrec u;

    sprintf(s1,"%s%s",dir,stripfn(fn));
    f1=findfirst(s1,&ff,0);
    ok=1;
    while ((f1==0) && (ok)) {
        sprintf(s,"%s%s",dir,ff.ff_name);
        if (po)
            npr("Deleting %s\r\n",ff.ff_name);
        _chmod(s,1,0);
        unlink(s);
        f1=findnext(&ff);
    }
}


void showfile(char *fn)
{
    int i,abort=0;
    long l,l1;
    char *b;

    i=open(fn,O_BINARY|O_RDWR);
    if(i<0)
        return;

    l=filelength(i);
    b=malloca(l);
    read(i,b,l);
    close(i);

    show_message(&i,abort,b,l);

    farfree(b);
}

void printmenu(int which)
{
    int i,abort=0;
    long l,l1;
    char *b,ch,s[81];

    sprintf(s,"%smnudata.dat",syscfg.gfilesdir);

    i=open(s,O_BINARY|O_RDWR);
    if(i<0)
        return;

    lseek(i,menus[which].storage_type,0);
    l1=menus[which].stored_as;

    b=malloca(l1);
    read(i,b,l1);
    close(i);

    show_message(&i,abort,b,l1);

    farfree(b);
}

int printfile(char *fn)
{
    char s[81],s1[81],s2[3],tmp[81];
    int done=0;

    strcpy(s,syscfg.gfilesdir);

    strcat(s,fn);
    if (strchr(s,'.')==NULL) {
        strcpy(tmp,s);

        if(thisuser.sysstatus & sysstatus_rip) {
            strcat(s,".RIP");
            if(exist(s))
                done=1;
        }

        if(!done&&(thisuser.sysstatus & sysstatus_avatar)) {
            strcpy(s,tmp);
            strcat(s,".AVT");
            if(exist(s))
                done=1;
        }

        if(!done&&(thisuser.sysstatus & sysstatus_color)) {
            strcpy(s,tmp);
            strcat(s,".ANS");
            if(exist(s))
                done=1;
        }

        if(!done&&(thisuser.sysstatus & sysstatus_ansi)) {
            strcpy(s,tmp);
            strcat(s,".B&W");
            if(exist(s))
                done=1;
        }

        if(!done) {
            strcpy(s,tmp);
            strcat(s,".MSG");
        }
    }

    if(!exist(s))
        return 0;
    showfile(s);
    return(1);
}

