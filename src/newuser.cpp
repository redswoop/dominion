#include "newuser.h"
#include "platform.h"
#include "fcns.h"
#include "conio.h"
#include "bbsutl.h"
#include "file1.h"
#include "bbsutl2.h"
#include "timest.h"
#include "disk.h"
#include "utility.h"
#include "jam_bbs.h"
#include "utility1.h"
#include "mm1.h"
#include "session.h"
#include "system.h"
#include "cmd_registry.h"
#include "nuv.h"
#include "terminal_bridge.h"
#include "misccmd.h"
#include "personal.h"
#pragma hdrstop


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

#define gotop() goin(2,4);


int check_name(char *nn)
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
    if (finduser(nn)!=0)
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

char withansi;

void input_comment(void)
{
    auto& sess = Session::instance();
    char s[MAX_PATH_LEN];

    if(withansi) gotop();
    pl("0Enter your Comment");
    if(withansi) goin(14,18); 
    else
        outchr(':');
    inputl(s,35);
    strcpy(sess.user.comment,s);
}

void input_name(char *namer)
{
    auto& sess = Session::instance();
    int ok,count;
    char s[41];

    count=0;
    do {
        nl();
        if(withansi) gotop();
        outstr("0Enter a handle or your real name: ");
        if (withansi) goin(7,18);
        inputl(s,30);
        strcpy(sess.user.name,s);
        strcpy(namer,sess.user.name);
        strupr(sess.user.name);
        ok=check_name(sess.user.name);
        if (!ok) {
            nl();
            if(withansi) gotop();
            pl("I'm sorry, you can't use that name.");
            delay(200);
            ++count;
            if (count==5)
                io.hangup=1;
        }
    } 
    while ((!ok) && (!io.hangup));
}

void input_realname(char *namer)
{
    auto& sess = Session::instance();
    do {
        nl();
        if(withansi)  gotop();
        pl("0Enter your real name, or = if same as alias.");
        if(withansi) goin(8,18); 
        else outstr(": ");
        inputl(sess.user.realname,20);

        if (sess.user.realname[0]==0) {
            nl();
            if(withansi) gotop();
            pl("Sorry, you must enter your real name.");
        }

        if (sess.user.realname[0]=='=') strcpy(sess.user.realname,namer);
    } 
    while ((sess.user.realname[0]==0) && (!io.hangup));
}

void input_city()
{
    auto& sess = Session::instance();
    char s[MAX_PATH_LEN],s1[MAX_PATH_LEN];
    nl();

    do {
        if(withansi) gotop();
        pl("0Enter Your Street Address");
        if(withansi) goin(12,18);
        else outstr("3:0");
        inputl(s,35);
    } 
    while(!s[0] && !io.hangup);
    strcpy(sess.user.street,s);

    nl();

    do {
        if(withansi) gotop();
        pl("Enter Your City and State: ");
        if(withansi) goin(13,18);
        else outstr(": ");
        inputl(s,35);
        strcpy(sess.user.city,s);
    } 
    while(!sess.user.city[0]&&!io.hangup);
}

void input_phone()
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    int ok,i;

    do {
        nl();
        if(withansi) gotop();
        pl("0Enter your voice phone number");
        if(withansi) goin(11,18); 
        else
            outstr("3:0");
        i=inputfone(sess.user.phone);
        if(i)
            ok=1;
        else {

            ok=1;
            if ((sys.cfg.sysconfig & sysconfig_free_phone)==0) {
                if (strlen(sess.user.phone)!=12)
                    ok=0;
                if ((sess.user.phone[3]!='-') || (sess.user.phone[7]!='-'))
                    ok=0;
                /* 1993 area code rule (second digit must be 0 or 1) removed —
                   no longer valid after mid-90s area code expansion */
                for (i=0; i<12; i++)
                    if ((i!=3) && (i!=7))
                        if ((sess.user.phone[i]<'0') || (sess.user.phone[i]>'9'))
                            ok=0;
            }

            if (!ok) {
                nl();
                if(withansi) gotop();
                pl("Please enter a valid phone number in the correct format.");
                delay(300);
            }
        }
    } 
    while ((!ok) && (!io.hangup));

}

void input_sex(userrec *u)
{
    nl();
    if(withansi) gotop();
    outstr("0Sex <M>ale,<F>emale,<Y>es,<L>ots?0 ");
    if(withansi) goin(9,18);
    u->sex=onek("MFYL");
}

void input_age(userrec *u)
{
    int a,ok,y,m,d;
    char s[10];

    do {
        if(withansi) gotop();
        pl("0Enter your birthdate, in 00/00/00 Form.");
        if(withansi) goin(10,18); 
        else
            outstr("3:0 ");
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

    u->month=(unsigned char) m;
    u->day=(unsigned char) d;
    u->year=(unsigned char) (y-1900);
    u->age=years_old(u->month,u->day,u->year);
    nl();
}


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
            npr("0%d>2 %s\r\n",i+1,getComputerType(i));
        }
        nl();
        if(withansi) gotop();
        pl("2Enter your computer type");
        if(withansi) goin(15,18); 
        else
            outstr("3:0");
        input(c,2);
        ct=atoi(c);

        ok=1;
        if ((ct<1) || (ct>i))
            ok=0;

    } 
    while ((!ok) && (!io.hangup));
    sess.user.comp_type=ct-1;
    if (io.hangup)
        sess.user.comp_type=0;
}

void input_screensize()
{
    auto& sess = Session::instance();
    int ok,x,y;
    char s[5];

    do {
        nl();
        pl("0How wide is your screen (7chars, <CR>=800) ?");
        outstr("3:0");
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
        pl("0How tall is your screen (7lines, <CR>=250) ?");
        outstr("3:0");
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

    sess.user.screenchars=x;
    sess.user.screenlines=y;
    io.screenlinest=y;
}

void input_pw()
{
    auto& sess = Session::instance();
    int ok;
    char s[MAX_PATH_LEN];

    do {
        nl();
        if(withansi) gotop();
        pl("2Please enter a password, 3-20 chars.");
        if(withansi) goin(16,18); 
        else
            outstr("3:0");
        input(s,20);

        ok=1;
        if (strlen(s)<3)
            ok=0;
    } 
    while ((!ok) && (!io.hangup));
    if (ok)
        strcpy(sess.user.pw,s);
    else
        pl("Password not changed.");
}


void newuser()
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    int i,ok;
    char s[255],s1[MAX_PATH_LEN],ch;
    userrec u;
    long l1,l2;
    hdrinfo hdr;

    sprintf(s,"7!! 0New User 4%s 0at 5%s0, %s Baud",date(),times(),io.curspeed);
    sl1(0,"");
    sl1(0,s);
    if (userdb_user_count()>=(int)sys.cfg.maxusers) {
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

    strcpy(sess.user.firston,date());
    strcpy(sess.user.laston,"Never.");
    strcpy(&sess.user.macros[0][0],"Trust me, no.");
    strcpy(&sess.user.macros[1][0],"Dominion=Good?");
    strcpy(&sess.user.macros[2][0],"Glub Glub Glub...");
    strcpy(&sess.user.macros[3][0],"Use the Force Luke!");
    strcpy(sess.user.note,"");
    strcpy(sess.user.pw,"");

    sess.user.screenlines=25;
    io.screenlinest=25;

    sess.user.inact=0;
    sess.user.defprot=0;
    sess.user.defed=0;
    sess.user.nuv=-1;
    sess.user.sl=sys.cfg.newusersl;
    sess.user.dsl=sys.cfg.newuserdsl;
    sess.user.exempt=0;
    for (i=0; i<20; i++) sess.user.votes[i]=0;
    for (i=0; i<20; i++) sess.user.colors[i]=7;
    sess.user.illegal=0;
    sess.user.waiting=0;
    sess.user.subop=255;
    sess.user.ontoday=1;
    sess.user.month=0;
    sess.user.day=0;
    sess.user.year=0;
    sess.user.age=0;
    sess.user.pcr=0;
    sess.user.ratio=0;

    sess.user.forwardusr=0;
    sess.user.msgpost=0;
    sess.user.emailsent=0;
    sess.user.feedbacksent=0;
    sess.user.posttoday=0;
    sess.user.etoday=0;
    sess.user.ar=0;
    sess.user.dar=0;
    sess.user.restrict=sys.cfg.newuser_restrict;
    sess.user.uploaded=0;
    sess.user.downloaded=0;
    sess.user.lastrate=0;
    sess.user.logons=0;
    sess.user.msgread=0;
    sess.user.uk=0;
    sess.user.dk=0;
    for(i=0;i<200;i++) sess.user.qscn[i]=0;
    for(i=0;i<200;i++) sess.user.nscn[i]=0;


    setcolors(&sess.user);
    sess.user.daten=0;
    sess.user.sysstatus=0;

    sess.user.timeontoday=0.0;
    sess.user.extratime=0.0;
    sess.user.timeon=0.0;
    sess.user.pos_account=0.0;
    sess.user.neg_account=0.0;
    sess.user.timebank=0;
    sess.user.fpts=sys.cfg.newusergold;
    sess.user.emailnet=0;
    sess.user.postnet=0;
    sess.user.fsenttoday1=0;
    sess.user.lastrate=sess.modem_speed;
    set_autoval(sys.nifty.nulevel);

    sess.actsl=sess.user.sl;
    /* Default to ANSI + color, skip Avatar/RIP questions */
    sess.user.sysstatus |= sysstatus_ansi;
    sess.user.sysstatus |= sysstatus_color;
    setcolors(&sess.user);
    input_screensize();

    if (!io.hangup) {
        if (incom) {
            if (printfile("system")) sl1(0,"9# 0Aborted System Info Message!");
            pausescr();
            if (printfile("newuser")) sl1(0,"9# 0Aborted Newuser Message!");
            pausescr();
        }
        outchr(12);
        withansi=sess.user.sysstatus & sysstatus_ansi;
        if(withansi) {
            int saved_incom = incom;
            io.mciok=0;
            incom=0;
            printfile("newans.ans");
            incom=saved_incom;
        }
        input_name(s);
        input_realname(s);
        input_sex(&sess.user);
        input_age(&sess.user);
        input_phone();
        input_city();
        input_comment();
        input_comptype();
        input_pw();
        sess.user.helplevel=2;
        sess.user.lastconf=1;
        sess.user.lastsub=0;
        sess.user.lastdir=0;
    }

    if (!io.hangup)
    do {
        nl();
        if(!withansi) {
            outchr(12);
            npr("<1> Name            %s\r\n",sess.user.name);
            npr("<2> Real Name       %s\r\n",sess.user.realname);
            npr("<3> Sex             %c\r\n",sess.user.sex);
            npr("<4> Birthdate       %02d/%02d/%02d\r\n",(int) sess.user.month,(int) sess.user.day,(int) sess.user.year);
            npr("<5> Phone Number    %s\r\n",sess.user.phone);
            npr("<6> Address         %s\r\n%-20s%s\r\n",sess.user.street,"",sess.user.city);
            npr("<7> Computer type   %s\r\n",getComputerType(sess.user.comp_type));
            npr("<8> Comment         %s\r\n",sess.user.comment);
            npr("<9> Password        %s\r\n",sess.user.pw);
            npr("<Q> No changes.");
            nl();
            nl();
        }
        if(withansi) gotop();
        npr("New User Configuration (Q=Quit) ");
        ch=onek("Q123456789");
        ok=0;
        switch(ch) {
        case 'Q': 
            ok=1; 
            break;
        case '1': 
            input_name(s); 
            break;
        case '2': 
            input_realname(s); 
            break;
        case '3': 
            input_sex(&sess.user); 
            break;
        case '4': 
            input_age(&sess.user); 
            break;
        case '5': 
            input_phone(); 
            break;
        case '6': 
            input_city(); 
            break;
        case '7': 
            input_comment(); 
            break;
        case '8': 
            input_comptype(); 
            break;
        case '9': 
            input_pw(); 
            break;
        }
    } 
    while ((!ok) && (!io.hangup));

    outchr(12);
    /* Set sensible defaults — skip the barrage of post-reg questions.
       Users can change these later from the Options menu. */
    sess.user.flisttype=1;
    sess.user.helplevel=2;


    if (!io.hangup) {
        nl();
        outstr("Saving Your Info:");

        /* Assign next user number.  JSON users live in data/users/NNNN.json
           so we just take the highest existing number + 1. */
        sess.usernum = userdb_max_num() + 1;

        userdb_save(sess.usernum,&sess.user);
        pl(" Done...");
        userdb_index_add(sess.usernum,sess.user.name);
        sys.status.users = userdb_user_count();
        save_status();
        ok=0;
        topscreen();
        logpr("9!! 0Added New User 4%s0 to user list",nam(&sess.user,sess.usernum));
        nl();
        npr("0Your user number is 3%d0.\r\n",sess.usernum);
        npr("0Your password is '3%s0'\r\n",sess.user.pw);
        nl();
        pausescr();
        sprintf(s,"7! 0Newuser 4%s 0at %s",sess.user.name,times());
        ssm(1,0,s);
        if(incom) {
            infoform(sys.nifty.nuinf,0);
            if(printfile("feedback")) sl1(0,"9# 0Aborted Feedback Message!");
            email(1,"NewUser Validation Feedback",0);
            if (sys.cfg.newuser_c[0]) ex("D1",sys.cfg.newuser_c);
        }
    }
#ifdef NUV
    if(sys.nifty.nifstatus & nif_nuv)
        sess.user.nuv=enter_nuv(sess.user,sess.usernum,1);
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
        logpr("7! 0Infoform 4%s 0not found",fn);
        return;
    }

    if(once) {
        fno=fopen(s,"rt");
        found=0;
        while(fgets(s,300,fno)!=NULL&&!found) {
            if(s[0]=='~') {
                filter(s,'\n');
                strcpy(s1,s+1);
                if(!strcmp(sess.user.name,s1)) {
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

    sprintf(s,"~%s\n",sess.user.name);
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
    userrec u;

    sprintf(s,"%s%s.ser",sys.cfg.gfilesdir,fn);
    if(!exist(s)) {
        pl("not found"); 
        return; 
    }

    fnin=fopen(s,"rt");
    i1=finduser(i);
    if(i1>0)
        userdb_load(i1,&u);
    else
        return;


    while(fgets(s,300,fnin)!=NULL&&go&&!abort) {
        if(s[0]=='~') {
            red=1;
            filter(s,'\n');
            strcpy(s1,s+1);
            if(strcmp(u.name,s1)==0) {
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
