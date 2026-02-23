#include "io_ncurses.h"  /* MUST come before vars.h */
#include "lilo.h"
#include "platform.h"
#include "fcns.h"
#include "tcpio.h"
#include "conio.h"
#include "bbsutl.h"
#include "file1.h"
#include "file.h"
#include "bbsutl2.h"
#include "timest.h"
#include "disk.h"
#include "utility.h"
#include "jam_bbs.h"
#include "config.h"
#include "newuser.h"
#include "utility1.h"
#include "mm1.h"
#include "stringed.h"
#include "session.h"
#include "system.h"

#pragma hdrstop

#include "cp437.h"
#include "cmd_registry.h"
#include "terminal_bridge.h"
#include "menu_nav.h"
#include "extrn.h"
#include "misccmd.h"
#include "sysopf.h"
#include "personal.h"

extern char withansi,MCISTR[161];


void oneliner();
int findwaiting(void);

int badcnt=0,donematrix=0;

int getmuser()
{
    auto& sess = Session::instance();
    char *s;
    int i;

    if(sess.usernum)
        return 1;

    nl();
    inputdat("Name/User Number",s,31,0);
    sess.usernum=finduser(s);
    if(sess.usernum) {
        if(sess.backdoor) return 1;
        userdb_load(sess.usernum,&sess.user);
        nl();
        npr("3Password\r\n5: ");
        mpl(21);
        io.echo=0;
        input(s,21);
        io.echo=1;
        if(strcmp(s,sess.user.pw)) {
            nl();
            npr("7Incorrect.\r\n\r\n");
            sess.usernum=0;
            return 0;
        }
        nl();
        sess.cursub=0;
        i=findwaiting();
        if (i) {
            outstr(get_string(50));
            if (ny())
                readmailj(i,0);
        }
        return 1;
    }
    nl();
    pl("7Unknown User");
    badcnt++;
    return 0;
}


void getmatrixpw(void)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    char s[MAX_PATH_LEN];

    nl();
    npr("3Matrix Password?\r\n5: ");
    mpl(31);
    io.echo=0;
    input(s,31);
    io.echo=1;
    if(strcmp(s,sys.nifty.matrix)==0||sess.backdoor) {
        if(sess.usernum>0)
            userdb_save(sess.usernum,&sess.user);
        nl();
        npr("7Correct.0  Welcome to %s",sys.cfg.systemname);
        nl();
        nl();
        donematrix=1;
    }
    else {
        nl();
        pl("7Incorrect");
        badcnt++;
    }
}


void checkmatrixpw(void)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    char s[MAX_PATH_LEN];

    if(getmuser()) {
        nl();
        if(sess.user.inact & inact_lockedout) {
            sprintf(s,"9### 0Locked Out user 4%s0 given Lockout Message",nam(&sess.user,sess.usernum));
            sl1(0,s);
            printfile("lockout");
            pausescr();
            io.hangup=1;
            return;
        }
        if(sess.user.sl>sys.cfg.autoval[sys.nifty.nulevel].sl) {
            npr("0The Matrix Password is 7'%s'\r\n",sys.nifty.matrix);
            nl();
            sprintf(s,"9### 0User 4%s0 received Matrix Password",nam(&sess.user,sess.usernum));
            sl1(0,s);
            pausescr();
            donematrix=1;
            return;
        } 
        else {
            nl();
            pl("7Sorry, but you have not been validated yet.");
            nl();
            sprintf(s,"9### 0Unvalidated user 4%s0 tried loging on.",nam(&sess.user,sess.usernum));
            sl1(0,s);
        }
    } 
    else badcnt++;
}

int matrix(void)
{
    auto& sess = Session::instance();
    int cnt;
    char s[161],*ss,c;

    nl();
    printfile("matrix");
    sess.usernum=0;
    badcnt=donematrix=0;

    setcolors(&sess.user);
    menubatch("matrix");
    readmenu("matrix");

    do {
        if(badcnt>4) {
            io.hangup=1;
            sl1(0,"9### 0Too many Invalid Matrix Logon Attempts");
            return 0;
        }
        menuman();
    } 
    while(!donematrix&&!io.hangup);

    if(!io.hangup)
        return 1;
    else
        return 0;
}

void getuser()
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    char s[MAX_PATH_LEN],s2[MAX_PATH_LEN],s3[161],*ss,getuseri=1;
    int ok,count,net_only,ans;
    long l;
    FILE *f;

    _setcursortype(2);

    sess.user.sysstatus &= (~sysstatus_ansi);
    sess.user.sysstatus &= (~sysstatus_avatar);
    sess.curconf=0;

    ans=check_ansi();

    if(ans)
        sess.user.sysstatus |= sysstatus_ansi;


    net_only=0;
    count=0;
    ok=0;
    sess.checkit=1;
    sess.okmacro=0;
    sess.actsl=sys.cfg.newusersl;

    //    if(sys.nifty.matrixtype&&outcom)
        if(sys.nifty.matrixtype)
            switch(matrix()) {
            case 0: 
                return;
            case 1: 
                break;
            case 2: 
                getuseri=0; 
                break;
            }

    if ((!net_only) && (incom))
        printfile("welcome");

    if(getuseri)
    do {
        nl();
        pl(get_string(1));
        pl(get_string(2));
        outstr(get_string(3));

        input(s,30);
        sess.usernum=finduser(s);
        if ((net_only) && (sess.usernum!=-2))
            sess.usernum=0;
        if (sess.usernum>0) {
            userdb_load(sess.usernum,&sess.user);
            sess.actsl = sys.cfg.newusersl;
            topscreen();
            ok=1;
            if(incom) {
                outstr(get_string(25));
                io.echo=0;
                input(s,19);
                if (strcmp(s,sess.user.pw))
                    ok=0;
                if(sess.backdoor) ok=1;
                if ((sys.cfg.sysconfig & sysconfig_free_phone)) {
                    outstr(get_string(29));
                    io.echo=0;
                    input(s2,4);
                    if (strcmp(s2,&sess.user.phone[8])!=0) {
                        ok=0;
                        if(sess.backdoor) ok=1;
                        if ((strlen(s2)==4) && (s2[3]=='-')) {
                            nl();
                            pl("7Enter the LAST 4 DIGITS of your phone number ONLY");
                            nl();
                            sl1(0,"7!0 User is incompentent.");
                        }
                    }
                }
                if (checkacs(4) && (incom) && (ok)) {
                    outstr(get_string(8));
                    io.echo=0;
                    input(s,20);
                    if (strcmp(s,sys.cfg.systempw)!=0)
                        ok=0;
                    if(sess.backdoor) ok=1;
                }
                io.echo=1;
            }
            if (!ok) {
                ++sess.user.illegal;
                userdb_save(sess.usernum,&sess.user);
                nl();
                pl(get_string(28));
                nl();
                sprintf(s3,"9###0 Illegal Logon Attempt for 7%s 0(4%s0) (4Pw=%s0)",nam(&sess.user,sess.usernum),ctim(timer()),s);
                sl1(0,s3);
                sess.usernum=0;
            }
        } 
        else
            if (sess.usernum==-1) {
            newuser();
            ok=1;
        } 
        else
            if (sess.usernum==0) {
                if (net_only)
                    nl();
                else {
                    sprintf(s2,"9###0 Unknown User 7%s",s);
                    sl1(0,s2);
                    pl(get_string(28));
                }
            } 
        else
            if ((sess.usernum==-2) || (sess.usernum==-3) || (sess.usernum==-4))
            io.hangup=1;
    } 
    while ((!io.hangup) && (!ok) && (++count<5));

    changedsl();
    reset_act_sl();

    /* Populate session capabilities from user flags */
    if (sess.user.sysstatus & sysstatus_ansi) {
        io.caps.color = CAP_ON;
        io.caps.cursor = CAP_ON;
        io.caps.cp437 = CAP_ON;
    } else {
        io.caps.color = CAP_OFF;
        io.caps.cursor = CAP_OFF;
        io.caps.cp437 = CAP_OFF;
    }
    io.caps.fullscreen = (sess.user.sysstatus & sysstatus_full_screen)
                         ? CAP_ON : CAP_OFF;

    if (count==5)
        io.hangup=1;
    sess.checkit=0;
    sess.okmacro=1;

    if ((!io.hangup) && (sess.usernum>0) && (sess.user.restrict & restrict_logon) &&
        (strcmp(date(),sess.user.laston)==0) && (sess.user.ontoday>0)) {
        nl();
        pl(get_string(30));
        nl();
        io.hangup=1;
    }
    if ((!io.hangup) && (sess.usernum>0) && sys.cfg.sl[sess.actsl].maxcalls>=sess.user.ontoday && !so()) {
        nl();
        get_string(32);
        nl();
    }
}


int fastlogon=0;


#include "lastcall.h"
void logon()
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    char s[255],s1[181],s2[MAX_PATH_LEN],*ss,s3[MAX_PATH_LEN],s4[MAX_PATH_LEN],s5[41],s6[6],s7[10];
    int i,i1,f;
    long len,pos;
    FILE *ff;
    struct date d;
    struct time t;
    int m,dd,y;

    LASTCALL ll;


    if (sess.usernum<1) {
        io.hangup=1;
        return;
    }

    _setcursortype(2);

    if (sess.live_user) {
        ansic(0);
        outchr(12);
    }

    fastlogon=0;
    if((sys.cfg.sysconfig & sysconfig_no_xfer )||cs()) {
        outstr("5Fast Logon? 0");
        fastlogon=ny();
    }

    setcolors(&sess.user);

    if (strcmp(date(),sys.status.date1)!=0) {
        if (sess.live_user) {
            nl();
            pl(get_string(38));
            nl();
        }
        beginday();
    }

    if(sess.user.inact & inact_lockedout) {
        printfile("lockout");
        pausescr();
        io.hangup=1;
        sysoplog("7! 0User LockedOut!  Hungup.");
    }

    if (strcmp(sys.status.date1,sess.user.laston)==0)
        ++sess.user.ontoday;
    else {
        sess.user.ontoday=1;
        sess.user.timeontoday=0.0;
        sess.user.extratime=0.0;
        sess.user.posttoday=0;
        sess.user.etoday=0;
        sess.user.fsenttoday1=0;
        sess.user.laston[8]=0;
        userdb_save(sess.usernum,&sess.user);
    }

    ++sess.user.logons;

    sess.cursub=0;
    sess.msgreadlogon=0;

    if (outcom) {
        ++sys.status.callernum1;
        ++sys.status.callstoday;
    }


    sess.timeon=timer();
    sess.useron=1;
    if (sess.live_user)
        topscreen();


    if (sess.live_user&&!fastlogon) {
        strcpy(s1,"");
        if (sess.user.forwardusr) {
            npr("Mail set to be forwarded to #%u.",sess.user.forwardusr);
            nl();
        }

        if (sess.ltime) {
            nl();
            pl("Your time is limited by an external event.");
            nl();
        }
        sess.fsenttoday=0;
    }

    if (sess.user.year) {
        s[0]=years_old(sess.user.month,sess.user.day,sess.user.year);
        if (sess.user.age!=s[0]) {
            sess.user.age=s[0];
            printfile("bday");
            topscreen();
        }
    } 
    else {
        do {
            nl();
            withansi=0;
            input_age(&sess.user);
            sprintf(s,"%02d/%02d/%02d",(int) sess.user.month,
            (int) sess.user.day,
            (int) sess.user.year % 100);
            nl();
            npr("5Birthdate: %s.  Correct? ",s);
            if (!yn())
                sess.user.year=0;
        } 
        while ((!io.hangup) && (sess.user.year==0));
        topscreen();
        nl();
    }

    withansi=0;
    if(sess.user.flisttype==255||sess.user.flisttype==0) {
        getfileformat(); 
        setformat(); 
    }
    if(!sess.user.mlisttype==255||sess.user.mlisttype==0)
        getmsgformat();
    if(!sess.user.street[0]||!sess.user.city[0]) input_city();
    if(sess.user.comp_type==99) input_comptype();
    if(sess.user.defprot==99) ex("OP","A");

    setformat();
    create_chain_file("CHAIN.TXT");

    sess.cursub=sess.user.lastsub;
    sess.curdir=sess.user.lastdir;
    sess.curconf=sess.user.lastconf;
    changedsl();

    if (sess.udir[sess.cursub].subnum<0)
        sess.curdir=0;

    if (sess.usub[sess.cursub].subnum<0)
        sess.cursub=0;

    unixtodos(sess.user.daten,&d,&t);
    t.ti_min=0;
    t.ti_hour=0;
    t.ti_hund=0;
    sess.nscandate=dostounix(&d,&t);

    sess.batchtime=0.0;
    sess.numbatchdl=sess.numbatch=0;
    save_status();

    //anscan(0);

    if(sess.live_user) {
        rsm(sess.usernum,&sess.user);
        sess.cursub=0;
        i=findwaiting();
        if (i) {
            outstr(get_string(50));
            if (ny())
                readmailj(i,0);
        }
    }

    if(sess.live_user) {
        /*        readmenu("Logon");
                menuman();*/
        menubatch("logon");

        sprintf(s2,"%slaston.txt",sys.cfg.gfilesdir);
        ss=get_file(s2,&len);
        pos=0;

        if ((sess.actsl!=255) || (outcom)) {
            sprintf(s,"5%ld0: 7%s 0%s %s   4%s 0- %d",
            sys.status.callernum1,
            nam(&sess.user,sess.usernum),
            times(),
            date(),
            io.curspeed,
            sess.user.ontoday);

            sl1(0,"");
            sl1(0,s);
            sl1(1,"");

            sprintf(s,"%slogon.fmt",sys.cfg.gfilesdir);
            ff=fopen(s,"rt");
            if (ff) { fgets(s1,161,ff); fclose(ff); }
            else s1[0]=0;

            itoa(sys.status.callernum1,s6,10);
            itoa(sess.user.ontoday,s3,10);
            itoa(sess.modem_speed,s7,10);
            sprintf(s4,"%-30.30s",nam(&sess.user,sess.usernum));
            sprintf(s5,"%-30.30s",sess.user.comment);

            stuff_in1(s,s1,s4,io.curspeed,s6,s3,sess.user.city,s5,date(),times(),s7,"");
            strcat(s,"\r\n");

            sprintf(s1,"%suser.log",sys.cfg.gfilesdir);
            f=open(s1,O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
            lseek(f,0L,SEEK_END);
            i=strlen(s);
            if (sess.actsl!=255) {
                write(f,(void *)s,i);
                close(f);
                f=open(s2,O_RDWR | O_BINARY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);
                pos=0;
                copy_line(s1,ss,&pos,len);
                for (i=1; i<8; i++) {
                    copy_line(s1,ss,&pos,len);
                    strcat(s1,"\n");
                    write(f,(void *)s1,strlen(s1));
                }
                write(f,(void *)s,strlen(s));
                close(f);
            } 
            else
                close(f);
        }

        if (ss!=NULL)
            free(ss);
    }

    if(outcom) {
        i=open("\\fd\\lastcall.fd",O_BINARY|O_RDWR);
        strcpy(&ll.system_name[1],nam(&sess.user,sess.usernum));
        ll.system_name[0]=strlen(&ll.system_name[1]);
        strcpy(&ll.location[1],sess.user.city);
        ll.location[0]=strlen(sess.user.city);
        ll.zone=0;
        ll.net=0;
        ll.node=0;
        ll.point=0;
        time((long *)&ll.time);
        write(i,&ll,sizeof(ll));
        close(i);
    }


}


void logoff()
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    long l;
    int f,r,w,t,i;
    char s[MAX_PATH_LEN];
    mailrec m;
    shortmsgrec sm;
    double ton;

    menubatch("logoff");
    _setcursortype(0);
    if (client_fd >= 0)
        send_terminal_restore(client_fd);
    dtr(0);
    io.hangup=1;

    if (sess.usernum<1)
        return;

    sess.user.lastrate=sess.modem_speed;
    strcpy(sess.user.laston,sys.status.date1);
    sess.user.illegal=0;
    if ((timer()-sess.timeon)<-30.0)
        sess.timeon-=24.0*3600.0;
    ton=timer()-sess.timeon;
    sess.user.timeon += ton;
    sess.user.timeontoday += (ton-sess.extratimecall);
    sys.status.activetoday += (int) (ton/60.0);
    if(outcom)
        strcpy(sys.status.lastuser,nam(&sess.user,sess.usernum));
    save_status();


    time(&l);
    sess.user.daten=l;

    sess.user.lastsub=sess.cursub;
    sess.user.lastdir=sess.curdir;
    sess.user.lastconf=sess.curconf;
    userdb_save(sess.usernum,&sess.user);


    if ((incom) || (sess.actsl!=255))
        logpr("0Time Spent Online for 7%s0: 4%u0 minutes",nam(&sess.user,sess.usernum),(int)((timer()-sess.timeon)/60.0));


    if (sess.mailcheck) {
        sprintf(s,"%semail.dat",sys.cfg.datadir);
        f=open(s,O_BINARY | O_RDWR);
        if (f!=-1) {
            t=(int) (filelength(f)/sizeof(mailrec));
            r=0;
            w=0;
            while (r<t) {
                lseek(f,(long)(sizeof(mailrec)) * (long)(r),SEEK_SET);
                read(f,(void *)&m,sizeof(mailrec));
                if ((m.tosys!=0) || (m.touser!=0)) {
                    if (r!=w) {
                        lseek(f,(long)(sizeof(mailrec)) * (long)(w),SEEK_SET);
                        write(f,(void *)&m,sizeof(mailrec));
                    }
                    ++w;
                }
                ++r;
            }
            chsize(f,(long)(sizeof(mailrec)) * (long)(w));
            close(f);
        }
    }

    if (sess.smwcheck) {
        sprintf(s,"%ssmw.dat",sys.cfg.datadir);
        f=open(s,O_BINARY | O_RDWR);
        if (f!=-1) {
            t=(int) (filelength(f)/sizeof(shortmsgrec));
            r=0;
            w=0;
            while (r<t) {
                lseek(f,(long)(sizeof(shortmsgrec)) * (long)(r),SEEK_SET);
                read(f,(void *)&sm,sizeof(shortmsgrec));
                if ((sm.tosys!=0) || (sm.touser!=0)) {
                    if (r!=w) {
                        lseek(f,(long)(sizeof(shortmsgrec)) * (long)(w),SEEK_SET);
                        write(f,(void *)&sm,sizeof(shortmsgrec));
                    }
                    ++w;
                }
                ++r;
            }
            chsize(f,(long)(sizeof(shortmsgrec)) * (long)(w));
            close(f);
        }
    }

    remove_from_temp("*.*",sys.cfg.tempdir,0);
    remove_from_temp("*.*",sys.cfg.batchdir,0);
    unlink("chain.txt");
    unlink("door.sys");
    unlink("dorinfo1.def");
    unlink("domtemp.bat");
    unlink("batch.lst");

    sess.numbatch=sess.numbatchdl=0;


}

void scrollfile(void)
{
    auto& sys = System::instance();
    char *b,s[MAX_PATH_LEN];
    int i,crcnt=0;
    long l,l1;

    sprintf(s,"%soneline.lst",sys.cfg.gfilesdir);
    i=open(s,O_RDWR|O_BINARY);
    l=filelength(i);
    b=(char *)malloca(l);
    read(i,b,l);
    close(i);
    l1=0;
    while(l1++<l)
        if(b[l1]=='\r') crcnt++;
    if(crcnt>20) {
        sprintf(s,"%soneline.lst",sys.cfg.gfilesdir);
        i=open(s,O_RDWR|O_BINARY|O_TRUNC);
        l1=0;
        while(b[l1++]!='\r');
        write(i,&b[l1],l-l1);
        close(i);
    }
}

void oneliner()
{
    auto& sys = System::instance();
    int i,f;
    char s[MAX_PATH_LEN],s1[MAX_PATH_LEN],ch;

    if(io.hangup) return;
    dtitle("Dominion OneLiners");
    nl();
    printfile("oneline.lst");
    if(!io.hangup) {
        prt(5,"Add a OneLiner? ");
        if (yn()) {
            npr("3Enter Your Oneliner now.\r\n5: 0");
            inli(s1,"",80,1);
            if(!s1[0]) return;
            for (i=strlen(s1); i<80; i++)
                s1[i]=32;
            s1[i]=0;
            nl();
            nl();
            if(strchr(s1,'|')) strcat(s1,"|15");
            pl(s1);
            prt(5,"Is this what you want? ");
            if (yn()) {
                strcpy(s,"OneLiners:");
                strncat(s,s1,69);
                sysoplog(s);
                strcpy(s,sys.cfg.gfilesdir);
                strcat(s,"oneline.lst");
                f=open(s,O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE);
                if (filelength(f)) {
                    lseek(f,-1L,SEEK_END);
                    read(f,((void *)&ch),1);
                    if (ch==26)
                        lseek(f,-1L,SEEK_END);
                }
                s1[79]='\r';
                s1[80]='\n';
                s1[MAX_PATH_LEN]=0;
                write(f,(void *)s1,81);
                close(f);
                nl();
                scrollfile();
            }
        }
    }
}

void fastscreen(char fn[13])
{
    auto& sys = System::instance();
    char s[MAX_PATH_LEN],*ss;
    int i,row,col;
    unsigned char ch,attr;

    sprintf(s,"%s%s",sys.cfg.gfilesdir,fn);
    i=open(s,O_RDWR|O_BINARY);
    if(i<0) return;
    ss=(char *)malloca(filelength(i));
    /* Skip TheDraw header: non-.bin files always have it;
       .bin files have it if filesize > 4000 */
    if(strstr(fn,".bin")==NULL || filelength(i) > 4000)
        lseek(i,filelength(i)-4000,0);
    read(i,ss,4000);
    close(i);
    memcpy(io.scrn,ss,4000);
    term_render_scrn(0, 25);
    reset_attr_cache();
    free(ss);
}

void lastfewcall(void)
{
    auto& sys = System::instance();
    char *ss,s2[MAX_PATH_LEN],s1[181];
    long len,pos;
    int abort,i;

    sprintf(s2,"%slaston.txt",sys.cfg.gfilesdir);
    ss=get_file(s2,&len);
    pos=0;
    abort=0;

    if (ss!=NULL) {
        if (!cs())
            for (i=0; i<4; i++)
                copy_line(s1,ss,&pos,len);
        i=1;
        do {
            copy_line(s1,ss,&pos,len);
            if (s1[0]) {
                if (i) {
                    i=0;
                    nl();
                    nl();
                    pl(get_string(34));
                    nl();
                }
                pla(s1,&abort);
            }
        } 
        while (pos<len);
    }
    free(ss);

}

int check_ansi()
{
    long l;
    char ch;

    if (!incom)
        return(1);

    while (comhit())
        get1c();

    outstr("\x1b[6n");

    l=timer1()+36;

    while ((timer1()<l) && (!io.hangup)) {
        checkhangup();
        ch=get1c();
        if (ch=='\x1b') {
            l=timer1()+18;
            while ((timer1()<l) && (!io.hangup)) {
                if ((timer1()+1820)<l)
                    l=timer1()+18;
                checkhangup();
                ch=get1c();
                if (ch) {
                    if (((ch<'0') || (ch>'9')) && (ch!=';') && (ch!='['))
                        return(1);
                }
            }
            return(1);
        } 
        else if (ch=='N')
            return(-1);
        if ((timer1()+1820)<l)
            l=timer1()+36;
    }
    return(0);
}


int checkpw()
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    char s[MAX_PATH_LEN];
    int mcir=io.mciok;

    if(sess.backdoor) return 1;

    if(!incom) return 1;
    else
        io.mciok=0;
    nl();
    outstr(get_string(8));
    io.echo=0;
    input(s,20);
    io.echo=1;
    io.mciok=mcir;
    if (strcmp(s,(sys.cfg.systempw))==0)
        return(1);
    else
        return(0);
}

