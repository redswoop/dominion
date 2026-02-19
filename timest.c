#include "vars.h"
#pragma hdrstop


#include <dir.h>
#include <math.h>
#include <share.h>

void check_event()
{
    double tl;

    if (syscfg.executetime) {
        tl=time_event-timer();
        if (tl<0.0)
            tl += 24.0*3600.0;
        if ((tl-last_time)>2.0)
            do_event=1;
        last_time=tl;
    }
}


void run_event()
{
    if ((do_event) && (syscfg.executetime)) {
        do_event=0;
        nl();
        pl("Now running external event.");
        nl();
        if (syscfg.executestr[0]) {
            holdphone(1,0);
            runprog(syscfg.executestr,1);
            /* run_external(syscfg.executestr); */
            holdphone(0,0);
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
/* This function initializes the high-resolution timer */
{
    outportb(0x43,0x34);
    outportb(0x40,0x00);
    outportb(0x40,0x00);

}

double timer()
/* This function returns the time, in seconds since midnight. */
{
    double cputim;
    unsigned short int h,m,l1,l2;

    disable();
    outportb(0x43,0x00);
    m=peek(0x0040,0x006c);
    h=peek(0x0040,0x006e);
    l1=inportb(0x40);
    l2=inportb(0x40);
    enable();
    l1=((l2*256)+l1) ^ 65535;
    cputim=((h*65536. + m)*65536. + l1)*8.380955e-7;
    return (cputim);
}


long timer1()
/* This function returns the time, in ticks since midnight. */
{
    unsigned short h,m;
    long l;

    m=peek(0x0040,0x006c);
    h=peek(0x0040,0x006e);
    l=((long)h)*65536 + ((long)m);
    return(l);
}

double nsl()
{
    double tlt,tlc,tot,tpl,tpd,dd,rtn;
    slrec xx;

    dd=timer();
    if (useron) {
        if (timeon>(dd+60.0))
            timeon -= 24.0*3600.0;
        tot=(dd-timeon);
        xx=syscfg.sl[actsl];
        tpl=((double) xx.time_per_logon) * 60.0;
        tpd=((double) xx.time_per_day) * 60.0;
        tlc = tpl - tot + (thisuser.extratime) + extratimecall;
        tlt = tpd - tot - ((double) thisuser.timeontoday) + (thisuser.extratime);

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
    ltime=0;
    if (syscfg.executetime) {
        tlt=time_event-dd;
        if (tlt<0.0)
            tlt += 24.0*3600.0;
        if (rtn>tlt) {
            rtn=tlt;
            ltime=1;
        }
        check_event();
        if (do_event)
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
    sprintf(s,"%02d/%02d/%02d",today.da_mon,today.da_day,today.da_year-1900);
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
    char xl[81], cl[81], atr[81], cc, s[81];
    long l;
    int mcir=mciok;

    mciok=1;

    savel(cl, atr, xl, &cc);

    ansic(0);
    nl();
    nl();
    time(&l);
    strcpy(s, ctime(&l));
    s[strlen(s) - 1] = 0;
    npr("3It is 0%s3.\r\n",s);
    if (useron)
        npr("3You have been on for 0%s3, and have 0`T3 left\r\n",ctim(timer()-timeon));
    nl();

    restorel(cl, atr, xl, &cc);
    mciok=mcir;
}

char *curtime(void)
{
    struct time t;
    struct date d;
    static char s[81],an[3];

    getdate(&d);
    gettime(&t);
    if(t.ti_hour>11) strcpy(an,"pm");
    else strcpy(an,"am");
    if(t.ti_hour==0) {
        t.ti_hour=12;
        strcpy(an,"pm");
    }
    if(t.ti_hour>12) t.ti_hour-=12;

    sprintf(s,"%02d:%02d:%02d%s  %02d/%02d/%02d",t.ti_hour,t.ti_min,t.ti_sec,an,d.da_mon,d.da_day,d.da_year-1900);
    return(s);
}



