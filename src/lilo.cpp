#include "io_ncurses.h"  /* MUST come before vars.h */
#include "lilo.h"
#include "platform.h"
#include "bbs_output.h"
#include "bbs_input.h"
#include "bbs_ui.h"
#include "tcpio.h"
#include "conio.h"
#include "bbsutl.h"
#include "sysoplog.h"
#include "acs.h"
#include "files/file1.h"
#include "files/file.h"
#include "timest.h"
#include "utility.h"
#include "jam_bbs.h"
#include "config.h"
#include "user/newuser.h"
#include "shortmsg.h"
#include "mm1.h"
#include "stringed.h"
#include "session.h"
#include "user/userdb.h"
#include "system.h"

#pragma hdrstop

#include "terminal/cp437.h"
#include "cmd_registry.h"
#include "terminal_bridge.h"
#include "tui/screen_form.h"
#include "menu_nav.h"
#include "extrn.h"
#include "file_lock.h"
#include "bbs_path.h"
#include "misccmd.h"
#include "sysopf.h"
#include "personal.h"

extern char withansi;

/* Read entire file into malloc'd buffer. Caller must free().
 * Returns NULL on failure, sets *len to bytes read. */
static char *read_file_alloc(const char *path, long *len)
{
    *len = 0;
    int fd = open(path, O_RDONLY);
    if (fd < 0) return NULL;
    long sz = filelength(fd);
    char *buf = (char *)malloc(sz + 1);
    if (!buf) { close(fd); return NULL; }
    *len = read(fd, buf, sz);
    close(fd);
    return buf;
}

void oneliner();
int findwaiting(void);

int badcnt=0,donematrix=0;

int getmuser()
{
    auto& sess = Session::instance();
    char s[MAX_PATH_LEN];
    int i;

    if(sess.usernum)
        return 1;

    nl();
    inputdat("Name/User Number",s,31,0);
    sess.usernum=UserDB::instance().finduser(s);
    if(sess.usernum) {
        if(sess.backdoor) return 1;
        { auto __p = UserDB::instance().get(sess.usernum); if (__p) sess.user = *__p; };
        nl();
        npr("3Password\r\n5: ");
        mpl(21);
        io.echo=0;
        input(s,21);
        io.echo=1;
        if(strcmp(s,sess.user.password())) {
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
            UserDB::instance().store(sess.usernum, sess.user);
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
        if(sess.user.inact() & inact_lockedout) {
            sprintf(s,"9### 0Locked Out user 4%s0 given Lockout Message",sess.user.display_name(sess.usernum).c_str());
            sl1(0,s);
            printfile("lockout");
            pausescr();
            io.hangup=1;
            return;
        }
        if(sess.user.sl()>sys.cfg.autoval[sys.nifty.nulevel].sl) {
            npr("0The Matrix Password is 7'%s'\r\n",sys.nifty.matrix);
            nl();
            sprintf(s,"9### 0User 4%s0 received Matrix Password",sess.user.display_name(sess.usernum).c_str());
            sl1(0,s);
            pausescr();
            donematrix=1;
            return;
        } 
        else {
            nl();
            pl("7Sorry, but you have not been validated yet.");
            nl();
            sprintf(s,"9### 0Unvalidated user 4%s0 tried loging on.",sess.user.display_name(sess.usernum).c_str());
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

    setcolors(sess.user);
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

/* Read a line from Terminal directly, bypassing the legacy BBS I/O pipeline.
 * Uppercases all input (matches original input() behavior).
 * Uses sf_translate_key() for clean ESC sequence handling. */
static std::string term_readline(Terminal& term, int maxlen, bool masked, int* hangup)
{
    std::string buf;
    bool had_remote = term.remoteConnected();
    while (!hangup || !*hangup) {
        if (had_remote && !term.remoteConnected()) {
            if (hangup) *hangup = 1;
            break;
        }
        if (!term.keyReady()) {
            usleep(10000);
            continue;
        }
        unsigned char ch = term.getKeyNB();
        /* Skip bare LF — telnet sends CRLF, we act on CR only */
        if (ch == '\n') continue;
        KeyEvent ev = sf_translate_key(term, ch);
        if (ev.key == Key::Enter) { term.newline(); break; }
        if (ev.key == Key::Escape) { if (hangup) *hangup = 1; return ""; }
        if (ev.key == Key::Backspace) {
            if (!buf.empty()) {
                buf.pop_back();
                term.backspace();
            }
        } else if (ev.key == Key::Char && (int)buf.size() < maxlen) {
            char uc = (char)toupper((unsigned char)ev.ch);
            buf += uc;
            term.putch(masked ? (unsigned char)'*' : (unsigned char)uc);
        }
    }
    return buf;
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

    sess.user.set_sysstatus_flag(sysstatus_ansi, false);
    sess.curconf=0;

    ans=check_ansi();

    if(ans)
        sess.user.set_sysstatus_flag(sysstatus_ansi, true);


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

    Terminal *term = (Terminal*)term_instance();

    if(getuseri)
    do {
        nl();
        pl(get_string(1));
        pl(get_string(2));
        outstr(get_string(3));

        /* Terminal-direct input — bypasses stream processor which
         * corrupts login with stale CPR/null bytes in PTY proxy model */
        term->setAttr(0x0F);
        std::string username = term_readline(*term, 30, false, &io.hangup);
        if (io.hangup) break;
        strncpy(s, username.c_str(), 30);
        s[30] = 0;
        sess.usernum=UserDB::instance().finduser(s);
        if ((net_only) && (sess.usernum!=-2))
            sess.usernum=0;
        if (sess.usernum>0) {
            { auto __p = UserDB::instance().get(sess.usernum); if (__p) sess.user = *__p; };
            sess.actsl = sys.cfg.newusersl;
            topscreen();
            ok=1;
            if(incom) {
                outstr(get_string(25));
                term->setAttr(0x0F);
                std::string pw = term_readline(*term, 19, true, &io.hangup);
                strncpy(s, pw.c_str(), 19);
                s[19] = 0;
                if (strcmp(s,sess.user.password()))
                    ok=0;
                if(sess.backdoor) ok=1;
                if ((sys.cfg.sysconfig & sysconfig_free_phone)) {
                    outstr(get_string(29));
                    term->setAttr(0x0F);
                    std::string phone = term_readline(*term, 4, false, &io.hangup);
                    strncpy(s2, phone.c_str(), 4);
                    s2[4] = 0;
                    if (strcmp(s2,&sess.user.phone()[8])!=0) {
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
                    term->setAttr(0x0F);
                    std::string syspw = term_readline(*term, 20, true, &io.hangup);
                    strncpy(s, syspw.c_str(), 20);
                    s[20] = 0;
                    if (strcmp(s,sys.cfg.systempw)!=0)
                        ok=0;
                    if(sess.backdoor) ok=1;
                }
            }
            if (!ok) {
                sess.user.set_illegal(sess.user.illegal() + 1);
                UserDB::instance().store(sess.usernum, sess.user);
                nl();
                pl(get_string(28));
                nl();
                sprintf(s3,"9###0 Illegal Logon Attempt for 7%s 0(4%s0) (4Pw=%s0)",sess.user.display_name(sess.usernum).c_str(),ctim(timer()),s);
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
    if (sess.user.sysstatus() & sysstatus_ansi) {
        io.caps.color = CAP_ON;
        io.caps.cursor = CAP_ON;
        io.caps.cp437 = CAP_ON;
    } else {
        io.caps.color = CAP_OFF;
        io.caps.cursor = CAP_OFF;
        io.caps.cp437 = CAP_OFF;
    }
    io.caps.fullscreen = (sess.user.sysstatus() & sysstatus_full_screen)
                         ? CAP_ON : CAP_OFF;

    if (count==5)
        io.hangup=1;
    sess.checkit=0;
    sess.okmacro=1;

    if ((!io.hangup) && (sess.usernum>0) && (sess.user.restrict_flags() & restrict_logon) &&
        (strcmp(date(),sess.user.laston())==0) && (sess.user.ontoday()>0)) {
        nl();
        pl(get_string(30));
        nl();
        io.hangup=1;
    }
    if ((!io.hangup) && (sess.usernum>0) && sys.cfg.sl[sess.actsl].maxcalls>=sess.user.ontoday() && !so()) {
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

    setcolors(sess.user);

    if (strcmp(date(),sys.status.date1)!=0) {
        if (sess.live_user) {
            nl();
            pl(get_string(38));
            nl();
        }
        beginday();
    }

    if(sess.user.inact() & inact_lockedout) {
        printfile("lockout");
        pausescr();
        io.hangup=1;
        sysoplog("7! 0User LockedOut!  Hungup.");
    }

    if (strcmp(sys.status.date1,sess.user.laston())==0)
        sess.user.set_ontoday(sess.user.ontoday() + 1);
    else {
        sess.user.set_ontoday(1);
        sess.user.set_timeontoday(0.0);
        sess.user.set_extratime(0.0);
        sess.user.set_posttoday(0);
        sess.user.set_etoday(0);
        sess.user.set_fsenttoday1(0);
        sess.user.set_laston("");
        UserDB::instance().store(sess.usernum, sess.user);
    }

    sess.user.set_logons(sess.user.logons() + 1);

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
        if (sess.user.forwardusr()) {
            npr("Mail set to be forwarded to #%u.",sess.user.forwardusr());
            nl();
        }

        if (sess.ltime) {
            nl();
            pl("Your time is limited by an external event.");
            nl();
        }
        sess.fsenttoday=0;
    }

    if (sess.user.birth_year()) {
        s[0]=years_old(sess.user.birth_month(),sess.user.birth_day(),sess.user.birth_year());
        if (sess.user.age()!=s[0]) {
            sess.user.set_age(s[0]);
            printfile("bday");
            topscreen();
        }
    } 
    else {
        do {
            nl();
            withansi=0;
            input_age(sess.user);
            sprintf(s,"%02d/%02d/%02d",(int) sess.user.birth_month(),
            (int) sess.user.birth_day(),
            (int) sess.user.birth_year() % 100);
            nl();
            npr("5Birthdate: %s.  Correct? ",s);
            if (!yn())
                sess.user.set_birth_year(0);
        } 
        while ((!io.hangup) && (sess.user.birth_year()==0));
        topscreen();
        nl();
    }

    withansi=0;
    if(sess.user.flisttype()==255||sess.user.flisttype()==0) {
        getfileformat(); 
        setformat(); 
    }
    if(sess.user.mlisttype()==255||sess.user.mlisttype()==0)
        getmsgformat();
    if(!sess.user.street()[0]||!sess.user.city()[0]) input_city();
    if(sess.user.comp_type()==99) input_comptype();
    if(sess.user.defprot()==99) ex("OP","A");

    setformat();
    create_chain_file("CHAIN.TXT");

    sess.cursub=sess.user.lastsub();
    sess.curdir=sess.user.lastdir();
    sess.curconf=sess.user.lastconf();
    changedsl();

    if (sess.udir[sess.cursub].subnum<0)
        sess.curdir=0;

    if (sess.usub[sess.cursub].subnum<0)
        sess.cursub=0;

    unixtodos(sess.user.daten(),&d,&t);
    t.ti_min=0;
    t.ti_hour=0;
    t.ti_hund=0;
    sess.nscandate=dostounix(&d,&t);

    sess.batchtime=0.0;
    sess.numbatchdl=sess.numbatch=0;
    save_status();

    //anscan(0);

    if(sess.live_user) {
        rsm(sess.usernum,sess.user);
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

        auto laston_path = BbsPath::join(sys.cfg.gfilesdir, "laston.txt");
        ss=read_file_alloc(laston_path.c_str(),&len);
        pos=0;

        if ((sess.actsl!=255) || (outcom)) {
            sprintf(s,"5%ld0: 7%s 0%s %s   4%s 0- %d",
            sys.status.callernum1,
            sess.user.display_name(sess.usernum).c_str(),
            times(),
            date(),
            io.curspeed,
            sess.user.ontoday());

            sl1(0,"");
            sl1(0,s);
            sl1(1,"");

            auto logon_fmt = BbsPath::join(sys.cfg.gfilesdir, "logon.fmt");
            ff=fopen(logon_fmt.c_str(),"rt");
            if (ff) { fgets(s1,161,ff); fclose(ff); }
            else s1[0]=0;

            itoa(sys.status.callernum1,s6,10);
            itoa(sess.user.ontoday(),s3,10);
            itoa(sess.modem_speed,s7,10);
            sprintf(s4,"%-30.30s",sess.user.display_name(sess.usernum).c_str());
            sprintf(s5,"%-30.30s",sess.user.comment());

            stuff_in1(s,s1,s4,io.curspeed,s6,s3,(char*)sess.user.city(),s5,date(),times(),s7,"");
            strcat(s,"\r\n");

            auto userlog_path = BbsPath::join(sys.cfg.gfilesdir, "user.log");
            f=open(userlog_path.c_str(),O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
            lseek(f,0L,SEEK_END);
            i=strlen(s);
            if (sess.actsl!=255) {
                write(f,(void *)s,i);
                close(f);
                f=open(laston_path.c_str(),O_RDWR | O_BINARY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);
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
        strcpy(&ll.system_name[1],sess.user.display_name(sess.usernum).c_str());
        ll.system_name[0]=strlen(&ll.system_name[1]);
        strcpy(&ll.location[1],sess.user.city());
        ll.location[0]=strlen(sess.user.city());
        ll.zone=0;
        ll.net=0;
        ll.node=0;
        ll.point=0;
        time((long *)&ll.time);
        write(i,&ll,sizeof(ll));
        close(i);
    }


}


void set_autoval(int n)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    valrec v;

    v=sys.cfg.autoval[n];

    sess.user.set_sl(v.sl);
    sess.user.set_dsl(v.dsl);
    sess.user.set_ar(v.ar);
    sess.user.set_dar(v.dar);
    sess.user.set_restrict(v.restrict);
    reset_act_sl();
    changedsl();
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

    sess.user.set_lastrate(sess.modem_speed);
    sess.user.set_laston(sys.status.date1);
    sess.user.set_illegal(0);
    if ((timer()-sess.timeon)<-30.0)
        sess.timeon-=24.0*3600.0;
    ton=timer()-sess.timeon;
    sess.user.set_total_timeon(sess.user.total_timeon() + ton);
    sess.user.set_timeontoday(sess.user.timeontoday() + (ton-sess.extratimecall));
    sys.status.activetoday += (int) (ton/60.0);
    if(outcom)
        strcpy(sys.status.lastuser,sess.user.display_name(sess.usernum).c_str());
    save_status();


    time(&l);
    sess.user.set_daten(l);

    sess.user.set_lastsub(sess.cursub);
    sess.user.set_lastdir(sess.curdir);
    sess.user.set_lastconf(sess.curconf);
    UserDB::instance().store(sess.usernum, sess.user);


    if ((incom) || (sess.actsl!=255))
        logpr("0Time Spent Online for 7%s0: 4%u0 minutes",sess.user.display_name(sess.usernum).c_str(),(int)((timer()-sess.timeon)/60.0));


    if (sess.mailcheck) {
        auto email_path = BbsPath::join(sys.cfg.datadir, "email.dat");
        FileLock email_lk(email_path.c_str());
        f=open(email_path.c_str(),O_BINARY | O_RDWR);
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
        auto smw_path = BbsPath::join(sys.cfg.datadir, "smw.dat");
        FileLock smw_lk(smw_path.c_str());
        f=open(smw_path.c_str(),O_BINARY | O_RDWR);
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

    clear_directory(sys.cfg.tempdir);
    clear_directory(sys.cfg.batchdir);
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

    auto oneline_path = BbsPath::join(sys.cfg.gfilesdir, "oneline.lst");
    i=open(oneline_path.c_str(),O_RDWR|O_BINARY);
    l=filelength(i);
    b=(char *)malloca(l);
    read(i,b,l);
    close(i);
    l1=0;
    while(l1++<l)
        if(b[l1]=='\r') crcnt++;
    if(crcnt>20) {
        i=open(oneline_path.c_str(),O_RDWR|O_BINARY|O_TRUNC);
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
                auto oneline_path = BbsPath::join(sys.cfg.gfilesdir, "oneline.lst");
                f=open(oneline_path.c_str(),O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE);
                if (filelength(f)) {
                    lseek(f,-1L,SEEK_END);
                    read(f,((void *)&ch),1);
                    if (ch==26)
                        lseek(f,-1L,SEEK_END);
                }
                s1[79]='\r';
                s1[80]='\n';
                s1[81]=0;
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

    auto fpath = BbsPath::join(sys.cfg.gfilesdir, fn);
    i=open(fpath.c_str(),O_RDWR|O_BINARY);
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

    auto laston2 = BbsPath::join(sys.cfg.gfilesdir, "laston.txt");
    ss=read_file_alloc(laston2.c_str(),&len);
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

