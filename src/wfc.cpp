#include "wfc.h"
#include "platform.h"
#include "bbs_output.h"
#include "bbs_input.h"
#include "bbs_ui.h"
#include "tcpio.h"
#include "conio.h"
#include "bbsutl.h"
#include "sysoplog.h"
#include "acs.h"
#include "timest.h"
#include "disk.h"
#include "utility.h"
#include "jam_bbs.h"
#include "config.h"
#include "files/diredit.h"
#include "user/uedit.h"
#include "stringed.h"
#include "session.h"
#include "user/userdb.h"
#include "system.h"
#include "version.h"
#include "cmd_registry.h"
#include "extrn.h"
#include "sysopf.h"
#include "xinit.h"
#include "menued.h"
#include "subedit.h"
#include "lilo.h"
#include "node_registry.h"

#pragma hdrstop

#include <math.h>


double thing=0.00;

int getcaller(void)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    char s[MAX_PATH_LEN],s1[MAX_PATH_LEN],ch,done,lokb;
    int i,i1,i2,i3,x,y,r,c,hold,numhit=0;
    double d,d1,tt;
    long l,l1;


    frequent_init();
    sl1(1,"");
    sess.usernum=0;
    sys.wfc=0;
    hold=0;
    { auto __p = UserDB::instance().get(1); if (__p) sess.user = *__p; };
    sess.usernum=1;
    reset_act_sl();
    sess.cursub=0;
    sess.fwaiting=numwaiting(sess.user);
    if (sess.user.inact() & inact_deleted) {
        sess.user.set_screenchars(80);
        sess.user.set_screenlines(25);
    }
    io.screenlinest=io.defscreenbottom+1;
    d=(1.0+timer()) / 102.723;
    d-=floor(d);
    d*=10000.0;
    srand((unsigned int)d);
    wfcs();
    if (sys.tcp_port > 0 && ok_modem_stuff)
        initport(0);
    topit();
    strcpy(io.curspeed,"KB");
    do {
        sys.wfc=1;
        wfct();
        if(hold) {
            gotoxy(60,24);
            textcolor(8);
            cprintf("  ");
            textcolor(12);
            cprintf("Phone Off Hook");
            textcolor(8);
            cprintf(" c");
        }
        check_event();
        if (sys.do_event) {
            run_event();
            wfcs();
        }
        lokb=0;
        sess.okskey=0;
        ch=toupper(inkey());
        if (ch) {
            _setcursortype(2);
            switch(ch) {
            case   1: 
                input(s,3); 
                sys.status.activetoday=atoi(s); 
                break;
            case ' ':
                if(!ok_local()) {
                    clrscr();
                    if(!checkpw()) break;
                }
                topit();
                movecsr(38,1);
                outs("Log on? ");
                d=timer();
                while ((!kbhitb()) && (fabs(timer()-d)<60.0));
                if (kbhitb()) {
                    ch=toupper(getchd1());
                    if (ch=='Y') {
                        outs("Yes\r\n");
                        lokb=1;
                        sess.com_speed=sess.modem_speed=sys.cfg.baudrate[sys.cfg.primaryport];
                        if ((sys.cfg.sysconfig & sysconfig_off_hook)==0)
                            dtr(0);
                    }
                    if ((ch=='F') && (ok_local())) {
                        outs("Fast\r\n");
                        { auto __p = UserDB::instance().get(1); if (__p) sess.user = *__p; };
                        reset_act_sl();
                        sess.com_speed=sess.modem_speed=sys.cfg.baudrate[sys.cfg.primaryport];
                        if (sess.user.inact() & inact_deleted) {
                            out1ch(12);
                            break;
                        }
                        lokb=2;
                        if ((sys.cfg.sysconfig & sysconfig_off_hook)==0)
                            dtr(0);
                    }
                    if (ch==0)
                        getchd1();
                }
                topit();
                break;
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            case '0':
                sprintf(s,"wfcbat%c.bat",ch);
                runprog(s,1);
                wfcs();
                break;
            case '(':
            case '*':
            case ')':
            case 'O':
            case 'X':
            case '@':
            case 'S':
            case 'E':
            case '#':
            case 'F':
            case 'B':
            case 'V':
            case 'M':
            case 'C':
            case 'U':
                sess.okskey=1;
                if (ok_local()) {
                    outchr(12);
                    if(ch=='B') boardedit();
                    else if(ch=='F') diredit();
                    else if(ch=='X') protedit();
                    else if(ch=='E') text_edit();
                    /*                    else if(ch=='(') mailsys('S');
                                        else if(ch=='*') mailsys('P');
                                        else if(ch==')') mailsys('T');*/
                        //            else if(ch=='V') ivotes();
                    else if(ch=='#') menu("");
                    else if(ch=='M') readmailj(0,0);
                    else if(ch=='U') uedit(1);
                    else if(ch=='S') edstring(0);
                    else if(ch=='C') confedit();
                    else if(ch=='O') config();
                }
                sess.okskey=0;
                wfcs();
                break;
            case 'D':
                if (ok_local()) {
                    clrscrb();
                    nl();
                    pl("Type \"EXIT\" to return to the BBS");
                    nl();
                    runprog(getenv("COMSPEC"),1);
                    wfcs();
                }
                break;
            case 'H': 
                hold=opp(hold); 
                wfcs(); 
                break;
            case 'L': 
                if(!ok_local()) break; 
                clrscr(); 
                viewlog(); 
                wfcs(); 
                break;
            case 'Q': 
                if(!ok_local()) checkpw();
                end_bbs(sys.oklevel); 
                break;
            case 'R':
                if (ok_local()) {
                    clrscrb();
                    sess.usernum=1;
                    if (sess.user.waiting()) {
                            sess.okskey=1;
                        readmailj(0,0);
                        sess.okskey=0;
                        UserDB::instance().store(1, sess.user);
                        }
                }
                wfcs();
                break;
            case 'T':
                if ((ok_local())) {
                    runprog("term.bat",1);
                }
                wfcs();
                break;
            case 'W':
                if (ok_local()) {
                    clrscrb();
                    sess.usernum=1;
                    sess.useron=1;
                    sess.okskey=1;
                    post(sess.cursub=0);
                    sess.okskey=0;
                    sess.useron=0;
                    UserDB::instance().store(1, sess.user);
                }
                wfcs();
                break;
            case 'Z': 
                clrscr(); 
                zlog(); 
                wfcs();  
                break;
            case '|': 
                if(!ok_local()) break;
                clrscr();
                sess.usernum=1;
                { auto __p = UserDB::instance().get(1); if (__p) sess.user = *__p; };
                changedsl();
                getcmdtype();
                pausescr();
                wfcs();
                break;
            case '=': 
                wfcs(); 
                break;
            case '-': 
                thing+=70.0; 
                break;
            case 13: 
                tt=timer();
                tt-=thing;
                if(tt>60.0) wfcs();
                break;
            }
            if (!incom) {
                _setcursortype(0);
                frequent_init();
                { auto __p = UserDB::instance().get(1); if (__p) sess.user = *__p; };
                sess.fwaiting=numwaiting(sess.user);
                reset_act_sl();
                sess.usernum=1;
            }
            sess.okskey=0;
        }

        /* Check for incoming TCP connection (replaces modem ring detection) */
        if (!hold && ok_modem_stuff && !lokb && sys.listen_fd >= 0) {
            struct timeval tv = {0, 50000}; /* 50ms poll */
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(sys.listen_fd, &fds);
            if (select(sys.listen_fd + 1, &fds, NULL, NULL, &tv) > 0) {
                struct sockaddr_in caddr;
                socklen_t clen = sizeof(caddr);
                int accepted_fd = accept(sys.listen_fd, (struct sockaddr *)&caddr, &clen);
                if (accepted_fd >= 0) {
                    io.stream[IO_REMOTE].fd_in = accepted_fd;
                    io.stream[IO_REMOTE].fd_out = accepted_fd;
                    io.stream[IO_REMOTE].needs_iac = 1;
                    send_telnet_negotiation(accepted_fd);
                    send_terminal_init(accepted_fd);
                    incom = 1;
                    outcom = 1;
                    sess.com_speed = sess.modem_speed = 38400;
                    strcpy(io.curspeed, "TCP/IP");
                }
            }
        } else if (!hold && !ok_modem_stuff) {
            /* No modem/TCP — just poll briefly so we don't spin CPU */
            usleep(50000);
        }
    }
    while ((!incom) && (!lokb) && (!sys.endday));

    using_modem=incom;
    if (lokb==2)
        using_modem=-1;
    sess.okskey=1;
    if (!sys.endday) {
        clrscr();
        cprintf("Connection Established at: %s\r\n",io.curspeed);
    }
    sys.wfc=0;
    return 0;
}

void gotcaller(unsigned int ms, unsigned int cs)
{
    auto& sess = Session::instance();
    char s[MAX_PATH_LEN];
    double d;

    frequent_init();
    sess.com_speed = cs;
    sess.modem_speed = ms;
    sl1(1,"");
    if(sess.already_on==1) {
        incom=1;
        outcom=1;
        send_terminal_init(client_fd);
    }
    { auto __p = UserDB::instance().get(1); if (__p) sess.user = *__p; };
    reset_act_sl();
    sess.usernum=1;
    if (sess.user.inact() & inact_deleted) {
        sess.user.set_screenchars(80);
        sess.user.set_screenlines(25);
    }
    io.screenlinest=25;
    clrscrb();
    sprintf(s,"Connection Established at: %s\r\n",io.curspeed);
    outs(s);
    if(sess.already_on==2)
        using_modem=-1;
    else
        using_modem=1;
    d=(timer()) / 102.723;
    d-=floor(d);
    d*=10000.0;
    srand((unsigned int)d);
    _setcursortype(2);
}


void topit(void)
{
    int i;

    gotoxy(39,2);
    for(i=39;i<77;i++) cprintf(" ");
}


void topit2(void)
{
    int i;

    gotoxy(39,3);
    for(i=39;i<77;i++) cprintf(" ");
}


char *curt(void)
{
    struct time t;
    static char s[MAX_PATH_LEN];
    char an[3];

    gettime(&t);
    if(t.ti_hour>11) strcpy(an,"pm");
    else strcpy(an,"am");
    if(t.ti_hour==0) {
        t.ti_hour=12;
        strcpy(an,"pm");
    }
    if(t.ti_hour>12) t.ti_hour-=12;

    sprintf(s,"%02d:%02d:%02d%s",t.ti_hour,t.ti_min,t.ti_sec,an);
    return(s);
}

int timex=6,timey=17,timeattr=15;

void wfct(void)
{
    double tt;

    tt=timer();
    tt-=thing;
    if(tt<60.0) {
        textattr(timeattr);
        gotoxy(timex,timey);
        cprintf(curt());
    }
    else clrscr();
}

void wfcs(void)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    char s[MAX_PATH_LEN],*p;
    FILE *f;
    long l;
    int i,x,y,attr,type;
    io.echo=1;
    clrscr();
    thing=timer();

    _setcursortype(0);
    fastscreen("wfc.bin");

    sprintf(s,"%swfc.dat",sys.cfg.gfilesdir);
    f=fopen(s,"rt");

    while(f && fgets(s,81,f)!=NULL) {

        filter(s,'\n');
        p=strtok(s,",");
        x=atoi(p);
        p=strtok(NULL,",");
        y=atoi(p);
        p=strtok(NULL,",");
        attr=atoi(p);
        p=strtok(NULL,",");
        type=atoi(p);

        gotoxy(x,y);
        textattr(attr);

        switch(type) {
        case 1:
            cprintf(wwiv_version);
            break;
        case 2:
            cprintf("%d",sys.status.callstoday);
            break;
        case 3:
            cprintf("%d%%",(10*sys.status.activetoday/144));
            break;
        case 4:
            cprintf("%d",sys.status.msgposttoday);
            break;
        case 5:
            cprintf("%svailable",sysop2()?"A":"Una");
            break;
        case 6:
            cprintf("%s",sys.status.lastuser);
            break;
        case 7:
            cprintf("%d",sys.status.uptoday);
            break;
        case 8:
            cprintf("%d",sys.status.dltoday);
            break;
        case 9:
            l=(long) freek(_getdrive());
            cprintf("%ldk",l);
            break;
        case 10:
            cprintf("%d",sess.fwaiting);
            break;
        case 11:
            bargraph(10*sys.status.activetoday/144);
            break;
        case 12:
            timex=x;
            timey=y;
            timeattr=attr;
            break;
        }
    }
    if(f) fclose(f);
}

int ok_local()
{
    auto& sys = System::instance();
    if (sys.cfg.sysconfig& sysconfig_no_local)
        return(0);
    else
        return(1);
}

void bargraph(int percent)
{
    int x;
    textattr(15);
    for(x=0;x<50;x++)
        cprintf("%c",177);
    for(x=50;x>0;x--)
        cprintf("\b");
    for(x=0;x<percent/2;x++)
        cprintf("%c",219);
}


/*
 * wfc_poll_accept() — supervisor-mode accept loop.
 *
 * Polls the listen socket with select() (1-second timeout).
 * Returns accepted_fd on connection, -1 if nothing to accept
 * or if the supervisor should shut down (endday, Q key).
 */
int wfc_poll_accept(void)
{
    auto& sys = System::instance();

    if (sys.listen_fd < 0)
        return -1;

    struct timeval tv = {1, 0}; /* 1-second poll */
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(sys.listen_fd, &fds);
    int r = select(sys.listen_fd + 1, &fds, NULL, NULL, &tv);

    if (r > 0 && FD_ISSET(sys.listen_fd, &fds)) {
        struct sockaddr_in caddr;
        socklen_t clen = sizeof(caddr);
        int accepted_fd = accept(sys.listen_fd, (struct sockaddr *)&caddr, &clen);
        return accepted_fd;
    }

    return -1;
}

