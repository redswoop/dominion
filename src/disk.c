#include "platform.h"
#include "fcns.h"
#include "session.h"
#include "system.h"
#pragma hdrstop


#include <math.h>
#include "json_io.h"



static auto& sys = System::instance();
static auto& sess = Session::instance();

void read_in_file(char *fn, messagerec *m, int maxary)
{
    int i,which;
    char *buf,s[161];
    long l,l1;

    for (i=0; i<maxary; i++) {
        m[i].stored_as=0L;
        m[i].storage_type=0;
    }

    sprintf(s,"%s%s",sys.cfg.gfilesdir,fn);
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


void save_status()
{
    char path[MAX_PATH_LEN];
    cJSON *root;

    sprintf(path, "%sstatus.json", sys.cfg.datadir);
    root = statusrec_to_json(&sys.status);
    write_json_file(path, root);
    cJSON_Delete(root);
}


double freek1(char *s)
{
    int d;

    d=sys.cdir[0];
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
    if ((s=(char *)malloca(filelength(i)+50))==NULL) {
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
    char s[MAX_PATH_LEN];

    if (x_only)
        return;

    if (i) {
        if (!global_handle) {
            sprintf(s,"%sGLOBAL.TXT",sys.cfg.gfilesdir);
            global_handle=open(s,O_RDWR | O_APPEND | O_BINARY | O_CREAT,
            S_IREAD | S_IWRITE);
            global_ptr=0;
            global_buf=(char *)malloca(GLOBAL_SIZE);
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
    char s[MAX_PATH_LEN],s1[MAX_PATH_LEN];
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
    b=(char *)malloca(l);
    read(i,b,l);
    close(i);

    show_message(&i,abort,b,l);

    farfree(b);
}

void printmenu(int which)
{
    int i,abort=0;
    long l,l1;
    char *b,ch,s[MAX_PATH_LEN];

    sprintf(s,"%smnudata.dat",sys.cfg.gfilesdir);

    i=open(s,O_BINARY|O_RDWR);
    if(i<0)
        return;

    lseek(i,sys.menus[which].storage_type,0);
    l1=sys.menus[which].stored_as;

    b=(char *)malloca(l1);
    read(i,b,l1);
    close(i);

    show_message(&i,abort,b,l1);

    farfree(b);
}

int printfile(char *fn)
{
    char s[MAX_PATH_LEN],s1[MAX_PATH_LEN],s2[3],tmp[MAX_PATH_LEN];
    int done=0;

    strcpy(s,sys.cfg.gfilesdir);

    strcat(s,fn);
    if (strchr(s,'.')==NULL) {
        strcpy(tmp,s);

        if(sess.user.sysstatus & sysstatus_rip) {
            strcat(s,".rip");
            if(exist(s))
                done=1;
        }

        if(!done&&(sess.user.sysstatus & sysstatus_avatar)) {
            strcpy(s,tmp);
            strcat(s,".avt");
            if(exist(s))
                done=1;
        }

        if(!done&&(sess.user.sysstatus & sysstatus_color)) {
            strcpy(s,tmp);
            strcat(s,".ans");
            if(exist(s))
                done=1;
        }

        if(!done&&(sess.user.sysstatus & sysstatus_ansi)) {
            strcpy(s,tmp);
            strcat(s,".b&w");
            if(exist(s))
                done=1;
        }

        if(!done) {
            strcpy(s,tmp);
            strcat(s,".msg");
        }
    }

    if(!exist(s))
        return 0;
    showfile(s);
    return(1);
}
