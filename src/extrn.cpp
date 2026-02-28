#include "extrn.h"
#include "platform.h"
#include "bbs_output.h"
#include "bbs_input.h"
#include "bbs_ui.h"
#include "tcpio.h"
#include "conio.h"
#include "bbsutl.h"
#include "sysoplog.h"
#include "files/file1.h"
#include "timest.h"
#include "utility.h"
#include "session.h"
#include "system.h"
#include "cmd_registry.h"
#include "acs.h"
#include "sysopf.h"
#include "user/userdb.h"

#pragma hdrstop

#include <math.h>


void cd_to(char *s)
{
    char s1[MAX_PATH_LEN];
    int i;

    if (!s) return;
    strcpy(s1,s);
    i=strlen(s1)-1;
    if (i>0 && (s1[i]=='\\' || s1[i]=='/'))
        s1[i]=0;
    /* skip DOS drive letter prefix (e.g. "A:\") */
    if (s1[1]==':' && (s1[2]=='\\' || s1[2]=='/'))
        chdir(&s1[2]);
    else if (s1[1]==':')
        chdir(&s1[2]);
    else
        chdir(s1);
}

void get_dir(char *s, int be)
{
    getcwd(s, MAX_PATH_LEN);
    if (be) {
        if (s[strlen(s)-1]!='/')
            strcat(s,"/");
    }
}


int do_it(char cl[MAX_PATH_LEN])
{
    auto& sys = System::instance();
    int i,i1,l;
    char s[160],*s1;
    char *ss[30];
    unsigned char ex;

    sl1(1,"");
    strcpy(s,cl);
    ss[0]=s;
    i=1;
    l=strlen(s);
    for (i1=1; i1<l; i1++)
        if (s[i1]==32) {
            s[i1]=0;
            ss[i++]=&(s[i1+1]);
        }
    ss[i]=NULL;
    i=spawnvpe(P_WAIT,ss[0],ss,sys.xenviron);

    return(i);
}


int runprog(char *s, int swp)
{
    auto& sys = System::instance();
    int rc;
    char x[161];

    (void)swp; /* DOS XMS/EMS swap no longer relevant */
    sl1(1,"");
    checkhangup();
    sprintf(x,"%s /C %s", getenv("COMSPEC"), s);
    rc=do_it(x);
    initport(sys.cfg.primaryport);
    inkey();
    inkey();
    cd_to(sys.cdir);
    return(rc);
}


void alf(int f, const char *s)
{
    char s1[100];

    strcpy(s1,s);
    strcat(s1,"\r\n");
    write(f,(void *)s1,strlen(s1));
}


char *create_chain_file(char *fn)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    int i,i1,f;
    char s[MAX_PATH_LEN],s1[MAX_PATH_LEN],gd[MAX_PATH_LEN],dd[MAX_PATH_LEN];
    static char fpn[MAX_PATH_LEN];
    long l;

    cd_to(sys.cfg.gfilesdir);
    get_dir(gd,1);
    cd_to(sys.cdir);
    cd_to(sys.cfg.datadir);
    get_dir(dd,1);
    cd_to(sys.cdir);

    unlink(fn);
    f=open(fn,O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE);
    itoa(sess.usernum,s,10);
    alf(f,s);
    alf(f,sess.user.name());
    alf(f,sess.user.realname());
    alf(f,sess.user.callsign());
    itoa(sess.user.age(),s,10);
    alf(f,s);
    s[0]=sess.user.sex();
    s[1]=0;
    alf(f,s);
    sprintf(s,"%10.2f",sess.user.fpts());
    alf(f,s);
    alf(f,sess.user.laston());
    itoa(sess.user.screenchars(),s,10);
    alf(f,s);
    itoa(sess.user.screenlines(),s,10);
    alf(f,s);
    itoa(sess.user.sl(),s,10);
    alf(f,s);
    if (cs())
        alf(f,"1");
    else
        alf(f,"0");
    if (so())
        alf(f,"1");
    else
        alf(f,"0");
    if (okansi())
        alf(f,"1");
    else
        alf(f,"0");
    if (incom)
        alf(f,"1");
    else
        alf(f,"0");
    sprintf(s,"%10.2f",nsl());
    alf(f,s);
    alf(f,gd);
    alf(f,dd);
    sl1(3,s);
    alf(f,s);
    sprintf(s,"%u",sess.modem_speed);
    if (!using_modem)
        strcpy(s,"KB");
    alf(f,s);
    itoa(sys.cfg.primaryport,s,10);
    alf(f,s);
    alf(f,sys.cfg.systemname);
    alf(f,sys.cfg.sysopname);
    l=(long) (sess.timeon);
    if (l<0)
        l += 3600*24;
    ltoa(l,s,10);
    alf(f,s);
    l=(long) (timer()-sess.timeon);
    if (l<0)
        l += 3600*24;
    ltoa(l,s,10);
    alf(f,s);
    ltoa(sess.user.uk(),s,10);
    alf(f,s);
    itoa(sess.user.uploaded(),s,10);
    alf(f,s);
    ltoa(sess.user.dk(),s,10);
    alf(f,s);
    itoa(sess.user.downloaded(),s,10);
    alf(f,s);
    alf(f,"8N1");
    sprintf(s,"%u",sess.com_speed);
    alf(f,s);
    sprintf(s,"%u",sys.cfg.systemnumber);
    alf(f,s);
    close(f);
    get_dir(fpn,1);
    strcat(fpn,fn);
    return(fpn);
}


#define READ(x) read(f,&(x),sizeof(x))

#define WRITE(x) write(f,&(x),sizeof(x))

/* sess.menuat, sess.mstack, sess.mdepth now in vars.h (Phase B0) */

int restore_data(char *s)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    int f,stat;

    f=open(s,O_RDONLY | O_BINARY);
    if (f<0)
        return(-1);

    READ(stat);
    READ(sys.oklevel);
    READ(sys.noklevel);
    READ(sys.ooneuser);
    READ(sys.no_hangup);
    READ(ok_modem_stuff);
    READ(sess.topdata);
    READ(sys.last_time_c);
    READ(sess.sysop_alert);
    READ(sys.do_event);
    READ(sess.usernum);
    READ(io.chatcall);
    READ(sess.chatreason);
    READ(sess.timeon);
    READ(sess.extratimecall);
    READ(io.curspeed);
    READ(sess.modem_speed);
    READ(sess.com_speed);
    READ(sess.cursub);
    READ(sess.curdir);
    READ(sess.msgreadlogon);
    READ(sess.nscandate);
    READ(sess.mailcheck);
    READ(sess.smwcheck);
    READ(sess.use_workspace);
    READ(using_modem);
    READ(sys.last_time);
    READ(sess.fsenttoday);
    READ(sys.global_xx);
    READ(sys.xtime);
    READ(sys.xdate);
    READ(incom);
    READ(outcom);
    READ(io.global_handle);
    READ(sess.actsl);
    READ(sess.numbatch);
    READ(sess.numbatchdl);
    READ(sess.batchtime);
    READ(sess.batchsize);
    READ(sess.menuat);
    READ(sess.mstack);
    READ(sess.mdepth);

    if (io.global_handle) {
        io.global_handle=0;
        set_global_handle(1);
    }

    { auto __p = UserDB::instance().get(sess.usernum); if (__p) sess.user = *__p; };
    sess.useron=1;
    changedsl();
    topscreen();

    close(f);
    unlink(s);
    return(stat);
}

/****************************************************************************/

void save_state(char *s, int state)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    int f;

    save_status();

    f=open(s,O_RDWR | O_BINARY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);
    if (f<0)
        return;

    WRITE(state);
    WRITE(sys.oklevel);
    WRITE(sys.noklevel);
    WRITE(sys.ooneuser);
    WRITE(sys.no_hangup);
    WRITE(ok_modem_stuff);
    WRITE(sess.topdata);
    WRITE(sys.last_time_c);
    WRITE(sess.sysop_alert);
    WRITE(sys.do_event);
    WRITE(sess.usernum);
    WRITE(io.chatcall);
    WRITE(sess.chatreason);
    WRITE(sess.timeon);
    WRITE(sess.extratimecall);
    WRITE(io.curspeed);
    WRITE(sess.modem_speed);
    WRITE(sess.com_speed);
    WRITE(sess.cursub);
    WRITE(sess.curdir);
    WRITE(sess.msgreadlogon);
    WRITE(sess.nscandate);
    WRITE(sess.mailcheck);
    WRITE(sess.smwcheck);
    WRITE(sess.use_workspace);
    WRITE(using_modem);
    WRITE(sys.last_time);
    WRITE(sess.fsenttoday);
    WRITE(sys.global_xx);
    WRITE(sys.xtime);
    WRITE(sys.xdate);
    WRITE(incom);
    WRITE(outcom);
    WRITE(io.global_handle);
    WRITE(sess.actsl);
    WRITE(sess.numbatch);
    WRITE(sess.numbatchdl);
    WRITE(sess.batchtime);
    WRITE(sess.batchsize);
    WRITE(sess.menuat);
    WRITE(sess.mstack);
    WRITE(sess.mdepth);

    close(f);

    set_global_handle(0);
}

void dorinfo_def(void)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    int i,i1,f;
    char s[MAX_PATH_LEN];
    long l;

    f=open("dorinfo1.def",O_RDWR|O_CREAT|O_BINARY|O_TRUNC, S_IREAD|S_IWRITE);
    alf(f,sys.cfg.systemname);
    alf(f,"Dom");
    alf(f,"SysOp");
    sprintf(s,"COM%d",outcom ? sys.cfg.primaryport:0);
    alf(f,s);
    sprintf(s,"%u BAUD,8,N,1",sess.com_speed);
    alf(f,s);
    alf(f,"0");
    alf(f,sess.user.name());
    alf(f,"");
    alf(f,sess.user.city());
    alf(f,okansi()?"1":"0");
    itoa(sess.user.sl(),s,10);
    alf(f,s);
    l=(long) (sess.timeon);
    if (l<0)
        l += 3600*24;
    ltoa(l,s,10);
    alf(f,s);
    alf(f,"1");
    close(f);
}

void write_door_sys(int rname)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    int i,fp;
    char s[MAX_PATH_LEN];

    fp=open("door.sys",O_BINARY|O_RDWR|O_CREAT|O_TRUNC,S_IREAD|S_IWRITE);
    lseek(fp,0L,SEEK_SET);

    sprintf(s,"COM%d:",outcom?sys.cfg.primaryport:0);
    alf(fp,s);
    sprintf(s,"%u",sess.com_speed);
    alf(fp,s);
    alf(fp,"8");
    alf(fp,"1");
    alf(fp,"N");

    alf(fp,"N");
    alf(fp,"Y");
    alf(fp,cs()?"Y":"N");
    alf(fp,"N");

    alf(fp,rname?sess.user.realname():sess.user.name());
    alf(fp,sess.user.city());
    alf(fp,sess.user.phone());
    alf(fp,sess.user.phone());
    alf(fp,sess.user.password());
    sprintf(s,"%d",sess.user.sl()); 
    alf(fp,s);
    alf(fp,"0");
    alf(fp,sess.user.laston());
    sprintf(s,"%6.0f",nsl());
    alf(fp,s);
    sprintf(s,"%6.0f",nsl()/60.0);
    alf(fp,s);
    alf(fp,okansi()?"GR":"NG");
    sprintf(s,"%d",sess.user.screenlines()); 
    alf(fp,s);
    alf(fp,"Y");
    alf(fp,"123456");
    alf(fp,"7");
    alf(fp,"12/31/99");
    sprintf(s,"%d",sess.usernum); 
    alf(fp,s);
    alf(fp,"X");
    sprintf(s,"%d",sess.user.uploaded()); 
    alf(fp,s);
    sprintf(s,"%d",sess.user.downloaded()); 
    alf(fp,s);
    sprintf(s,"%d",sess.user.dk()); 
    alf(fp,s);
    alf(fp,"999999");
    close(fp);
}


void rundoor(char type,char ms[MAX_PATH_LEN])
{
    create_chain_file("CHAIN.TXT");
    write_door_sys(0);
    dorinfo_def();
    save_status();
    clrscr();
    logtypes(2,"Ran Door 0%s 3(%s)",ms,ctim(timer()));
    sl1(1,"");
    switch(type) {
    case '1': 
        runprog(ms,0); 
        break;
    case '2': 
        runprog(ms,1); 
        break;
    default: 
        badcommand('D',type);
    }
    logtypes(2,"Closed Door %s 3(%s)",ms,ctim(timer()));
    topscreen();
}

