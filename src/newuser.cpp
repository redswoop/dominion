#include "newuser.h"
#include "newuser_form.h"
#include "screen_form.h"
#include "platform.h"
#include "bbs_output.h"
#include "bbs_input.h"
#include "bbs_ui.h"
#include "conio.h"
#include "bbsutl.h"
#include "sysoplog.h"
#include "file1.h"
#include "lilo.h"
#include "timest.h"
#include "disk.h"
#include "utility.h"
#include "jam_bbs.h"
#include "shortmsg.h"
#include "mm1.h"
#include "session.h"
#include "userdb.h"
#include "system.h"
#include "cmd_registry.h"
#include "nuv.h"
#include "terminal_bridge.h"
#include "misccmd.h"
#include "personal.h"
#pragma hdrstop


/* go/goin are still used by cmd_registry.cpp and config.cpp */
void go(int x,int y)
{
    /* x = row (1-based), y = col (1-based) — ANSI convention */
    term_goto(y - 1, x - 1);
}

void goin(int x,int y)
{
    char c;

    go(x,y);
    ansic(0);
    for(c=0;c<32;c++)
        outchr(32);
    go(x,y);
}

#define gotop() goin(2,4)


int check_name(const char *nn)
{
    auto& sys = System::instance();
    int ok,f,i;
    char s[181],s1[181],s2[MAX_PATH_LEN];
    long p,l;

    ok=1;
    if (nn[strlen(nn)-1]==32)
        ok=0;
    if (nn[0]<65)
        ok=0;
    if (UserDB::instance().finduser(nn)!=0)
        ok=0;
    if (strchr(nn,'@')!=NULL)
        ok=0;
    if (strchr(nn,'#')!=NULL)
        ok=0;

    if (!ok)
        return(ok);

    sprintf(s,"%strashcan.lst",sys.cfg.gfilesdir);
    f=open(s,O_RDWR | O_BINARY);

    if (f<0)
        return(ok);

    lseek(f,0L,SEEK_SET);
    l=filelength(f);
    p=0;
    sprintf(s2," %s ",nn);
    while ((p<l) && (ok)) {
        lseek(f,p,SEEK_SET);
        read(f,(void *)s,150);
        i=0;
        while ((i<150) && (s[i])) {
            if (s[i]==13)
                s[i]=0;
            else
                ++i;
        }
        s[150]=0;
        p += (long) (i+2);
        if (s[i-1]==1)
            s[i-1]=0;
        for (i=0; i<strlen(s); i++)
            s[i]=toupper(s[i]);
        sprintf(s1," %s ",s);
        if (strstr(s2,s1)!=NULL)
            ok=0;
    }
    close(f);

    if(!ok) {
        printfile("trashcan");
        io.hangup=1;
    }

    return(ok);
}

/* withansi: still referenced from lilo.cpp (extern char withansi) */
char withansi;

/* input_sex: still called from uedit.cpp */
void input_sex(User& u)
{
    nl();
    if(withansi) gotop();
    outstr("0Sex <M>ale,<F>emale,<Y>es,<L>ots?0 ");
    if(withansi) goin(9,18);
    u.set_sex(onek("MFYL"));
}

/* input_age: still called from lilo.cpp */
void input_age(User& u)
{
    int ok,y,m,d;
    char s[10];

    do {
        if(withansi) gotop();
        pl("0Enter your birthdate, in 00/00/00 Form.");
        if(withansi) goin(10,18);
        else
            outstr("3:0 ");
        inputdate(s,0);
        nl();
        m=atoi(s);
        d=atoi(&(s[3]));
        y=atoi(&(s[6]))+1900;
        if ((m>0) && (m<=12) && (d>0) && (d<32) && (y>=1900))
            ok=1;
        else ok=0;
    }
    while(!ok&&!io.hangup);

    u.set_birth_month((unsigned short) m);
    u.set_birth_day((unsigned short) d);
    u.set_birth_year((unsigned short) (y-1900));
    u.set_age(years_old(u.birth_month(),u.birth_day(),u.birth_year()));
    nl();
}

/* input_city: still called from lilo.cpp */
void input_city()
{
    auto& sess = Session::instance();
    char s[MAX_PATH_LEN];
    nl();

    do {
        if(withansi) gotop();
        pl("0Enter Your Street Address");
        if(withansi) goin(12,18);
        else outstr("3:0");
        inputl(s,35);
    }
    while(!s[0] && !io.hangup);
    sess.user.set_street(s);

    nl();

    do {
        if(withansi) gotop();
        pl("Enter Your City and State: ");
        if(withansi) goin(13,18);
        else outstr(": ");
        inputl(s,35);
        sess.user.set_city(s);
    }
    while(!sess.user.city()[0]&&!io.hangup);
}

/* input_comptype: still called from lilo.cpp */
void input_comptype()
{
    auto& sess = Session::instance();
    int i,ok,ct,numct;
    char c[5];

    do {
        nl();
        nl();
        if(withansi)
            go(8,57);
        numct=numComputerTypes();
        for (i=0; i<numct; i++) {
            if(withansi) go(8+i,57);
            npr("0%d>2 %s\r\n",i+1,getComputerType(i));
        }
        nl();
        if(withansi) gotop();
        pl("2Enter your computer type");
        if(withansi) goin(15,18);
        else
            outstr("3:0");
        input(c,2);
        ct=atoi(c);

        ok=1;
        if ((ct<1) || (ct>i))
            ok=0;

    }
    while ((!ok) && (!io.hangup));
    sess.user.set_comp_type(ct-1);
    if (io.hangup)
        sess.user.set_comp_type(0);
}

void input_screensize()
{
    auto& sess = Session::instance();
    int ok,x,y;
    char s[5];

    do {
        nl();
        pl("0How wide is your screen (7chars, <CR>=800) ?");
        outstr("3:0");
        input(s,2);
        x=atoi(s);
        if (s[0]==0)
            x=80;

        if ((x<32) || (x>80))
            ok=0;
        else
            ok=1;
    }
    while ((!ok) && (!io.hangup));

    do {
        nl();
        pl("0How tall is your screen (7lines, <CR>=250) ?");
        outstr("3:0");
        input(s,2);
        y=atoi(s);
        if (s[0]==0)
            y=25;

        if ((y<4) || (y>60))
            ok=0;
        else
            ok=1;
    }
    while ((!ok) && (!io.hangup));

    sess.user.set_screenchars(x);
    sess.user.set_screenlines(y);
    io.screenlinest=y;
}


void newuser()
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    int i,ok;
    char s[255],s1[MAX_PATH_LEN];
    hdrinfo hdr;

    sprintf(s,"7!! 0New User 4%s 0at 5%s0, %s Baud",date(),times(),io.curspeed);
    sl1(0,"");
    sl1(0,s);
    if (UserDB::instance().user_count()>=(int)sys.cfg.maxusers) {
        nl();
        nl();
        pl("I'm sorry, but the system currently has the maximum number of users it can handle.");
        nl();
        io.hangup=1;
    }
    if (sys.cfg.closedsystem) {
        nl();
        nl();
        pl("I'm sorry, but the system is currently closed, and not accepting new users.");
        nl();
        io.hangup=1;
    }
    if ((sys.cfg.newuserpw[0]!=0) && (incom)) {
        nl();
        nl();
        ok=0;
        i=0;
        do {
            outstr("New User Password :");
            input(s,20);
            if (strcmp(s,sys.cfg.newuserpw)==0)
                ok=1;
            else {
                sprintf(s1,"Wrong newuser password: %s",s);
                sl1(0,s1);
            }
        }
        while ((!ok) && (!io.hangup) && (i++<4));
        if (!ok)
            io.hangup=1;
    }

    sess.user.set_firston(date());
    sess.user.set_laston("Never.");
    strcpy(sess.user.macro_mut(0),"Trust me, no.");
    strcpy(sess.user.macro_mut(1),"Dominion=Good?");
    strcpy(sess.user.macro_mut(2),"Glub Glub Glub...");
    strcpy(sess.user.macro_mut(3),"Use the Force Luke!");
    sess.user.set_note("");
    sess.user.set_password("");

    sess.user.set_screenlines(25);
    io.screenlinest=25;

    sess.user.set_inact(0);
    sess.user.set_defprot(0);
    sess.user.set_defed(0);
    sess.user.set_nuv_status((unsigned long)-1);
    sess.user.set_sl(sys.cfg.newusersl);
    sess.user.set_dsl(sys.cfg.newuserdsl);
    sess.user.set_exempt(0);
    for (i=0; i<20; i++) sess.user.votes_mut()[i]=0;
    for (i=0; i<20; i++) sess.user.colors_mut()[i]=7;
    sess.user.set_illegal(0);
    sess.user.set_waiting(0);
    sess.user.set_subop(255);
    sess.user.set_ontoday(1);
    sess.user.set_birth_month(0);
    sess.user.set_birth_day(0);
    sess.user.set_birth_year(0);
    sess.user.set_age(0);
    sess.user.set_pcr(0);
    sess.user.set_ul_dl_ratio(0);

    sess.user.set_forwardusr(0);
    sess.user.set_msgpost(0);
    sess.user.set_emailsent(0);
    sess.user.set_feedbacksent(0);
    sess.user.set_posttoday(0);
    sess.user.set_etoday(0);
    sess.user.set_ar(0);
    sess.user.set_dar(0);
    sess.user.set_restrict(sys.cfg.newuser_restrict);
    sess.user.set_uploaded(0);
    sess.user.set_downloaded(0);
    sess.user.set_lastrate(0);
    sess.user.set_logons(0);
    sess.user.set_msgread(0);
    sess.user.set_uk(0);
    sess.user.set_dk(0);
    for(i=0;i<200;i++) sess.user.qscn_mut()[i]=0;
    for(i=0;i<200;i++) sess.user.nscn_mut()[i]=0;


    setcolors(sess.user);
    sess.user.set_daten(0);
    sess.user.set_sysstatus(0);

    sess.user.set_timeontoday(0.0);
    sess.user.set_extratime(0.0);
    sess.user.set_total_timeon(0.0);
    sess.user.set_pos_account(0.0);
    sess.user.set_neg_account(0.0);
    sess.user.set_timebank(0);
    sess.user.set_fpts(sys.cfg.newusergold);
    sess.user.set_emailnet(0);
    sess.user.set_postnet(0);
    sess.user.set_fsenttoday1(0);
    sess.user.set_lastrate(sess.modem_speed);
    set_autoval(sys.nifty.nulevel);

    sess.actsl=sess.user.sl();
    /* Default to ANSI + color, skip Avatar/RIP questions */
    sess.user.set_sysstatus_flag(sysstatus_ansi, true);
    sess.user.set_sysstatus_flag(sysstatus_color, true);
    setcolors(sess.user);
    input_screensize();

    if (!io.hangup) {
        if (incom) {
            if (printfile("system")) sl1(0,"9# 0Aborted System Info Message!");
            pausescr();
            if (printfile("newuser")) sl1(0,"9# 0Aborted Newuser Message!");
            pausescr();
        }

        /* Run ScreenForm — clears screen, renders newans.ans, collects all
           user fields.  on_submit calls newuser_form_apply() which populates
           sess.user.  on_cancel sets hangup. */
        ScreenForm form = make_newuser_form(std::string(sys.cfg.gfilesdir));
        SFContext ctx;
        ctx.hangup = &io.hangup;
        ctx.echo   = &io.echo;
        Terminal *term = (Terminal*)term_instance();
        sf_run(*term, ctx, form);
    }

    outchr(12);

    if (!io.hangup) {
        nl();
        outstr("Saving Your Info:");

        /* Assign next user number.  JSON users live in data/users/NNNN.json
           so we just take the highest existing number + 1. */
        sess.usernum = UserDB::instance().max_id() + 1;

        UserDB::instance().store(sess.usernum, sess.user);
        pl(" Done...");
        UserDB::instance().index_add(sess.usernum,sess.user.name());
        sys.status.users = UserDB::instance().user_count();
        save_status();
        ok=0;
        topscreen();
        logpr("9!! 0Added New User 4%s0 to user list",sess.user.display_name(sess.usernum).c_str());
        nl();
        npr("0Your user number is 3%d0.\r\n",sess.usernum);
        npr("0Your password is '3%s0'\r\n",sess.user.password());
        nl();
        pausescr();
        sprintf(s,"7! 0Newuser 4%s 0at %s",sess.user.name(),times());
        ssm(1,0,s);
        if(incom) {
            infoform(sys.nifty.nuinf,0);
            if(printfile("feedback")) sl1(0,"9# 0Aborted Feedback Message!");
            email(1,"NewUser Validation Feedback",0);
            if (sys.cfg.newuser_c[0]) ex("D1",sys.cfg.newuser_c);
        }
    }
#ifdef NUV
    if(sys.nifty.nifstatus & nif_nuv)
        sess.user.set_nuv_status(enter_nuv(sess.user,sess.usernum,1));
#endif

    menubatch("newuser");
}

void infoform(char fn[8],int once)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    char s[300],s1[MAX_PATH_LEN],i,found=0;
    FILE *fnin,*fno;


    sprintf(s,"%s%s.inf",sys.cfg.gfilesdir,fn);

    if(!exist(s)) {
        pl("Infoform Not Found.");
        logpr("7! 0Infoform 4%s 0not found",fn);
        return;
    }

    if(once) {
        fno=fopen(s,"rt");
        found=0;
        while(fgets(s,300,fno)!=NULL&&!found) {
            if(s[0]=='~') {
                filter(s,'\n');
                strcpy(s1,s+1);
                if(!strcmp(sess.user.name(),s1)) {
                    found=1;
                }
            }
        }
        fclose(fno);
        if(found) return;
    }

    sprintf(s,"%s%s.ser",sys.cfg.gfilesdir,fn);
    fno=fopen(s,"a");

    sprintf(s1,"%s%s.inf",sys.cfg.gfilesdir,fn);
    fnin=fopen(s1,"rt");

    sprintf(s,"~%s\n",sess.user.name());
    fputs(s,fno);

    while(fgets(s,255,fnin)!=NULL&&!io.hangup) {
        filter(s,'\n');
        if(s[0]==';') {
            fputs(s,fno);
            fputs("\n",fno);
        }
        else if(strchr(s,'*')) {
            filter(s,'*');
            do {
                outstr(s);
                inputl(s1,51);
            }
            while(!s1[0]&&!io.hangup);
            strcat(s,s1);
            strcat(s,"\n");
            fputs(s,fno);
        }
        else if(strchr(s,'@')) {
            filter(s,'@');
            outstr(s);
            i=ny();
            strcat(s,i?"Yes":"No");
            strcat(s,"\n");
            fputs(s,fno);
        }
        else if(strchr(s,'^')) {
            filter(s,'^');
            printfile(s);
        }
        else {
            pl(s);
            strcat(s,"\n");
            fputs(s,fno);
        }
    }
    if(io.hangup) {
        fputs("\n",fno);
        fputs("- HUNGUP -\n",fno);
    }

    fclose(fno);
    fclose(fnin);
}


void readform(char fn[8],char i[31])
{
    auto& sys = System::instance();
    char s[300],s1[300],go=1;
    int i1,red=0,abort=0;
    FILE *fnin;
    User u;

    sprintf(s,"%s%s.ser",sys.cfg.gfilesdir,fn);
    if(!exist(s)) {
        pl("not found");
        return;
    }

    fnin=fopen(s,"rt");
    i1=UserDB::instance().finduser(i);
    if(i1>0) {
        auto p = UserDB::instance().get(i1); if (p) u = *p;
    }
    else
        return;


    while(fgets(s,300,fnin)!=NULL&&go&&!abort) {
        if(s[0]=='~') {
            red=1;
            filter(s,'\n');
            strcpy(s1,s+1);
            if(strcmp(u.name(),s1)==0) {
                go=0;
                while((fgets(s,300,fnin))!=NULL&&s[0]!='~') {
                    filter(s,'\n');
                    pla(s,&abort);
                }
            }
        }
    }

    if(!red)
        npr("No Infoform found for %s",i);

    fclose(fnin);
    pausescr();
}
