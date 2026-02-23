#include "platform.h"
#include "fcns.h"
#include "session.h"
#include "system.h"
#pragma hdrstop
/* sess.menuat now in vars.h (Phase B0) */
#include <stdarg.h>

#ifdef PD


static auto& sys = System::instance();
static auto& sess = Session::instance();

char menutitles[4][20],ml[4][20];
int curitem=0,curtitle=0,numtitles=0,numitems[4],usepldns=0;
/* sess.pp, sess.tg, sess.maxcmd now in vars.h (Phase B0) */

void popup(char *fn);
int usepop;
#endif

void getcmdtype(void)
{
    menurec mm;

    nl();
    inputdat("Type",mm.type,2,0);
    nl();
    inputdat("Parameters",mm.ms,40,1);
    ex(mm.type,mm.ms);
}



void logtypes(char type,char *fmt, ...)
{
    va_list ap;
    char s[512],s1[MAX_PATH_LEN];

    va_start(ap, fmt);
    vsprintf(s, fmt, ap);
    va_end(ap);

    switch(type) {
    case 0: 
        strcpy(s1,"7�7>"); 
        break;
    case 1: 
        strcpy(s1,"5�5�"); 
        break;
    case 2: 
        strcpy(s1,"1�1>"); 
        break;
    case 3: 
        strcpy(s1,"2�2�"); 
        break;
    case 4: 
        strcpy(s1,"3�3>"); 
        break;
    case 5: 
        strcpy(s1,"9#9#9#"); 
        break;
    }

    strcat(s1,"0 ");
    strcat(s1,s);
    if(type==5) sl1(0,s1);
    else
        sysoplog(s1);
}



void badcommand(char onf,char tw)
{
    char s[MAX_PATH_LEN];

    nl();
    sprintf(s,"2�2� 0Invalid Command Type %c%c",onf,tw);
    sysoplog(s);
    pl(s);
    nl();
}


void matrixcmd(char type)
{
    switch(type) {
    case 'C': 
        checkmatrixpw(); 
        break;
    case 'L': 
        getmatrixpw(); 
        break;
    case 'N': 
        nl();
        npr("5Logon as New? ");
        if(yn())
            newuser();
        break;
    default: 
        badcommand('W',type);
    }
}

void amsgcommand(char type)
{
    switch(type) {
    case 'W': 
        write_automessage(); 
        break;
    case 'R': 
        read_automessage();
#ifdef PD
        if(usepldns) pausescr();
#endif
        break;
    case 'A':
        if(sys.status.amsguser)
            email(sys.status.amsguser,"Reply to AutoMessage",1);
        break;
    default: 
        badcommand('J',type);
    }
}



void hangupcmd(char type,char ms[40])
{
    if(sess.numbatchdl) {
        outstr(get_string(78));
        if(!yn()) return;
    }
    switch(type) {
    case 'H': 
        hangup=1; 
        break;
    case 'A':
    case 'L':
    case 'C': 
        nl();
        outstr(ms);
        if(yn()) {
            if(type=='C'||type=='L') {
                outstr("5Leave Feedback to SysOp? ");
                if(yn()) {
                    //                    strcpy(irt,"LogOff Feedback.");
                }
                nl();
                if(type=='L') {
                    outstr("5Leave Message to Next User? ");
                    if(yn()) {
                        amsgcommand('W');
                    }
                }
            }
            printfile("logoff");
            hangup=1;
        }
        break;
    default: 
        badcommand('I',type);
    }
}

void sysopcmd(char type,char ms[41])
{
    switch(type)
    {
    case 'B': 
        logtypes(3,"Edited Message Areas");
        boardedit(); 
        break;
    case '-': 
        glocolor(); 
        break;
    case 'P': 
        logtypes(3,"Edited Configuration");
        config(); 
        break;
    case 'F': 
        logtypes(3,"Edited Directories");
        diredit(); 
        break;
    case 'M': 
        logtypes(3,"Read All Mail");
        break;
    case 'H': 
        logtypes(3,"Changed Users");
        chuser(); 
        break;
    case 'C': 
        { static char _sysop_avail_flag = 0;
        _sysop_avail_flag ^= 0x10;
        pl(_sysop_avail_flag & 0x10 ?
        (char *)"Sysop now unavailable" : (char *)"Sysop now available"); }
        logtypes(3,"Changed Chat Availability");
        topscreen();
        break;
    case 'I':
        //        voteprint();
        break;
    case 'U': 
        logtypes(3,"Edited Users");
        uedit(sess.usernum); 
        break;
    case 'V': 
        logtypes(3,"Editing Voting");
        //                     ivotes(); 
        break;
    case 'Z': 
        zlog(); 
        break;
    case 'E': 
        logtypes(3,"Edited Strings");
        if(ms[0]) edstring(atoi(ms));
        else edstring(0); 
        break;
    case 'R': 
        reset_files(1); 
        break;
    case 'X': 
        logtypes(3,"Edited Protocols");
        protedit(); 
        break;
    case 'L': 
        logtypes(3,"Edited Conferences");
        confedit(); 
        break;
    case 'O': 
        viewlog(); 
        break;
    case '#': 
        logtypes(3,"Edited Menus");
        if(ms[0]=='!') menued(sess.menuat);
        else menu("");
        break;
    default: 
        badcommand('S',type);
    }
}


#ifdef PD

char *retfrompldn;
#define myxy(x,y) npr("[%d;%dH",x,y)

int pmmkey(char *s)
{
    static char cmd1[10],cmd2[MAX_PATH_LEN]; unsigned char ch;
    int i,i1,i2,p;

    do {
        do {
            ch=getkey();
            if(ch==';') ch=0;
        } 
        while ((((ch<27) && (ch!=13)) || (ch>126)) && (hangup==0));
        if(ch=='2') return -2;
        else if(ch=='8') return -1;
        else if(ch=='4') return -4;
        else if(ch=='6') return -3;
        else if(ch==27) {
            ch=getkey();
            ch=getkey();
            switch(ch) {
            case 'A': 
                return -1;
            case 'B': 
                return -2;
            case 'C': 
                return -3;
            case 'D': 
                return -4;
            case 'H': 
                return -5;
            case 'K': 
                return -6;
            default: 
                return 0;
            }
        }
        ch=toupper(ch);
        outchr(ch);
        if (ch==13)
            cmd1[0]=0;
        else
            cmd1[0]=ch;
        cmd1[1]=0;
        p=0;
        if (p) {
            do {
                ch=getkey();
            } 
            while ((((ch<' ') && (ch!=13) && (ch!=8)) || (ch>126)) && (hangup==0));
            ch=toupper(ch);
            if (ch==13) {
                strcpy(s,cmd1);
                return(0);
            } 
            else
                if (ch==8) {
                backspace();
            } 
            else {
                cmd1[1]=ch;
                cmd1[2]=0;
                outchr(ch);
                if (ch=='/') {
                    outstr("\b\b  \b\b");
                    input(cmd2,50);
                    strcpy(s,cmd2);
                    return 0;
                }
                strcpy(s,cmd1);
                return(0);
            }
        } 
        else {
            strcpy(s,cmd1);
            return 0;
        }
    } 
    while (hangup==0);

    cmd1[0]=0;
    strcpy(s,cmd1);
    return 0;
}


void readmenup()
{
    int i,i1;
    int comn=0,done;


    numtitles=0;
    for(i=0;i<5;i++)
        numitems[i]=0;
    curitem=0;
    curtitle=0;
    memset(&ml[0][0],0,80);

    i=0;
    strcpy(menutitles[i++],sess.tg[0].desc);

    do {
        if(!(sess.tg[i].attr & command_pulldown)&&!(sess.tg[i].attr & command_title)&&!(sess.tg[i].attr & command_hidden)&&comn<20) {
            ml[numtitles][comn++]=i;
        } 
        else if(!(sess.tg[i].attr & command_hidden)&&numtitles<4) {
            numitems[numtitles]=comn;
            comn=0;
            numtitles++;
            strcpy(menutitles[numtitles],sess.tg[i].desc);
        }
    } 
    while(i++<sess.maxcmd);

    numitems[numtitles]=comn-1;
    numtitles++;
}


void bar(int where)
{
    int i;

    myxy(where,1);
    outstr("[K�[79C�");
}

void drawheader(void)
{
    int i;

    bar(1);
    myxy(1,2);
    for(i=0;i<numtitles;i++) {
        if(i==curtitle) npr(""); 
        else npr("");
        npr("%-19.19s�",menutitles[i]);
    }
    ansic(0);
}


void pldn(void)
{
    int done=0,draw=4,lastnum=0;
    int i,ch,x,y,r,slen,tlen;
    char s[212],fmt[MAX_PATH_LEN],fmto[MAX_PATH_LEN],s1[212],desc[MAX_PATH_LEN],key[20];
    FILE *f;

    sprintf(s,"%s%s.fmt",sys.cfg.menudir,"pulldown");
    f=fopen(s,"rt");
    if (!f) return;

    fgets(s,211,f);
    fgets(s,211,f);
    fgets(fmt,211,f);
    filter(fmt,'\n');
    fgets(fmto,211,f);
    filter(fmto,'\n');
    fgets(s,211,f);
    slen=atoi(s);
    fclose(f);


    do {
        if(draw==1||draw==4) {
            if(draw==4)
                outchr(12);
            drawheader();
            myxy(2,1);
            if(draw!=4)
                for(i=0;i<lastnum+2;i++)
                    pl("[K");
            x=20*curtitle;
            if(curtitle==3) x--;
            y=3;
            makerembox(x,y-1,y+numitems[curtitle]-1,"pulldown");
            for(i=0;i<numitems[curtitle];i++) {
                myxy(y+i,x);
                if(i==curitem)
                    strcpy(s1,fmt);
                else
                    strcpy(s1,fmto);

                aligncmd1(sess.tg[ml[curtitle][i]].desc,key,desc);
                tlen=slen;
                if(strlen(key)>1)
                    tlen-=(strlen(key)-1);
                stuff_in(s,s1,key,makelen(desc,tlen),sess.tg[ml[curtitle][i]].desc,"","");
                npr(s);
            }

            if(draw==4) {
                bar(22);
                myxy(21,0);
                ansic(0);
                outstr(sess.pp.prompt2);
                draw=0;
            }
        }
        if(draw==2||draw==3) {
            if(draw==2)
                i=-1;
            else
                i=1;

            aligncmd1(sess.tg[ml[curtitle][curitem+i]].desc,key,desc);
            tlen=slen;
            if(strlen(key)>1)
                tlen-=(strlen(key)-1);

            myxy(curitem+y+i,x);
            stuff_in(s,fmto,key,makelen(desc,tlen),sess.tg[ml[curtitle][curitem+i]].desc,"","");
            npr(s);

            myxy(curitem+y,x);

            aligncmd1(sess.tg[ml[curtitle][curitem]].desc,key,desc);
            tlen=slen;
            if(strlen(key)>1)
               tlen-=(strlen(key)-1);

            stuff_in(s,fmt,key,makelen(desc,tlen),sess.tg[ml[curtitle][curitem]].desc,"","");
            npr(s);
        }
        r=pmmkey(retfrompldn);
        if(r<0) {
            switch(r) {
            case -2: 
                if(curitem<numitems[curtitle]-1) {
                    curitem++;
                    draw=2;
                }
                break;
            case -1: 
                if(curitem>0) {
                    curitem--;
                    draw=3;
                }
                break;
            case -4: 
                if(curtitle>0) {
                    lastnum=numitems[curtitle];
                    curtitle--;
                    curitem=0;
                    draw=1;
                }
                break;
            case -3: 
                if(curtitle<numtitles-1) {
                    lastnum=numitems[curtitle];
                    curtitle++;
                    curitem=0;
                    draw=1;
                }
                break;
            }
        } 
        else {
            if(!retfrompldn[0]) {
                npr("[24;1H");
                nl();
                strcpy(retfrompldn,sess.tg[ml[curtitle][curitem]].key);
                ansic(0);
                return;
            } 
            else if(retfrompldn[0]=='~') {
                usepldns=0;
                strcpy(retfrompldn,"");
                done=1;
            } 
            else {
                npr("[24;1H");
                nl();
                ansic(0);
                return;
            }
        }
    } 
    while(!done&&!hangup);
    ansic(0);
    return;
}


#endif

void configpldn(int config)
{
#ifdef PD
    if(config==1) {
        nl();
        npr("5Do you want Pulldowns automatically when you logon? ");
        if(yn())
            sess.user.sysstatus |= sysstatus_clr_scrn;
        else
            sess.user.sysstatus ^= sysstatus_clr_scrn;
        return;
    }

    if(config==2||config==3) {
        if(sess.user.sysstatus & sysstatus_clr_scrn) {
            if(!incom) {
                sess.topdata=0;
                topscreen();
            }
            usepldns=1;
            return;
        }
        if(config==3) return;
    }

    if(okansi()) {
        printfile("pulldown");
        nl();
        outstr(get_string(31));
        usepldns=yn();
    }
#endif
}


#ifdef PD

void makerembox(int x,int y,int ylen,char *fn)
{
    int i,xx,yy,old;
    char s[212];
    FILE *f;

    sprintf(s,"%s%s.fmt",sys.cfg.menudir,fn);
    f=fopen(s,"rt");
    if (!f) return;


    fgets(s,211,f);
    filter(s,'\n');
    npr("[%d;%dH%s",y,x,s);

    fgets(s,211,f);
    filter(s,'\n');
    npr("[%d;%dH%s",y+ylen-1,x,s);


    fclose(f);
}


void aligncmd1(char in[MAX_PATH_LEN],char *cmd,char *desc)
{
    int r=1,w=0,init=0;
    char s1[MAX_PATH_LEN],s2[12],s3[50],s[MAX_PATH_LEN];

    strcpy(s1,noc2(in));
    init=s1[0];
    while(s1[r]!=bo(init)&&r<strlen(s1))
        s2[w++]=s1[r++];
    s2[w]=0;
    r++;
    strcpy(s3,s1+r);

    strcpy(cmd,s2);
    strcpy(desc,s3);
}

char *makelen(char *in ,int len)
{
    int r;
    static char out[161];

    r=ccount(in);

    memset(out,32,70);
    out[len]=0;
    strcpy(out,in);
    out[strlen(out)]=32;
    out[len+r]=0;

    return out;
}

void popup(char *fn)
{
    int done=0,draw=4;
    int i,ch,x,y,r,slen;
    char s[MAX_PATH_LEN],fmt[200],fmto[200],s1[MAX_PATH_LEN],s2[MAX_PATH_LEN],desc[MAX_PATH_LEN],key[20];
    FILE *f;

    x=50;
    y=24-numitems[0];


    makerembox(x,y-1,(24-y)+2,fn);

    sprintf(s,"%s%s.fmt",sys.cfg.menudir,fn);
    f=fopen(s,"rt");
    if (!f) return;


    fgets(s,211,f);
    fgets(s,211,f);
    fgets(fmt,211,f);
    filter(fmt,'\n');
    fgets(fmto,211,f);
    filter(fmto,'\n');
    fgets(s,211,f);
    slen=atoi(s);
    fclose(f);

    curtitle=0;

    myxy(y,x);

    for(i=0;i<numitems[curtitle];i++) {
        myxy(y+i,x);
        if(i==curitem)
            strcpy(s1,fmt);
        else
            strcpy(s1,fmto);

        aligncmd1(sess.tg[ml[curtitle][curitem+i]].desc,key,desc);
        stuff_in(s,s1,key,makelen(desc,slen),sess.tg[ml[curtitle][curitem+i]].desc,"","");
        npr(s);
    }

    do {
        if(draw==2||draw==3) {
            if(draw==2)
                i=-1;
            else
                i=1;

            myxy(curitem+y+i,x);
            aligncmd1(sess.tg[ml[curtitle][curitem+i]].desc,key,desc);
            stuff_in(s,fmto,key,makelen(desc,slen),sess.tg[ml[curtitle][curitem+i]].desc,"","");
            npr(s);

            myxy(curitem+y,x);
            aligncmd1(sess.tg[ml[curtitle][curitem]].desc,key,desc);
            stuff_in(s,fmt,key,makelen(desc,slen),sess.tg[ml[curtitle][curitem]].desc,"","");
            npr(s);
        }

        r=pmmkey(retfrompldn);
        if(r<0) {
            switch(r) {
            case -2: 
                if(curitem<numitems[curtitle]-1) {
                    curitem++;
                    draw=2;
                }
                break;
            case -1: 
                if(curitem>0) {
                    curitem--;
                    draw=3;
                }
                break;
            }
        } 
        else {
            if(!retfrompldn[0]) {
                strcpy(retfrompldn,sess.tg[ml[curtitle][curitem]].key);
                ansic(0);
                return;
            } 
            else if(retfrompldn[0]=='~') {
                usepldns=0;
                strcpy(retfrompldn,"");
                done=1;
            } 
            else {
                ansic(0);
                return;
            }
        }
    } 
    while(!done&&!hangup);
    ansic(0);

    return;
}
#endif
