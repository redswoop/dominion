#pragma hdrstop

#include <stdio.h>
#include "fcns.h"
#include "io_stream.h"
#include "json_io.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <stdarg.h>


#ifndef DOS
extern configrec syscfg;
extern niftyrec nifty;
extern int topdata;
extern userrec thisuser;
extern int usernum;
extern xarcrec xarc[8];

#else
int mciok;
configrec syscfg;
niftyrec nifty;
xarcrec xarc[8];

int hangup=0;

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
    int i,mcik=mciok;

    mciok=0;
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

    mciok=mcik;
}

void getselectt(unsigned short *i,int row,int col,int len)
{
    unsigned int i1;
    char s[41];
    unsigned int h,m,mcik=mciok;

    mciok=0;
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
    mciok=mcik;
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
    while(!done&&!hangup);
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
    int i,done=0;
    char s[MAX_PATH_LEN];

    do {
        outchr(12);
        lpr("5� 0File Area Configuration");
        nl();
        bitset(get_string2(47),nifty.nifstatus,nif_ratio);
        bitset(get_string2(48),nifty.nifstatus,nif_fpts);
        bitset(get_string2(49),nifty.nifstatus,nif_pcr);
        bitset(get_string2(50),nifty.nifstatus,nif_autocredit);
        nl();
        lpr("5. KiloByte Ratio    : %-5.3f",syscfg.req_ratio);
        lpr("6. File Point Ratio  : 1 to %d ",nifty.fptsratio);
        lpr("7. Post Call Ratio   : %-5.3f",syscfg.post_call_ratio);
        nl();
        if(syscfg.newuploads<255) itoa(syscfg.newuploads,s,10);
        else strcpy(s,"None.");
        lpr("8. New Uploads Area  : %s",s);
        lpr("9. Dl Commision      : %d",nifty.fcom);
        nl();

        outstr(get_string2(11));
        switch(toupper(getkey())) {
        case 'Q': 
            done=1; 
            break;

        case '1':
            togglebit((long *)&nifty.nifstatus,nif_ratio);
            break;
        case '2': 
            togglebit((long *)&nifty.nifstatus,nif_fpts);
            break;
        case '3': 
            togglebit((long *)&nifty.nifstatus,nif_pcr);
            break;
        case '4': 
            togglebit((long *)&nifty.nifstatus,nif_autocredit);
            break;

        case '5':
            getselect(s,7,23,5,1);
            sscanf(s,"%f",&syscfg.req_ratio);
            break;
        case '6':
            nifty.fptsratio = getselectd(8,23,20);
            if(nifty.fptsratio<1)
                nifty.fptsratio=1;
            break;
        case '7':
            getselect(s,9,23,5,1);
            sscanf(s,"%f",&syscfg.post_call_ratio);
            break;

        case '8':
#ifndef DOS
            inputdat("New Uploads Area, [CR] to unset",s,3,0);
#else
            input(s,3);
#endif
            if(s[0]) {
                syscfg.newuploads=atoi(s);
            } 
            else syscfg.newuploads=255;
            break;
        case '9':
            nifty.fcom = getselectd(12,23,3);
            break;
        }
    } 
    while(!done);
}

void namepath()
{
    int done=0;
    char s[MAX_PATH_LEN];

    do {
        outchr(12);
        lpr("5� 0System Info And Paths");
        nl();
        lpr("1. System Name       : %s",syscfg.systemname);
        lpr("2. System Phone      : %s",syscfg.systemphone);
        lpr("3. System Password   : %s",syscfg.systempw);
        lpr("4. SysOp Name        : %s",syscfg.sysopname);
        lpr("5. Matrix Password   : %s",nifty.matrix);
        lpr("6. New User Password : %s",syscfg.newuserpw);
        lpr("7. Lock Out Password : %s",nifty.lockoutpw);
        nl();
        lpr("8. Data Directory    : %s",syscfg.datadir);
        lpr("9. Afiles Directory  : %s",syscfg.gfilesdir);
        lpr("0. Msgs Directory    : %s",syscfg.msgsdir);
        lpr("A. Menu Directory    : %s",syscfg.menudir);
        lpr("B. Batch Directory   : %s",syscfg.batchdir);
        lpr("C. Temp Directory    : %s",syscfg.tempdir);
        lpr("D. Default Dls Dir   : %s",syscfg.dloadsdir);
        nl();
        outstr(get_string2(11));
        switch(toupper(getkey())) {
        case 'Q': 
            done=1; 
            break;
        case '1':
            getselect(syscfg.systemname,2,23, sizeof(syscfg.systemname)-1,1);
            break;
        case '2':
#ifndef DOS
            go(4,24); 
            inputfone(syscfg.systemphone); 
            break;
#else
            getselect(syscfg.systemphone,3,23,12,0); 
            break;
#endif
        case '3': 
            getselect(syscfg.systempw,4,23,   sizeof(syscfg.systempw)-1,0);
            break;
        case '4': 
            getselect(syscfg.sysopname,5,23,  sizeof(syscfg.sysopname)-1,1);
            break;
        case '5': 
            getselect(nifty.matrix,6,23,      sizeof(nifty.matrix)-1,0);
            break;
        case '6': 
            getselect(syscfg.newuserpw,7,23,  sizeof(syscfg.newuserpw)-1,0);
            break;
        case '7': 
            getselect(nifty.lockoutpw,8,23,  sizeof(nifty.lockoutpw)-1,0);
            break;
        case '8': 
            getselect(syscfg.datadir,10,23,    sizeof(syscfg.datadir)-1,1);
            break;
        case '9': 
            getselect(syscfg.gfilesdir,11,23, sizeof(syscfg.gfilesdir)-1,1);
            break;
        case '0': 
            getselect(syscfg.msgsdir,12,23,   sizeof(syscfg.msgsdir)-1,1);
            break;
        case 'A': 
            getselect(syscfg.menudir,13,23,    sizeof(syscfg.menudir)-1,1);
            break;
        case 'B': 
            getselect(syscfg.batchdir,14,23,  sizeof(syscfg.batchdir)-1,1);
            break;
        case 'C': 
            getselect(syscfg.tempdir,15,23,   sizeof(syscfg.tempdir)-1,1);
            break;
        case 'D': 
            getselect(syscfg.dloadsdir,16,23, sizeof(syscfg.dloadsdir)-1,1);
            break;
        }
    } 
    while(!done&&!hangup);
}

void flagged()
{
    int done=0;

    do {
        outchr(12);
        lpr("5� 0Flagged Information");
        nl();
#ifdef DOS
        bits("1. File Ratio",nifty.nifstatus,nif_ratio);
        bitset("2. File Point Ratio",nifty.nifstatus,nif_fpts);
        bits("3. Post Call Ratio",nifty.nifstatus,nif_pcr);
        bitset("4. Auto UL Validation",nifty.nifstatus,nif_autocredit);
        bits("5. 2Way Default",syscfg.sysconfig,sysconfig_2_way);
        lpr("%-30s: %s","6. Chat Call Type",nifty.nifstatus & nif_chattype?"Screech":"Beep");
        bitset("7. Log to Printer",syscfg.sysconfig,sysconfig_printer);
        bits("8. Disallow Handles",syscfg.sysconfig,sysconfig_no_alias);
        bitset("9. Phone Number in Logon",syscfg.sysconfig,sysconfig_free_phone);
        bits("0. AutoMessage in Logon",nifty.nifstatus,nif_automsg);
        bitset("A. Last Few Callers in Logon",nifty.nifstatus,nif_lastfew);
        bits("B. Your Info in Logon",nifty.nifstatus,nif_yourinfo);
        bitset("C. Automatic Chat Buffer Open",nifty.nifstatus,nif_autochat);
        bits("D. Local System Security",syscfg.sysconfig,sysconfig_no_local);
        bitset("E. Phone Off-Hook",syscfg.sysconfig,sysconfig_off_hook);
        bits("F. Allow Users to Fast Logon",syscfg.sysconfig,sysconfig_no_xfer);
        bitset("G. Strip Color from Logs",syscfg.sysconfig,sysconfig_shrink_term);
        bits("H. Forced Voting",nifty.nifstatus,nif_forcevote);
        bitset("!. All Uploads to SysOp",syscfg.sysconfig,sysconfig_all_sysop);
#else
        bits(get_string2(51),syscfg.sysconfig,sysconfig_2_way);
        lpr("%-30s: %s","2. Chat Call Type",nifty.nifstatus & nif_chattype?"Screech":"Beep");
        bits(get_string2(53),syscfg.sysconfig,sysconfig_printer);
        bitset(get_string2(54),syscfg.sysconfig,sysconfig_no_alias);
        bits(get_string2(55),syscfg.sysconfig,sysconfig_free_phone);
        bitset(get_string2(59),nifty.nifstatus,nif_autochat);
        bits(get_string2(60),syscfg.sysconfig,sysconfig_no_local);
        bitset(get_string2(61),syscfg.sysconfig,sysconfig_off_hook);
        bits(get_string2(62),syscfg.sysconfig,sysconfig_no_xfer);
        bitset(get_string2(63),syscfg.sysconfig,sysconfig_shrink_term);
        bitset(get_string2(65),syscfg.sysconfig,sysconfig_all_sysop);
#endif
        nl();
        outstr(get_string2(11));
        switch(toupper(getkey())) {
        case 'Q': 
            done=1; 
            break;
        case '1':
            togglebit((long *)&syscfg.sysconfig,sysconfig_2_way);
            break;
        case '2':
            togglebit((long *)&nifty.nifstatus,nif_chattype);
            break;
        case '3':
            togglebit((long *)&syscfg.sysconfig,sysconfig_printer);
            break;
        case '4':
            togglebit((long *)&syscfg.sysconfig,sysconfig_no_alias);
            break;
        case '5':
            togglebit((long *)&syscfg.sysconfig,sysconfig_free_phone);
            break;
        case '6':
            togglebit((long *)&nifty.nifstatus,nif_autochat);
            break;
        case '7':
            togglebit((long *)&syscfg.sysconfig,sysconfig_no_local);
            break;
        case '8':
            togglebit((long *)&syscfg.sysconfig,sysconfig_off_hook);
            break;
        case '9':
            togglebit((long *)&syscfg.sysconfig,sysconfig_no_xfer);
            break;
        case '0':
            togglebit((long *)&syscfg.sysconfig,sysconfig_shrink_term);
            break;
        case 'A':
            togglebit((long *)&syscfg.sysconfig,sysconfig_all_sysop);
            break;
        }
    } 
    while(!done&&!hangup);

}

void varible()
{
    int done=0,i;
    char *chattype[]={
        "TwoColor","Filtered","Rotating"        }
    ,*s;

    do {
        outchr(12);
        lpr("5� 0Variable System Data");
        nl();
        lpr("1. Start Out Menu    : %s",nifty.firstmenu);
        lpr("2. New User Menu     : %s",nifty.newusermenu);
        lpr("3. Echo Character    : %c",nifty.echochar);
        nl();
        lpr("4. Max Waiting Mail  : %d",syscfg.maxwaiting);
        lpr("5. Input Prompt Type : %s",nifty.nifstatus & nif_comment?"Dots":"Blue");
        lpr("6. Maxtrix Active    : %s",nifty.matrixtype?"Yes":"No");
        lpr("7. Lock Out Rate     : %d",nifty.lockoutrate);
        nl();
        lpr("8. Chat Hours Start  : %s ",ctim((double)syscfg.sysoplowtime*60));
        lpr("9. Chat Hours End    : %s ",ctim((double)syscfg.sysophightime*60));
#ifndef DOS
        npr("0. Set Rotating Clrs : SysOp=");
        for(i=0;i<5;i++) {
            ansic(nifty.rotate[i]);
            npr("%d,",i);
        }
        outstr(" User=");
        for(i=0;i<5;i++) {
            ansic(syscfg.dszbatchdl[i]);
            npr("%d,",i);
        }
        backspace();
        nl();
#endif
        lpr("0A. Chat Type         : %s",chattype[nifty.chatcolor]);
        nl();
        lpr("B. Days to Keep Logs : %d",nifty.systemtype);
        lpr("C. QWK Packet Name   : %s",nifty.menudir);
        nl();
        outstr(get_string2(11));
        switch(toupper(getkey())) {
        case 'Q': 
            done=1;
            break;
        case '1':
            getselect(nifty.firstmenu,2,23,sizeof(nifty.firstmenu),1);
            break;
        case '2': 
            getselect(nifty.newusermenu,3,23,sizeof(nifty.newusermenu),1); 
            break;
        case '3': 
            getselect(s,4,23,1,1);
            nifty.echochar=s[0];
            break;
        case '4':
            syscfg.maxwaiting = getselectd(6,23,4);
            break;
        case '5':
            togglebit((long *)&nifty.nifstatus,nif_comment);
            break;
        case '6':
            nifty.matrixtype=!(nifty.matrixtype); 
            break;
        case '7':
            nifty.lockoutrate=getselectd(9,23,20);
            break;
        case '8':
            getselectt(&syscfg.sysoplowtime,11,23,20);
            break;
        case '9':
            getselectt(&syscfg.sysophightime,12,23,20);
            break;
        case '0':
            nl();
            for(i=0;i<5;i++) {
                npr("Color %d: ",i);
                input(s,3);
                if(s[0])
                    nifty.rotate[i]=atoi(s);
            }
            for(i=0;i<5;i++) {
                npr("Color %d: ",i);
                input(s,3);
                if(s[0])
                    syscfg.dszbatchdl[i]=atoi(s);
            }
            break;
        case 'A':
            { int _tmp = nifty.chatcolor; getlist(&_tmp,chattype,3); nifty.chatcolor = _tmp; }
            break;
        case 'B':
            nifty.systemtype=getselectd(16,23,20);
            break;

        case 'C':
            getselect(nifty.menudir,17,23,8,1);
            break;

        }
    } 
    while(!done&&!hangup);

}

void events()
{
    int done=0;
    char s[MAX_PATH_LEN];

    do {
        outchr(12);
        lpr("5� 0Event Manager");
        nl();
        lpr("1. Logon Event     : %s",syscfg.logon_c);
        lpr("2. Logoff Event    : %s",syscfg.upload_c);
        lpr("3. Begin Day Event : %s",syscfg.beginday_c);
        lpr("4. Newuser Event   : %s",syscfg.newuser_c);
        nl();
        lpr("5. Timed Event Execute Time : %s ",ctim((double)syscfg.executetime*60));
        lpr("6. Timed Event File Name    : %s",syscfg.executestr);
        nl();
        outstr(get_string2(11));
        switch(toupper(getkey())) {
        case 'Q': 
            done=1; 
            break;
        case '1': 
            getselect(syscfg.logon_c,2,21,sizeof(syscfg.logon_c),1); 
            break;
        case '2': 
            getselect(syscfg.upload_c,3,21,sizeof(syscfg.upload_c),1); 
            break;
        case '3': 
            getselect(syscfg.beginday_c,4,21,sizeof(syscfg.beginday_c),1); 
            break;
        case '4': 
            getselect(syscfg.newuser_c,5,21,sizeof(syscfg.newuser_c),1); 
            break;
        case '5': 
            getselectt(&syscfg.executetime,7,30,30); 
            break;
        case '6': 
            getselect(syscfg.executestr,8,30,sizeof(syscfg.executestr),1); 
            break;
        }
    } 
    while(!done&&!hangup);
}

void modeminfo()
{
    int done=0;
    char s[MAX_PATH_LEN];

    do {
        outchr(12);
        lpr("5� 0Modem Information");
        nl();
        lpr("1. Com Port     : %d",syscfg.primaryport);
        lpr("2. Interrupt    : %d",syscfg.com_ISR[syscfg.primaryport]);
        lpr("3. Base Address : %x",syscfg.com_base[syscfg.primaryport]);
        nl();
        outstr(get_string2(11));
        switch(toupper(getkey())) {
        case 'Q': 
            done=1; 
            break;
        case '1': 
            syscfg.primaryport = getselectd(2,18,2); 
            break;
        case '2': 
            syscfg.com_ISR[syscfg.primaryport] = getselectd(3,18,2); 
            break;
        case '3': 
            getselect(s,4,18,4,1);
            sscanf(s,"%x",&syscfg.com_base[syscfg.primaryport]);
            break;
        }
    } 
    while(!done&&!hangup);
}

void autoval()
{
    char ar[17],dar[17],res[17],s[161];
    int i,done=0,i1,numed=0;

    strcpy(s,restrict_string);

    do {
        outchr(12);
        pl("5� 0Security Profiles");
        nl();
        for(i1=0;i1<10;i1++) {
            for (i=0; i<=15; i++) {
                if (syscfg.autoval[i1].ar & (1 << i)) ar[i]='A'+i;
                else ar[i]='-';
                if (syscfg.autoval[i1].dar & (1 << i)) dar[i]='A'+i;
                else dar[i]='-';
                if (syscfg.autoval[i1].restrict & (1 << i)) res[i]=s[i];
                else res[i]='-';
            }
            ar[16]=0;
            dar[16]=0;
            res[16]=0;
            sprintf(s,"%d%2d. SL=%3d DL=%3d AR=%s IR=%s R=%s",i1==numed?0:3,i1+1,syscfg.autoval[i1].sl,syscfg.autoval[i1].dsl,ar,dar,res);
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
            syscfg.autoval[i1].sl = getselectd(i1+2,7,3); 
            break;
        case 'D': 
            syscfg.autoval[i1].dsl = getselectd(i1+2,15,3); 
            break;
        case 'A': 
            { int _tmp = syscfg.autoval[i1].ar; setbit(i1+2,21,"ABCDEFGHIJKLMNOP",&_tmp); syscfg.autoval[i1].ar = _tmp; } 
            break;
        case 'I': 
            { int _tmp = syscfg.autoval[i1].dar; setbit(i1+2,41,"ABCDEFGHIJKLMNOP",&_tmp); syscfg.autoval[i1].dar = _tmp; } 
            break;
        case 'R': 
            { int _tmp = syscfg.autoval[i1].restrict; setbit(i1+2,60,restrict_string,&_tmp); syscfg.autoval[i1].restrict = _tmp; } 
            break;
        }
    } 
    while(!done&&!hangup);
}

void archive()
{
    char s[MAX_PATH_LEN];
    int i,done=0,i1=0;

    sprintf(s,"%sarchive.dat",syscfg.datadir);
    i=open(s,O_BINARY|O_RDWR);
    read(i,&xarc[0],8*sizeof(xarc[0]));
    close(i);

    do {
        outchr(12);
        pl("5� 0Archive Configuration");
        nl();
        lpr("3Number 9%d/70",i1);
        nl();
        lpr("1. Extension  : %s",xarc[i1].extension);
        lpr("2. Add to Arc : %s",xarc[i1].arca);
        lpr("3. Extract Arc: %s",xarc[i1].arce);
        lpr("4. View Arc   : %s",xarc[i1].arcl);
        lpr("5. Test Arc   : %s",xarc[i1].arct);
        lpr("6. Comment Arc: %s",xarc[i1].arcc);
        lpr("7. Swap to Run: %s",xarc[i1].attr & xarc_swap?"Yes":"No");
        lpr("8. Ok Level 1 : %d",xarc[i1].ok1);
        lpr("9. Ok Level 1 : %d",xarc[i1].ok2);
        lpr("0. Error 1    : %d",xarc[i1].nk1);
        lpr("A. Error 2    : %d",xarc[i1].nk2);
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
            getselect(xarc[i1].extension,4,16,3,0); 
            break;
        case '2': 
            getselect(xarc[i1].arca,5,16,sizeof(xarc[i1].arca),1); 
            break;
        case '3': 
            getselect(xarc[i1].arce,6,16,sizeof(xarc[i1].arce),1); 
            break;
        case '4': 
            getselect(xarc[i1].arcl,7,16,sizeof(xarc[i1].arcl),1); 
            break;
        case '5': 
            getselect(xarc[i1].arct,8,16,sizeof(xarc[i1].arct),1); 
            break;
        case '6': 
            getselect(xarc[i1].arcc,9,16,sizeof(xarc[i1].arcc),1); 
            break;
        case '7': 
            togglebit((long *)&xarc[i1].attr,xarc_swap);
            break;
        case '8': 
            xarc[i1].ok1 = getselectd(11,16,2);
        case '9': 
            xarc[i1].ok2 = getselectd(12,16,2);
        case '0': 
            xarc[i1].nk1 = getselectd(13,16,2);
        case 'A': 
            xarc[i1].nk2 = getselectd(14,16,2);
        }
    } 
    while(!done&&!hangup);

    sprintf(s,"%sarchive.dat",syscfg.datadir);
    i=open(s,O_BINARY|O_RDWR|O_CREAT,S_IREAD|S_IWRITE);
    write(i,&xarc[0],8*sizeof(xarc[0]));
    close(i);
}

void secleved()
{
    char s[MAX_PATH_LEN];
    int i,done=0,i1=0;

    do {
        outchr(12);
        pl("5� 0Security Level Data");
        nl();
        lpr("3Number 9%d0",i1);
        nl();
        lpr("1. Time Per Day          : %d",syscfg.sl[i1].time_per_day);
        lpr("2. Time Per Call         : %d",syscfg.sl[i1].time_per_logon);
        lpr("3. Max Calls per day     : %d",syscfg.sl[i1].maxcalls);
        lpr("4. Max Emails            : %d",syscfg.sl[i1].emails);
        lpr("5. Max Posts             : %d",syscfg.sl[i1].posts);
        bitset("6. Post Anonymous",syscfg.sl[i1].ability        ,ability_post_anony);
        bitset("7. Email Anonymous",syscfg.sl[i1].ability       ,ability_email_anony);
        bitset("8. Read Anonymous Posts",syscfg.sl[i1].ability  ,ability_read_post_anony);
        bitset("9. Read Anonymous Email",syscfg.sl[i1].ability  ,ability_read_email_anony);
        bitset("0. Posts Need Validation",syscfg.sl[i1].ability,ability_val_net);
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
            syscfg.sl[i1].time_per_day = getselectd(4,27,3); 
            break;
        case '2': 
            syscfg.sl[i1].time_per_logon = getselectd(5,27,3); 
            break;
        case '3': 
            syscfg.sl[i1].maxcalls = getselectd(6,27,3); 
            break;
        case '4': 
            syscfg.sl[i1].emails = getselectd(7,27,3); 
            break;
        case '5': 
            syscfg.sl[i1].posts = getselectd(8,27,3); 
            break;
        case '6': 
            togglebit((long *)&syscfg.sl[i1].ability,ability_post_anony);
            break;
        case '7': 
            togglebit((long *)&syscfg.sl[i1].ability,ability_email_anony);
            break;
        case '8': 
            togglebit((long *)&syscfg.sl[i1].ability,ability_read_post_anony);
            break;
        case '9': 
            togglebit((long *)&syscfg.sl[i1].ability,ability_read_email_anony);
            break;
        case '0': 
            togglebit((long *)&syscfg.sl[i1].ability,ability_val_net);
            break;
        }
    } 
    while(!done&&!hangup);

}

void nued()
{
    int done=0,i;
    char s[MAX_PATH_LEN];

    do {
        outchr(12);
        lpr("5� 0New User Data");
        nl();
        lpr(get_string2(66),nifty.nulevel+1);
        lpr(get_string2(67),nifty.nuinf);
        lpr(get_string2(68),i=syscfg.newusergold);
        lpr(get_string2(69),nifty.nifstatus & nif_nuv?"Active":"InActive");
        lpr(get_string2(70),nifty.nuvyes);
        lpr(get_string2(71),nifty.nuvbad);
        lpr(get_string2(72),nifty.nuvlevel);
        lpr(get_string2(73),nifty.nuvinf);
        lpr(get_string2(74),nifty.nuvsl);
        lpr("0. Accepting New Users       : %s",syscfg.closedsystem?"No":"Yes");
        lpr("A. Maximum Users             : %d",syscfg.maxusers);
        outstr("B. NUV Action                : ");
        switch(nifty.nuvaction) {
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
        lpr("C. NUV Bad Level             : %d",nifty.nuvbadlevel);
        nl();
        outstr(get_string2(11));
        switch(toupper(getkey())) {
        case 'Q': 
            done=1; 
            break;
        case '1': 
            nifty.nulevel=getselectd(2,31,2);
            nifty.nulevel--;
            syscfg.newusersl=syscfg.autoval[nifty.nulevel].sl;
            syscfg.newuserdsl=syscfg.autoval[nifty.nulevel].dsl;
            break;
        case '2': 
            getselect(nifty.nuinf,3,31,8,0); 
            break;
        case '3': 
            syscfg.newusergold=getselectd(4,31,8);
            break;
#ifdef NUV
        case '4': 
            togglebit((long *)&nifty.nifstatus,nif_nuv);
            break;
        case '5': 
            nifty.nuvyes=getselectd(6,31,8);
            break;
        case '6': 
            nifty.nuvbad=getselectd(7,31,8); 
            break;
        case '7': 
            nifty.nuvlevel=getselectd(8,31,8); 
            break;
        case '8': 
            getselect(nifty.nuvinf,9,31,8,0); 
            break;
        case '9': 
            getselect(nifty.nuvsl,10,31,10,0); 
            break;
#endif
        case '0': 
            syscfg.closedsystem=!(syscfg.closedsystem); 
            break;
        case 'A': 
            syscfg.maxusers=getselectd(12,31,8); 
            break;
        case 'C': 
            nifty.nuvbadlevel=getselectd(14,31,8); 
            break;
        case 'B': 
            nifty.nuvaction++;
            if(nifty.nuvaction==3)
                nifty.nuvaction=0;
            break;
        }
    } 
    while(!done&&!hangup);

}

/* FidoNet config editors removed */

void acscfg(void)
{
    int done=0,i;
    char s[MAX_PATH_LEN];
    acsrec acs;

    sprintf(s,"%sacs.dat",syscfg.datadir);
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
    while(!done&&!hangup);

    i=open(s,O_BINARY|O_RDWR|O_CREAT|O_TRUNC,S_IREAD|S_IWRITE);
    write(i,&acs,sizeof(acsrec));
    close(i);

}


#ifndef DOS
void defcoled()
{
    int i;
    userrec u;

    outchr(12);
    userdb_load(1,&u);

    colblock=1;
    change_colors(&u);
    colblock=0;
}
#endif

#ifndef DOS
void config()
#else
void main(int argc, char *argv[])
#endif
{
    int i,done=0;
    FILE *f;
    char s[MAX_PATH_LEN];

#ifndef DOS
    if(!checkpw())
        return;

    topdata=0;
    topscreen();
#else
    {
        cJSON *cfg_root = read_json_file("config.json");
        if (!cfg_root) { printf("config.json not found!\n"); exit(1); }
        json_to_configrec(cfg_root, &syscfg, &nifty);
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
    while(!done&&!hangup);

    {
        cJSON *cfg_root = configrec_to_json(&syscfg, &nifty);
        write_json_file("config.json", cfg_root);
        cJSON_Delete(cfg_root);
    }
}
