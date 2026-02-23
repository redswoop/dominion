/* misccmd.cpp — Miscellaneous user commands. */

#include "misccmd.h"
#include "platform.h"
#include "fcns.h"
#include "mm1.h"
#include "session.h"
#include "system.h"
#include "version.h"

#include "acs.h"
#include "sysopf.h"
#include "subedit.h"

#pragma hdrstop


void list_users()
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    subboardrec s;
    userrec u;
    int i,nu,abort,ok,num;
    char st[161];

    if (sess.usub[sess.cursub].subnum==-1||(sess.user.restrict & restrict_userlist)) {
        nl();
        pl("Sorry, you cannot currently view the user list");
        nl();
        return;
    }
    s=sys.subboards[sess.usub[sess.cursub].subnum];
    nl();
    pl("Users with access to current sub:");
    nl();
    npr("0%-30s 1[0%-40s1]\r\n","Users Name","Comment");
    abort=0;
    num=0;
    for (i=0; (i<userdb_user_count()) && (!abort) && (!io.hangup); i++) {
        smalrec sr;
        userdb_get_entry(i, &sr);
        userdb_load(sr.number,&u);
        ok=1;
        if (u.age<s.age)
            ok=0;
        if ((s.ar!=0) && ((u.ar & s.ar)==0))
            ok=0;
        if(u.inact & inact_lockedout)
            ok=0;
        if(u.exempt & exempt_userlist)
            ok=0;
        if (ok) {
            npr("3%-30s 1[0%-40s1]\r\n",nam(&u,sr.number),u.comment);
            ++num;
        }
    }
    if (!abort) {
        nl();
        npr("%d users.\r\n",num);
        nl();
    }
}


void yourinfo()
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    char s[MAX_PATH_LEN];
    originrec orig;

    outchr(12);
    dtitle("Your Information");

    npr("0Your Handle is 5%s0, Your Voice Phone Number is 5%s0\r\n",nam(&sess.user,sess.usernum),sess.user.phone);
    npr("Your Acting Security Level is 5%d0, And Your Transfer Level is 5%d0\r\n",sess.actsl,sess.user.dsl);
    nl();
    npr("You have Downloaded 5%d0 files, And Uploaded 5%d0 files\r\n",sess.user.downloaded,sess.user.uploaded);
    npr("You have posted 5%d0 times, and Called 5%d0 times\r\n",sess.user.msgpost,sess.user.logons);
    nl();
    npr("You have 5%d0 minutes left for this call\r\n",(int)((nsl()+30)/60.0));
    sprintf(s,"0, And have been on 5%d0 times today.",sess.user.ontoday);
    npr("You last called 5%s%s0\r\n",sess.user.laston,sess.user.ontoday?s:"");
    nl();
    npr("System is 5%s\r\n",wwiv_version);
    getorigin(sys.subboards[sess.usub[sess.cursub].subnum].origin,&orig);
    if(orig.add.zone)
        npr("Network is %s, Address %d:%d/%d\r\n",orig.netname,orig.add.zone,orig.add.net,orig.add.node);
    nl();
    if(sess.user.helplevel==2)
        pausescr();
}


void jumpconf(char ms[41])
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    char c,s[MAX_PATH_LEN];
    int i,ok,type=0;

    if(atoi(ms)) {
        if(slok(sys.conf[atoi(ms)].sl,0)) {
            sess.curconf=atoi(ms);
            changedsl();
            return;
        }
    }
    else if(ms[0]) type=ms[0];

    dtitle("Conferences Available: ");
    for(c=0;c<sys.num_conf;c++) {
        ok=1;
        if(!slok(sys.conf[c].sl,0)) ok=0;
        if(type=='M'||type=='F')
            if(sys.conf[c].type!=type&&sys.conf[c].type)
                ok=0;
        if(ok)
            npr("0<%d> %s\r\n",c+1,sys.conf[c].name);
    }
    nl();
    outstr("Select: ");
    mpl(2);
    input(s,2);
    i=atoi(s);
    if(i==0) return;
    i--;
    if(i>sys.num_conf) return;
    if(slok(sys.conf[i].sl,0)) sess.curconf=i;
    else {
        pl("Unavailable Conference");
        sess.curconf=0;
        return;
    }
    changedsl();
}


char *get_date_filename()
{
    auto& sys = System::instance();
    struct date today;
    char *dats[]={
        "jan","fab","mar","apr","may","jun","jul","aug","sep","oct","nov","dec"        };
    static char s[MAX_PATH_LEN];

    getdate(&today);

    sprintf(s,"%stoday.%s",sys.cfg.gfilesdir,dats[today.da_mon-1]);
    return(s);
}

void today_history()
{

    char s[181],s1[10],s2[MAX_PATH_LEN],*p;
    FILE *f;
    FILE *fbak;
    struct date today;
    int ok=0,day,h;
    char dfname[MAX_PATH_LEN];

    getdate(&today);
    strcpy(dfname,get_date_filename());

    outchr(12);
    npr("[� Famous Happenings for Today (%s) �]\r\n",date());

    /* PRINT BIRTHDAYS FOR CURRENT DAY */
    npr("Birthdays\n\r");
    if ((f=fopen(dfname,"r"))!=NULL) {
        while ((fgets(s,80,f)) != NULL) {
            filter(s,'\n');

            sprintf(s1,"%c%c\n",s[3],s[4]);
            day=atoi(s1);

            if(day==today.da_day) ok=1;
            else ok=0;

            if(s[0]=='B' && ok==1) {
                p=strtok(&s[5]," ");
                npr("3%s ",p);
                p=strtok(NULL,"");
                npr("3%s\r\n",p);
            }
        }
        fclose(f);
    }


    /* PRINT EVENTS FOR CURRENT DAY */
    nl();
    nl();
    npr("Events\n\r");
    if ((f=fopen(dfname,"r"))!=NULL) {
        while ((fgets(s,80,f)) != NULL) {
            filter(s,'\n');

            sprintf(s1,"%c%c\n",s[3],s[4]);
            day=atoi(s1);

            if(day==today.da_day) ok=1;
            else ok=0;

            if(s[0]=='S' && ok==1) {
                if(s[5]=='C') {
                    outstr("    ");
                    strcpy(p,&s[6]);
                }
                else {
                    p=strtok(&s[5]," ");
                    npr("3%s ",p);
                    p=strtok(NULL,"");
                }
                npr("3%s\r\n",p);
            }
        }
        fclose(f);
    }
}

void dtitle(char msg[MAX_PATH_LEN])
{
    auto& sys = System::instance();
    FILE *f;
    char s[MAX_PATH_LEN],b1,b2,s1[MAX_PATH_LEN],bs2[MAX_PATH_LEN],bs1[MAX_PATH_LEN];
    char msg_stripped[MAX_PATH_LEN];
    int i;

    sprintf(s,"%sbox.fmt",sys.cfg.gfilesdir);
    f=fopen(s,"rt");
    if (!f) { pl(msg); return; }
    noc(msg_stripped,msg);
    fgets(s,4,f);
    b1=s[0];
    b2=s[1];
    for(i=0;i<strlenc(msg_stripped);i++) {
        bs1[i]=b1;
        bs2[i]=b2;
    }
    bs1[i]=0;
    bs2[i]=0;
    while(fgets(s,81,f)!=NULL) {
        filter(s,'\n');
        stuff_in(s1,s,msg,bs1,bs2,"","");
        plfmt(s1);
    }
    fclose(f);
}


void selfValidationCheck(char *param)
{
    char *p;
    char s[MAX_PATH_LEN];

    p=strtok(param,",");
    nl();
    inputdat("Enter Self-Validation password",s,31,0);
    if(!s[0])
        return;
    if(stricmp(p,s)==0) {
        p=strtok(NULL,"");
        set_autoval(atoi(p)-1);
        pl("Self-Validation was successful");
        logtypes(2,"User Self-Validated to level %d",atoi(p));
        changedsl();
    }
    else {
        nl();
        pl("7Incorrect Password");
        logtypes(2,"Illegal Self-Validation attempt");
    }
    nl();
}

char *getComputerType(int which)
{
    auto& sys = System::instance();
    static char s[MAX_PATH_LEN];
    char s1[MAX_PATH_LEN];
    FILE *f;
    int i=0;

    sprintf(s1,"%sctype.txt",sys.cfg.gfilesdir);
    f=fopen(s1,"rt");
    if (!f) { strcpy(s, "Unknown"); return(s); }
    while(fgets(s,81,f)!=NULL&&i++<which);
    fclose(f);
    filter(s,'\n');
    return(s);
}

int numComputerTypes(void)
{
    auto& sys = System::instance();
    char s1[MAX_PATH_LEN];
    FILE *f;
    int i=0;

    sprintf(s1,"%sctype.txt",sys.cfg.gfilesdir);
    f=fopen(s1,"rt");
    if (!f) return(0);
    while(fgets(s1,81,f)!=NULL) i++;
    fclose(f);
    return(i);
}
