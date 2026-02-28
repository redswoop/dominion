#include "utility.h"
#include "platform.h"
#include "bbs_output.h"
#include "bbs_input.h"
#include "bbs_ui.h"
#include "conio.h"
#include "disk.h"
#include "user/userdb.h"
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

void Session::reset_act_sl()
{
#ifdef BACK
    if(backdoor)
        actsl=255;
    else actsl=user.sl();
#else
    actsl = user.sl();
#endif
}


int Session::okansi()
{
    return(io.caps.color != CAP_OFF);
}


void Session::frequent_init()
{
    auto& sys = System::instance();
    doinghelp=0;
    io.mciok=1;
    msgr=1;
    sys.ARC_NUMBER=-1;
    chatsoundon=1;
    curlsub=-1;
    stream_reset();
    io.curatr=0x07;
    outcom=0;
    incom=0;
    io.charbufferpointer=0;
    checkit=0;
    io.topline=0;
    io.screenlinest=io.defscreenbottom+1;
    io.endofline[0]=0;
    io.hangup=0;
    io.chatcall=0;
    chatreason[0]=0;
    useron=0;
    io.chatting=0;
    io.echo=1;
    okskey=0;
    io.lines_listed=0;
    okmacro=1;
    okskey=1;
    mailcheck=0;
    smwcheck=0;
    in_extern=0;
    use_workspace=0;
    extratimecall=0.0;
    using_modem=0;
    set_global_handle(0);
    live_user=1;
    _chmod(dszlog,1,0);
    unlink(dszlog);
    ltime=0;
    backdoor=0;
    sys.status.net_edit_stuff=topdata;
}


double Session::ratio()
{
    double r;

    if (user.dk()==0)
        return(99.999);
    r=((float) user.uk()) / ((float) user.dk());
    if (r>99.998)
        r=99.998;
    return(r);
}


double Session::post_ratio()
{
    double r;

    if (user.logons()==0)
        return(99.999);
    r=((float) user.msgpost()) / ((float) user.logons());
    if (r>99.998)
        r=99.998;
    return(r);
}


/* Free-function wrappers for backward compat — delegate to Session methods */
void reset_act_sl()   { Session::instance().reset_act_sl(); }
int okansi()          { return Session::instance().okansi(); }
void frequent_init()  { Session::instance().frequent_init(); }
double ratio()        { return Session::instance().ratio(); }
double post_ratio()   { return Session::instance().post_ratio(); }
void changedsl()      { Session::instance().changedsl(); }


void Session::changedsl()
{
    auto& sys = System::instance();
    int i,i1,i2,i3,i4,ok;
    subboardrec s;
    directoryrec d;
    usersubrec s1;

    topscreen();
    umaxsubs=0;
    umaxdirs=0;
    for (i=0; i<3; i++) {
        s1.keys[i]=0;
    }
    s1.subnum=-1;
    for (i=0; i<MAX_SUBS; i++)
        usub[i]=s1;
    for (i=0; i<MAX_DIRS; i++)
        udir[i]=s1;
    i1=1;
    i2=0;
    i3=0;
    if(confmode)
    if(!slok(sys.conf[curconf].sl,0))
        jumpconf("");
    for (i=0; i<sys.num_subs; i++) {
        ok=1;
        s=sys.subboards[i];
        if (s.attr & mattr_deleted) ok=0;
        else {
            if (!slok((char *)s.readacs,0)) ok=0;
            if (user.age()<s.age) ok=0;
            if (s.ar) if(!(user.ar() & s.ar)) ok=0;
            if ((s.attr & mattr_ansi_only) && (!okansi())) ok=0;
            if(confmode)
                if (!strchr(sys.conf[curconf].flagstr,s.conf)&&s.conf!='@'&&!strchr(sys.conf[curconf].flagstr,'@')) ok=0;
        }
        if (ok) {
            s1.subnum=i;
            itoa(i1++,s1.keys,10);
            s1.subnum=i;
            for (i4=i3; i4>i2; i4--)
                usub[i4]=usub[i4-1];
            i3++;
            usub[i2++]=s1;
            umaxsubs++;
        }
    }
    i1=1;
    i2=0;
    for (i=0; i<sys.num_dirs; i++) {
        ok=1;
        d=sys.directories[i];
        if (!slok(d.acs,1)) ok=0;
        if (d.dar) if ((d.dar & user.dar())==0) ok=0;
        if(!strchr(sys.conf[curconf].flagstr,d.confnum)&&d.confnum!='@'&&!strchr(sys.conf[curconf].flagstr,'@')) ok=0;
        if (ok) {
            s1.subnum=i;
            if (i==0)
                strcpy(s1.keys,"0");
            else {
                itoa(i1++,s1.keys,10);
            }
            udir[i2++]=s1;
            umaxdirs++;
        }
    }
}
