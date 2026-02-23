#include "platform.h"
#include "fcns.h"
#include "session.h"
#include "system.h"
#pragma hdrstop


#include <math.h>


static auto& sys = System::instance();
static auto& sess = Session::instance();

void check_event()
{
    double tl;

    if (sys.cfg.executetime) {
        tl=sys.time_event-timer();
        if (tl<0.0)
            tl += 24.0*3600.0;
        if ((tl-sys.last_time)>2.0)
            sys.do_event=1;
        sys.last_time=tl;
    }
}


void run_event()
{
    if ((sys.do_event) && (sys.cfg.executetime)) {
        sys.do_event=0;
        nl();
        pl("Now running external event.");
        nl();
        if (sys.cfg.executestr[0]) {
            runprog(sys.cfg.executestr,1);
        } 
        else
            end_bbs(10);
    }
    clrscrb();
}


unsigned char years_old(unsigned char m, unsigned char d, unsigned char y)
{
    struct date today;
    int a;

    getdate(&today);
    a=(int) (today.da_year-1900-y);
    if (today.da_mon<m)
        --a;
    else
        if ((today.da_mon==m) && (today.da_day<d))
            --a;
    return((unsigned char)a);
}


void itimer()
/* This function initializes the high-resolution timer.
 * On macOS, nothing to initialize — we use gettimeofday(). */
{
}

double timer()
/* This function returns the time, in seconds since midnight. */
{
    struct timeval tv;
    struct tm *tm;

    gettimeofday(&tv, NULL);
    tm = localtime(&tv.tv_sec);
    return (double)tm->tm_hour * 3600.0
         + (double)tm->tm_min  * 60.0
         + (double)tm->tm_sec
         + (double)tv.tv_usec / 1000000.0;
}


long timer1()
/* This function returns the time, in ticks since midnight.
 * Original: reads BIOS tick counter at 18.2 Hz.
 * macOS: compute from gettimeofday(). */
{
    return (long)(timer() * 18.2);
}

double nsl()
{
    double tlt,tlc,tot,tpl,tpd,dd,rtn;
    slrec xx;

    dd=timer();
    if (sess.useron) {
        if (sess.timeon>(dd+60.0))
            sess.timeon -= 24.0*3600.0;
        tot=(dd-sess.timeon);
        xx=sys.cfg.sl[sess.actsl];
        tpl=((double) xx.time_per_logon) * 60.0;
        tpd=((double) xx.time_per_day) * 60.0;
        tlc = tpl - tot + (sess.user.extratime) + sess.extratimecall;
        tlt = tpd - tot - ((double) sess.user.timeontoday) + (sess.user.extratime);

        tlt=(((tlc)<(tlt)) ? (tlc) : (tlt));
        if (tlt<0.0)
            tlt=0.0;
        if (tlt>32767.0)
            tlt=32767.0;
        rtn=tlt;
    } 
    else {
        rtn=1.00;
    }
    sess.ltime=0;
    if (sys.cfg.executetime) {
        tlt=sys.time_event-dd;
        if (tlt<0.0)
            tlt += 24.0*3600.0;
        if (rtn>tlt) {
            rtn=tlt;
            sess.ltime=1;
        }
        check_event();
        if (sys.do_event)
            rtn=0.0;
    }
    if (rtn<0.0)
        rtn=0.0;
    if (rtn>32767.0)
        rtn=32767.0;
    return(rtn);
}


char *date()
{
    static char s[9];
    struct date today;

    getdate(&today);
    sprintf(s,"%02d/%02d/%02d",today.da_mon,today.da_day,(today.da_year-1900)%100);
    return(s);
}

char *times()
{
    static char ti[9];
    int h,m,s;
    double t;

    t=timer();
    h=(int) (t/3600.0);
    t-=((double) (h)) * 3600.0;
    m=(int) (t/60.0);
    t-=((double) (m)) * 60.0;
    s=(int) (t);
    sprintf(ti,"%02d:%02d:%02d",h,m,s);
    return(ti);
}

void wait(double d)
{
    long l1;

    l1=((long) (18.2*d));
    l1 += timer1();

    enable();
    while (timer1()<l1)
        ;
}

void wait1(long l)
{
    long l1;

    l1 = timer1()+l;

    enable();
    while (timer1()<l1)
        ;
}

char *ctim(double d)
{
    static char ch[10];
    long h,m,s;

    if (d<0)
        d += 24.0*3600.0;
    h=(long) (d/3600.0);
    d-=(double) (h*3600);
    m=(long) (d/60.0);
    d-=(double) (m*60);
    s=(long) (d);
    sprintf(ch,"%02.2ld:%02.2ld:%02.2ld",h,m,s);

    return(ch);
}


void ptime()
{
    char xl[MAX_PATH_LEN], cl[MAX_PATH_LEN], atr[MAX_PATH_LEN], cc, s[MAX_PATH_LEN];
    long l;
    int mcir=io.mciok;

    io.mciok=1;

    savel(cl, atr, xl, &cc);

    ansic(0);
    nl();
    nl();
    time(&l);
    strcpy(s, ctime(&l));
    s[strlen(s) - 1] = 0;
    npr("3It is 0%s3.\r\n",s);
    if (sess.useron)
        npr("3You have been on for 0%s3, and have 0`T3 left\r\n",ctim(timer()-sess.timeon));
    nl();

    restorel(cl, atr, xl, &cc);
    io.mciok=mcir;
}

char *curtime(void)
{
    struct time t;
    struct date d;
    static char s[MAX_PATH_LEN],an[3];

    getdate(&d);
    gettime(&t);
    if(t.ti_hour>11) strcpy(an,"pm");
    else strcpy(an,"am");
    if(t.ti_hour==0) {
        t.ti_hour=12;
        strcpy(an,"pm");
    }
    if(t.ti_hour>12) t.ti_hour-=12;

    sprintf(s,"%02d:%02d:%02d%s  %02d/%02d/%02d",t.ti_hour,t.ti_min,t.ti_sec,an,d.da_mon,d.da_day,(d.da_year-1900)%100);
    return(s);
}



