/*
 * bbs_input.c — BBS input multiplexer (Layer 5)
 *
 * How keys arrive from any source: local keyboard, remote TCP,
 * macro buffer, sess.quote buffer.  Extracted from com.c.
 */

#include "io_ncurses.h"  /* MUST come before vars.h */
#include "platform.h"
#include "bbs_input.h"
#include "bbs_output.h"
#include "bbs_ui.h"
#include "user/userdb.h"
#include "tcpio.h"
#include "conio.h"
#include "bbsutl.h"
#include "sysoplog.h"
#include "utility.h"
#include "stringed.h"
#include "timest.h"
#include "session.h"
#include "system.h"
#include "terminal_bridge.h"
#include "terminal/ansi_attr.h"
#include "cmd_registry.h"
#include "acs.h"

#pragma hdrstop


/***********************************************************************
 * 1. LOW-LEVEL INPUT
 ***********************************************************************/

/* kbhitb — non-blocking local keyboard check.
 * Delegates to Terminal which handles ncurses wgetch/ungetch internally. */


int kbhitb()
{
    return term_local_key_ready();
}

/* getchd — BLOCKING read of one key from local keyboard.
 * Terminal handles: ncurses special keys → DOS scan codes (two-byte pattern),
 * KEY_ENTER/LF → CR, KEY_BACKSPACE/127(DEL) → 8(BS). */
char getchd()
{
    return (char)term_local_get_key();
}

/* getchd1 — NON-BLOCKING read of one key from local keyboard.
 * Returns 255 if nothing available (not 0, since 0 is the NUL scan-code prefix).
 * Terminal handles same key translations as getchd(). */
char getchd1()
{
    return (char)term_local_get_key_nb();
}


/***********************************************************************
 * 2. CONNECTION CHECK
 ***********************************************************************/

void checkhangup()
{
    auto& sess = Session::instance();
    int i, ok;

    if (!io.hangup && using_modem && !cdet()) {
        ok = 0;
        for (i = 0; (i < 500) && !ok; i++)
            if (cdet())
                ok = 1;
        if (!ok) {
            io.hangup = io.hungup = 1;
            if (sess.useron && !sess.in_extern)
                sysoplog("Hung Up.");
        }
    }
}

/* empty — returns 1 if no input is available from ANY source.
 * Checks: local keyboard (kbhitb), remote TCP (comhit), macro buffer
 * (io.charbufferpointer), external program pipe (sess.in_extern==2), and
 * sess.quote buffer (sess.bquote).  Used by getkey() to spin-wait for input. */
int empty()
{
    auto& sess = Session::instance();
    if(io.x_only) return 0;

    if (kbhitb() || (incom && comhit()) ||
        (io.charbufferpointer && io.charbuffer[io.charbufferpointer]) ||
        (sess.in_extern == 2)||sess.bquote)
        return(0);
    return(1);
}


/***********************************************************************
 * 3. KEY POST-PROCESSING
 ***********************************************************************/

/* skey1 — post-process a key after reading from any source.
 * Called by inkey() on every character.  Handles:
 *   - 127→8 mapping (DEL→BS, defense in depth)
 *   - Ctrl-A/D/F/Y → expand user macros from sess.user.macro(N)
 *   - Ctrl-T → display time remaining (ptime)
 *   - Ctrl-R → reprint last line (reprint)
 * Writes result back through *ch pointer. */
void skey1(char *ch)
{
    auto& sess = Session::instance();
    char c;
    User u;

    c = *ch;
    if (c == 127)
        c = 8;
    switch(c) {
    case 1:
    case 4:
    case 6:
    case 25:
        if (sess.okmacro && !io.charbufferpointer) {
            if (c == 1)
                c = 2;
            else if (c == 4)
                c = 0;
            else if (c == 6)
                c = 1;
            else if (c== 25)
                c=3;
            if(io.lastcon) {
                { auto p = UserDB::instance().get(1); if (p) u = *p; }
                strcpy(io.charbuffer, u.macro(c));
            }
            else if (sess.okskey)
                strcpy(io.charbuffer, sess.user.macro(c));
            c = io.charbuffer[0];
            if (c)
                io.charbufferpointer = 1;
        }
        break;
    case 20:
        if (io.echo)
            ptime();
        break;
    case 18:
        if (io.echo)
            reprint();
        break;
    case '\x15':
        if (sess.okskey) break;
        nl();
        ex("OP","3");
        break;
    }
    *ch = c;
}


/***********************************************************************
 * 4. INPUT ROUTING
 ***********************************************************************/

/* inkey — non-blocking read from ANY input source.
 *
 * Input priority (first match wins):
 *   1. Quote buffer (sess.bquote)  — auto-sess.quoting in message reply
 *   2. Macro buffer (io.charbufferpointer) — Ctrl-A/D/F/Y macro expansion
 *   3. Local keyboard (kbhitb → getchd1) — sysop console via ncurses
 *   4. Remote TCP (comhit → get1c) — telnet user
 *
 * Returns 0 if nothing available.  Calls skey1() to post-process
 * (macro expansion, 127→8 mapping).  Sets io.lastcon=1 for local, 0 for remote.
 *
 * For the scan-code two-byte sequence: first call returns 0 (NUL prefix),
 * getkey() loops on !ch, second call returns the scan code via getchd1()
 * which reads _pending_scancode, then skey() dispatches the function key. */
char inkey()
{
    auto& sess = Session::instance();
    char ch=0;
    static int qpointer=0,cpointer;

    if (sess.bquote) {
        if (!qpointer) {
            io.charbuffer[1]=0;
            cpointer=0;
            qpointer=1;
            while (qpointer<sess.bquote) {
                if (sess.quote[cpointer++]==13)
                    ++qpointer;
            }
            io.charbufferpointer=1;
        }
        if (sess.quote[cpointer]==3)
            sess.quote[cpointer]=16;
        if (sess.quote[cpointer]==14)
            sess.quote[cpointer]=5;
        if (sess.quote[cpointer]==0) {
            qpointer=0;
            sess.bquote=0;
            sess.equote=0;
            return(13);
        }
        return(sess.quote[cpointer++]);
    }


    if (io.x_only)
        return(0);

    if (io.charbufferpointer) {
        if (!io.charbuffer[io.charbufferpointer])
            io.charbufferpointer = io.charbuffer[0] = 0;
        else {
            if ((io.charbuffer[io.charbufferpointer])==3)
                io.charbuffer[io.charbufferpointer]=16;
            return(io.charbuffer[io.charbufferpointer++]);
        }
    }
    if (kbhitb() || (sess.in_extern == 2)) {
        ch = getchd1();
        io.lastcon = 1;
        if (!ch) {
            if (sess.in_extern)
                sess.in_extern = 2;
            else {
                ch = getchd1();
                skey(ch);
                ch = (((ch == 68) || (ch==103)) ? 2 : 0);
            }
        }
        else if (sess.in_extern)
            sess.in_extern = 1;
        sess.timelastchar1=timer1();
    }
    else if (incom && comhit()) {
        ch = get1c();
        io.lastcon = 0;
    }
    skey1(&ch);
    return(ch);
}


/***********************************************************************
 * 5. BLOCKING INPUT
 ***********************************************************************/

/* getkey — BLOCKING read of one key from any source.
 *
 * The main input entry point for most BBS code.  Spins on empty() until
 * input arrives (from keyboard, TCP, or macro buffer), then returns the
 * character from inkey().  Has an inactivity timeout: beeps after half
 * the limit, hangs up after the full limit ("you appear to have fallen asleep").
 *
 * For function keys: inkey() returns 0 (NUL prefix) on first call, getkey()
 * loops (!ch), inkey() returns the scan code on second call, getkey() returns it.
 *
 * Call chain: getkey → inkey → [kbhitb + getchd1] or [comhit + get1c] */
unsigned char getkey()
{
    auto& sess = Session::instance();
    unsigned char ch;
    int beepyet;
    long dd,tv,tv1;

    beepyet = 0;
    sess.timelastchar1=timer1();

    tv=3276L;

    tv1=tv/2;

    io.lines_listed = 0;
    do {
        while (empty() && !io.hangup) {
            dd = timer1();
            if ((dd<sess.timelastchar1) && ((dd+1000)>sess.timelastchar1))
                sess.timelastchar1=dd;
            if (labs(dd - sess.timelastchar1) > 65536L)
                sess.timelastchar1 -= 1572480L;
            if (((dd - sess.timelastchar1) > tv1) && (!beepyet)) {
                beepyet = 1;
                outchr(7);
            }
            if (labs(dd - sess.timelastchar1) > tv) {
                nl();
                pl("Sorry, but you appear to have fallen asleep!");
                nl();
                io.hangup=1;
                wait1(27);
            }
            checkhangup();
        }
        ch = inkey();
    }
    while (!ch && !sess.in_extern && !io.hangup);
    if (ch == 127) ch = 8;  /* macOS sends DEL for backspace */
    return(ch);
}


/***********************************************************************
 * 6. MESSAGE EDITOR LINE INPUT (moved from bbsutl.cpp)
 ***********************************************************************/

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
    auto& sys = System::instance();
    auto& sess = Session::instance();
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
                if( ( wherex()< (sess.user.screenchars()-1) || !roll)   && (cp<maxlen)) {
                s[cp++]=ch;
                outchr(ch);
                if (wherex()==(sess.user.screenchars()-1)&&roll)
                    done=1;
            }
            else {
                if (wherex()>=(sess.user.screenchars()-1)&&roll)
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
                    pl("0[�9Previous Line0�]");
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
                if (((cp+i)<maxlen) && ((wherex()+i)<sess.user.screenchars())) {
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


/***********************************************************************
 * 7. MENU KEY INPUT (moved from bbsutl.cpp)
 ***********************************************************************/

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

    sprintf(s,"%s\b\r",avail);

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
