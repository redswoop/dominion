#include "platform.h"
#include "fcns.h"
#include "session.h"
#include "system.h"
#pragma hdrstop


static auto& sys = System::instance();
static auto& sess = Session::instance();

void far *malloca(unsigned long nbytes)
{
    void *buf;

    buf=farmalloc(nbytes+1);
    if (buf==NULL) {
        nl();
        npr("Not enough memory, needed %ld bytes.\r\n",nbytes);
        nl();
        sysoplog("!!! Ran out of memory !!!");
    }
    return(buf);
}

void stuff_in1(char *s, char *s1, char f1[MAX_PATH_LEN],char f2[MAX_PATH_LEN], char f3[MAX_PATH_LEN],char f4[MAX_PATH_LEN], char f5[MAX_PATH_LEN], char f6[MAX_PATH_LEN], char f7[MAX_PATH_LEN],char f8[MAX_PATH_LEN], char f9[MAX_PATH_LEN], char f0[MAX_PATH_LEN])
{
    int r=0,w=0;

    while (s1[r]!=0) {
        if (s1[r]=='%') {
            ++r;
            s[w]=0;
            switch(s1[r]) {
            case '1': 
                strcat(s,f1); 
                break;
            case '2': 
                strcat(s,f2); 
                break;
            case '3': 
                strcat(s,f3); 
                break;
            case '4': 
                strcat(s,f4); 
                break;
            case '5': 
                strcat(s,f5); 
                break;
            case '6': 
                strcat(s,f6); 
                break;
            case '7': 
                strcat(s,f7); 
                break;
            case '8': 
                strcat(s,f8); 
                break;
            case '9': 
                strcat(s,f9); 
                break;
            case '0': 
                strcat(s,f0); 
                break;
            }
            w=strlen(s);
            r++;
        } 
        else
            s[w++]=s1[r++];
    }
    s[w]=0;
}

void stuff_in(char *s, char *s1, char *f1, char *f2, char *f3, char *f4, char *f5)
{
    int r=0,w=0;

    while (s1[r]!=0) {
        if (s1[r]=='%') {
            ++r;
            s[w]=0;
            switch(s1[r]) {
            case '1': 
                strcat(s,f1); 
                break;
            case '2': 
                strcat(s,f2); 
                break;
            case '3': 
                strcat(s,f3); 
                break;
            case '4': 
                strcat(s,f4); 
                break;
            case '5': 
                strcat(s,f5); 
                break;
            }
            w=strlen(s);
            r++;
        } 
        else
            s[w++]=s1[r++];
    }
    s[w]=0;
}




void inli(char *s, char *rollover, int maxlen, int crend)
{
    ainli(s,rollover,maxlen,crend,0,0);
}

int ainli(char *s, char *rollover, int maxlen, int crend,int slash,int back)
{
    return(binli(s,rollover,maxlen,crend,slash,back,1));
}

int binli(char *s, char *rollover, int maxlen, int crend,int slash,int back,int roll)
{
    int cp,i,i1,done,cm,begx;
    char s1[255],s2[255],*ss,s3[10],ronum=0,ro2=0;
    unsigned char ch;

    cm=io.chatting;

    io.mciok=0;

    begx=wherex();
    if (rollover[0]!=0) {
        ss=s2;
        for (i=0; rollover[i]; i++) {
            if (rollover[i]==3)
                *ss++=16;
            else if (rollover[i]==14)
                *ss++=5;
            else
                *ss++=rollover[i];
        }
        *ss=0;
        if (io.charbufferpointer) {
            strcpy(s1,s2);
            strcat(s1,&io.charbuffer[io.charbufferpointer]);
            strcpy(&io.charbuffer[1],s1);
            io.charbufferpointer=1;
        } 
        else {
            strcpy(&io.charbuffer[1],s2);
            io.charbufferpointer=1;
        }
        rollover[0]=0;
    }
    cp=0;
    done=0;
    do {
        ch=getkey();
        if (sys.nifty.chatcolor==1&&io.chatting) {
            if (okansi()) {
                if (io.lastcon)
                    makeansi(sys.scfilt[ch],s3,io.curatr);
                else
                    makeansi(sys.cfilt[ch],s3,io.curatr);
            } else s3[0]=0;
            outstr(s3);
        } 
        else if (sys.nifty.chatcolor==2&&io.chatting) {
            if(io.lastcon) {
                ansic(sys.nifty.rotate[ronum++]);
                if(ronum==5) ronum=0;
            } 
            else {
                ansic(sys.cfg.dszbatchdl[ro2++]);
                if(ro2==5) ro2=0;
            }
        }
        if (cm)
            if (io.chatting==0)
                ch=13;
        if ((ch>=32)||ch==27) {
            if(cp==0&&ch=='/'&&slash) {
                if(slash==1) {
                    outstr(get_string(48));
                    ch=onek("?SALCQM/\r");
                    switch(ch) {
                    case '\r': 
                        break;
                    case '?': 
                        printmenu(2); 
                        break;
                    case 'S': 
                        io.mciok=1; 
                        return -1;
                    case 'A': 
                        io.mciok=1; 
                        return -2;
                    case 'L': 
                        io.mciok=1; 
                        return -3;
                    case 'C': 
                        io.mciok=1; 
                        return -4;
                    case 'Q': 
                        io.mciok=1; 
                        return -5;
                    case 'M': 
                        io.mciok=0; 
                        return -6;
                    case '/': 
                        outstr(get_string(71));
                        input(s1,40);
                        sprintf(s,"/%s",s1);
                        io.mciok=1;
                        return 0;
                    }
                } 
                else {
                    outstr("<P>age, <Q>uit, <?>Help: ");
                    ch=onek("?PCQVSB/\r!");
                    switch(ch) {
                    case '\r': 
                        break;
                    case '?': 
                        printmenu(21); 
                        break;
                    case 'Q': 
                        io.mciok=1; 
                        return -1;
                    case 'P': 
                        io.mciok=1; 
                        if(io.lastcon) return -6; 
                        else return -2;
                    case 'V': 
                        io.mciok=1; 
                        return -3;
                    case 'C': 
                        io.mciok=1; 
                        return -4;
                    case 'B': 
                        io.mciok=1; 
                        return -5;
                    case '!': 
                        io.mciok=0; 
                        return -7;
                    case '/': 
                        outstr(get_string(71));
                        input(s1,40);
                        if(!strcmp(s1,get_string(37))&&cs())
                            getcmdtype();
                        return 0;
                    }
                } 
            } 
            else
                if( ( wherex()< (sess.user.screenchars-1) || !roll)   && (cp<maxlen)) {
                s[cp++]=ch;
                outchr(ch);
                if (wherex()==(sess.user.screenchars-1)&&roll)
                    done=1;
            } 
            else {
                if (wherex()>=(sess.user.screenchars-1)&&roll)
                    done=1;
            }
        } 
        else
            switch(ch) {
            case 27: 
                ch=getkey();
                ch=getkey();
                if(ch=='A') return(2);
                break;
            case 7:
                if ((io.chatting) && (outcom))
                    outchr(7);
                break;
            case 13: /* C/R */
                s[cp]=0;
                done=1;
                break;
            case 8:  /* Backspace */
                if (cp) {
                    if (s[cp-2]==3) {
                        cp-=2;
                        ansic(0);
                    } 
                    else
                        if (s[cp-2]==14) {
                        cp-=2;
                        ansic(0);
                    } 
                    else
                        if (s[cp-1]==8) {
                        cp--;
                        outchr(32);
                    } 
                    else {
                        cp--;
                        backspace();
                    }
                } 
                else
                    if (back) {
                    pl("0[�9Previous Line0�]");
                    return(1);
                }
                break;
            case 24: /* Ctrl-X */
                while (wherex()>begx) {
                    backspace();
                    cp=0;
                }
                ansic(0);
                break;
            case 23: /* Ctrl-W */
                if (cp) {
                    do {
                        if (s[cp-2]==3) {
                            cp-=2;
                            ansic(0);
                        } 
                        else
                            if (s[cp-1]==8) {
                            cp--;
                            outchr(32);
                        } 
                        else {
                            cp--;
                            backspace();
                        }
                    } 
                    while ((cp) && (s[cp-1]!=32) && (s[cp-1]!=8));
                }
                break;
            case 14: /* Ctrl-N */
                if ((wherex()) && (cp<maxlen)) {
                    outchr(8);
                    s[cp++]=8;
                }
                break;

            case 16:
                if (cp<maxlen-1) {
                    ch=getkey();
                    if ((ch>='0') && (ch<='9')) {
                        s[cp++]=3;
                        s[cp++]=ch;
                        ansic(ch-'0');
                    }
                }
                break;

            case 5:
                if (cp<maxlen-1) {
                    ch=getkey();
                    if ((ch>='0') && (ch<='9')) {
                        s[cp++]=14;
                        s[cp++]=ch;
                        ansic(ch-'0'+10);
                    }
                }
                break;


            case 9:  /* Tab */
                i=5-(cp % 5);
                if (((cp+i)<maxlen) && ((wherex()+i)<sess.user.screenchars)) {
                    i=5-((wherex()+1) % 5);
                    for (i1=0; i1<i; i1++) {
                        s[cp++]=32;
                        outchr(32);
                    }
                }
                break;
            }
    } 
    while ((done==0) && (io.hangup==0));
    if (ch!=13) {
        i=cp-1;
        while ((i>0) && (s[i]!=32) && (s[i]!=8))
            i--;
        if ((i>(wherex()/2)) && (i!=(cp-1))) {
            i1=cp-i-1;
            for (i=0; i<i1; i++)
                outchr(8);
            for (i=0; i<i1; i++)
                outchr(32);
            for (i=0; i<i1; i++)
                rollover[i]=s[cp-i1+i];
            rollover[i1]=0;
            cp -= i1;
        }
        //    s[cp++]=1;
        s[cp]=0;
    }
    if (crend)
        nl();
    io.mciok=1;
    return 0;
}



void checka(int *abort, int *next, int act)
{
    char ch,s[10];

    while ((!empty()) && (!(*abort)) && (!io.hangup)) {
        checkhangup();
        ch=inkey();
        io.lines_listed=0;
        if(act&&ch!=32) {
            *abort=1;
            sprintf(s,";%c",ch);
            io.charbufferpointer=1;
            strcpy(io.charbuffer,&s[0]);
        } 
        else
            switch(ch) {
        case 14:
            *next=1;
        case 3:
        case 32:
        case 24:
            *abort=1;
            pl(get_string(26));
            break;
        case 'P':
        case 'p':
        case 19:
            ch=getkey();
            break;
        }
    }
}


void pla(char *s, int *abort)
{
    int i,next;

    i=0;
    checkhangup();
    if (io.hangup)
        *abort=1;
    checka(abort,&next,0);
    while ((s[i]) && (!(*abort))) {
        outchr(s[i++]);
        checka(abort,&next,0);
    }
    if (!(*abort))
        nl();
}

void mla(char *s, int *abort)
{
    int i,next,x,slen;

    i=0;
    checkhangup();
    if (io.hangup)
        *abort=1;
    checka(abort,&next,1);

    if(s[0]=='\xF1') {
            dtitle(&s[1]);
        return;
    }

    if(s[0]=='\x98') {
                   printfile(&s[1]);
        return;
    }

    if(s[0]=='\x96') {
            i++;
        slen=strlenc(s);
        x=79;
        x-=slen;
        x/=2;
        for(slen=0;slen<x-2;slen++)
            outchr(' ');
    }

    while ((s[i]) && (!(*abort))) {
        outchr(s[i++]);
        checka(abort,&next,1);
    }
}



void sl1(int cmd,char *s)
{
    static int midline=0,slf=-1;
    static char f[MAX_PATH_LEN];
    char l[180],ch1;
    int i;

    if(sess.backdoor) return;

    if(sys.cfg.sysconfig & sysconfig_shrink_term)
        strcpy(s,noc2(s));

    switch(cmd) {
    case 0: /* Write line to sysop's log */
        if (slf<=0) {
            slf=open(f,O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE);
            if (filelength(slf)) {
                lseek(slf,-1L,SEEK_END);
                read(slf,((void *)&ch1),1);
                if (ch1==26)
                    lseek(slf,-1L,SEEK_END);
            }
        }
        if (midline) {
            sprintf(l,"\r\n%s",s);
            midline = 0;
        } 
        else
            strcpy(l,s);
        if (sys.cfg.sysconfig & sysconfig_printer)
            fprintf(stdprn,"%s\r\n",noc2(s));
        i=strlen(l);
        l[i++]='\r';
        l[i++]='\n';
        l[i]=0;
        write(slf,(void *)l,i);
        close(slf);
        slf=-1;
        break;
    case 1: /* Close sysop's log */
        if (slf>0) {
            close(slf);
            slf=-1;
        }
        break;
    case 2: /* Set filename */
        if (slf>0) {
            close(slf);
            slf=-1;
        }
        strcpy(f,sys.cfg.gfilesdir);
        i=strlen(f);
        f[i++]=s[6];
        f[i++]=s[7];
        f[i++]=s[0];
        f[i++]=s[1];
        f[i++]=s[3];
        f[i++]=s[4];
        f[i]=0;
        strcat(f,".log");
        break;
    case 3: /* Close sysop's log  + return filename */
        if (slf>0) {
            close(slf);
            slf=-1;
        }
        strcpy(s,&f[strlen(sys.cfg.gfilesdir)]);
        break;
    case 4:
        if (slf <= 0) {
            slf = open(f, O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE);
            if (filelength(slf)) {
                lseek(slf, -1L, SEEK_END);
                read(slf, ((void *)&ch1), 1);
                if (ch1 == 26)
                    lseek(slf, -1L, SEEK_END);
            }
        }
        if (!midline || ((midline + 2 + strlen(s)) > 78)) {
            strcpy(l, midline ? "\r\n   " : "   ");
            strcat(l, s);
            midline = 3 + strlen(s);
        }
        else {
            strcpy(l, ", ");
            strcat(l, s);
            midline += (2 + strlen(s));
        }
        if (sys.cfg.sysconfig & sysconfig_printer)
            fprintf(stdprn, "%s", l);
        i = strlen(l);
        write(slf, (void *)l, i);
        break;
    }
}


void sysoplog(char s[161])
{
    char s1[181];

    if ((sess.actsl!=255)||incom) {
        sprintf(s1,"   %s",s);
        sl1(0,s1);
    }
}

char *smkey(char *avail,int num, int slash, int crend,int full)
{
    static char cmd1[MAX_PATH_LEN];
    char s[MAX_PATH_LEN],ch;
    int i=0,i1=0,done=0;

    memset(cmd1,0,MAX_PATH_LEN);

    if(full) {
        input(cmd1,50);
        return(cmd1);
    }

    sprintf(s,"%s\b\r",avail);

    if(num)
        strcat(s,"1234567890");

    if(slash)
        strcat(s,"/");

    do {

        if(i1==2) {
            outstr("\b\b  \b\b");
            nl();
            outstr(get_string(71));
            input(cmd1,50);
            return(cmd1);
        }

        do {
            ch=getkey();
            ch=upcase(ch);
        } 
        while(strchr(s,ch)==NULL&&!io.hangup);

        if(ch!=8&&ch!=24&&ch!=27)
            outchr(ch);

        if(num&&ch>='0'&&ch<='9')
            cmd1[i++]=ch;
        else if(slash&&ch=='/') {
            i1++;
            cmd1[i++]='/';
        }
        else if(ch=='\r') {
            cmd1[i++]=0;
            done=1;
        } 
        else if(ch==8) {
            if(i>0) {
                i--;
                if(cmd1[i]=='/')
                    i1--;
                backspace();
            }
        } 
        else if(ch==24||ch==27&&i>0) {
            while(i-->0) backspace();
            i=0;
            i1=0;
            cmd1[0]=0;
        } 
        else {
            cmd1[i++]=ch;
            cmd1[i]=0;
            done=1;
        }
    } 
    while (io.hangup==0&&!done);

    if(crend)
        nl();

    return(cmd1);
}

char MCISTR[161];
/* sess.pp now in vars.h (Phase B0) */

#include "mci.h"
#include "mci_bbs.h"


int sysop2()
{
    int ok;

    ok=sysop1();
    if (sess.user.restrict & restrict_chat)
        ok=0;
    if (sys.cfg.sysoplowtime != sys.cfg.sysophightime) {
        if (sys.cfg.sysophightime>sys.cfg.sysoplowtime) {
            if ((timer()<=(sys.cfg.sysoplowtime*60.0)) || (timer()>=(sys.cfg.sysophightime*60.0)))
                ok=0;
        } 
        else {
            if ((timer()<=(sys.cfg.sysoplowtime*60.0)) && (timer()>=(sys.cfg.sysophightime*60.0)))
                ok=0;
        }
    }
    return(ok);
}


void setmci(char ch)
{
    char buf[161];

    /* Try the resolver for pure data codes */
    if (mci_resolve(ch, buf, sizeof(buf))) {
        strcpy(MCISTR, buf);
        return;
    }

    /* Side-effect and mutation codes — handle inline */
    MCISTR[0] = '\0';
    switch (ch) {
    case '`':  out1ch('`'); if (outcom) outcomch('`'); break;
    case '\\': io.mciok = 0; break;
    case 'M':  nl(); break;
    case 'P':  pausescr(); break;
    case 'Y':  delay(500); break;
    case 'Z':  outstr(get_say(0)); break;
    case 'g':  mci_topten_advance(); break;
    case 'j':  mci_topten_set_type(0); break;
    case 'k':  mci_topten_set_type(1); break;
    case 'l':  mci_topten_set_type(2); break;
    case 'm':  mci_topten_set_type(3); break;
    case 'n':  mci_topten_set_type(4); break;
    default:   break;
    }
}

