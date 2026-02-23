#include "vars.h"

#pragma hdrstop

#include <math.h>
#include "swap.h"

void cd_to(char *s)
{
    char s1[MAX_PATH_LEN];
    int i;

    if (!s) return;
    strcpy(s1,s);
    i=strlen(s1)-1;
    if (i>0 && (s1[i]=='\\' || s1[i]=='/'))
        s1[i]=0;
    /* skip DOS drive letter prefix (e.g. "A:\") */
    if (s1[1]==':' && (s1[2]=='\\' || s1[2]=='/'))
        chdir(&s1[2]);
    else if (s1[1]==':')
        chdir(&s1[2]);
    else
        chdir(s1);
}

void get_dir(char *s, int be)
{
    getcwd(s, MAX_PATH_LEN);
    if (be) {
        if (s[strlen(s)-1]!='/')
            strcat(s,"/");
    }
}


int do_it(char cl[MAX_PATH_LEN])
{
    int i,i1,l;
    char s[160],*s1;
    char *ss[30];
    unsigned char ex;

    sl1(1,"");
    strcpy(s,cl);
    ss[0]=s;
    i=1;
    l=strlen(s);
    for (i1=1; i1<l; i1++)
        if (s[i1]==32) {
            s[i1]=0;
            ss[i++]=&(s[i1+1]);
        }
    ss[i]=NULL;
    i=spawnvpe(P_WAIT,ss[0],ss,xenviron);

    return(i);
}


int runprog(char *s, int swp)
{
    int rc;
    char x[161];
    unsigned char f;

    sl1(1,"");
    checkhangup();
    if(swp) {
        sprintf(x,"/C %s",s);
        swap((unsigned char *)getenv("COMSPEC"),(unsigned char *)x,&f,(unsigned char *)"swap.dom");
        rc=f;
    } 
    else {
        sprintf(x,"%s /C %s", getenv("COMSPEC"), s);
        rc=do_it(x);
    }
    initport(syscfg.primaryport);
    inkey();
    inkey();
    cd_to(cdir);
    return(rc);
}


void alf(int f, const char *s)
{
    char s1[100];

    strcpy(s1,s);
    strcat(s1,"\r\n");
    write(f,(void *)s1,strlen(s1));
}


char *create_chain_file(char *fn)
{
    int i,i1,f;
    char s[MAX_PATH_LEN],s1[MAX_PATH_LEN],gd[MAX_PATH_LEN],dd[MAX_PATH_LEN];
    static char fpn[MAX_PATH_LEN];
    long l;

    cd_to(syscfg.gfilesdir);
    get_dir(gd,1);
    cd_to(cdir);
    cd_to(syscfg.datadir);
    get_dir(dd,1);
    cd_to(cdir);

    unlink(fn);
    f=open(fn,O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE);
    itoa(usernum,s,10);
    alf(f,s);
    alf(f,thisuser.name);
    alf(f,thisuser.realname);
    alf(f,thisuser.callsign);
    itoa(thisuser.age,s,10);
    alf(f,s);
    s[0]=thisuser.sex;
    s[1]=0;
    alf(f,s);
    sprintf(s,"%10.2f",thisuser.fpts);
    alf(f,s);
    alf(f,thisuser.laston);
    itoa(thisuser.screenchars,s,10);
    alf(f,s);
    itoa(thisuser.screenlines,s,10);
    alf(f,s);
    itoa(thisuser.sl,s,10);
    alf(f,s);
    if (cs())
        alf(f,"1");
    else
        alf(f,"0");
    if (so())
        alf(f,"1");
    else
        alf(f,"0");
    if (okansi())
        alf(f,"1");
    else
        alf(f,"0");
    if (incom)
        alf(f,"1");
    else
        alf(f,"0");
    sprintf(s,"%10.2f",nsl());
    alf(f,s);
    alf(f,gd);
    alf(f,dd);
    sl1(3,s);
    alf(f,s);
    sprintf(s,"%u",modem_speed);
    if (!using_modem)
        strcpy(s,"KB");
    alf(f,s);
    itoa(syscfg.primaryport,s,10);
    alf(f,s);
    alf(f,syscfg.systemname);
    alf(f,syscfg.sysopname);
    l=(long) (timeon);
    if (l<0)
        l += 3600*24;
    ltoa(l,s,10);
    alf(f,s);
    l=(long) (timer()-timeon);
    if (l<0)
        l += 3600*24;
    ltoa(l,s,10);
    alf(f,s);
    ltoa(thisuser.uk,s,10);
    alf(f,s);
    itoa(thisuser.uploaded,s,10);
    alf(f,s);
    ltoa(thisuser.dk,s,10);
    alf(f,s);
    itoa(thisuser.downloaded,s,10);
    alf(f,s);
    alf(f,"8N1");
    sprintf(s,"%u",com_speed);
    alf(f,s);
    sprintf(s,"%u",syscfg.systemnumber);
    alf(f,s);
    close(f);
    get_dir(fpn,1);
    strcat(fpn,fn);
    return(fpn);
}


#define READ(x) read(f,&(x),sizeof(x))

#define WRITE(x) write(f,&(x),sizeof(x))

extern char menuat[15],mstack[10][15],mdepth;

int restore_data(char *s)
{
    int f,stat;

    f=open(s,O_RDONLY | O_BINARY);
    if (f<0)
        return(-1);

    READ(stat);
    READ(oklevel);
    READ(noklevel);
    READ(ooneuser);
    READ(no_hangup);
    READ(ok_modem_stuff);
    READ(topdata);
    READ(last_time_c);
    READ(sysop_alert);
    READ(do_event);
    READ(usernum);
    READ(chatcall);
    READ(chatreason);
    READ(timeon);
    READ(extratimecall);
    READ(curspeed);
    READ(modem_speed);
    READ(com_speed);
    READ(cursub);
    READ(curdir);
    READ(msgreadlogon);
    READ(nscandate);
    READ(mailcheck);
    READ(smwcheck);
    READ(use_workspace);
    READ(using_modem);
    READ(last_time);
    READ(fsenttoday);
    READ(global_xx);
    READ(xtime);
    READ(xdate);
    READ(incom);
    READ(outcom);
    READ(global_handle);
    READ(actsl);
    READ(numbatch);
    READ(numbatchdl);
    READ(batchtime);
    READ(batchsize);
    READ(menuat);
    READ(mstack);
    READ(mdepth);

    if (global_handle) {
        global_handle=0;
        set_global_handle(1);
    }

    userdb_load(usernum,&thisuser);
    useron=1;
    changedsl();
    topscreen();

    close(f);
    unlink(s);
    return(stat);
}

/****************************************************************************/

void save_state(char *s, int state)
{
    int f;

    save_status();

    f=open(s,O_RDWR | O_BINARY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);
    if (f<0)
        return;

    WRITE(state);
    WRITE(oklevel);
    WRITE(noklevel);
    WRITE(ooneuser);
    WRITE(no_hangup);
    WRITE(ok_modem_stuff);
    WRITE(topdata);
    WRITE(last_time_c);
    WRITE(sysop_alert);
    WRITE(do_event);
    WRITE(usernum);
    WRITE(chatcall);
    WRITE(chatreason);
    WRITE(timeon);
    WRITE(extratimecall);
    WRITE(curspeed);
    WRITE(modem_speed);
    WRITE(com_speed);
    WRITE(cursub);
    WRITE(curdir);
    WRITE(msgreadlogon);
    WRITE(nscandate);
    WRITE(mailcheck);
    WRITE(smwcheck);
    WRITE(use_workspace);
    WRITE(using_modem);
    WRITE(last_time);
    WRITE(fsenttoday);
    WRITE(global_xx);
    WRITE(xtime);
    WRITE(xdate);
    WRITE(incom);
    WRITE(outcom);
    WRITE(global_handle);
    WRITE(actsl);
    WRITE(numbatch);
    WRITE(numbatchdl);
    WRITE(batchtime);
    WRITE(batchsize);
    WRITE(menuat);
    WRITE(mstack);
    WRITE(mdepth);

    close(f);

    set_global_handle(0);
}

void dorinfo_def(void)
{
    int i,i1,f;
    char s[MAX_PATH_LEN];
    long l;

    f=open("dorinfo1.def",O_RDWR|O_CREAT|O_BINARY|O_TRUNC, S_IREAD|S_IWRITE);
    alf(f,syscfg.systemname);
    alf(f,"Dom");
    alf(f,"SysOp");
    sprintf(s,"COM%d",outcom ? syscfg.primaryport:0);
    alf(f,s);
    sprintf(s,"%u BAUD,8,N,1",com_speed);
    alf(f,s);
    alf(f,"0");
    alf(f,thisuser.name);
    alf(f,"");
    alf(f,thisuser.city);
    alf(f,okansi()?"1":"0");
    itoa(thisuser.sl,s,10);
    alf(f,s);
    l=(long) (timeon);
    if (l<0)
        l += 3600*24;
    ltoa(l,s,10);
    alf(f,s);
    alf(f,"1");
    close(f);
}

void write_door_sys(int rname)
{
    int i,fp;
    char s[MAX_PATH_LEN];

    fp=open("door.sys",O_BINARY|O_RDWR|O_CREAT|O_TRUNC,S_IREAD|S_IWRITE);
    lseek(fp,0L,SEEK_SET);

    sprintf(s,"COM%d:",outcom?syscfg.primaryport:0);
    alf(fp,s);
    sprintf(s,"%u",com_speed);
    alf(fp,s);
    alf(fp,"8");
    alf(fp,"1");
    alf(fp,"N");

    alf(fp,"N");
    alf(fp,"Y");
    alf(fp,cs()?"Y":"N");
    alf(fp,"N");

    alf(fp,rname?thisuser.realname:thisuser.name);
    alf(fp,thisuser.city);
    alf(fp,thisuser.phone);
    alf(fp,thisuser.phone);
    alf(fp,thisuser.pw);
    sprintf(s,"%d",thisuser.sl); 
    alf(fp,s);
    alf(fp,"0");
    alf(fp,thisuser.laston);
    sprintf(s,"%6.0f",nsl());
    alf(fp,s);
    sprintf(s,"%6.0f",nsl()/60.0);
    alf(fp,s);
    alf(fp,okansi()?"GR":"NG");
    sprintf(s,"%d",thisuser.screenlines); 
    alf(fp,s);
    alf(fp,"Y");
    alf(fp,"123456");
    alf(fp,"7");
    alf(fp,"12/31/99");
    sprintf(s,"%d",usernum); 
    alf(fp,s);
    alf(fp,"X");
    sprintf(s,"%d",thisuser.uploaded); 
    alf(fp,s);
    sprintf(s,"%d",thisuser.downloaded); 
    alf(fp,s);
    sprintf(s,"%d",thisuser.dk); 
    alf(fp,s);
    alf(fp,"999999");
    close(fp);
}


void rundoor(char type,char ms[MAX_PATH_LEN])
{
    create_chain_file("CHAIN.TXT");
    write_door_sys(0);
    dorinfo_def();
    save_status();
    clrscr();
    logtypes(2,"Ran Door 0%s 3(%s)",ms,ctim(timer()));
    sl1(1,"");
    switch(type) {
    case '1': 
        runprog(ms,0); 
        break;
    case '2': 
        runprog(ms,1); 
        break;
    default: 
        badcommand('D',type);
    }
    logtypes(2,"Closed Door %s 3(%s)",ms,ctim(timer()));
    topscreen();
}

