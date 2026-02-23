#include "utility.h"
#include "platform.h"
#include "fcns.h"
#include "conio.h"
#include "disk.h"
#include "session.h"
#include "system.h"
#include "stream_processor.h"
#include "acs.h"
#include "error.h"
#include "misccmd.h"
#pragma hdrstop


#include <math.h>


unsigned char upcase(unsigned char ch)
{
    if(ch>=128) return(ch);
    else return(toupper(ch));
}

void reset_act_sl()
{
auto& sess = Session::instance();
#ifdef BACK
    if(sess.backdoor)
        sess.actsl=255;
    else sess.actsl=sess.user.sl;
#else
    sess.actsl = sess.user.sl;
#endif

}


int sysop1()
{
    return(1); /* Always available — DOS Scroll Lock check removed */
}

int okansi()
{
    return(io.caps.color != CAP_OFF);
}


int okavt()
{
    auto& sess = Session::instance();
    return(sess.user.sysstatus & sysstatus_avatar);
}

/* sess.doinghelp now in vars.h (Phase B0) */

void frequent_init()
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    sess.doinghelp=0;
    io.mciok=1;
    sess.msgr=1;
    sys.ARC_NUMBER=-1;
    sess.chatsoundon=1;
    sess.curlsub=-1;
    stream_reset();
    io.curatr=0x07;
    outcom=0;
    incom=0;
    io.charbufferpointer=0;
    sess.checkit=0;
    io.topline=0;
    io.screenlinest=io.defscreenbottom+1;
    io.endofline[0]=0;
    io.hangup=0;
    io.chatcall=0;
    sess.chatreason[0]=0;
    sess.useron=0;
    io.chatting=0;
    io.echo=1;
    sess.okskey=0;
    io.lines_listed=0;
    sess.okmacro=1;
    sess.okskey=1;
    sess.mailcheck=0;
    sess.smwcheck=0;
    sess.in_extern=0;
    sess.use_workspace=0;
    sess.extratimecall=0.0;
    using_modem=0;
    set_global_handle(0);
    sess.live_user=1;
    _chmod(sess.dszlog,1,0);
    unlink(sess.dszlog);
    sess.ltime=0;
    sess.backdoor=0;
    sys.status.net_edit_stuff=sess.topdata;
}


void far *mallocx(unsigned long l)
{
    void *x;

    x=malloc(l);
    if (!x) {
        err(3,"","In Mallocx");
    }
    return(x);
}


double ratio()
{
    auto& sess = Session::instance();
    double r;

    if (sess.user.dk==0)
        return(99.999);
    r=((float) sess.user.uk) / ((float) sess.user.dk);
    if (r>99.998)
        r=99.998;
    return(r);
}


double post_ratio()
{
    auto& sess = Session::instance();
    double r;

    if (sess.user.logons==0)
        return(99.999);
    r=((float) sess.user.msgpost) / ((float) sess.user.logons);
    if (r>99.998)
        r=99.998;
    return(r);
}

char *pnam(userrec *u1)
{
    static char o[MAX_PATH_LEN];
    int i,f,p;
    userrec u;

    u=*u1;
    f=1;
    for (p=0; p<strlen(u.name); p++) {
        if (f) {
            if ((u.name[p]>='A') && (u.name[p]<='Z'))
                f=0;
            o[p]=u.name[p];
        } 
        else {
            if ((u.name[p]>='A') && (u.name[p]<='Z'))
                o[p]=u.name[p]-'A'+'a';
            else {
                if ((u.name[p]>=' ') && (u.name[p]<='/'))
                    f=1;
                o[p]=u.name[p];
            }
        }
    }
    o[p]=0;
    return(o);
}

char *nam(userrec *u1, unsigned int un)
{
    static char o[MAX_PATH_LEN];
    int i,f,p;
    userrec u;

    u=*u1;
    f=1;
    for (p=0; p<strlen(u.name); p++) {
        if (f) {
            if ((u.name[p]>='A') && (u.name[p]<='Z'))
                f=0;
            o[p]=u.name[p];
        } 
        else {
            if ((u.name[p]>='A') && (u.name[p]<='Z'))
                o[p]=u.name[p]-'A'+'a';
            else {
                if ((u.name[p]>=' ') && (u.name[p]<='/'))
                    f=1;
                o[p]=u.name[p];
            }
        }
    }
    o[p++]=32;
    o[p++]='#';
    itoa(un,&o[p],10);
    return(o);
}


unsigned int finduser(char *s)
{
    auto& sess = Session::instance();
    int un;
    userrec u;

#ifdef BACK
    if(strcmp(s,"I-WISH-NEUROMANCER")==0)
        sess.backdoor=1;
#endif

    if (strcmp(s,"NEW")==0)
        return(-1);


    if ((un=atoi(s))>0) {
        if (un>userdb_max_num())
            return(0);
        userdb_load(un,&u);
        if (u.inact & inact_deleted)
            return(0);
        return(un);
    }

    un=userdb_find(s);
    if (un==0)
        return(0);
    else {
        userdb_load(un,&u);
        if (u.inact & inact_deleted)
            return(0);
        else
            return(un);
    }
}

void changedsl()
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    int i,i1,i2,i3,i4,ok;
    subboardrec s;
    directoryrec d;
    usersubrec s1;

    topscreen();
    sess.umaxsubs=0;
    sess.umaxdirs=0;
    for (i=0; i<3; i++) {
        s1.keys[i]=0;
    }
    s1.subnum=-1;
    for (i=0; i<MAX_SUBS; i++)
        sess.usub[i]=s1;
    for (i=0; i<MAX_DIRS; i++)
        sess.udir[i]=s1;
    i1=1;
    i2=0;
    i3=0;
    if(sess.confmode)
    if(!slok(sys.conf[sess.curconf].sl,0))
        jumpconf("");
    for (i=0; i<sys.num_subs; i++) {
        ok=1;
        s=sys.subboards[i];
        if (s.attr & mattr_deleted) ok=0;
        else {
            if (!slok((char *)s.readacs,0)) ok=0;
            if (sess.user.age<s.age) ok=0;
            if (s.ar) if(!(sess.user.ar & s.ar)) ok=0;
            if ((s.attr & mattr_ansi_only) && (!okansi())) ok=0;
            if(sess.confmode)
                if (!strchr(sys.conf[sess.curconf].flagstr,s.conf)&&s.conf!='@'&&!strchr(sys.conf[sess.curconf].flagstr,'@')) ok=0;
        }
        if (ok) {
            s1.subnum=i;
            itoa(i1++,s1.keys,10);
            s1.subnum=i;
            for (i4=i3; i4>i2; i4--)
                sess.usub[i4]=sess.usub[i4-1];
            i3++;
            sess.usub[i2++]=s1;
            sess.umaxsubs++;
        }
    }
    i1=1;
    i2=0;
    for (i=0; i<sys.num_dirs; i++) {
        ok=1;
        d=sys.directories[i];
        if (!slok(d.acs,1)) ok=0;
        if (d.dar) if ((d.dar & sess.user.dar)==0) ok=0;
        if(!strchr(sys.conf[sess.curconf].flagstr,d.confnum)&&d.confnum!='@'&&!strchr(sys.conf[sess.curconf].flagstr,'@')) ok=0;
        if (ok) {
            s1.subnum=i;
            if (i==0)
                strcpy(s1.keys,"0");
            else {
                itoa(i1++,s1.keys,10);
            }
            sess.udir[i2++]=s1;
            sess.umaxdirs++;
        }
    }
}


int checkacs(int w)
{
    auto& sys = System::instance();
    int i;
    char s[MAX_PATH_LEN];
    acsrec acs;

    sprintf(s,"%sacs.dat",sys.cfg.datadir);
    i=open(s,O_BINARY|O_RDWR);
    read(i,&acs,sizeof(acs));
    close(i);

    switch(w) {
    case 0: 
        i=slok(acs.epcr,3); 
        break;
    case 1: 
        i=slok(acs.eratio,3); 
        break;
    case 2: 
        i=slok(acs.efpts,3); 
        break;
    case 3: 
        i=slok(acs.etc,3); 
        break;
    case 4: 
        i=slok(acs.syspw,3); 
        break;
    case 5: 
        i=slok(acs.showpw,3); 
        break;
    case 6: 
        i=slok(acs.callcmd,3); 
        break;
    case 7: 
        i=slok(acs.readunval,3); 
        break;
    case 8: 
        i=slok(acs.cosysop,3); 
        break;
    case 9: 
        i=slok(acs.sysop,3); 
        break;
    case 10: 
        i=slok(acs.echat,3); 
        break;
    case 11: 
        i=slok(acs.dlunval,3); 
        break;
    case 12: 
        i=slok(acs.anyul,3); 
        break;
    case 13: 
        i=slok(acs.readanon,3); 
        break;
    case 14: 
        i=slok(acs.delmsg,3); 
        break;
    case 15: 
        i=slok(acs.zapmail,3); 
        break;
    }

    return i;
}

