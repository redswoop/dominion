#include "vars.h"

#pragma hdrstop

#include <stdarg.h>
#include <math.h>

#define sysstatus_pause_on_message 0x0400

char mci=0,easycolor=0,bluein=0;
extern char MCISTR[161];
int colblock;
extern int readms;

void outstrm(char *s);

unsigned char upcase(unsigned char ch)
{
    if(ch>=128) return(ch);
    else return(toupper(ch));
}

int strlenc(char *s)
{
    int len=0,x=0;
    while(s[x]) {
        if(s[x]!=3&&s[x]!=14&&s[x]!='`') {
            len++; 
            x++;
        }
        else {
            x++;
            setmci(toupper(s[x]));
            len+=strlen(MCISTR);
            x++;
        }
    }
    return len;
}



void savel(char *cl, char *atr, char *xl, char *cc)
{
    int i, i1;

    *cc = curatr;
    strcpy(xl, endofline);
    i = ((wherey() + topline) * 80) * 2;
    for (i1 = 0; i1 < wherex(); i1++) {
        cl[i1]  = scrn[i + (i1 * 2)];
        atr[i1] = scrn[i + (i1 * 2) + 1];
    }
    cl[wherex()]  = 0;
    atr[wherex()] = 0;
}


void restorel(char *cl, char *atr, char *xl, char *cc)
{
    int i;

    if (wherex())
        nl();
    for (i = 0; cl[i] != 0; i++) {
        setc(atr[i]);
        outchr(cl[i]);
    }
    setc(*cc);
    strcpy(endofline, xl);
}



void checkhangup()
{
    int i, ok;

    if (!hangup && using_modem && !cdet()) {
        ok = 0;
        for (i = 0; (i < 500) && !ok; i++)
            if (cdet())
                ok = 1;
        if (!ok) {
            hangup = hungup = 1;
            if (useron && !in_extern)
                sysoplog("Hung Up.");
        }
    }
}


void addto(char *s, int i)
{
    char temp[20];

    if (s[0])
        strcat(s, ";");
    else
        strcpy(s, "\x1B[");
    itoa(i, temp, 10);
    strcat(s, temp);
}

void makeavt(unsigned char attr, char *s, int forceit)
{
    unsigned char catr;

    catr = curatr;
    s[0] = 0;

    if (attr != catr) sprintf(s,"%c",attr);
    if (!okavt() && !forceit)
        s[0]=0;
}

void makeansi(unsigned char attr, char *s, int forceit)
{
    unsigned char catr;
    char *temp = "04261537";

    catr = curatr;
    s[0] = 0;

    if(okavt()) {
        makeavt(attr,s,forceit);
        return;
    }

    if (attr != catr) {
        if ((catr & 0x88) ^ (attr & 0x88)) {
            addto(s, 0);
            addto(s, 30 + temp[attr & 0x07] - '0');
            addto(s, 40 + temp[(attr & 0x70) >> 4] - '0');
            catr = attr & 0x77;
        }
        if ((catr & 0x07) != (attr & 0x07))
            addto(s, 30 + temp[attr & 0x07] - '0');
        if ((catr & 0x70) != (attr & 0x70))
            addto(s, 40 + temp[(attr & 0x70) >> 4] - '0');
        if ((catr & 0x08) ^ (attr & 0x08))
            addto(s, 1);
        if ((catr & 0x80) ^ (attr & 0x80))
            addto(s, 5);
    }
    if (s[0])
        strcat(s, "m");
    if (!okansi() && !forceit)
        s[0]=0;
}



void setfgc(int i)
{
    curatr = (curatr & 0xf8) | i;
}



void setbgc(int i)
{
    curatr = (curatr & 0x8f) | (i << 4);
}


void execute_ansi()
{
    int args[11], argptr, count, ptr, tempptr, ox, oy;
    char cmd, temp[11], teol[81], *clrlst = "04261537";
    union REGS r;

    if (ansistr[1] != '[') {

    } 
    else {
        argptr = tempptr = 0;
        ptr = 2;
        for (count = 0; count < 10; count++)
            args[count] = temp[count] = 0;
        cmd = ansistr[ansiptr - 1];
        ansistr[ansiptr - 1] = 0;
        while ((ansistr[ptr]) && (argptr<10) && (tempptr<10)) {
            if (ansistr[ptr] == ';') {
                temp[tempptr] = 0;
                tempptr = 0;
                args[argptr++] = atoi(temp);
            } 
            else
                temp[tempptr++] = ansistr[ptr];
            ++ptr;
        }
        if (tempptr && (argptr<10)) {
            temp[tempptr]  = 0;
            args[argptr++] = atoi(temp);
        }
        if ((cmd >= 'A') && (cmd <= 'D') && !args[0])
            args[0] = 1;
        switch (cmd) {
        case 'f':
        case 'H':
            movecsr(args[1] - 1, args[0] - 1);
            break;
        case 'A':
            movecsr(wherex(), wherey() - args[0]);
            break;
        case 'B':
            movecsr(wherex(), wherey() + args[0]);
            break;
        case 'C':
            movecsr(wherex() + args[0], wherey());
            break;
        case 'D':
            movecsr(wherex() - args[0], wherey());
            break;
        case 's':
            oldx = wherex();
            oldy = wherey();
            break;
        case 'u':
            movecsr(oldx, oldy);
            break;
        case 'J':
            if (args[0] == 2) {
                clrscrb();
            }
            break;
        case 'k':
        case 'K':
            ox = wherex();
            oy = wherey();
            r.x.cx = 80 - ox;
            r.h.ah = 0x09;
            r.h.bh = 0x00;
            r.h.al = 32;
            r.h.bl = curatr;
            int86(0x10,&r,&r);
            movecsr(ox, oy);

            break;
        case 'm':
            if (!argptr) {
                argptr = 1;
                args[0] = 0;
            }
            for (count = 0; count < argptr; count++)
                switch (args[count]) {
                case 0: 
                    curatr = 0x07; 
                    break;
                case 1: 
                    curatr = curatr | 0x08; 
                    break;
                case 4: 
                    break;
                case 5: 
                    curatr = curatr | 0x80;
                    break;
                case 7:
                    ptr = curatr & 0x77;
                    curatr = (curatr & 0x88) | (ptr << 4) | (ptr >> 4);
                    break;
                case 8: 
                    curatr = 0; 
                    break;
                default:
                    if ((args[count] >= 30) && (args[count] <= 37))
                        setfgc(clrlst[args[count] - 30] - '0');
                    else if ((args[count] >= 40) && (args[count] <= 47))
                        setbgc(clrlst[args[count] - 40] - '0');
                }
            break;
        }
    }
    ansiptr = 0;
}

unsigned char ac=0,pipe=0,pipestr[5],ac2;

void outchr(unsigned char c)
{
    int i, i1;
    char s[15];

    if(pipe) {
        if(c>='0'&&c<='9') {
            pipestr[pipe-1]=c;
            pipe++;
            return;
        } 
        else {
            pipestr[pipe-1]=0;
            pipe=0;
            if(okansi()) {
                makeansi((unsigned char)atoi(pipestr),s,1);
                outstr(s);
            }
        }
    }

    if(c==5&&ac==0) {
        ac=10;
        return;
    }

    if(ac==10) {
        ac=0;
        if(okansi()) {
            makeansi(c,s,1);
            outstr(s);
        }
        return;
    }

    if(ac==101) {
        ac=0;
        for(i=0;i<c;i++)
            outchr(ac2);
        return;
    }

    if(ac==100) {
        ac2=c;
        ac=101;
        return;
    }

    if(c==22&&ac==0) {
        if(outcom)
            outcomch(c);
        ac=1;
        return;
    }

    if(ac==1) {
        ac=2;
        if(outcom)
            outcomch(c);
        return;
    }

    if(ac==2) {
        curatr=c;
        if(outcom)
            outcomch(c);
        ac=0;
        return;
    }

    if(easycolor) {
        easycolor=0;
        if(c>0&&c<=20)
            ansic(c-1);
        return;
    }


    if(mci) {
        mci=0;
        if(mciok) { 
            setmci(c); 
            outstr(MCISTR); 
            return; 
        }
    }

    if (change_color) {
        change_color = 0;
        if ((c >= '0') && (c <= '9'))
            ansic(c - '0');
        return;
    }

    if (change_ecolor) {
        change_ecolor = 0;
        if ((c >= '0') && (c <= '9'))
            ansic(c - '0' + 10);
        return;
    }

    if (c == 3) {
        change_color=1;
        return;
    }

    if (c == 14) {
        change_ecolor=1;
        return;
    }

    if(c==6) {
        easycolor=1;
        return;
    }

    if(c=='`') {
        if(mciok&&!mci) { 
            mci=1; 
            return; 
        }
    }

    if(c=='|') {
        if(mciok) { 
            pipe=1; 
            return; 
        } 
    }

    if(c==151) { 
        if(mciok) { 
            ac=100; 
            return; 
        } 
    }

    if ((c == 10) && endofline[0]) {
        outstr(endofline);
        endofline[0] = 0;
    }

    if (global_handle) {
        if (echo)
            global_char(c);
    }

    if (chatcall && !x_only && !(syscfg.sysconfig & sysconfig_no_beep))
        setbeep(1);

    if (outcom && (c != 9))
        outcomch(echo ? c : nifty.echochar);
    if (ansiptr) {
        ansistr[ansiptr++] = c;
        ansistr[ansiptr]   = 0;
        if ((((c < '0') || (c > '9')) && (c!='[') && (c!=';')) ||
            (ansistr[1] != '[') || (ansiptr>75))
            execute_ansi();
    } 
    else if (c == 27) {
        ansistr[0] = 27;
        ansiptr = 1;
        ansistr[ansiptr]=0;
    } 
    else {
        if (c == 9) {
            i1 = wherex();
            for (i = i1; i< (((i1 / 8) + 1) * 8); i++)
                outchr(32);
        } 
        else if (echo || lecho) {
            out1ch(c);
            if (c==12&&okansi()) outstrm("[0;1m");
            if (c == 10) {
                ++lines_listed;
                if (((sysstatus_pause_on_page & thisuser.sysstatus)) &&
                    (lines_listed >= screenlinest - 1)&&!listing) {
                    pausescr();
                    lines_listed=0;
                } 
                else
                    if (((sysstatus_pause_on_message & thisuser.sysstatus)) &&
                    (lines_listed >= screenlinest - 1)&&readms) {
                    pausescr();
                    lines_listed=0;
                }
            }
        } 
        else
            out1ch(nifty.echochar);
    }
    if (chatcall)
        setbeep(0);
}




void outstr(char *s)
{
    int i=0;
    int slen,x;


    checkhangup();

    if(hangup) return;

    if(s[0]=='–') {
            i++;
        slen=strlenc(s);
        x=79;
        x-=slen;
        x/=2;
        for(slen=0;slen<x-2;slen++)
            outchr(' ');
    }

    if(s[0]=='ñ') {
            dtitle(&s[1]);
        return;
    }

    if(s[0]=='˜') {
            printfile(&s[1]);
        return;
    }

    while (s[i]&&!hangup)
        outchr(s[i++]);

}

void outstrm(char *s)
{
    int i=0;

    if(!outcom||!incom||!strcmp(curspeed,"KB")) return;
    checkhangup();
    while (s[i]&&!hangup)
        outcomch(s[i++]);
}


void nl()
{
    if (endofline[0]) {
        outstr(endofline);
        endofline[0] = 0;
    }
    outstr("\r\n");
}

void backblue(void)
{
    int i;
    char s[15];

    i = echo;
    echo = 1;
    if(bluein==1)
        outstr("[D±[D");
    else {
        makeansi(8,s,1);
        outstr(s);
        outstr("[Dù[D");
        makeansi(15,s,1);
        outstr(s);
    }
    echo = i;
}

void backspace()
{
    int i;

    i = echo;
    echo = 1;
    outstr("\b \b");
    echo = i;
}



void setc(unsigned char ch)
{
    char s[30];

    makeansi(ch,s,0);
    outstr(s);
}

void pausescr()
{
    int i,i1,len;

    i=curatr;
    outstr(get_string(20));
    len=strlenc(get_string(20));
    getkey();
    for(i1=0;i1<len;i1++) backspace();
    curatr=i;
}


void npr(char *fmt, ...)
{
    va_list ap;
    char s[512];

    va_start(ap, fmt);
    vsprintf(s, fmt, ap);
    va_end(ap);
    outstr(s);
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


void logpr(char *fmt, ...)
{
    va_list ap;
    char s[512];

    va_start(ap, fmt);
    vsprintf(s, fmt, ap);
    va_end(ap);
    sysoplog(s);
}

void pl(char *s)
{
    outstr(s);
    nl();
}


int kbhitb()
{
    union REGS r;
#ifdef MOUSE
    if(checklclick()) executemouse(getmx(),getmy());
#endif

    r.h.ah = 1;
    int86(0x16, &r, &r);
    return((r.x.flags & 64) == 0);
}


int empty()
{
    if(x_only) return 0;

    if (kbhitb() || (incom && comhit()) ||
        (charbufferpointer && charbuffer[charbufferpointer]) ||
        (in_extern == 2)||bquote)
        return(0);
    return(1);
}


void skey1(char *ch)
{
    char c;
    userrec u;

    c = *ch;
    if (c == 127)
        c = 8;
    switch(c) {
    case 1:
    case 4:
    case 6:
    case 25:
        if (okmacro && !charbufferpointer) {
            if (c == 1)
                c = 2;
            else if (c == 4)
                c = 0;
            else if (c == 6)
                c = 1;
            else if (c== 25)
                c=3;
            if(lastcon) {
                read_user(1,&u);
                strcpy(charbuffer, &(u.macros[c][0]));
            } 
            else if (okskey)
                strcpy(charbuffer, &(thisuser.macros[c][0]));
            c = charbuffer[0];
            if (c)
                charbufferpointer = 1;
        }
        break;
    case 20:
        if (echo)
            ptime();
        break;
    case 18:
        if (echo)
            reprint();
        break;
    case '': 
        if (okskey) break;
        nl();
        ex("OP","3");
        break;
    }
    *ch = c;
}

char getchd()
{
    union REGS r;

    r.h.ah = 0x07;
    int86(0x21, &r, &r);
    return(r.h.al);
}


char getchd1()
{
    union REGS r;

    r.h.ah = 0x06;
    r.h.dl = 0xFF;
    int86(0x21, &r, &r);
    return((r.x.flags & 0x40) ? 255 : r.h.al);
}


char inkey()
{
    char ch=0;
    static int qpointer=0,cpointer;

    if (bquote) {
        if (!qpointer) {
            charbuffer[1]=0;
            cpointer=0;
            qpointer=1;
            while (qpointer<bquote) {
                if (quote[cpointer++]==13)
                    ++qpointer;
            }
            charbufferpointer=1;
        }
/*        if (quote[cpointer]==13) {
            ++qpointer;
            if (qpointer>equote) {
                qpointer=0; 
                bquote=0; 
                equote=0;

                return(13);
            } 
            else
                ++cpointer;
        }*/
        if (quote[cpointer]==3)
            quote[cpointer]=16;
        if (quote[cpointer]==14)
            quote[cpointer]=5;
        if (quote[cpointer]==0) {
            qpointer=0; 
            bquote=0; 
            equote=0;
            return(13);
        }
        return(quote[cpointer++]);
    }


    if (x_only)
        return(0);

    if (charbufferpointer) {
        if (!charbuffer[charbufferpointer])
            charbufferpointer = charbuffer[0] = 0;
        else {
            if ((charbuffer[charbufferpointer])==3)
                charbuffer[charbufferpointer]=16;
            return(charbuffer[charbufferpointer++]);
        }
    }
    if (kbhitb() || (in_extern == 2)) {
        ch = getchd1();
        lastcon = 1;
        if (!ch) {
            if (in_extern)
                in_extern = 2;
            else {
                ch = getchd1();
                skey(ch);
                ch = (((ch == 68) || (ch==103)) ? 2 : 0);
            }
        } 
        else if (in_extern)
            in_extern = 1;
        timelastchar1=timer1();
    } 
    else if (incom && comhit()) {
        ch = (get1c() & andwith);
        lastcon = 0;
    }
    skey1(&ch);
    return(ch);
}

void mpl1(int i);
void mpl(int i)
{
    int i1;
    char s[20];

    if(!okansi())
        return;

    if(nifty.nifstatus & nif_comment) {
        mpl1(i); 
        return;
    }
    bluein=1;
    makeansi(31,s,0); 
    outstr(s);
    for(i1=0;i1<i;i1++) outchr(177);
    for(i1=0;i1<i;i1++) outstr("[D");
}

void mpl1(int i)
{
    int i1;
    char s[20];

    bluein=2;
    makeansi(8,s,1); 
    outstr(s);
    for(i1=0;i1<i;i1++) outchr('ù');
    for(i1=0;i1<i;i1++) outstr("[D");
    makeansi(15,s,1); 
    outstr(s);
}


unsigned char getkey()
{
    unsigned char ch;
    int beepyet;
    long dd,tv,tv1;

    beepyet = 0;
    timelastchar1=timer1();

    tv=3276L;

    tv1=tv/2;

    lines_listed = 0;
    do {
        while (empty() && !hangup) {
            dd = timer1();
            if ((dd<timelastchar1) && ((dd+1000)>timelastchar1))
                timelastchar1=dd;
            if (labs(dd - timelastchar1) > 65536L)
                timelastchar1 -= 1572480L;
            if (((dd - timelastchar1) > tv1) && (!beepyet)) {
                beepyet = 1;
                outchr(7);
            }
            if (labs(dd - timelastchar1) > tv) {
                nl();
                pl("Sorry, but you appear to have fallen asleep!");
                nl();
                hangup=1;
                wait1(27);
            }
            checkhangup();
        }
        ch = inkey();
    } 
    while (!ch && !in_extern && !hangup);
    return(ch);
}

/*int input1(char *s,int maxlen, int lc, int crend)
{
    int i,done=0,cur_pos=0;
    char ch,s1[19];

    memset(s,32,maxlen);

    if(okansi()) {
        if(bluein==1) {
            makeansi(31,s1,1);
            outstr(s1);
        }
        else if(bluein==2) {
            makeansi(15,s1,1);
            outstr(s1);
        }
    }

    do {
        ch=getkey();
        if(ch>31) {
            if(cur_pos<maxlen) {
                if(!lc) ch=upcase(ch);
                s[cur_pos++]=ch;
                outchr(ch);
            }
        } else {
            switch(ch) {
               case 13: s[maxlen]=0;
                       done=echo=1;
                        break;
                case 8: if(cur_pos) {
                            if(bluein)
                                backblue();
                            else
                                backspace();
                            s[cur_pos]=32;
                            cur_pos--;
                        }
                        break;
               case 27: ch=getkey();
                        ch=getkey();
                        if(ch=='C') {
                            if(cur_pos<maxlen) {
                                cur_pos++;
                                outstr("[C");
                            }
                        } else if(ch=='D') {
                            if(cur_pos) {
                                cur_pos--;
                                outstr("[D");
                            }
                        } else if(ch=='H') {
                            while(cur_pos) {
                                outstr("[D");
                                cur_pos--;
                            }
                        } else if(ch=='K') {
                            while(cur_pos<maxlen) {
                                outstr("[C");
                                cur_pos++;
                            }
                        }
                        break;
               case 24: while(cur_pos) {
                            s[cur_pos]=32;
                            cur_pos--;
                            if(bluein)
                                backblue();
                            else
                                backspace();
         
                        }
                        cur_pos=0;
                        break;
            }
        }
    } while(!done&&!hangup);

    for(i=strlen(s)-1;i>=0 && s[i]==32;i--);
    s[i+1]=0;
    bluein=0;
    ansic(0);
    if(crend)
        nl();
 
    return 0;
}
*/

int input1(char *s, int maxlen, int lc, int crend)
{
    int curpos=0, done=0, in_ansi=0;
    unsigned char ch;
    char s1[19];

    if(!okansi())
        bluein=0;

    if(bluein==1) { 
        makeansi(31,s1,1); 
        outstr(s1); 
    }
    else if(bluein==2) {
        makeansi(15,s1,1); 
        outstr(s1);
    }

    while (!done && !hangup) {
        ch = getkey();
        if (in_ansi) {
            if ((in_ansi==1) && (ch!='['))
                in_ansi=0;
            else {
                if (in_ansi==1)
                    in_ansi=2;
                else if (((ch<'0') || (ch>'9')) && (ch!=';'))
                    in_ansi=3;
                else
                    in_ansi=2;
            }
        }
        if (!in_ansi) {
            if (ch > 31) {
                if (curpos < maxlen) {
                    if (!lc)
                        ch = upcase(ch);
                    s[curpos++] = ch;
                    outchr(ch);
                }
                if(!crend&&curpos==maxlen) done=1;
            } 
            else
                switch(ch) {
            case 14:
            case 13:
                s[curpos] = 0;
                done = echo = 1;
                break;
            case 23: /* Ctrl-W */
                if (curpos) {
                    do {
                        curpos--;
                        if(bluein)
                            backblue();
                        else
                            backspace();
                        if (s[curpos]==26) {
                            if(bluein)
                                backblue();
                            else
                                backspace();
                        }
                    } 
                    while ((curpos) && (s[curpos-1]!=32));
                }
                break;
            case 26:
                if (input_extern) {
                    s[curpos++] = 26;
                    outstr("^Z");
                }
                break;
            case 8:
                if (curpos) {
                    curpos--;
                    if(bluein)
                        backblue();
                    else
                        backspace();
                    if (s[curpos] == 26) {
                        if(bluein)
                            backblue();
                        else
                            backspace();

                    }
                } 
                else if(!crend) return -1;
                break;
            case 21:
            case 24:
                while (curpos) {
                    curpos--;
                    if(bluein)
                        backblue();
                    else
                        backspace();

                    if (s[curpos] == 26) {
                        if(bluein)
                            backblue();
                        else
                            backspace();

                    }
                }
                break;
            case 27:
                in_ansi=1;
                break;
            }
        }
        if (in_ansi==3)
            in_ansi=0;
    }
    if (hangup)
        s[0] = 0;
    if(bluein) { 
        bluein=0; 
        makeansi(15,s1,1); 
        outstr(s1); 
    }
    if(crend) nl();
    return 0;
}

void inputdate(char *s,int time)
{
    int curpos=0, done=0;
    unsigned char ch;
    char s1[19];

    if(bluein==1) { 
        makeansi(31,s1,1); 
        outstr(s1); 
    }
    else if(bluein==2) {
        makeansi(15,s1,1); 
        outstr(s1);
    }

    while (!done && !hangup) {
        sprintf(s1,"1234567890%c%c%c",8,13,14);
        ch = nek(s1,0);
        if (ch > 31) {
            if (curpos < 8) {
                s[curpos++] = ch;
                outchr(ch);
                if(curpos==2||curpos==5) {
                    s[curpos]=time?':':'/';
                    outchr(time?':':'/');
                    curpos++;
                }
            }
        } 
        else
            switch(ch) {
        case 13:
            s[curpos] = 0;
            done = echo = 1;
            break;
        case 8:
            if (curpos>0) {
                curpos--;
                backspace();
                if(curpos==2||curpos==5) {
                    curpos--;
                    backspace();
                }
            }
            break;
        }
    }
    if (hangup)
        s[0] = 0;
    ansic(0);
}

int inputfone(char *s)
{
    int curpos=0, done=0;
    unsigned char ch;
    char s1[19];

    if(bluein==1) { 
        makeansi(31,s1,1); 
        outstr(s1); 
    }
    else if(bluein==2) {
        makeansi(15,s1,1); 
        outstr(s1);
    }

    while (!done && !hangup) {
        sprintf(s1,"1234567890%c%c%c+",8,13,14);
        ch = nek(s1,0);
        if(ch=='+') {
            input(s,12);
            return 1;
        }
        if (ch > 31) {
            if (curpos < 12) {
                s[curpos++] = ch;
                outchr(ch);
                if(curpos==3||curpos==7) {
                    s[curpos]='-';
                    outchr('-');
                    curpos++;
                }
            }
        } 
        else
            switch(ch) {
        case 13:
            s[curpos] = 0;
            done = echo = 1;
            break;
        case 8:
            if (curpos>0) {
                curpos--;
                backspace();
                if(s[curpos]=='-') {
                    curpos--;
                    backspace();
                }
            }
            break;
        }
    }
    if (hangup)
        s[0] = 0;

    ansic(0);
    return 0;
}

void input(char *s, int len)
{
    input1(s, len, 0, 1);
}



void inputl(char *s, int len)
{
    input1(s, len, 1, 1);
}



int ynn(int pos)
{
    char ch=0,len,i,done=0;

    //  npr("pos=%d ",pos);

    len=strlenc(get_string(pos?52:51));
    outstr(get_string(pos?52:51));

    while(!hangup && !done) {
        switch(ch=getkey()) {
        case 'n':
        case 'N': 
            for(i=len;i>0;i--) backspace();
            pl(get_string(10));
            return 0;
        case 'y':
        case 'Y': 
            for(i=len;i>0;i--) backspace();
            pl(get_string(9));
            return 1;
        case 13: 
            for(i=len;i>0;i--) backspace();
            if(pos) pl(get_string(10));
            else pl(get_string(9));
            if(pos) return 0;
            else return 1;
        case 27: 
            ch=getkey();
            ch=getkey();
            if(ch=='C') {
                pos=1;
                for(i=len;i>0;i--) backspace();
                len=strlenc(get_string(52));
                outstr(get_string(52));
            }
            else if(ch=='D') {
                pos=0;
                for(i=len;i>0;i--) backspace();
                len=strlenc(get_string(51));
                outstr(get_string(51));
            }
            break;
        }
    }
    return 1;
}


int ny()
{
    return(ynn(0));
}

int yn()
{
    return(ynn(1));
}

void ansic(int n)
{
    char c,s[10];

    if(colblock)
        c=nifty.defaultcol[n];
    else
        c = thisuser.colors[n];
    if (c == curatr) return;

    setc(c);

    makeansi(thisuser.colors[0],endofline, 0);
}


char nek(char *s, int f)
{
    char ch;

    while (!strchr(s, ch = toupper(getkey())) && !hangup);
    if (hangup) ch = s[0];
    if(f) {
        outchr (ch);
        nl();
    }
    return(ch);
}

char onek(char *s)
{
    return(nek(s,1));
}

void prt(int i, char *s)
{
    ansic(i);
    outstr(s);
    ansic(0);
}

void inputdat(char msg[81],char *s, int len,int lc)
{
    npr("3%s\r\n5: ",msg);
    mpl(len);
    input1(s,len,lc,1);
}

