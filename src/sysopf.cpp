#include "sysopf.h"
#include "platform.h"
#include "bbs_output.h"
#include "bbs_input.h"
#include "bbs_ui.h"
#include "conio.h"
#include "bbsutl.h"
#include "sysoplog.h"
#include "timest.h"
#include "disk.h"
#include "utility.h"
#include "jam_bbs.h"
#include "filesys.h"
#include "shortmsg.h"
#include "session.h"
#include "userdb.h"
#include "system.h"
#include "topten.h"
#include "extrn.h"
#include "misccmd.h"
#include "lilo.h"
#include "personal.h"
#include "bbs_path.h"

#pragma hdrstop

#include "json_io.h"
#include <stdarg.h>

extern char commstr[41];


void logtypes(char type,char *fmt, ...)
{
    va_list ap;
    char s[512],s1[MAX_PATH_LEN];

    va_start(ap, fmt);
    vsprintf(s, fmt, ap);
    va_end(ap);

    switch(type) {
    case 0:
        strcpy(s1,"7\xFA""7>");
        break;
    case 1:
        strcpy(s1,"5\xFA""5\xFA");
        break;
    case 2:
        strcpy(s1,"1\xFA""1>");
        break;
    case 3:
        strcpy(s1,"2\xFA""2\xFA");
        break;
    case 4:
        strcpy(s1,"3\xFA""3>");
        break;
    case 5:
        strcpy(s1,"9#9#9#");
        break;
    }

    strcat(s1,"0 ");
    strcat(s1,s);
    if(type==5) sl1(0,s1);
    else
        sysoplog(s1);
}


void reset_files(int show)
{
    auto& sys = System::instance();
    (void)show;
    UserDB::instance().rebuild_index();
    sys.status.users = UserDB::instance().user_count();
    save_status();
}


void get_status()
{
    auto& sys = System::instance();
    cJSON *st_root;

    auto path = BbsPath::join(sys.cfg.datadir, "status.json");
    st_root = read_json_file(path.c_str());
    if (st_root) {
        json_to_statusrec(st_root, &sys.status);
        cJSON_Delete(st_root);
    } 
    else
        save_status();
}

void read_new_stuff()
{
    auto& sys = System::instance();
    int i;
    char s[MAX_PATH_LEN];

    read_in_file("mnudata.dat",(sys.menus),50);
}


void chuser()
{
    auto& sess = Session::instance();
    char s[MAX_PATH_LEN];
    int i;

    if (!checkpw())
        return;
    prt(2,"User to change to? ");
    input(s,30);
    i=finduser1(s);
    if (i>0) {
        UserDB::instance().store(sess.usernum, sess.user);
        { auto __p = UserDB::instance().get(i); if (__p) sess.user = *__p; }
        sess.usernum=i;
        sess.actsl=255;
        logtypes(3,"Changed to 4%s",sess.user.display_name(sess.usernum).c_str());
        topscreen();
    } 
    else
        pl("Unknown user.");
}

void zlog()
{
    auto& sys = System::instance();
    zlogrec z;
    char s[MAX_PATH_LEN];
    int abort,f,i,i1;

    auto path = BbsPath::join(sys.cfg.datadir, "history.dat");
    f=open(path.c_str(),O_RDWR | O_BINARY);
    if (f<0)
        return;
    i=0;
    abort=0;
    read(f,(void *)&z,sizeof(zlogrec));
    pla("0  Date     Calls  Active   Posts   Email   Fback    U/L   D/L   %Act   T/user",&abort);
    pla("7��������   �����  ������   �����   �����   �����    ���   ���   �����  ������0",&abort);
    while ((i<230) && (!abort) && (!io.hangup) && (z.date[0]!=0)) {
        if (z.calls)
            i1=z.active/z.calls;
        else
            i1=0;
        sprintf(s,"%s    %4d    %4d    %4d    %4d     %4d    %3d   %3d     %3d     %3d",
        z.date,z.calls,z.active,z.posts,z.email,z.fback,z.up,z.dl,10*z.active/144,i1);
        pla(s,&abort);
        ++i;
        if (i<230) {
            lseek(f,((long) i) * sizeof(zlogrec),SEEK_SET);
            read(f,(void *)&z,sizeof(zlogrec));
        }
    }
    close(f);
}

void beginday()
{
    auto& sys = System::instance();
    char s[255],s1[MAX_PATH_LEN];
    zlogrec z,z1;
    int f,i,i1;

    double fk;
    int    nus;

    pl("Updating Logs");
    logpr("");
    logpr("1�1>0Totals for 7%s1<1�",sys.status.date1); 
    logpr("   0Calls             1: 7%d",sys.status.callstoday); 
    logpr("   0Message Posted    1: 7%d",sys.status.msgposttoday);
    logpr("   0FeedBack          1: 7%d",sys.status.fbacktoday);
    logpr("   0Uploads           1: 7%d",sys.status.uptoday);
    logpr("   0Downloads         1: 7%d",sys.status.dltoday);
    logpr("   0Total Time Active 1: 7%d",sys.status.activetoday);
    logpr("");
    strcpy(z.date,sys.status.date1);
    z.active=sys.status.activetoday;
    z.calls=sys.status.callstoday;
    z.posts=sys.status.msgposttoday;
    z.email=sys.status.emailtoday;
    z.fback=sys.status.fbacktoday;
    z.up=sys.status.uptoday;
    z.dl=sys.status.dltoday;
    {
        auto hist_path = BbsPath::join(sys.cfg.datadir, "history.dat");
        f=open(hist_path.c_str(),O_RDWR|O_BINARY);
    }
    lseek(f,(sys.nifty.systemtype-1)*sizeof(zlogrec),SEEK_SET);
    read(f,&z1,sizeof(z1));
    close(f);
    s[0]=z1.date[6];
    s[1]=z1.date[7];
    s[2]=z1.date[0];
    s[3]=z1.date[1];
    s[4]=z1.date[3];
    s[5]=z1.date[4];
    s[6]=0;
    auto log_path = BbsPath::join(sys.cfg.gfilesdir, std::string(s) + ".log");
    unlink(log_path.c_str());

    sys.status.callstoday=0;
    sys.status.msgposttoday=0;
    sys.status.emailtoday=0;
    sys.status.fbacktoday=0;
    sys.status.uptoday=0;
    sys.status.dltoday=0;
    sys.status.activetoday=0;
    strcpy(sys.status.date1,date());
    sl1(2,date());
    auto userlog_path = BbsPath::join(sys.cfg.gfilesdir, "user.log");
    unlink(userlog_path.c_str());
    save_status();
    auto hist_path2 = BbsPath::join(sys.cfg.datadir, "history.dat");
    f=open(hist_path2.c_str(),O_RDWR | O_BINARY);
    if (f<0) {
        f=open(hist_path2.c_str(),O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
        z1.date[0]=0;
        z1.active=0;
        z1.calls=0;
        z1.posts=0;
        z1.email=0;
        z1.fback=0;
        z1.up=0;
        z1.dl=0;
        for (i=0; i<230; i++)
            write(f,(void *)&z1,sizeof(zlogrec));
    } 
    else {
        for (i=230; i>=1; i--) {
            lseek(f,(long) ((i-1) * sizeof(zlogrec)),SEEK_SET);
            read(f,(void *)&z1,sizeof(zlogrec));
            lseek(f,(long) (i * sizeof(zlogrec)),SEEK_SET);
            write(f,(void *)&z1,sizeof(zlogrec));
        }
    }
    lseek(f,0L,SEEK_SET);
    write(f,(void *)&z,sizeof(zlogrec));
    close(f);
    pl("Updating Top Ten");
    updtopten();
    pl("Sorting File Areas");
    sort_all("GA");

    if (sys.cfg.beginday_c[0]) {
        stuff_in(s,sys.cfg.beginday_c,create_chain_file("CHAIN.TXT"),"","","","");
        runprog(s,0);
    }

    fk=freek1(sys.cfg.datadir);
    nus=sys.cfg.maxusers-UserDB::instance().user_count();

    if (fk<512.0) {
        sprintf(s,"2! 0Only %dk free in data directory.",(int) fk);
        ssm(1,0,s);
        logpr(s);
    }

    fk=freek1(sys.cfg.batchdir);

    if (fk<2048.0) {
        sprintf(s,"2! 0Only %dk free in batch directory.",(int) fk);
        ssm(1,0,s);
        logpr(s);
    }

    if ((!sys.cfg.closedsystem) && (nus<15)){
        sprintf(s,"7!!>0Only %d new user slots left.",nus);
        ssm(1,0,s);
        logpr(s);
    }
}


void print_local_file(char ss[MAX_PATH_LEN])
{
    auto& sys = System::instance();
    char s[MAX_PATH_LEN];

    if ((sys.cfg.sysconfig & sysconfig_list) &&!incom) {
        auto cmd = "LST " + BbsPath::join(sys.cfg.gfilesdir, ss);
        strcpy(s, cmd.c_str());
        if(searchpath("LST.EXE")!=NULL) {
            runprog(s,0);
            if(!sys.wfc)
                topscreen();
            return;
        }
    }

    printfile(ss);
    nl();
    pausescr();
}

void text_edit()
{
    /*  char s[MAX_PATH_LEN],s1[MAX_PATH_LEN];
        
          nl();
          npr("3Filename\r\n5: ");
          mpl(71);
          input(s,71);
          if (strstr(s,".log")!=NULL)
            s[0]=0;
          if (s[0]) {
            logpr("5@0 Edited 4%s",s);
              tedit(s);
          }*/
}

void viewlog()
{
    auto& sys = System::instance();
    char s[15],s1[MAX_PATH_LEN];
    int i,f;
    zlogrec z;

    inputdat("View Logs from How Many Days Ago? (Enter=Today)",s,4,0);
    i=atoi(s);
    if(!i) {
        sl1(3,s);
        strcpy(s1,s);
        print_local_file(s1);
    } 
    else {
        auto hist_path = BbsPath::join(sys.cfg.datadir, "history.dat");
        f=open(hist_path.c_str(),O_BINARY|O_RDWR);
        i-=1;
        lseek(f,(i*sizeof(zlogrec)),SEEK_SET);
        read(f,&z,sizeof(zlogrec));
        close(f);
        s[0]=z.date[6];
        s[1]=z.date[7];
        s[2]=z.date[0];
        s[3]=z.date[1];
        s[4]=z.date[3];
        s[5]=z.date[4];
        s[6]=0;
        sprintf(s1,"%s.log",s);
        print_local_file(s1);
    }
}


void glocolor(void)
{
    auto& sess = Session::instance();
    User u;
    int x,uu,i;
    char s[MAX_PATH_LEN];

    int sl=-1,dsl=-1,color=-1,prot=-1,format=-1,address=0,resetpc=0,fixneg;
    int resetptr=-1;

    outstr("Global Sl: "); 
    input(s,3); 
    if(s[0]) sl=atoi(s);
    outstr("Global Dsl: "); 
    input(s,3); 
    if(s[0]) dsl=atoi(s);
    outstr("5Do Colors? "); 
    color=yn();
    outstr("5Global Protocol? "); 
    prot=yn();
    outstr("5Reset File Format? "); 
    format=yn();
    outstr("5Reset Address? "); 
    address=yn();
    outstr("5Reset Computer Types? "); 
    resetpc=yn();
    outstr("5Reset Message Pointers? "); 
    resetptr=yn();
    outstr("5Fix negative DL/UL/Fpts? "); 
    fixneg=yn();
    outstr("5Continue? "); 
    if(!yn()) return;

    UserDB::instance().store(sess.usernum, sess.user);

    dtitle("Setting Users to: ");
    if(sl!=-1)     npr("Sl    : %d\r\n",sl);
    if(dsl!=-1)    npr("Dsl   : %d\r\n",dsl);
    if(prot)       pl("Reseting Protocol");
    if(format)     pl("Reseting Format");
    if(address)    pl("Reseting Addresses");
    if(color)      pl("Reseting Colors");
    if(resetptr)   pl("Reseting Message Ponters");
    if(fixneg)     pl("Fixing negative DL/UL/Fpts");

    for(x=1;x<=UserDB::instance().user_count();x++) {
        { auto p = UserDB::instance().get(x); if (p) u = *p; }
        if(fixneg) {
            if((long)u.dk()<0)
                u.set_dk(0);
            if((long)u.uk()<0)
                u.set_uk(0);
            if(u.fpts()<0)
                u.set_fpts(0);
        }
        if(sl!=-1) u.set_sl(sl);
        if(dsl!=-1) u.set_dsl(dsl);
        if(color) {
            setcolors(u);
            u.set_sysstatus(u.sysstatus() ^ sysstatus_ansi);
            u.set_sysstatus(u.sysstatus() ^ sysstatus_color);
            u.set_sysstatus(u.sysstatus() ^ sysstatus_avatar);
        }
        if(format) u.set_flisttype(99);
        if(prot) u.set_defprot(99);
        if(resetpc) u.set_comp_type(99);
        u.set_lastsub(0);
        u.set_lastdir(0);
        u.set_lastconf(1);
        if(address) {
            u.set_city("");
            u.set_street("");
        }
        if(resetptr) {
            for(i=0;i<200;i++)
                u.qscn_mut()[i]=0;
        }
        UserDB::instance().store(x, u);
    }

    { auto __p = UserDB::instance().get(sess.usernum); if (__p) sess.user = *__p; };

    reset_files(1);
}

