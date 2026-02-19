#include "vars.h"
#pragma hdrstop

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

void stuff_in1(char *s, char *s1, char f1[81],char f2[81], char f3[81],char f4[81], char f5[81], char f6[81], char f7[81],char f8[81], char f9[81], char f0[81])
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

    cm=chatting;

    mciok=0;

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
        if (charbufferpointer) {
            strcpy(s1,s2);
            strcat(s1,&charbuffer[charbufferpointer]);
            strcpy(&charbuffer[1],s1);
            charbufferpointer=1;
        } 
        else {
            strcpy(&charbuffer[1],s2);
            charbufferpointer=1;
        }
        rollover[0]=0;
    }
    cp=0;
    done=0;
    do {
        ch=getkey();
        if (nifty.chatcolor==1&&chatting) {
            if (lastcon)
                makeansi(scfilt[ch],s3,0);
            else
                makeansi(cfilt[ch],s3,0);
            outstr(s3);
        } 
        else if (nifty.chatcolor==2&&chatting) {
            if(lastcon) {
                ansic(nifty.rotate[ronum++]);
                if(ronum==5) ronum=0;
            } 
            else {
                ansic(syscfg.dszbatchdl[ro2++]);
                if(ro2==5) ro2=0;
            }
        }
        if (cm)
            if (chatting==0)
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
                        mciok=1; 
                        return -1;
                    case 'A': 
                        mciok=1; 
                        return -2;
                    case 'L': 
                        mciok=1; 
                        return -3;
                    case 'C': 
                        mciok=1; 
                        return -4;
                    case 'Q': 
                        mciok=1; 
                        return -5;
                    case 'M': 
                        mciok=0; 
                        return -6;
                    case '/': 
                        outstr(get_string(71));
                        input(s1,40);
                        sprintf(s,"/%s",s1);
                        mciok=1;
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
                        mciok=1; 
                        return -1;
                    case 'P': 
                        mciok=1; 
                        if(lastcon) return -6; 
                        else return -2;
                    case 'V': 
                        mciok=1; 
                        return -3;
                    case 'C': 
                        mciok=1; 
                        return -4;
                    case 'B': 
                        mciok=1; 
                        return -5;
                    case '!': 
                        mciok=0; 
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
                if( ( wherex()< (thisuser.screenchars-1) || !roll)   && (cp<maxlen)) {
                s[cp++]=ch;
                outchr(ch);
                if (wherex()==(thisuser.screenchars-1)&&roll)
                    done=1;
            } 
            else {
                if (wherex()>=(thisuser.screenchars-1)&&roll)
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
                if ((chatting) && (outcom))
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
                    pl("0[þ9Previous Line0þ]");
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
                if (((cp+i)<maxlen) && ((wherex()+i)<thisuser.screenchars)) {
                    i=5-((wherex()+1) % 5);
                    for (i1=0; i1<i; i1++) {
                        s[cp++]=32;
                        outchr(32);
                    }
                }
                break;
            }
    } 
    while ((done==0) && (hangup==0));
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
    mciok=1;
    return 0;
}



void checka(int *abort, int *next, int act)
{
    char ch,s[10];

    while ((!empty()) && (!(*abort)) && (!hangup)) {
        checkhangup();
        ch=inkey();
        lines_listed=0;
        if(act&&ch!=32) {
            *abort=1;
            sprintf(s,";%c",ch);
            charbufferpointer=1;
            strcpy(charbuffer,&s[0]);
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
    if (hangup)
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
    if (hangup)
        *abort=1;
    checka(abort,&next,1);

    if(s[0]=='ñ') {
            dtitle(&s[1]);
        return;
    }

    if(s[0]=='˜') {
                   printfile(&s[1]);
        return;
    }

    if(s[0]=='–') {
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
    static char f[81];
    char l[180],ch1;
    int i;

    if(backdoor) return;

    if(syscfg.sysconfig & sysconfig_shrink_term)
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
        if (syscfg.sysconfig & sysconfig_printer)
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
        strcpy(f,syscfg.gfilesdir);
        i=strlen(f);
        f[i++]=s[6];
        f[i++]=s[7];
        f[i++]=s[0];
        f[i++]=s[1];
        f[i++]=s[3];
        f[i++]=s[4];
        f[i]=0;
        strcat(f,".LOG");
        break;
    case 3: /* Close sysop's log  + return filename */
        if (slf>0) {
            close(slf);
            slf=-1;
        }
        strcpy(s,&f[strlen(syscfg.gfilesdir)]);
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
        if (syscfg.sysconfig & sysconfig_printer)
            fprintf(stdprn, "%s", l);
        i = strlen(l);
        write(slf, (void *)l, i);
        break;
    }
}


void sysoplog(char s[161])
{
    char s1[181];

    if ((actsl!=255)||incom) {
        sprintf(s1,"   %s",s);
        sl1(0,s1);
    }
}

void isr1(int un, char *name)
{
    int cp,i;
    char s[81];
    smalrec sr;

    cp=0;
    while ((cp<status.users) && (strcmp(name,(smallist[cp].name))>0))
        ++cp;
    memmove(&(smallist[cp+1]),&(smallist[cp]),sizeof(smalrec)*(status.users-cp));
    strcpy(sr.name,name);
    sr.number=un;
    smallist[cp]=sr;
    ++status.users;
}

char *smkey(char *avail,int num, int slash, int crend,int full)
{
    static unsigned char cmd1[81];
    char s[81],ch;
    int i=0,i1=0,done=0;

    memset(cmd1,0,81);

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
        while(strchr(s,ch)==NULL&&!hangup);

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
    while (hangup==0&&!done);

    if(crend)
        nl();

    return(cmd1);
}

char MCISTR[161];
extern mmrec pp;
int toptentype=0,whichten=0;

char *topten(int type);


int sysop2()
{
    int ok;

    ok=sysop1();
    if (thisuser.restrict & restrict_chat)
        ok=0;
    if (syscfg.sysoplowtime != syscfg.sysophightime) {
        if (syscfg.sysophightime>syscfg.sysoplowtime) {
            if ((timer()<=(syscfg.sysoplowtime*60.0)) || (timer()>=(syscfg.sysophightime*60.0)))
                ok=0;
        } 
        else {
            if ((timer()<=(syscfg.sysoplowtime*60.0)) && (timer()>=(syscfg.sysophightime*60.0)))
                ok=0;
        }
    }
    return(ok);
}


void setmci(char ch)
{
    char s[161];
    int x;
    userrec u;

    strcpy(s,"");

    switch (ch) {
    case ']': 
        strcpy(s,pp.pausefile); 
        break;
    case '[': 
        strcpy(s,times()); 
        break;
    case '`': 
        out1ch('`'); 
        if(outcom) outcomch('`'); 
        break;
    case '!': 
        sprintf(s,"%.0f%%",syscfg.req_ratio*100); 
        break;
    case '%': 
        sprintf(s,"%.0f%%",syscfg.post_call_ratio*100); 
        break;
    case '#': 
        sprintf(s,"%.0f%%",ratio()*100); 
        break;
    case '$': 
        sprintf(s,"%.0f%%",post_ratio()*100); 
        break;
    case '^': 
        if(ratio()>=syscfg.req_ratio) strcpy(s,"Passed"); 
        else strcpy(s,"Failed"); 
        break;
    case '&': 
        if(post_ratio()>=syscfg.post_call_ratio) strcpy(s,"Passed"); 
        else strcpy(s,"Failed"); 
        break;
    case '*': 
        sprintf(s,"%-4d",thisuser.msgpost); 
        break;
    case '(': 
        sprintf(s,"%-4d",thisuser.logons); 
        break;
    case ')': 
        sprintf(s,"%-4d",thisuser.ontoday); 
        break;
    case '+': 
        sprintf(s,"%s",status.lastuser); 
        break;
    case '@': 
        strcpy(s,get_string(sysop2()?4:5)); 
        break;
    case '-': 
        sprintf(s,"%s [%s]",nam(&thisuser,usernum),thisuser.comment); 
        break;
    case '\\': 
        mciok=0; 
        break;
/*       case '/':
        okabort=0; 
        break;
*/
    case 'a': 
        read_user(status.amsguser,&u);
        if(status.amsganon==1) {
            if(so()) {
                strcpy(s,"®");
                strcat(s,nam(&u,status.amsguser));
                strcat(s,"¯");
            } 
            else strcpy(s,"Anonymous!");
        } 
        else strcpy(s,nam(&u,status.amsguser));
        break;
    case 'b': 
        strcpy(s,proto[thisuser.defprot].description); 
        break;
    case 'c': 
        sprintf(s,"%d",thisuser.timebank); 
        break;
    case 'd': 
        sprintf(s,"%d",numwaiting(&thisuser)); 
        break;
    case 'e': 
        strcpy(s,thisuser.comment); 
        break;
    case 'f': 
        sprintf(s,"%d",nummsgs-msgr); 
        break;

    case 'g': 
        whichten++; 
        if(whichten==10) whichten=0; 
        break;
    case 'h': 
        strcpy(s,topten(0)); 
        break;
    case 'i': 
        strcpy(s,topten(1)); 
        break;

    case 'j': 
        toptentype=0; 
        whichten=0; 
        break;
    case 'k': 
        toptentype=1; 
        whichten=0; 
        break;
    case 'l': 
        toptentype=2; 
        whichten=0; 
        break;
    case 'm': 
        toptentype=3; 
        whichten=0; 
        break;
    case 'n': 
        toptentype=4; 
        whichten=0; 
        break;
    case 'o': 
        sprintf(s,"%d",numbatchdl); 
        break;
    case 'p': 
        sprintf(s,"%d",numbatch-numbatchdl); 
        break;
    case 'q': 
        sprintf(s,"%-1.f",batchsize); 
        break;
    case 'r': 
        sprintf(s,"%s",ctim(batchtime)); 
        break;
    case 't': 
        sprintf(s,"%.0f",nsl()/60.0); 
        break;
    case 'A': 
        if(usub[cursub].subnum>-1)
            sprintf(s,"%s",usub[cursub].keys);
        else strcpy(s,"-1");
        break;
    case 'B': 
        if(usub[cursub].subnum>-1)
            sprintf(s,"%s",subboards[usub[cursub].subnum].name);
        else strcpy(s,"No Subs");
        break;
    case 'C': 
        if(udir[curdir].subnum>-1)
            sprintf(s,"%s",udir[curdir].keys);
        else strcpy(s,"-1");
        break;
    case 'D': 
        if(udir[curdir].subnum>-1)
            sprintf(s,"%s%s",directories[udir[curdir].subnum].name,(directories[udir[curdir].subnum].mask & mask_no_ratio)?" [NR]":"");
        else strcpy(s,"No Dirs");
        break;
    case 'E': 
        sprintf(s,"%s",thisuser.laston); 
        break;
    case 'F': 
        sprintf(s,"%d",thisuser.fpts); 
        break;
    case 'G': 
        sprintf(s,"%s",conf[curconf].name); 
        break;
    case 'H': 
        sprintf(s,"%s",pnam(&thisuser));
        break;
    case 'I': 
        strcpy(s,""); 
        break;
    case 'J': 
        sprintf(s,"%d",thisuser.dsl); 
        break;
    case 'K': 
        sprintf(s,"%-4ld",thisuser.uk); 
        break;
    case 'L': 
        sprintf(s,"%d",usernum); 
        break;
    case 'M': 
        nl(); 
        break;
    case 'N': 
        strcpy(s,nam(&thisuser,usernum)); 
        break;
    case 'O': 
        sprintf(s,"%-4d",thisuser.downloaded); 
        break;
    case 'P': 
        pausescr(); 
        break;
    case 'Q': 
        sprintf(s,"%d",numf); 
        break;
    case 'R': 
        sprintf(s,"%s",thisuser.realname); 
        break;
    case 'S': 
        itoa(thisuser.sl,s,10); 
        break;
    case 'T': 
        strcpy(s,ctim(nsl())); 
        break;
    case 'U': 
        sprintf(s,"%-4d",thisuser.uploaded); 
        break;
    case 'V': 
        sprintf(s,"%d",msgr); 
        break;
    case 'W': 
        sprintf(s,"%d",nummsgs); 
        break;
    case 'X': 
        sprintf(s,"%-4ld",thisuser.dk); 
        break;
    case 'Y': 
        delay(500); 
        break;
    case 'Z': 
        outstr(get_say(0)); 
        break;
    default: 
        break;
    }
    strcpy(MCISTR,s);
}

