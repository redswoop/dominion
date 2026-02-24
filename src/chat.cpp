#include "chat.h"
#include "platform.h"
#include "bbs_output.h"
#include "bbs_input.h"
#include "bbs_ui.h"
#include "tcpio.h"
#include "conio.h"
#include "bbsutl.h"
#include "sysoplog.h"
#include "userdb.h"
#include "file1.h"
#include "timest.h"
#include "disk.h"
#include "utility.h"
#include "jam_bbs.h"
#include "stringed.h"
#include "session.h"
#include "system.h"
#include "acs.h"
#include "ansi_attr.h"
#include "sysopf.h"
#pragma hdrstop


void playmod(void);
#define TWO_WAY 1
int x1,y1,x2,y2,cp0,cp1;

#ifdef TWO_WAY

void two_way_chat(char *s, char *rollover, int maxlen, int crend)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    char s2[100], temp1[50], side0[12] [80], side1[12] [80];
    int side, cnt, cnt2, cntr;
    int i,i1,done,cm,begx,ronum=0,ronum2=0;
    char s1[255],s3[10];
    unsigned char ch;


    cm=io.chatting;
    begx=wherex();
    if (rollover[0]!=0) {
        if (io.charbufferpointer) {
            strcpy(s1,rollover);
            strcat(s1,&io.charbuffer[io.charbufferpointer]);
            strcpy(&io.charbuffer[1],s1);
            io.charbufferpointer=1;
        } 
        else {
            strcpy(&io.charbuffer[1],rollover);
            io.charbufferpointer=1;
        }
        rollover[0]=0;
    }

    done=0;
    side = 0;
    do {
        ch=getkey();
        if(ch==17) io.chatting=0;
        if (io.lastcon)
        {
            if (wherey() == 11)
            {
                outstr("\x1b[12;1H");
                pl(get_string(76));
                s2[0]=0;
                temp1[0]=0;
                for(cntr=1;cntr<12;cntr++)
                {
                    sprintf(s2,"\x1b[%d;%dH",cntr,1);
                    outstr(s2);
                    if ((cntr >=0) && (cntr <5))
                    {
                        outstr(side0[cntr+6]);
                    }
                    outstr("\x1b[K");
                    s2[0]=0;
                }
                sprintf(s2,"\x1b[%d;%dH",5,1);
                outstr(s2);
                s2[0]=0;
            }
            else
                if (wherey() > 11)
                {
                    x2=(wherex()+1);
                    y2=(wherey()+1);
                    sprintf(s2,"\x1b[%d;%dH",y1,x1);
                    outstr(s2);
                    s2[0]=0;
                }
            side = 0;
            ansic(0);
        }
        else
            {
            if (wherey() >= 23)
            {
                for(cntr=13;cntr<25;cntr++)
                {
                    sprintf(s2,"\x1b[%d;%dH",cntr,1);
                    outstr(s2);
                    if ((cntr >= 13) && (cntr <17))
                    {
                        outstr(side1[cntr-7]);
                    }
                    outstr("\x1b[K");
                    s2[0]=0;
                }
                sprintf(s2,"\x1b[%d;%dH",17,1);
                outstr(s2);
                s2[0]=0;
            }
            else
                if ((wherey() < 12) && (side == 0))
                {
                    x1=(wherex()+1);
                    y1=(wherey()+1);
                    sprintf(s2,"\x1b[%d;%dH",y2,x2);
                    outstr(s2);
                    s2[0]=0;
                }
            side=1;
            ansic(5);
        }
        if (cm)
            if (io.chatting==0)
                ch=13;
        if ((ch>=32)) {
            if (side==0)
            {
                if ((wherex()<(sess.user.screenchars()-1)) && (cp0<maxlen))
                {
                    if (wherey() < 11)
                    {
                        side0[wherey()][cp0++]=ch;
                        if(sys.nifty.chatcolor==1) {
                            if (okansi()) makeansi(sys.scfilt[ch],s2,io.curatr); else s2[0]=0;
                            outstr(s2);
                        }
                        if(sys.nifty.chatcolor==2) {
                            if(ch==13) ronum=0;
                            ansic(sys.nifty.rotate[ronum++]);
                            if(ronum==5) ronum=0;
                        }
                        outchr(ch);
                    }
                    else
                        {
                        side0[wherey()][cp0++]=ch;
                        side0[wherey()][cp0]=0;
                        for(cntr=0;cntr<12;cntr++)
                        {
                            sprintf(s2,"\x1b[%d;%dH",cntr,1);
                            outstr(s2);
                            if ((cntr >=0) && (cntr <6))
                            {
                                ansic(0);
                                outstr(side0[cntr+6]);
                                y1=wherey()+1;
                                x1=wherex()+1;
                            }
                            outstr("\x1b[K");
                            s2[0]=0;
                        }
                        sprintf(s2,"\x1b[%d;%dH",y1,x1);
                        outstr(s2);
                        s2[0]=0;
                    }
                    if (wherex()==(sess.user.screenchars()-1))
                        done=1;
                } 
                else {
                    if (wherex()>=(sess.user.screenchars()-1))
                        done=1;
                }
            }
            else
            {
                if ((wherex()<(sess.user.screenchars()-1)) && (cp1<maxlen) )
                {
                    if (wherey() < 23)
                    {
                        side1[wherey()-13][cp1++]=ch;
                        if(sys.nifty.chatcolor==1) {
                            if (okansi()) makeansi(sys.cfilt[ch],s2,io.curatr); else s2[0]=0;
                            outstr(s2);
                        }
                        if(sys.nifty.chatcolor==2) {
                            if(ch==13) ronum2=0;
                            ansic(sys.cfg.dszbatchdl[ronum2++]);
                            if(ronum2==5) ronum2=0;
                        }
                        outchr(ch);
                    }
                    else
                        {
                        side1[wherey()-13][cp1++]=ch;
                        side1[wherey()-13][cp1]=0;
                        for(cntr=13;cntr<25;cntr++)
                        {
                            sprintf(s2,"\x1b[%d;%dH",cntr,1);
                            outstr(s2);
                            if ((cntr >=13) && (cntr <18))
                            {
                                ansic(5);
                                outstr(side1[cntr-7]);
                                y2=wherey()+1;
                                x2=wherex()+1;
                            }
                            outstr("\x1b[K");
                            s2[0]=0;
                        }
                        sprintf(s2,"\x1b[%d;%dH",y2,x2);
                        outstr(s2);
                        s2[0]=0;
                    }
                    if (wherex()==(sess.user.screenchars()-1))
                        done=1;
                } 
                else {
                    if (wherex()>=(sess.user.screenchars()-1))
                        done=1;
                }
            }
        } 
        else
            switch(ch) {
            case 7:
                if ((io.chatting) && (outcom))
                    outcomch(7);
                break;
            case 13: /* C/R */
                if (side == 0)
                    side0[wherey()][cp0]=0;
                else
                    side1[wherey()-13][cp1]=0;
                done=1;
                break;
            case 8:  /* Backspace */
                if (side==0)
                {
                    if (cp0)
                    {
                        if (side0[wherey()][cp0-2]==3)
                        {
                            cp0-=2;
                            ansic(0);
                        }
                        else
                            if (side0[wherey()][cp0-1]==8)
                        {
                            cp0--;
                            outchr(32);
                        }
                        else
                            {
                            cp0--;
                            backspace();
                        }
                    }
                }
                else
                    if (cp1)
                {
                    if (side1[wherey()-13][cp1-2]==3)
                    {
                        cp1-=2;
                        ansic(0);
                    }
                    else
                        if (side1[wherey()-13][cp1-1]==8)
                    {
                        cp1--;
                        outchr(32);
                    }
                    else
                        {
                        cp1--;
                        backspace();
                    }
                }
                break;
            case 24: /* Ctrl-X */
                while (wherex()>begx) {
                    backspace();
                    if (side==0)
                        cp0=0;
                    else
                        cp1=0;
                }
                ansic(0);
                break;
            case 23: /* Ctrl-W */
                if (side==0)
                {
                    if (cp0) {
                        do {
                            if (side0[wherey()][cp0-2]==3) {
                                cp0-=2;
                                ansic(0);
                            } 
                            else
                                if (side0[wherey()][cp0-1]==8) {
                                cp0--;
                                outchr(32);
                            } 
                            else {
                                cp0--;
                                backspace();
                            }
                        } 
                        while ((cp0) && (side0[wherey()][cp0-1]!=32) &&
                            (side0[wherey()][cp0-1]!=8) &&
                            (side0[wherey()][cp0-2]!=3));
                    }
                }
                else
                {
                    if (cp1) {
                        do {
                            if (side1[wherey()-13][cp1-2]==3) {
                                cp1-=2;
                                ansic(0);
                            } 
                            else
                                if (side1[wherey()-13][cp1-1]==8) {
                                cp1--;
                                outchr(32);
                            } 
                            else {
                                cp1--;
                                backspace();
                            }
                        } 
                        while ((cp1) && (side1[wherey()-13][cp1-1]!=32) &&
                            (side1[wherey()-13][cp1-1]!=8) &&
                            (side1[wherey()-13][cp1-2]));
                    }
                }
                break;
            case 14: /* Ctrl-N */
                if (side == 0)
                {
                    if ((wherex()) && (cp0<maxlen))
                    {
                        outchr(8);
                        side0[wherey()][cp0++]=8;
                    }
                }
                else
                    if ((wherex()) && (cp1<maxlen))
                    {
                        outchr(8);
                        side1[wherey()-13][cp1++]=8;
                    }
                break;
            case 16: /* Ctrl-P */
                if (side==0)
                {
                    if (cp0<maxlen-1)
                    {
                        ch=getkey();
                        if ((ch>='0') && (ch<='7'))
                        {
                            side0[wherey()][cp0++]=3;
                            side0[wherey()][cp0++]=ch;
                            ansic(ch-'0');
                        }
                    }
                }
                else
                    {
                    if (cp1<maxlen-1)
                    {
                        ch=getkey();
                        if ((ch>='0') && (ch<='7'))
                        {
                            side1[wherey()-13][cp1++]=3;
                            side1[wherey()-13][cp1++]=ch;
                            ansic(ch-'0');
                        }
                    }
                }
                break;
            case 9:  /* Tab */
                if (side==0)
                {
                    i=5-(cp0 % 5);
                    if (((cp0+i)<maxlen) && ((wherex()+i)<sess.user.screenchars()))
                    {
                        i=5-((wherex()+1) % 5);
                        for (i1=0; i1<i; i1++)
                        {
                            side0[wherey()][cp0++]=32;
                            outchr(32);
                        }
                    }
                }
                else
                    {
                    i=5-(cp1 % 5);
                    if (((cp1+i)<maxlen) && ((wherex()+i)<sess.user.screenchars()))
                    {
                        i=5-((wherex()+1) % 5);
                        for (i1=0; i1<i; i1++)
                        {
                            side1[wherey()-13][cp1++]=32;
                            outchr(32);
                        }
                    }
                }
                break;
            }
    } 
    while ((done==0) && (io.hangup==0));

    if (ch!=13)
    {
        if (side==0)
        {
            i=cp0-1;
            while ((i>0) && (side0[wherey()][i]!=32) &&
                (side0[wherey()][i]!=8) || (side0[wherey()][i-1]==3))
                i--;
            if ((i>(wherex()/2)) && (i!=(cp0-1)))
            {
                i1=cp0-i-1;
                for (i=0; i<i1; i++)
                    outchr(8);
                for (i=0; i<i1; i++)
                    outchr(32);
                for (i=0; i<i1; i++)
                    rollover[i]=side0[wherey()][cp0-i1+i];
                rollover[i1]=0;
                cp0 -= i1;
            }
            side0[wherey()][cp0]=0;
        }
        else
        {
            i=cp1-1;
            while ((i>0) && (side1[wherey()-13][i]!=32) &&
                (side1[wherey()-13][i]!=8) || (side1[wherey()-13][i-1]==3))
                i--;
            if ((i>(wherex()/2)) && (i!=(cp1-1)))
            {
                i1=cp1-i-1;
                for (i=0; i<i1; i++)
                    outchr(8);
                for (i=0; i<i1; i++)
                    outchr(32);
                for (i=0; i<i1; i++)
                    rollover[i]=side1[wherey()-13][cp1-i1+i];
                rollover[i1]=0;
                cp1 -= i1;
            }
            side1[wherey()-13][cp1]=0;
        }
    }
    if ((crend) && (wherey() != 11) && (wherey()<23))
        nl();
    if (side == 0)
        cp0=0;
    else
        cp1=0;
    s[0]=0;
}


#endif


void chat1(char *chatline, int two_way)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();

    int tempdata, cnt;
    char cl[MAX_PATH_LEN],xl[MAX_PATH_LEN],s[161],s1[161],atr[MAX_PATH_LEN],s2[MAX_PATH_LEN],cc;
    int i,i1,cf,oe,i2;
    double tc;
    int mcir=io.mciok;

    io.mciok=1;

    io.chatcall=0;
    io.chatting=1;
    tc=timer();
    cf=0;

    savel(cl,atr,xl,&cc);
    s1[0]=0;
    oe=io.echo;
    io.echo=1;
    nl();
    nl();
    tempdata=sess.topdata;
    if (!okansi())
        two_way=0;
    if (sess.modem_speed==300)
        two_way=0;

#ifndef TWO_WAY
    two_way=0;
#endif


#ifdef TWO_WAY
    if (two_way) {
        clrscrb();
        cp0=0;
        cp1=0;
        if (io.defscreenbottom==24) {
            sess.topdata=0;
            topscreen();
        }
        outstr("\x1b[2J");
        x2=1;
        y2=13;
        outstr("\x1b[1;1H");
        x1=wherex();
        y1=wherey();
        outstr("\x1b[12;1H");
        pl(get_string(76));
        outstr("\x1b[1;1H");
        s[0]=0;
        s1[0]=0;
        s2[0]=0;
    }
#endif

    pl(get_string(6));
    nl();
    nl();
    strcpy(s1,chatline);
    readfilter("user.flt","sysop.flt");

    if(!two_way)
        io.chat_file=sys.nifty.nifstatus & nif_autochat;

    do {
#ifdef TWO_WAY
        if (two_way)
            two_way_chat(s,s1,160,1);
        else
#endif
            i=ainli(s,s1,160,1,2,0);
        switch(i) {
        case -1: 
            io.chatting=0; 
            break;
        case -2: 
            chatsound(); 
            break;
        case -3: 
            viewfile(); 
            break;
        case -4: 
            outchr(12); 
            break;
        case -5: 
            nl(); 
            outstr("5Sure? "); 
            io.hangup=yn(); 
            break;
        case -6: 
            for(i=0;i<5;i++) outchr(7); 
            break;
        case -7: 
            nl(); 
            input(s,21); 
            if(slok(s,0)) pl("good"); 
            else pl("Bad");
        }

        if ((io.chat_file) && (!two_way)) {
            if (cf==0) {
                if (!two_way)
                    outs("-] Chat file opened.\r\n");
                sprintf(s2,"%sCHAT.TXT",sys.cfg.gfilesdir);
                cf=open(s2,O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
                lseek(cf,0L,SEEK_END);
                sprintf(s2,"\r\n\r\nChat file opened %s %s\r\n",date(),times());
                write(cf,(void *)s2,strlen(s2));
                sprintf(s2,"User: %s\r\n",sess.user.display_name(sess.usernum).c_str());
                write(cf,(void *)s2,strlen(s2));
                if(sess.chatreason[0]) {
                    sprintf(s2,"%s\r\n",sess.chatreason);
                    write(cf,&s2,strlen(s2));
                }

                s2[0]=196;
                for(i2=0;i2<41;i2++) write(cf,&s2[0],1L);
                strcpy(s2,"\r\n\r\n");
                write(cf,(void *)s2,strlen(s2));
            }
            strcat(s,"\r\n");
            write(cf,(void *)s,strlen(s));
        } 
        else
            if (cf) {
            close(cf);
            cf=0;
            if (!two_way)
                outs("-] Chat file closed.\r\n");
        }
        if (io.hangup)
            io.chatting=0;
    } 
    while (io.chatting);
    if (io.chat_file) {
        close(cf);
        io.chat_file=0;
    }
    ansic(0);


    if (two_way)
        outstr("\x1b[2J");

    nl();
    pl(get_string(7));
    nl();
    io.chatting=0;
    tc=timer()-tc;
    if (tc<0)
        tc += 86400.0;
    sess.extratimecall += tc;
    sess.topdata=tempdata;
    if (sess.useron)
        topscreen();
    io.echo=oe;
    restorel(cl,atr,xl,&cc);
    io.mciok=mcir;
}


void reqchat1(char reason[MAX_PATH_LEN])
{
    auto& sess = Session::instance();
    int ok;
    char s[MAX_PATH_LEN];

    nl();
    nl();
    ok=sysop2();
    if(checkacs(10)) ok=1;
    if (restrict_chat & sess.user.restrict_flags())
        ok=0;
    if (ok) {
        if (io.chatcall) {
            io.chatcall=0;
            pl("Chat call Deactivated");
            topscreen();
        } 
        else {
            inputdat(reason,s,70,1);
            if (s[0]) {
                io.chatcall=1;
                strcpy(sess.chatreason,s);
                nl();
                sysoplog(sess.chatreason);
                for (ok=strlen(sess.chatreason); ok<80; ok++)
                    sess.chatreason[ok]=32;
                sess.chatreason[80]=0;
                topscreen();
                pl("Chat call Activated");
                nl();
            }
        }
    } 
    else {
        pl("Sysop not available.");
        nl();
        printfile("nosysop");
        logtypes(3,"Chat 2[2%s2]",sess.chatreason);
        if(sess.usernum) {
            pl("Use feedback instead.");
            sess.cursub=0;
            email(1,"Chat Request",1);
        }
    }
}


void chatsound()
{
    auto& sess = Session::instance();
    int i;

    for(i=100;i<2000;i+=200) {
        if(sess.chatsoundon)
            sound(i);
        delay(30);
    }
    nosound();
}

//#define MOD

void reqchat(char reason[MAX_PATH_LEN])
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    int ok,i;
    char s[MAX_PATH_LEN];

    nl();
    nl();
    inputdat(reason,sess.chatreason,70,1);
    if(!sess.chatreason[0]) return;
    topscreen();
    nl();

    ok=sysop2();
    if (restrict_chat & sess.user.restrict_flags())
        ok=0;
#ifdef MOD
    playmod();
#else
    if(ok) {
        i=0;
        do {
            chatsound();
            outstr(get_string(17));
            if(kbhitb()) {
                s[0]=getch();
                if(s[0]==13) sess.chatsoundon=0;
                else if(s[0]==32) {
                    sess.chatreason[0]=0;
                    topscreen();
                    chat1("",sys.cfg.sysconfig & sysconfig_2_way);
                    return;
                }
            }
        } 
        while(i++<10);
    }
#endif
    if(!ok||i>9) {
        logtypes(3,"Chat 2[2%s2]",sess.chatreason);
        printfile("nosysop");
        if(!ok) {
            io.chatcall=0;
            topscreen();
        }
        if(sess.usernum) {
            sess.cursub=0;
            email(1,"Chat Request",1);
        }
    }
}

void readfilter(char fn[15],char fn2[15])
{
    auto& sys = System::instance();
    char s[100];
    int i;
    sprintf(s,"%s%s",sys.cfg.gfilesdir,fn);
    i=open(s,O_RDONLY|O_BINARY);
    read(i,sys.cfilt,255);
    close(i);
    sprintf(s,"%s%s",sys.cfg.gfilesdir,fn2);
    i=open(s,O_RDONLY|O_BINARY);
    read(i,sys.scfilt,255);
    close(i);
}

void viewfile()
{
    char s[MAX_PATH_LEN];

    inputdat("Filename to View",s,71,0);
    if(exist(s)) showfile(s);
}

#ifdef MOD
void far moddevice( int *device );
void far modvolume( int vol1, int vol2,int vol3,int vol4);
void far modsetup( char *filenm, int looping, int prot,int mixspeed,
int device, int *status);
void far modstop(void);
void far modinit(void);

void playmod(void)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    int dev,mix,vol,stat,i,i1;
    char md[41];

    modinit();
    vol = 255;
    mix = 10000;
    if(exist("sbchat"))
        dev=7;
    else
        dev=0;
    modvolume(vol,vol,vol,vol);
    modsetup("io.chatcall.mod", 4, 0 ,mix, dev, &stat );
    i=0;
    do {
        wait1(15);
        outstr(get_string(17));
        if(kbhitb()) {
            dev=getch();
            if(dev==13) {
                sess.chatsoundon=0;
                modstop();
            }
            else if(dev==32) {
                sess.chatreason[0]=0;
                topscreen();
                modstop();
                chat1("",sys.cfg.sysconfig & sysconfig_2_way);
                return;
            }
        }
    } 
    while(i++<40);
    modstop();
}
#endif
