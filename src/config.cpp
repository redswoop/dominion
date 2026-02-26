#pragma hdrstop

#include "config.h"
#include <stdio.h>
#include "platform.h"
#include "bbs_output.h"
#include "bbs_input.h"
#include "bbs_ui.h"
#include "conio.h"
#include "file1.h"
#include "timest.h"
#include "disk.h"
#include "newuser.h"
#include "stringed.h"
#include "session.h"
#include "userdb.h"
#include "system.h"
#include "lilo.h"
#include "personal.h"
#include "json_io.h"
#include "bbs_path.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <stdarg.h>


#ifndef DOS
/* sys.cfg, sys.nifty, sys.xarc from vars.h; sess.user, sess.usernum, sess.topdata via session macros */

#else

int io.mciok;
configrec sys.cfg;
niftyrec sys.nifty;
xarcrec sys.xarc[8];

int io.hangup=0;

unsigned char getkey()
{
    return(getch());
}

void lpr(char *fmt, ...)
{
    va_list ap;
    char s[512];

    va_start(ap, fmt);
    vsprintf(s, fmt, ap);
    va_end(ap);
    pl(s);
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

char *get_string2(int f)
{
    return("5Select (?=Help) :0");
}


#endif

#pragma warn -sus

void bitset(char *msg,int byte,int bit)
{
    lpr("%-30s: %-3s",msg,byte & bit?"Yes":"No");
}

void togglebit(long *byte,int bit)
{
    if(*byte & bit) *byte ^=bit;
    else *byte |=bit;
}


void bits(char *msg,int byte,int bit)
{
    char s[MAX_PATH_LEN];
    sprintf(s,"%-30s: %-3s  ",msg,byte & bit?"Yes":"No");
    outstr(s);
}


void getselect(char *s,int row,int col,int len,int lc)
{
    int i,mcik=io.mciok;

    io.mciok=0;
#ifndef DOS
    npr("[%d;%dH",row+1,col+1);
    for(i=0;i<len;i++) outchr(32);
    npr("[%d;%dH",row+1,col+1);
#else
    pr("[%d;%dH",row+1,col+1);
    for(i=0;i<len;i++) outchr(32);
    pr("[%d;%dH",row+1,col+1);
#endif
    input1(s,len,lc,1);

    io.mciok=mcik;
}

void getselectt(unsigned short *i,int row,int col,int len)
{
    unsigned int i1;
    char s[41];
    unsigned int h,m,mcik=io.mciok;

    io.mciok=0;
#ifndef DOS
    npr("[%d;%dH",row+1,col+1);
    for(i1=0;i1<len;i1++) outchr(32);
    npr("[%d;%dH",row+1,col+1);
#else
    pr("[%d;%dH",row+1,col+1);
    for(i=0;i<len;i++) outchr(32);
    pr("[%d;%dH",row+1,col+1);
#endif
    input(s,5);
    h=atoi(s);
    m=atoi(&(s[3]));

    i1=h*60+m;
    *i=i1;
    io.mciok=mcik;
}

int getselectd(int row,int col,int len)
{
    int i1;
    char s[41];

#ifndef DOS
    npr("[%d;%dH",row+1,col+1);
    for(i1=0;i1<len;i1++) outchr(32);
    npr("[%d;%dH",row+1,col+1);
#else
    pr("[%d;%dH",row+1,col+1);
    for(i1=0;i1<len;i1++) outchr(32);
    pr("[%d;%dH",row+1,col+1);
#endif
    input(s,len);
    return(atoi(s));
}


void setbit(int row, int col,char bits[16],int *byte)
{
    char s[16],ch;
    int i,done=0,i1;

    do {
        for (i=0; i<=15; i++)
            if(*byte & (1 << i)) s[i]=bits[i];
            else s[i]='-';
        s[16]=0;

        npr("[%d;%dH",row+1,col+1);
        outstr(s);
        npr("[%d;%dH",row+1,col+1);
        i=toupper(getkey());
        switch(i) {
        case 13 :
        case 'Q': 
            done=1; 
            break;
        default : 
            ch=-1;
            for (i1=0; i1<16; i1++)
                if (i==bits[i1])
                    ch=i1;
            if (ch>-1)
                (*byte) ^= (1 << ch);
            break;
        }
    } 
    while(!done&&!io.hangup);
}


void getlist(int *val,char **list,int num)
{
    char c;
    int i;

    nl();
    for(i=0;i<num;i++)
        lpr("0%d. %s",i+1,list[i]);
    nl();
    outstr("5Select : 0");
    c=getkey()-'1';
    *val=c;
}

void filecfg()
{
    auto& sys = System::instance();
    int i,done=0;
    char s[MAX_PATH_LEN];

    do {
        outchr(12);
        lpr("5� 0File Area Configuration");
        nl();
        bitset(get_string2(47),sys.nifty.nifstatus,nif_ratio);
        bitset(get_string2(48),sys.nifty.nifstatus,nif_fpts);
        bitset(get_string2(49),sys.nifty.nifstatus,nif_pcr);
        bitset(get_string2(50),sys.nifty.nifstatus,nif_autocredit);
        nl();
        lpr("5. KiloByte Ratio    : %-5.3f",sys.cfg.req_ratio);
        lpr("6. File Point Ratio  : 1 to %d ",sys.nifty.fptsratio);
        lpr("7. Post Call Ratio   : %-5.3f",sys.cfg.post_call_ratio);
        nl();
        if(sys.cfg.newuploads<255) itoa(sys.cfg.newuploads,s,10);
        else strcpy(s,"None.");
        lpr("8. New Uploads Area  : %s",s);
        lpr("9. Dl Commision      : %d",sys.nifty.fcom);
        nl();

        outstr(get_string2(11));
        switch(toupper(getkey())) {
        case 'Q': 
            done=1; 
            break;

        case '1':
            togglebit((long *)&sys.nifty.nifstatus,nif_ratio);
            break;
        case '2': 
            togglebit((long *)&sys.nifty.nifstatus,nif_fpts);
            break;
        case '3': 
            togglebit((long *)&sys.nifty.nifstatus,nif_pcr);
            break;
        case '4': 
            togglebit((long *)&sys.nifty.nifstatus,nif_autocredit);
            break;

        case '5':
            getselect(s,7,23,5,1);
            sscanf(s,"%f",&sys.cfg.req_ratio);
            break;
        case '6':
            sys.nifty.fptsratio = getselectd(8,23,20);
            if(sys.nifty.fptsratio<1)
                sys.nifty.fptsratio=1;
            break;
        case '7':
            getselect(s,9,23,5,1);
            sscanf(s,"%f",&sys.cfg.post_call_ratio);
            break;

        case '8':
#ifndef DOS
            inputdat("New Uploads Area, [CR] to unset",s,3,0);
#else
            input(s,3);
#endif
            if(s[0]) {
                sys.cfg.newuploads=atoi(s);
            } 
            else sys.cfg.newuploads=255;
            break;
        case '9':
            sys.nifty.fcom = getselectd(12,23,3);
            break;
        }
    } 
    while(!done);
}

void namepath()
{
    auto& sys = System::instance();
    int done=0;
    char s[MAX_PATH_LEN];

    do {
        outchr(12);
        lpr("5� 0System Info And Paths");
        nl();
        lpr("1. System Name       : %s",sys.cfg.systemname);
        lpr("2. System Phone      : %s",sys.cfg.systemphone);
        lpr("3. System Password   : %s",sys.cfg.systempw);
        lpr("4. SysOp Name        : %s",sys.cfg.sysopname);
        lpr("5. Matrix Password   : %s",sys.nifty.matrix);
        lpr("6. New User Password : %s",sys.cfg.newuserpw);
        lpr("7. Lock Out Password : %s",sys.nifty.lockoutpw);
        nl();
        lpr("8. Data Directory    : %s",sys.cfg.datadir);
        lpr("9. Afiles Directory  : %s",sys.cfg.gfilesdir);
        lpr("0. Msgs Directory    : %s",sys.cfg.msgsdir);
        lpr("A. Menu Directory    : %s",sys.cfg.menudir);
        lpr("B. Batch Directory   : %s",sys.cfg.batchdir);
        lpr("C. Temp Directory    : %s",sys.cfg.tempdir);
        lpr("D. Default Dls Dir   : %s",sys.cfg.dloadsdir);
        nl();
        outstr(get_string2(11));
        switch(toupper(getkey())) {
        case 'Q': 
            done=1; 
            break;
        case '1':
            getselect(sys.cfg.systemname,2,23, sizeof(sys.cfg.systemname)-1,1);
            break;
        case '2':
#ifndef DOS
            go(4,24); 
            inputfone(sys.cfg.systemphone); 
            break;
#else
            getselect(sys.cfg.systemphone,3,23,12,0); 
            break;
#endif
        case '3': 
            getselect(sys.cfg.systempw,4,23,   sizeof(sys.cfg.systempw)-1,0);
            break;
        case '4': 
            getselect(sys.cfg.sysopname,5,23,  sizeof(sys.cfg.sysopname)-1,1);
            break;
        case '5': 
            getselect(sys.nifty.matrix,6,23,      sizeof(sys.nifty.matrix)-1,0);
            break;
        case '6': 
            getselect(sys.cfg.newuserpw,7,23,  sizeof(sys.cfg.newuserpw)-1,0);
            break;
        case '7': 
            getselect(sys.nifty.lockoutpw,8,23,  sizeof(sys.nifty.lockoutpw)-1,0);
            break;
        case '8': 
            getselect(sys.cfg.datadir,10,23,    sizeof(sys.cfg.datadir)-1,1);
            break;
        case '9': 
            getselect(sys.cfg.gfilesdir,11,23, sizeof(sys.cfg.gfilesdir)-1,1);
            break;
        case '0': 
            getselect(sys.cfg.msgsdir,12,23,   sizeof(sys.cfg.msgsdir)-1,1);
            break;
        case 'A': 
            getselect(sys.cfg.menudir,13,23,    sizeof(sys.cfg.menudir)-1,1);
            break;
        case 'B': 
            getselect(sys.cfg.batchdir,14,23,  sizeof(sys.cfg.batchdir)-1,1);
            break;
        case 'C': 
            getselect(sys.cfg.tempdir,15,23,   sizeof(sys.cfg.tempdir)-1,1);
            break;
        case 'D': 
            getselect(sys.cfg.dloadsdir,16,23, sizeof(sys.cfg.dloadsdir)-1,1);
            break;
        }
    } 
    while(!done&&!io.hangup);
}

void flagged()
{
    auto& sys = System::instance();
    int done=0;

    do {
        outchr(12);
        lpr("5� 0Flagged Information");
        nl();
#ifdef DOS
        bits("1. File Ratio",sys.nifty.nifstatus,nif_ratio);
        bitset("2. File Point Ratio",sys.nifty.nifstatus,nif_fpts);
        bits("3. Post Call Ratio",sys.nifty.nifstatus,nif_pcr);
        bitset("4. Auto UL Validation",sys.nifty.nifstatus,nif_autocredit);
        bits("5. 2Way Default",sys.cfg.sysconfig,sysconfig_2_way);
        lpr("%-30s: %s","6. Chat Call Type",sys.nifty.nifstatus & nif_chattype?"Screech":"Beep");
        bitset("7. Log to Printer",sys.cfg.sysconfig,sysconfig_printer);
        bits("8. Disallow Handles",sys.cfg.sysconfig,sysconfig_no_alias);
        bitset("9. Phone Number in Logon",sys.cfg.sysconfig,sysconfig_free_phone);
        bits("0. AutoMessage in Logon",sys.nifty.nifstatus,nif_automsg);
        bitset("A. Last Few Callers in Logon",sys.nifty.nifstatus,nif_lastfew);
        bits("B. Your Info in Logon",sys.nifty.nifstatus,nif_yourinfo);
        bitset("C. Automatic Chat Buffer Open",sys.nifty.nifstatus,nif_autochat);
        bits("D. Local System Security",sys.cfg.sysconfig,sysconfig_no_local);
        bitset("E. Phone Off-Hook",sys.cfg.sysconfig,sysconfig_off_hook);
        bits("F. Allow Users to Fast Logon",sys.cfg.sysconfig,sysconfig_no_xfer);
        bitset("G. Strip Color from Logs",sys.cfg.sysconfig,sysconfig_shrink_term);
        bits("H. Forced Voting",sys.nifty.nifstatus,nif_forcevote);
        bitset("!. All Uploads to SysOp",sys.cfg.sysconfig,sysconfig_all_sysop);
#else
        bits(get_string2(51),sys.cfg.sysconfig,sysconfig_2_way);
        lpr("%-30s: %s","2. Chat Call Type",sys.nifty.nifstatus & nif_chattype?"Screech":"Beep");
        bits(get_string2(53),sys.cfg.sysconfig,sysconfig_printer);
        bitset(get_string2(54),sys.cfg.sysconfig,sysconfig_no_alias);
        bits(get_string2(55),sys.cfg.sysconfig,sysconfig_free_phone);
        bitset(get_string2(59),sys.nifty.nifstatus,nif_autochat);
        bits(get_string2(60),sys.cfg.sysconfig,sysconfig_no_local);
        bitset(get_string2(61),sys.cfg.sysconfig,sysconfig_off_hook);
        bits(get_string2(62),sys.cfg.sysconfig,sysconfig_no_xfer);
        bitset(get_string2(63),sys.cfg.sysconfig,sysconfig_shrink_term);
        bitset(get_string2(65),sys.cfg.sysconfig,sysconfig_all_sysop);
#endif
        nl();
        outstr(get_string2(11));
        switch(toupper(getkey())) {
        case 'Q': 
            done=1; 
            break;
        case '1':
            togglebit((long *)&sys.cfg.sysconfig,sysconfig_2_way);
            break;
        case '2':
            togglebit((long *)&sys.nifty.nifstatus,nif_chattype);
            break;
        case '3':
            togglebit((long *)&sys.cfg.sysconfig,sysconfig_printer);
            break;
        case '4':
            togglebit((long *)&sys.cfg.sysconfig,sysconfig_no_alias);
            break;
        case '5':
            togglebit((long *)&sys.cfg.sysconfig,sysconfig_free_phone);
            break;
        case '6':
            togglebit((long *)&sys.nifty.nifstatus,nif_autochat);
            break;
        case '7':
            togglebit((long *)&sys.cfg.sysconfig,sysconfig_no_local);
            break;
        case '8':
            togglebit((long *)&sys.cfg.sysconfig,sysconfig_off_hook);
            break;
        case '9':
            togglebit((long *)&sys.cfg.sysconfig,sysconfig_no_xfer);
            break;
        case '0':
            togglebit((long *)&sys.cfg.sysconfig,sysconfig_shrink_term);
            break;
        case 'A':
            togglebit((long *)&sys.cfg.sysconfig,sysconfig_all_sysop);
            break;
        }
    } 
    while(!done&&!io.hangup);

}

void varible()
{
    auto& sys = System::instance();
    int done=0,i;
    char *chattype[]={
        "TwoColor","Filtered","Rotating"        }
    ,*s;

    do {
        outchr(12);
        lpr("5� 0Variable System Data");
        nl();
        lpr("1. Start Out Menu    : %s",sys.nifty.firstmenu);
        lpr("2. New User Menu     : %s",sys.nifty.newusermenu);
        lpr("3. Echo Character    : %c",sys.nifty.echochar);
        nl();
        lpr("4. Max Waiting Mail  : %d",sys.cfg.maxwaiting);
        lpr("5. Input Prompt Type : %s",sys.nifty.nifstatus & nif_comment?"Dots":"Blue");
        lpr("6. Maxtrix Active    : %s",sys.nifty.matrixtype?"Yes":"No");
        lpr("7. Lock Out Rate     : %d",sys.nifty.lockoutrate);
        nl();
        lpr("8. Chat Hours Start  : %s ",ctim((double)sys.cfg.sysoplowtime*60));
        lpr("9. Chat Hours End    : %s ",ctim((double)sys.cfg.sysophightime*60));
#ifndef DOS
        npr("0. Set Rotating Clrs : SysOp=");
        for(i=0;i<5;i++) {
            ansic(sys.nifty.rotate[i]);
            npr("%d,",i);
        }
        outstr(" User=");
        for(i=0;i<5;i++) {
            ansic(sys.cfg.dszbatchdl[i]);
            npr("%d,",i);
        }
        backspace();
        nl();
#endif
        lpr("0A. Chat Type         : %s",chattype[sys.nifty.chatcolor]);
        nl();
        lpr("B. Days to Keep Logs : %d",sys.nifty.systemtype);
        lpr("C. QWK Packet Name   : %s",sys.nifty.menudir);
        nl();
        outstr(get_string2(11));
        switch(toupper(getkey())) {
        case 'Q': 
            done=1;
            break;
        case '1':
            getselect(sys.nifty.firstmenu,2,23,sizeof(sys.nifty.firstmenu),1);
            break;
        case '2': 
            getselect(sys.nifty.newusermenu,3,23,sizeof(sys.nifty.newusermenu),1); 
            break;
        case '3': 
            getselect(s,4,23,1,1);
            sys.nifty.echochar=s[0];
            break;
        case '4':
            sys.cfg.maxwaiting = getselectd(6,23,4);
            break;
        case '5':
            togglebit((long *)&sys.nifty.nifstatus,nif_comment);
            break;
        case '6':
            sys.nifty.matrixtype=!(sys.nifty.matrixtype); 
            break;
        case '7':
            sys.nifty.lockoutrate=getselectd(9,23,20);
            break;
        case '8':
            getselectt(&sys.cfg.sysoplowtime,11,23,20);
            break;
        case '9':
            getselectt(&sys.cfg.sysophightime,12,23,20);
            break;
        case '0':
            nl();
            for(i=0;i<5;i++) {
                npr("Color %d: ",i);
                input(s,3);
                if(s[0])
                    sys.nifty.rotate[i]=atoi(s);
            }
            for(i=0;i<5;i++) {
                npr("Color %d: ",i);
                input(s,3);
                if(s[0])
                    sys.cfg.dszbatchdl[i]=atoi(s);
            }
            break;
        case 'A':
            { int _tmp = sys.nifty.chatcolor; getlist(&_tmp,chattype,3); sys.nifty.chatcolor = _tmp; }
            break;
        case 'B':
            sys.nifty.systemtype=getselectd(16,23,20);
            break;

        case 'C':
            getselect(sys.nifty.menudir,17,23,8,1);
            break;

        }
    } 
    while(!done&&!io.hangup);

}

void events()
{
    auto& sys = System::instance();
    int done=0;
    char s[MAX_PATH_LEN];

    do {
        outchr(12);
        lpr("5� 0Event Manager");
        nl();
        lpr("1. Logon Event     : %s",sys.cfg.logon_c);
        lpr("2. Logoff Event    : %s",sys.cfg.upload_c);
        lpr("3. Begin Day Event : %s",sys.cfg.beginday_c);
        lpr("4. Newuser Event   : %s",sys.cfg.newuser_c);
        nl();
        lpr("5. Timed Event Execute Time : %s ",ctim((double)sys.cfg.executetime*60));
        lpr("6. Timed Event File Name    : %s",sys.cfg.executestr);
        nl();
        outstr(get_string2(11));
        switch(toupper(getkey())) {
        case 'Q': 
            done=1; 
            break;
        case '1': 
            getselect(sys.cfg.logon_c,2,21,sizeof(sys.cfg.logon_c),1); 
            break;
        case '2': 
            getselect(sys.cfg.upload_c,3,21,sizeof(sys.cfg.upload_c),1); 
            break;
        case '3': 
            getselect(sys.cfg.beginday_c,4,21,sizeof(sys.cfg.beginday_c),1); 
            break;
        case '4': 
            getselect(sys.cfg.newuser_c,5,21,sizeof(sys.cfg.newuser_c),1); 
            break;
        case '5': 
            getselectt(&sys.cfg.executetime,7,30,30); 
            break;
        case '6': 
            getselect(sys.cfg.executestr,8,30,sizeof(sys.cfg.executestr),1); 
            break;
        }
    } 
    while(!done&&!io.hangup);
}

void modeminfo()
{
    auto& sys = System::instance();
    int done=0;
    char s[MAX_PATH_LEN];

    do {
        outchr(12);
        lpr("5� 0Modem Information");
        nl();
        lpr("1. Com Port     : %d",sys.cfg.primaryport);
        lpr("2. Interrupt    : %d",sys.cfg.com_ISR[sys.cfg.primaryport]);
        lpr("3. Base Address : %x",sys.cfg.com_base[sys.cfg.primaryport]);
        nl();
        outstr(get_string2(11));
        switch(toupper(getkey())) {
        case 'Q': 
            done=1; 
            break;
        case '1': 
            sys.cfg.primaryport = getselectd(2,18,2); 
            break;
        case '2': 
            sys.cfg.com_ISR[sys.cfg.primaryport] = getselectd(3,18,2); 
            break;
        case '3': 
            getselect(s,4,18,4,1);
            sscanf(s,"%x",&sys.cfg.com_base[sys.cfg.primaryport]);
            break;
        }
    } 
    while(!done&&!io.hangup);
}

void autoval()
{
    auto& sys = System::instance();
    char ar[17],dar[17],res[17],s[161];
    int i,done=0,i1,numed=0;

    strcpy(s,restrict_string);

    do {
        outchr(12);
        pl("5� 0Security Profiles");
        nl();
        for(i1=0;i1<10;i1++) {
            for (i=0; i<=15; i++) {
                if (sys.cfg.autoval[i1].ar & (1 << i)) ar[i]='A'+i;
                else ar[i]='-';
                if (sys.cfg.autoval[i1].dar & (1 << i)) dar[i]='A'+i;
                else dar[i]='-';
                if (sys.cfg.autoval[i1].restrict & (1 << i)) res[i]=s[i];
                else res[i]='-';
            }
            ar[16]=0;
            dar[16]=0;
            res[16]=0;
            sprintf(s,"%d%2d. SL=%3d DL=%3d AR=%s IR=%s R=%s",i1==numed?0:3,i1+1,sys.cfg.autoval[i1].sl,sys.cfg.autoval[i1].dsl,ar,dar,res);
            pl(s);
        }
        nl();
        outstr("5Select (Q=Quit, #,],[ to Select)0 ");
        i1=numed;
        switch(toupper(getkey())) {
        case 'Q': 
            done=1; 
            break;
        case ']': 
            if(numed<9) numed++; 
            break;
        case '[': 
            if(numed>0) numed--; 
            break;
        case 'S': 
            sys.cfg.autoval[i1].sl = getselectd(i1+2,7,3); 
            break;
        case 'D': 
            sys.cfg.autoval[i1].dsl = getselectd(i1+2,15,3); 
            break;
        case 'A': 
            { int _tmp = sys.cfg.autoval[i1].ar; setbit(i1+2,21,"ABCDEFGHIJKLMNOP",&_tmp); sys.cfg.autoval[i1].ar = _tmp; } 
            break;
        case 'I': 
            { int _tmp = sys.cfg.autoval[i1].dar; setbit(i1+2,41,"ABCDEFGHIJKLMNOP",&_tmp); sys.cfg.autoval[i1].dar = _tmp; } 
            break;
        case 'R': 
            { int _tmp = sys.cfg.autoval[i1].restrict; setbit(i1+2,60,restrict_string,&_tmp); sys.cfg.autoval[i1].restrict = _tmp; } 
            break;
        }
    } 
    while(!done&&!io.hangup);
}

void archive()
{
    auto& sys = System::instance();
    char s[MAX_PATH_LEN];
    int i,done=0,i1=0;

    strcpy(s, BbsPath::join(sys.cfg.datadir, "archive.dat").c_str());
    i=open(s,O_BINARY|O_RDWR);
    read(i,&sys.xarc[0],8*sizeof(sys.xarc[0]));
    close(i);

    do {
        outchr(12);
        pl("5� 0Archive Configuration");
        nl();
        lpr("3Number 9%d/70",i1);
        nl();
        lpr("1. Extension  : %s",sys.xarc[i1].extension);
        lpr("2. Add to Arc : %s",sys.xarc[i1].arca);
        lpr("3. Extract Arc: %s",sys.xarc[i1].arce);
        lpr("4. View Arc   : %s",sys.xarc[i1].arcl);
        lpr("5. Test Arc   : %s",sys.xarc[i1].arct);
        lpr("6. Comment Arc: %s",sys.xarc[i1].arcc);
        lpr("7. Swap to Run: %s",sys.xarc[i1].attr & xarc_swap?"Yes":"No");
        lpr("8. Ok Level 1 : %d",sys.xarc[i1].ok1);
        lpr("9. Ok Level 1 : %d",sys.xarc[i1].ok2);
        lpr("0. Error 1    : %d",sys.xarc[i1].nk1);
        lpr("A. Error 2    : %d",sys.xarc[i1].nk2);
        nl();
        pl(" %1 = Archive File Name");
        pl(" %2 = File to Touch");
        pl(" %5 = Comment File Name");
        nl();
        outstr("5Select (Q=Quit, #,],[ to Select)5 :");
        switch(toupper(getkey())) {
        case 'Q': 
            done=1; 
            break;
        case '[': 
            if(i1) i1--; 
            break;
        case ']': 
            if(i1<7) i1++; 
            break;
        case '1': 
            getselect(sys.xarc[i1].extension,4,16,3,0); 
            break;
        case '2': 
            getselect(sys.xarc[i1].arca,5,16,sizeof(sys.xarc[i1].arca),1); 
            break;
        case '3': 
            getselect(sys.xarc[i1].arce,6,16,sizeof(sys.xarc[i1].arce),1); 
            break;
        case '4': 
            getselect(sys.xarc[i1].arcl,7,16,sizeof(sys.xarc[i1].arcl),1); 
            break;
        case '5': 
            getselect(sys.xarc[i1].arct,8,16,sizeof(sys.xarc[i1].arct),1); 
            break;
        case '6': 
            getselect(sys.xarc[i1].arcc,9,16,sizeof(sys.xarc[i1].arcc),1); 
            break;
        case '7': 
            togglebit((long *)&sys.xarc[i1].attr,xarc_swap);
            break;
        case '8': 
            sys.xarc[i1].ok1 = getselectd(11,16,2);
        case '9': 
            sys.xarc[i1].ok2 = getselectd(12,16,2);
        case '0': 
            sys.xarc[i1].nk1 = getselectd(13,16,2);
        case 'A': 
            sys.xarc[i1].nk2 = getselectd(14,16,2);
        }
    } 
    while(!done&&!io.hangup);

    strcpy(s, BbsPath::join(sys.cfg.datadir, "archive.dat").c_str());
    i=open(s,O_BINARY|O_RDWR|O_CREAT,S_IREAD|S_IWRITE);
    write(i,&sys.xarc[0],8*sizeof(sys.xarc[0]));
    close(i);
}

void secleved()
{
    auto& sys = System::instance();
    char s[MAX_PATH_LEN];
    int i,done=0,i1=0;

    do {
        outchr(12);
        pl("5� 0Security Level Data");
        nl();
        lpr("3Number 9%d0",i1);
        nl();
        lpr("1. Time Per Day          : %d",sys.cfg.sl[i1].time_per_day);
        lpr("2. Time Per Call         : %d",sys.cfg.sl[i1].time_per_logon);
        lpr("3. Max Calls per day     : %d",sys.cfg.sl[i1].maxcalls);
        lpr("4. Max Emails            : %d",sys.cfg.sl[i1].emails);
        lpr("5. Max Posts             : %d",sys.cfg.sl[i1].posts);
        bitset("6. Post Anonymous",sys.cfg.sl[i1].ability        ,ability_post_anony);
        bitset("7. Email Anonymous",sys.cfg.sl[i1].ability       ,ability_email_anony);
        bitset("8. Read Anonymous Posts",sys.cfg.sl[i1].ability  ,ability_read_post_anony);
        bitset("9. Read Anonymous Email",sys.cfg.sl[i1].ability  ,ability_read_email_anony);
        bitset("0. Posts Need Validation",sys.cfg.sl[i1].ability,ability_val_net);
        nl();
        outstr("5Select (Q=Quit, #,J=Jump,],[ to Select)5 :");
        switch(toupper(getkey())) {
        case 'Q': 
            done=1; 
            break;
        case 'J':
#ifndef DOS
            nl(); 
            inputdat("To Which",s,3,0); 
#else
            input(s,3);
#endif
            if(s[0]) i1=atoi(s); 
            break;
        case '[': 
            if(i1) i1--; 
            break;
        case ']': 
            if(i1<255) i1++; 
            break;
        case '{': 
            if(i1-10) i1-=10; 
            if(i1<0) i1=0; 
            break;
        case '}': 
            if(i1+10<255) i1+=10; 
            if(i1>255) i1=255; 
            break;
        case '1': 
            sys.cfg.sl[i1].time_per_day = getselectd(4,27,3); 
            break;
        case '2': 
            sys.cfg.sl[i1].time_per_logon = getselectd(5,27,3); 
            break;
        case '3': 
            sys.cfg.sl[i1].maxcalls = getselectd(6,27,3); 
            break;
        case '4': 
            sys.cfg.sl[i1].emails = getselectd(7,27,3); 
            break;
        case '5': 
            sys.cfg.sl[i1].posts = getselectd(8,27,3); 
            break;
        case '6': 
            togglebit((long *)&sys.cfg.sl[i1].ability,ability_post_anony);
            break;
        case '7': 
            togglebit((long *)&sys.cfg.sl[i1].ability,ability_email_anony);
            break;
        case '8': 
            togglebit((long *)&sys.cfg.sl[i1].ability,ability_read_post_anony);
            break;
        case '9': 
            togglebit((long *)&sys.cfg.sl[i1].ability,ability_read_email_anony);
            break;
        case '0': 
            togglebit((long *)&sys.cfg.sl[i1].ability,ability_val_net);
            break;
        }
    } 
    while(!done&&!io.hangup);

}

void nued()
{
    auto& sys = System::instance();
    int done=0,i;
    char s[MAX_PATH_LEN];

    do {
        outchr(12);
        lpr("5� 0New User Data");
        nl();
        lpr(get_string2(66),sys.nifty.nulevel+1);
        lpr(get_string2(67),sys.nifty.nuinf);
        lpr(get_string2(68),i=sys.cfg.newusergold);
        lpr(get_string2(69),sys.nifty.nifstatus & nif_nuv?"Active":"InActive");
        lpr(get_string2(70),sys.nifty.nuvyes);
        lpr(get_string2(71),sys.nifty.nuvbad);
        lpr(get_string2(72),sys.nifty.nuvlevel);
        lpr(get_string2(73),sys.nifty.nuvinf);
        lpr(get_string2(74),sys.nifty.nuvsl);
        lpr("0. Accepting New Users       : %s",sys.cfg.closedsystem?"No":"Yes");
        lpr("A. Maximum Users             : %d",sys.cfg.maxusers);
        outstr("B. NUV Action                : ");
        switch(sys.nifty.nuvaction) {
        case 0: 
            pl("Deleted if voted bad"); 
            break;
        case 1: 
            pl("Lock out if voted bad"); 
            break;
        case 2: 
            pl("Set to Bad Profile"); 
            break;
        }
        lpr("C. NUV Bad Level             : %d",sys.nifty.nuvbadlevel);
        nl();
        outstr(get_string2(11));
        switch(toupper(getkey())) {
        case 'Q': 
            done=1; 
            break;
        case '1': 
            sys.nifty.nulevel=getselectd(2,31,2);
            sys.nifty.nulevel--;
            sys.cfg.newusersl=sys.cfg.autoval[sys.nifty.nulevel].sl;
            sys.cfg.newuserdsl=sys.cfg.autoval[sys.nifty.nulevel].dsl;
            break;
        case '2': 
            getselect(sys.nifty.nuinf,3,31,8,0); 
            break;
        case '3': 
            sys.cfg.newusergold=getselectd(4,31,8);
            break;
#ifdef NUV
        case '4': 
            togglebit((long *)&sys.nifty.nifstatus,nif_nuv);
            break;
        case '5': 
            sys.nifty.nuvyes=getselectd(6,31,8);
            break;
        case '6': 
            sys.nifty.nuvbad=getselectd(7,31,8); 
            break;
        case '7': 
            sys.nifty.nuvlevel=getselectd(8,31,8); 
            break;
        case '8': 
            getselect(sys.nifty.nuvinf,9,31,8,0); 
            break;
        case '9': 
            getselect(sys.nifty.nuvsl,10,31,10,0); 
            break;
#endif
        case '0': 
            sys.cfg.closedsystem=!(sys.cfg.closedsystem); 
            break;
        case 'A': 
            sys.cfg.maxusers=getselectd(12,31,8); 
            break;
        case 'C': 
            sys.nifty.nuvbadlevel=getselectd(14,31,8); 
            break;
        case 'B': 
            sys.nifty.nuvaction++;
            if(sys.nifty.nuvaction==3)
                sys.nifty.nuvaction=0;
            break;
        }
    } 
    while(!done&&!io.hangup);

}

/* FidoNet config editors removed */

void acscfg(void)
{
    auto& sys = System::instance();
    int done=0,i;
    char s[MAX_PATH_LEN];
    acsrec acs;

    strcpy(s, BbsPath::join(sys.cfg.datadir, "acs.dat").c_str());
    i=open(s,O_BINARY|O_RDWR|O_CREAT,S_IREAD|S_IWRITE);
    read(i,&acs,sizeof(acsrec));
    close(i);

    do {
        outchr(12);
        lpr("5� 0Access Level Configuration");
        nl();
        lpr("A. Exempt from PCR           : %s",acs.epcr);
        lpr("B. Exempt from Ratio         : %s",acs.eratio);
        lpr("C. Exempt from File Points   : %s",acs.efpts);
        lpr("D. Exempt from Time Check    : %s",acs.etc);
        lpr("E. Asks SysPW at Logon       : %s",acs.syspw);
        lpr("F. Echo Passowrds Remotely   : %s",acs.showpw);
        lpr("G. Call Command Types        : %s",acs.callcmd);
        lpr("H. Read Unvalidated Messages : %s",acs.readunval);
        lpr("I. Considered CoSysOp        : %s",acs.cosysop);
        lpr("J. Considered SysOp          : %s",acs.sysop);
        lpr("K. Can Chat Page Anytime     : %s",acs.echat);
        lpr("L. Upload to Any Area        : %s",acs.anyul);
        lpr("M. Read Anonymous Messages   : %s",acs.readanon);
        lpr("N. Delete Any Messages       : %s",acs.delmsg);
        lpr("O. Zap Private Mail          : %s",acs.zapmail);
        nl();
        outstr(get_string2(11));
        switch(toupper(getkey())) {
        case 'Q': 
            done=1; 
            break;
        case 'A': 
            getselect(acs.epcr,2,31,31,0); 
            break;
        case 'B': 
            getselect(acs.eratio,3,31,31,0); 
            break;
        case 'C': 
            getselect(acs.efpts,4,31,31,0); 
            break;
        case 'D': 
            getselect(acs.etc,5,31,31,0); 
            break;
        case 'E': 
            getselect(acs.syspw,6,31,31,0); 
            break;
        case 'F': 
            getselect(acs.showpw,7,31,31,0); 
            break;
        case 'G': 
            getselect(acs.callcmd,8,31,31,0); 
            break;
        case 'H': 
            getselect(acs.readunval,9,31,31,0); 
            break;
        case 'I': 
            getselect(acs.cosysop,10,31,31,0); 
            break;
        case 'J': 
            getselect(acs.sysop,11,31,31,0); 
            break;
        case 'K': 
            getselect(acs.echat,12,31,31,0); 
            break;
        case 'L': 
            getselect(acs.anyul,13,31,31,0); 
            break;
        case 'M': 
            getselect(acs.readanon,14,31,31,0); 
            break;
        case 'N': 
            getselect(acs.delmsg,15,31,31,0); 
            break;
        case 'O': 
            getselect(acs.zapmail,16,31,31,0); 
            break;
        }
    } 
    while(!done&&!io.hangup);

    i=open(s,O_BINARY|O_RDWR|O_CREAT|O_TRUNC,S_IREAD|S_IWRITE);
    write(i,&acs,sizeof(acsrec));
    close(i);

}


#ifndef DOS
void defcoled()
{
    int i;
    User u;

    outchr(12);
    { auto p = UserDB::instance().get(1); if (p) u = *p; }

    io.colblock=1;
    change_colors(u);
    io.colblock=0;
}
#endif

#ifndef DOS
void config()
#else
void main(int argc, char *argv[])
#endif
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    int i,done=0;
    FILE *f;
    char s[MAX_PATH_LEN];

#ifndef DOS
    if(!checkpw())
        return;

    sess.topdata=0;
    topscreen();
#else
    {
        cJSON *cfg_root = read_json_file("config.json");
        if (!cfg_root) { printf("config.json not found!\n"); exit(1); }
        json_to_configrec(cfg_root, &sys.cfg, &sys.nifty);
        cJSON_Delete(cfg_root);
    }
#endif
    do {
        outchr(12);
        pl("5� 0Dominion System Configuration");
        nl();
#ifndef DOS
        printmenu(19);
#else
        pl("71.0 Names and Paths                 72.0 Flagged Info");
        pl("73.0 Varible System Data             74.0 Event Manager");
        pl("75.0 Modem Info                      76.0 AutoVal Data");
        pl("77.0 Archive Configuration           78.0 FullScreen Editors");
        pl("79.0 Security Level Data             70.0 FidoNet Configuration");
        pl("7F.0 File Area Config");
#endif
        nl();
        outstr(get_string2(11));
        switch(toupper(getkey())) {
        case 'Q': 
            done=1; 
            break;
        case '1': 
            namepath(); 
            break;
        case '2': 
            flagged(); 
            break;
        case '3': 
            varible(); 
            break;
        case '4': 
            events(); 
            break;
        case '5': 
            modeminfo(); 
            break;
        case '6': 
            autoval(); 
            break;
        case '7': 
            archive(); 
            break;
        case '8': 
            nued(); 
            break;
        case '9': 
            secleved(); 
            break;
        case 'F':
            filecfg();
            break;
        case 'S':
            acscfg();
            break;
#ifndef DOS
        case 'A': 
            defcoled(); 
            break;
#endif
        }
    } 
    while(!done&&!io.hangup);

    {
        cJSON *cfg_root = configrec_to_json(&sys.cfg, &sys.nifty);
        write_json_file("config.json", cfg_root);
        cJSON_Delete(cfg_root);
    }
}
