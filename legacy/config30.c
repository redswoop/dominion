#pragma hdrstop
#include <stdio.h>
#include "fcns.h"


#ifdef BBS
extern fnetrec fnet;
extern configrec syscfg;
extern niftyrec nifty;
extern int hangup,topdata;
extern userrec thisuser;
extern int usernum,colblock;
#else
fnetrec fnet;
configrec syscfg;
niftyrec nifty;
int hangup=0;

unsigned char getkey()
{
    return(getch());
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

#endif

#pragma warn -sus


void getselect(char *s,int row,int col,int len,int lc)
{
    int i;

#ifdef BBS
    npr("[%d;%dH",row+1,col+1);
    for(i=0;i<len;i++) outchr(32);
    npr("[%d;%dH",row+1,col+1);
#else
    pr("[%d;%dH",row+1,col+1);
    for(i=0;i<len;i++) outchr(32);
    pr("[%d;%dH",row+1,col+1);
#endif
    input1(s,len,lc,1);
}

void getselectt(unsigned short *i,int row,int col,int len)
{
    unsigned int i1;
    char s[41];
    unsigned int h,m;

#ifdef BBS
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
}

void getselectd(int *i,int row,int col,int len)
{
    int i1;
    char s[41];

#ifdef BBS
    npr("[%d;%dH",row+1,col+1);
    for(i1=0;i1<len;i1++) outchr(32);
    npr("[%d;%dH",row+1,col+1);
#else
    pr("[%d;%dH",row+1,col+1);
    for(i=0;i<len;i++) outchr(32);
    pr("[%d;%dH",row+1,col+1);
#endif
    input(s,len);
    *i=atoi(s);
}

void togglebit(int *byte,int bit)
{
    if(*byte & bit) *byte ^=bit;
    else *byte |=bit;
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
            case 'Q': done=1; break;
            default : ch=-1;
                      for (i1=0; i1<16; i1++)
                        if (i==bits[i1])
                          ch=i1;
                      if (ch>-1)
                        (*byte) ^= (1 << ch);
                      break;
        }
    } while(!done&&!hangup);
}

void bitset(char *msg,int byte,int bit)
{
   lpr("%-30s: %s",msg,byte & bit?"Yes":"No");
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

void namepath()
{
    int done=0;
    char s[81];

    do {
        outchr(12);
        lpr("5þ 0System Info And Paths");
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
        outstr("5Select (?=Help) ");
        switch(toupper(getkey())) {
            case 'Q': done=1; break;
            case '1': getselect(syscfg.systemname,2,23, sizeof(syscfg.systemname),1); break;
            case '2':
#ifdef BBS
                      go(4,24); inputfone(syscfg.systemphone); break;
#else
                      getselect(syscfg.systemphone,3,23,12,0); break;
#endif
            case '3': getselect(syscfg.systempw,4,23,   sizeof(syscfg.systempw),0); break;
            case '4': getselect(syscfg.sysopname,5,23,  sizeof(syscfg.sysopname),1); break;
            case '5': getselect(nifty.matrix,6,23,      sizeof(nifty.matrix),0); break;
            case '6': getselect(syscfg.newuserpw,7,23,  sizeof(syscfg.newuserpw),0); break;
            case '7': getselect(nifty.lockoutpw,8,23,  sizeof(nifty.lockoutpw),0); break;
            case '8': getselect(syscfg.datadir,10,23,    sizeof(syscfg.datadir),1); break;
            case '9': getselect(syscfg.gfilesdir,11,23, sizeof(syscfg.gfilesdir),1); break;
            case '0': getselect(syscfg.msgsdir,12,23,   sizeof(syscfg.msgsdir),1); break;
            case 'A': getselect(syscfg.menudir,13,23,    sizeof(syscfg.menudir),1); break;
            case 'B': getselect(syscfg.batchdir,14,23,  sizeof(syscfg.batchdir),1); break;
            case 'C': getselect(syscfg.tempdir,15,23,   sizeof(syscfg.tempdir),1); break;
            case 'D': getselect(syscfg.dloadsdir,16,23, sizeof(syscfg.dloadsdir),1); break;
        }
    } while(!done&&!hangup);
}

void flagged()
{
    int done=0;

    do {
        outchr(12);
        lpr("5þ 0Flagged Information");
        nl();
        bitset("1. File Ratio",nifty.nifstatus,nif_ratio);
        bitset("2. File Point Ratio",nifty.nifstatus,nif_fpts);
        bitset("3. Post Call Ratio",nifty.nifstatus,nif_pcr);
        bitset("4. Auto UL Validation",nifty.nifstatus,nif_autocredit);
        bitset("!. All Uploads to SysOp",syscfg.sysconfig,sysconfig_all_sysop);
        nl();
        bitset("5. 2Way Default",syscfg.sysconfig,sysconfig_2_way);
        lpr("%-30s: %s","6. Chat Call Type",nifty.nifstatus & nif_chattype?"Screech":"Beep");
        nl();
        bitset("7. Log to Printer",syscfg.sysconfig,sysconfig_printer);
        bitset("8. Disallow Handles",syscfg.sysconfig,sysconfig_no_alias);
        bitset("9. Phone Number in Logon",syscfg.sysconfig,sysconfig_free_phone);
        bitset("0. AutoMessage in Logon",nifty.nifstatus,nif_automsg);
        bitset("A. Last Few Callers in Logon",nifty.nifstatus,nif_lastfew);
        bitset("B. Your Info in Logon",nifty.nifstatus,nif_yourinfo);
        bitset("C. Automatic Chat Buffer Open",nifty.nifstatus,nif_autochat);
        bitset("D. Local System Security",syscfg.sysconfig,sysconfig_no_local);
        bitset("E. Phone Off-Hook",syscfg.sysconfig,sysconfig_off_hook);
        bitset("F. Allow Users to Fast Logon",syscfg.sysconfig,sysconfig_no_xfer);
        bitset("G. Strip Color from Logs",syscfg.sysconfig,sysconfig_shrink_term);
        nl();
        outstr("5Select (Q=Quit) 0");
        switch(toupper(getkey())) {
            case 'Q': done=1; break;
            case '1': togglebit(&nifty.nifstatus,nif_ratio); break;
            case '2': togglebit(&nifty.nifstatus,nif_fpts); break;
            case '3': togglebit(&nifty.nifstatus,nif_pcr); break;
            case '4': togglebit(&nifty.nifstatus,nif_autocredit); break;
            case '5': togglebit(&syscfg.sysconfig,sysconfig_2_way); break;
            case '!': togglebit(&syscfg.sysconfig,sysconfig_all_sysop); break;
            case '6': togglebit(&nifty.nifstatus,nif_chattype); break;
            case '7': togglebit(&syscfg.sysconfig,sysconfig_printer); break;
            case '8': togglebit(&syscfg.sysconfig,sysconfig_no_alias); break;
            case '9': togglebit(&syscfg.sysconfig,sysconfig_free_phone); break;
            case '0': togglebit(&nifty.nifstatus,nif_automsg); break;
            case 'A': togglebit(&nifty.nifstatus,nif_lastfew); break;
            case 'B': togglebit(&nifty.nifstatus,nif_yourinfo); break;
            case 'C': togglebit(&nifty.nifstatus,nif_autochat); break;
            case 'D': togglebit(&syscfg.sysconfig,sysconfig_no_local); break;
            case 'E': togglebit(&syscfg.sysconfig,sysconfig_off_hook); break;
            case 'F': togglebit(&syscfg.sysconfig,sysconfig_no_xfer); break;
            case 'G': togglebit(&syscfg.sysconfig,sysconfig_shrink_term); break;
        }
    } while(!done&&!hangup);

}

void varible()
{
    int done=0,i;
    char *chattype[]={"TwoColor","Filtered","Rotating"},*s;

    do {
        outchr(12);
        lpr("5þ 0Variable System Data");
        nl();
        lpr("1. Start Out Menu    : %s",nifty.firstmenu);
        lpr("2. New User Menu     : %s",nifty.newusermenu);
        lpr("3. Echo Character    : %c",nifty.echochar);
        nl();
        lpr("4. Maximum  Users    : %d",syscfg.maxusers);
        lpr("5. Max Waiting Mail  : %d",syscfg.maxwaiting);
        nl();
        lpr("6. KiloByte Ratio    : %-5.3f",syscfg.req_ratio);
        lpr("7. File Point Ratio  : 1 to %d ",nifty.fptsratio);
        lpr("8. Post Call Ratio   : %-5.3f",syscfg.post_call_ratio);
        nl();
        lpr("9. Input Prompt Type : %s",nifty.nifstatus & nif_comment?"Dots":"Blue");
        lpr("0. Maxtrix Active    : %s",nifty.matrixtype?"Yes":"No");
        lpr("A. Lock Out Rate     : %d",nifty.lockoutrate);
        nl();
        lpr("B. Chat Hours Start  : %s ",ctim((double)syscfg.sysoplowtime*60));
        lpr("C. Chat Hours End    : %s ",ctim((double)syscfg.sysophightime*60));
        nl();
#ifdef BBS
        npr("D. Set Rotating Clrs : SysOp=");
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
        lpr("0E. Chat Type         : %s",chattype[nifty.chatcolor]);
       outstr("F. New Uploads Area  : ");
        if(syscfg.newuploads<255) npr("%d",syscfg.newuploads);
        else lpr("None.");
        nl();
        nl();
        outstr("5Select (?=Help) ");
        switch(toupper(getkey())) {
            case 'Q': done=1; break;
            case '1': getselect(nifty.firstmenu,2,23,sizeof(nifty.firstmenu),1); break;
            case '2': getselect(nifty.newusermenu,3,23,sizeof(nifty.newusermenu),1); break;
            case '3': getselect(s,4,23,1,1);
                      nifty.echochar=s[0];
                      break;
            case '4': getselectd(&syscfg.maxusers,6,23,4); break;
            case '5': getselectd(&syscfg.maxwaiting,7,23,4); break;
            case '6': getselect(s,9,23,5,1);
                      sscanf(s,"%f",&syscfg.req_ratio);
                      break;
            case '7': getselectd(&nifty.fptsratio,10,23,20); break;
            case '8': getselect(s,11,23,5,1);
                      sscanf(s,"%f",&syscfg.post_call_ratio);
                      break;
            case '9': togglebit(&nifty.nifstatus,nif_comment); break;
            case '0': nifty.matrixtype=!(nifty.matrixtype); break;
            case 'A': getselectd(&nifty.lockoutrate,15,23,20); break;
            case 'B': getselectt(&syscfg.sysoplowtime,17,23,20); break;
            case 'C': getselectt(&syscfg.sysophightime,18,23,20); break;
            case 'D': nl();
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
           case 'E': getlist(&nifty.chatcolor,chattype,3); break;
           case 'F':
#ifdef BBS
                     inputdat("New Uploads Area, [CR] to unset",s,3,0);
#else
                     input(s,3);
#endif
                     if(s[0]) {
                        syscfg.newuploads=atoi(s);
                    } else syscfg.newuploads=255;
                    break;
        }
    } while(!done&&!hangup);

}

void events()
{
    int done=0;
    char s[81];

    do {
        outchr(12);
        lpr("5þ 0Event Manager");
        nl();
        lpr("1. Logon Event     : %s",syscfg.logon_c);
        lpr("2. Logoff Event    : %s",syscfg.upload_c);
        lpr("3. Begin Day Event : %s",syscfg.beginday_c);
        lpr("4. Newuser Event   : %s",syscfg.newuser_c);
        nl();
        lpr("5. Timed Event Execute Time : %s ",ctim((double)syscfg.executetime*60));
        lpr("6. Timed Event File Name    : %s",syscfg.executestr);
        nl();
        outstr("5Select (?=Help) ");
        switch(toupper(getkey())) {
            case 'Q': done=1; break;
            case '1': getselect(syscfg.logon_c,2,21,sizeof(syscfg.logon_c),1); break;
            case '2': getselect(syscfg.upload_c,3,21,sizeof(syscfg.upload_c),1); break;
            case '3': getselect(syscfg.beginday_c,4,21,sizeof(syscfg.beginday_c),1); break;
            case '4': getselect(syscfg.newuser_c,5,21,sizeof(syscfg.newuser_c),1); break;
            case '5': getselectt(&syscfg.executetime,7,30,30); break;
            case '6': getselect(syscfg.executestr,8,30,sizeof(syscfg.executestr),1); break;
        }
    } while(!done&&!hangup);
}

void modeminfo()
{
    int done=0;
    char s[81];

    do {
        outchr(12);
        lpr("5þ 0Modem Information");
        nl();
        lpr("1. Com Port     : %d",syscfg.primaryport);
        lpr("2. Interrupt    : %d",syscfg.com_ISR[syscfg.primaryport]);
        lpr("3. Base Address : %x",syscfg.com_base[syscfg.primaryport]);
        nl();
        outstr("5Select (?=Help) ");
        switch(toupper(getkey())) {
            case 'Q': done=1; break;
            case '1': getselectd(&syscfg.primaryport,2,18,2); break;
            case '2': getselectd(&syscfg.com_ISR[syscfg.primaryport],3,18,2); break;
            case '3': getselect(s,4,18,4,1);
                      sscanf(s,"%x",&syscfg.com_base[syscfg.primaryport]);
                      break;
        }
    } while(!done&&!hangup);
}

void autoval()
{
    char ar[17],dar[17],res[17],s[16];
    int i,done=0,i1,numed=0;

    strcpy(s,restrict_string);

    do {
        outchr(12);
        pl("5þ 0Security Profiles");
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
#ifndef BBS
            npr("%d%2d. SL=%3d DL=%3d AR=%s IR=%s R=%s",i1==numed?0:3,i1+1,syscfg.autoval[i1].sl,syscfg.autoval[i1].dsl,ar,dar,res);
#else
            lpr("%d%2d. SL=%3d DL=%3d AR=%s IR=%s R=%s",i1==numed?0:3,i1+1,syscfg.autoval[i1].sl,syscfg.autoval[i1].dsl,ar,dar,res);
#endif
        }
        nl();
        outstr("5Select (Q=Quit, #,],[ to Select)0 ");
        i1=numed;
        switch(toupper(getkey())) {
            case 'Q': done=1; break;
            case ']': if(numed<9) numed++; break;
            case '[': if(numed>0) numed--; break;
            case 'S': getselectd(&syscfg.autoval[i1].sl,i1+2,7,3); break;
            case 'D': getselectd(&syscfg.autoval[i1].dsl,i1+2,15,3); break;
            case 'A': setbit(i1+2,21,"ABCDEFGHIJKLMNOP",&syscfg.autoval[i1].ar); break;
            case 'I': setbit(i1+2,41,"ABCDEFGHIJKLMNOP",&syscfg.autoval[i1].dar); break;
            case 'R': setbit(i1+2,60,restrict_string,&syscfg.autoval[i1].restrict); break;
        }
  } while(!done&&!hangup);
}

void archive()
{
    char s[81];
    int i,done=0,i1=0;

    do {
        outchr(12);
        pl("5þ 0Archive Configuration");
        nl();
        lpr("3Number 9%d0",i1);
        nl();
        lpr("1. Extension  : %s",syscfg.arcs[i1].extension);
        lpr("2. Add to Arc : %s",syscfg.arcs[i1].arca);
        lpr("3. Extract Arc: %s",syscfg.arcs[i1].arce);
        lpr("4. View Arc   : %s",syscfg.arcs[i1].arcl);
        lpr("5. Test Arc   : %s",nifty.arc[i1].arct);
        lpr("6. Comment Arc: %s",nifty.arc[i1].arcc);
        nl();
        pl(" %1 = Archive File Name");
        pl(" %2 = File to Touch");
        pl(" %5 = Comment File Name");
        nl();
        outstr("5Select (Q=Quit, [,])0 ");
        switch(toupper(getkey())) {
            case 'Q': done=1; break;
            case '[': if(i1) i1--; break;
            case ']': if(i1<3) i1++; break;
            case '1': getselect(syscfg.arcs[i1].extension,4,16,3,0); break;
            case '2': getselect(syscfg.arcs[i1].arca,5,16,sizeof(syscfg.arcs[i1].arca),1); break;
            case '3': getselect(syscfg.arcs[i1].arce,6,16,sizeof(syscfg.arcs[i1].arce),1); break;
            case '4': getselect(syscfg.arcs[i1].arcl,7,16,sizeof(syscfg.arcs[i1].arcl),1); break;
            case '5': getselect(nifty.arc[i1].arct,8,16,sizeof(nifty.arc[i1].arct),1); break;
            case '6': getselect(nifty.arc[i1].arcc,9,16,sizeof(nifty.arc[i1].arcc),1); break;
        }
    } while(!done&&!hangup);
}

void secleved()
{
    char s[81];
    int i,done=0,i1=0;

    do {
        outchr(12);
        pl("5þ 0Security Level Data");
        nl();
        lpr("3Number 9%d0",i1);
        nl();
        lpr("1. Time Per Day          : %d",syscfg.sl[i1].time_per_day);
        lpr("2. Time Per Call         : %d",syscfg.sl[i1].time_per_logon);
        lpr("3. Max Messages Read     : %d",syscfg.sl[i1].messages_read);
        lpr("4. Max Emails            : %d",syscfg.sl[i1].emails);
        lpr("5. Max Posts             : %d",syscfg.sl[i1].posts);
        bitset("6. Post Anonymous",syscfg.sl[i1].ability        ,ability_post_anony);
        bitset("7. Email Anonymous",syscfg.sl[i1].ability       ,ability_email_anony);
        bitset("8. Read Anonymous Posts",syscfg.sl[i1].ability  ,ability_read_post_anony);
        bitset("9. Read Anonymous Email",syscfg.sl[i1].ability  ,ability_read_email_anony);
        bitset("0. Posts Need Validation",syscfg.sl[i1].ability,ability_val_net);
        nl();
        outstr("5Select (Q=Quit,J=Jump, [,])0 ");
        switch(toupper(getkey())) {
            case 'Q': done=1; break;
            case 'J':
#ifdef BBS
            nl(); inputdat("To Which",s,3,0); 
#else
                  input(s,3);
#endif
                      if(s[0]) i1=atoi(s); break;
            case '[': if(i1) i1--; break;
            case ']': if(i1<255) i1++; break;
            case '{': if(i1-10) i1-=10; if(i1<0) i1=0; break;
            case '}': if(i1+10<255) i1+=10; if(i1>255) i1=255; break;
            case '1': getselectd(&syscfg.sl[i1].time_per_day,4,27,3); break;
            case '2': getselectd(&syscfg.sl[i1].time_per_logon,5,27,3); break;
            case '3': getselectd(&syscfg.sl[i1].messages_read,6,27,3); break;
            case '4': getselectd(&syscfg.sl[i1].emails,7,27,3); break;
            case '5': getselectd(&syscfg.sl[i1].posts,8,27,3); break;
            case '6': togglebit(&syscfg.sl[i1].ability,ability_post_anony);        break;
            case '7': togglebit(&syscfg.sl[i1].ability,ability_email_anony);       break;
            case '8': togglebit(&syscfg.sl[i1].ability,ability_read_post_anony);   break;
            case '9': togglebit(&syscfg.sl[i1].ability,ability_read_email_anony);  break;
            case '0': togglebit(&syscfg.sl[i1].ability,ability_val_net);           break;
        }
    } while(!done&&!hangup);

}

void nued()
{
    int done=0,i;
    char s[81];

    do {
        outchr(12);
        lpr("5þ 0New User Data");
        nl();
        lpr("1. Profile for New User      : %d",nifty.nulevel+1);
        lpr("2. NewUser Infoform          : %s",nifty.nuinf);
        lpr("3. New User File Points      : %d",i=syscfg.newusergold);
        lpr("14. NUV                       : %s",nifty.nifstatus & nif_nuv?"Active":"InActive");
        lpr("15. NUV Max Votes             : %d",nifty.nuvtotal);
        lpr("16. Max NUV No Votes          : %d",nifty.nuvbad);
        lpr("17. Profile for NUV           : %d",nifty.nuvlevel);
        lpr("18. NUV Infoform              : %s",nifty.nuvinf);
        nl();
        outstr("5Select (?=Help) ");
        switch(toupper(getkey())) {
            case 'Q': done=1; break;
            case '1': getselectd(&nifty.nulevel,2,31,2);
                      nifty.nulevel--;
                      syscfg.newusersl=syscfg.autoval[nifty.nulevel].sl;
                      syscfg.newuserdsl=syscfg.autoval[nifty.nulevel].dsl;
                      break;
            case '2': getselect(nifty.nuinf,3,31,8,0); break;
            case '3': getselectd(&i,4,31,8);
                      syscfg.newusergold=i;
                      break;
/*
            case '4': togglebit(&nifty.nifstatus,nif_nuv); break;
            case '5': getselectd(&nifty.nuvtotal,6,31,8); break;
            case '6': getselectd(&nifty.nuvbad,7,31,8); break;
            case '7': getselectd(&nifty.nuvlevel,8,31,8); break;
            case '8': getselect(nifty.nuvinf,9,31,8,0); break;
            */
          }
    } while(!done&&!hangup);

}

void fidocfg()
{
    int done=0;
    char s[81];

    do {
        outchr(12);
        lpr("5þ 0FidoNet Information");
        nl();
        lpr("1. Main Address : %d:%d/%d.%d",fnet.ad.zone,fnet.ad.net,fnet.ad.node,fnet.ad.point);
        lpr("2. NetWork Name : %s",fnet.netname);
        lpr("3. Origin 1     : %s",fnet.origin1);
        lpr("4. Origin 2     : %s",fnet.origin2);
        nl();
        outstr("5 Select (Q=Quit)0  ");
        switch(toupper(getkey())) {
            case 'Q': done=1; break;
            case '1': nl(); nl();
                      npr("Enter Main Zone: ");
                      input(s,3); if(s[0]) fnet.ad.zone=atoi(s);
                      npr("Enter Main Net: ");
                      input(s,3); if(s[0]) fnet.ad.net=atoi(s);
                      npr("Enter Main Node: ");
                      input(s,3); if(s[0]) fnet.ad.node=atoi(s);
                      npr("Enter Main Point: ");
                      input(s,3); if(s[0]) fnet.ad.point=atoi(s);
                      break;
            case '2': nl(); nl(); npr("3NetWork Name\r\n5: ");
#ifdef BBS
                      inputl(fnet.netname,71);
#else
                      input(fnet.netname,71);
#endif
                      break;
            case '3': nl(); nl(); npr("3Origin Line 1\r\n5: ");
#ifdef BBS
                      inputl(fnet.origin1,71);
#else
                      input(fnet.netname,71);
#endif
                       break;
            case '4': nl(); nl(); npr("3Origin Line 2\r\n5: ");
#ifdef BBS
                      inputl(fnet.origin2,71);
#else
                      input(fnet.netname,71);
#endif
                       break;
          }
    } while(!done&&!hangup);

}
  
#ifdef BBS
void defcoled()
{
    int i;
    userrec u;

    outchr(12);
    read_user(1,&u);

    colblock=1;
    change_colors(&u);
    colblock=0;
}
#endif

#ifdef BBS
void config()
#else
void main(int argc, char *argv[])
#endif
{
    int i,done=0;
    FILE *f;

#ifdef BBS
    if(!checkpw())
        return;

    topdata=0;
    topscreen();
#else
    f=fopen("config.dat","rb");
    fread(&syscfg,sizeof(configrec),1,f);
    fread(&nifty,sizeof(niftyrec),1,f);
    fclose(f);
    f=fopen("fnet.dat","rb");
    fread(&fnet,sizeof(fnetrec),1,f);
    fclose(f);
#endif
    do {
        outchr(12);
        pl("5þ 0Dominion System Configuration");
        nl();
#ifdef BBS
        printmenu(19);
#else
        pl("71.0 Names and Paths                 72.0 Flagged Info");
        pl("73.0 Varible System Data             74.0 Event Manager");
        pl("75.0 Modem Info                      76.0 AutoVal Data");
        pl("77.0 Archive Configuration           78.0 FullScreen Editors");
        pl("79.0 Security Level Data             70.0 FidoNet Configuration");
 
#endif
        nl();
        outstr("5Select (?=Help) ");
        switch(toupper(getkey())) {
            case 'Q': done=1; break;
            case '1': namepath(); break;
            case '2': flagged(); break;
            case '3': varible(); break;
            case '4': events(); break;
            case '5': modeminfo(); break;
            case '6': autoval(); break;
            case '7': archive(); break;
            case '8': nued(); break;
            case '9': secleved(); break;
            case '0': fidocfg(); break;
#ifdef BBS
            case 'A': defcoled(); break;
#endif
        }
    } while(!done&&!hangup);

    f=fopen("config.dat","wb");
    fwrite(&syscfg,sizeof(configrec),1,f);
    fwrite(&nifty,sizeof(niftyrec),1,f);
    fclose(f);
    f=fopen("fnet.dat","wb");
    fwrite(&fnet,sizeof(fnetrec),1,f);
    fclose(f);
}
