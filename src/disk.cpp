#include "disk.h"
#include "platform.h"
#include "bbs_output.h"
#include "bbs_input.h"
#include "bbs_ui.h"
#include "bbsutl.h"
#include "jam_bbs.h"
#include "file3.h"
#include "session.h"
#include "system.h"
#include "error.h"
#pragma hdrstop


#include <math.h>
#include "json_io.h"
#include "file_lock.h"
#include "bbs_path.h"


void read_in_file(char *fn, messagerec *m, int maxary)
{
    auto& sys = System::instance();
    int i,which;
    char *buf,s[161];
    long l,l1;

    for (i=0; i<maxary; i++) {
        m[i].stored_as=0L;
        m[i].storage_type=0;
    }

    auto path = BbsPath::join(sys.cfg.gfilesdir, fn);
    i=open(path.c_str(),O_RDWR | O_BINARY);
    if (i<0) {
        err(1,(char*)path.c_str(),"In Read_in_file");
    }
    l=filelength(i);
    buf=(char *) malloc(l);
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

    free((void *) buf);
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


/* Helper: return the larger of two unsigned values */
static unsigned long maxul(unsigned long a, unsigned long b) { return a > b ? a : b; }
static unsigned short maxus(unsigned short a, unsigned short b) { return a > b ? a : b; }

void save_status()
{
    auto& sys = System::instance();

    auto spath = BbsPath::join(sys.cfg.datadir, "status.json");

    /* Lock, read fresh from disk, merge monotonic counters, write back. */
    FileLock lk(spath.c_str());

    cJSON *fresh_json = read_json_file(spath.c_str());
    if (fresh_json) {
        statusrec fresh;
        memset(&fresh, 0, sizeof(fresh));
        json_to_statusrec(fresh_json, &fresh);
        cJSON_Delete(fresh_json);

        /* Merge: take max of monotonic counters */
        sys.status.callernum1 = maxul(sys.status.callernum1, fresh.callernum1);
        sys.status.callernum = maxus(sys.status.callernum, fresh.callernum);
        sys.status.callstoday = maxus(sys.status.callstoday, fresh.callstoday);
        sys.status.msgposttoday = maxus(sys.status.msgposttoday, fresh.msgposttoday);
        sys.status.emailtoday = maxus(sys.status.emailtoday, fresh.emailtoday);
        sys.status.fbacktoday = maxus(sys.status.fbacktoday, fresh.fbacktoday);
        sys.status.uptoday = maxus(sys.status.uptoday, fresh.uptoday);
        sys.status.activetoday = maxus(sys.status.activetoday, fresh.activetoday);
        sys.status.users = maxus(sys.status.users, fresh.users);
        sys.status.qscanptr = maxul(sys.status.qscanptr, fresh.qscanptr);
    }

    cJSON *root = statusrec_to_json(&sys.status);
    /* Write directly — we already hold the lock.  Can't call write_json_file()
       because it takes its own FileLock on the same path, and flock(2) on macOS
       self-deadlocks when the same process opens two independent fds. */
    char *str = cJSON_Print(root);
    if (str) {
        FILE *f = fopen(spath.c_str(), "w");
        if (f) {
            fputs(str, f);
            fputc('\n', f);
            fclose(f);
        }
        free(str);
    }
    cJSON_Delete(root);
}


double freek1(char *s)
{
    auto& sys = System::instance();
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
    auto& sys = System::instance();

    if (io.x_only)
        return;

    if (i) {
        if (!io.global_handle) {
            auto gpath = BbsPath::join(sys.cfg.gfilesdir, "GLOBAL.TXT");
            io.global_handle=open(gpath.c_str(),O_RDWR | O_APPEND | O_BINARY | O_CREAT,
            S_IREAD | S_IWRITE);
            global_ptr=0;
            global_buf=(char *)malloca(GLOBAL_SIZE);
            if ((io.global_handle<0) || (!global_buf)) {
                io.global_handle=0;
                if (global_buf) {
                    free(global_buf);
                    global_buf=NULL;
                }
            }

        }
    }
    else {
        if (io.global_handle) {
            write(io.global_handle,global_buf,global_ptr);
            close(io.global_handle);
            io.global_handle=0;
            if (global_buf) {
                free(global_buf);
                global_buf=NULL;
            }
        }
    }
}


void global_char(char ch)
{

    if (global_buf && io.global_handle) {
        global_buf[global_ptr++]=ch;
        if (global_ptr==GLOBAL_SIZE) {
            write(io.global_handle,global_buf,global_ptr);
            global_ptr=0;
        }
    }
}

int exist(char *s)
{
    return BbsPath::exists(s) ? 1 : 0;
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
    char s[MAX_PATH_LEN];
    struct ffblk ff;
    uploadsrec u;

    auto s1path = BbsPath::join(dir, stripfn(fn));
    f1=findfirst((char*)s1path.c_str(),&ff,0);
    ok=1;
    while ((f1==0) && (ok)) {
        auto fpath = BbsPath::join(dir, ff.ff_name);
        strcpy(s, fpath.c_str());
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

    free(b);
}

void printmenu(int which)
{
    auto& sys = System::instance();
    int i,abort=0;
    long l,l1;
    char *b,ch;

    auto mpath = BbsPath::join(sys.cfg.gfilesdir, "mnudata.dat");

    i=open(mpath.c_str(),O_BINARY|O_RDWR);
    if(i<0)
        return;

    lseek(i,sys.menus[which].storage_type,0);
    l1=sys.menus[which].stored_as;

    b=(char *)malloca(l1);
    read(i,b,l1);
    close(i);

    show_message(&i,abort,b,l1);

    free(b);
}

int printfile(char *fn)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    char s[MAX_PATH_LEN],s1[MAX_PATH_LEN],s2[3],tmp[MAX_PATH_LEN];
    int done=0;

    auto base = BbsPath::join(sys.cfg.gfilesdir, fn);
    strcpy(s, base.c_str());
    if (strchr(s,'.')==NULL) {
        strcpy(tmp,s);

        if(sess.user.sysstatus() & sysstatus_color) {
            strcpy(s,tmp);
            strcat(s,".ans");
            if(exist(s))
                done=1;
        }

        if(!done&&(sess.user.sysstatus() & sysstatus_ansi)) {
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
