/*
 * bbs_ui.c — BBS interactive widgets (Layer 5)
 *
 * User-facing input patterns built on output + input.
 * Extracted from com.c: field drawing, string editing, yes/no prompts,
 * single-key choice, pause screen.
 */

#include "io_ncurses.h"  /* MUST come before vars.h */
#include "platform.h"
#include "fcns.h"
#include "session.h"
#include "system.h"
#include "terminal_bridge.h"
#include "ansi_attr.h"

#pragma hdrstop

/* com.c file-scoped state → io_session_t (Phase 2) */
#define bluein      io.bluein


/***********************************************************************
 * 1. CHARACTER DELETION
 ***********************************************************************/

/* backblue — erase one character inside a blue input field (mpl-style).
 * Sends: ESC[D (cursor left), CP437 0xB1 (▒ field background), ESC[D (cursor left).
 * Net effect: overwrites the deleted char with the field fill character, leaves
 * cursor positioned on it.  Used when bluein!=0 (set by mpl/mpl1).
 *
 * The raw bytes in the string literals are:
 *   \x1b[D  = ANSI cursor-left
 *   \xb1    = CP437 ▒ (light shade — the mpl() field background)
 * Do NOT run encoding-aware tools (sed, iconv) on this function. */

static auto& sys = System::instance();
static auto& sess = Session::instance();

void backblue(void)
{
    int i;
    char s[15];

    i = echo;
    echo = 1;
    if(bluein==1)
        outstr("\x1b[D\xb1\x1b[D");
    else {
        makeansi(8,s,curatr);
        outstr(s);
        outstr("\x1b[D\xb1\x1b[D");
        makeansi(15,s,curatr);
        outstr(s);
    }
    echo = i;
}

/* backspace — erase one character in normal (non-blue-field) context.
 * Sends BS-SPACE-BS: moves cursor left, overwrites with space, moves left again.
 * Temporarily forces echo=1 so the output goes through even if echo is off. */
void backspace()
{
    int i;

    i = echo;
    echo = 1;
    outstr("\b \b");
    echo = i;
}


/***********************************************************************
 * 2. PAUSE SCREEN
 ***********************************************************************/

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


/***********************************************************************
 * 3. INPUT FIELD DRAWING
 ***********************************************************************/

/* mpl — draw a blue input field of i characters.
 * Sets bluein=1, which changes input1()'s backspace behavior from
 * backspace() (BS-SPACE-BS) to backblue() (cursor-left, redraw field bg, cursor-left).
 * Draws i copies of CP437 177 in white-on-blue, then backs the cursor
 * up to the field start so the user types over the fill characters.
 * bluein is cleared by input1 when Enter is pressed. */
void mpl1(int i);
void mpl(int i)
{
    int i1;
    char s[20];

    if(!okansi())
        return;

    if(sys.nifty.nifstatus & nif_comment) {
        mpl1(i);
        return;
    }
    bluein=1;
    makeansi(31,s,curatr);
    outstr(s);
    for(i1=0;i1<i;i1++) outchr(177);
    for(i1=0;i1<i;i1++) outstr("\x1b[D");
}

/* mpl1 — alt input field style (used when nif_comment is set).
 * Sets bluein=2.  Uses CP437 0xF9 as fill instead of 0xB1. */
void mpl1(int i)
{
    int i1;
    char s[20];

    bluein=2;
    makeansi(8,s,curatr);
    outstr(s);
    for(i1=0;i1<i;i1++) outchr('\xF9');
    for(i1=0;i1<i;i1++) outstr("\x1b[D");
    makeansi(15,s,curatr);
    outstr(s);
}


/***********************************************************************
 * 4. STRING INPUT
 ***********************************************************************/

/* input1 — core editable string input field.
 *
 *   s      — output buffer (must hold maxlen+1 bytes)
 *   maxlen — max characters to accept
 *   lc     — 0=force uppercase, 1=mixed case
 *   crend  — 1=require Enter to submit, 0=auto-submit when maxlen reached
 *
 * If mpl() was called first, bluein==1 and backspace uses backblue() to
 * redraw the field background character.  Otherwise uses plain backspace().
 *
 * Handles:  BS(8)=delete char, Ctrl-W(23)=delete word, Ctrl-U/X(21/24)=clear line,
 *           Ctrl-Z(26)=insert ^Z marker (if sess.input_extern), ESC(27)=swallow ANSI sequence.
 *
 * The ANSI swallowing (in_ansi state machine) eats ESC [ nn m sequences that
 * arrive when the remote terminal echoes back color codes — without this,
 * those bytes would be inserted into the input buffer as garbage.
 *
 * Wrappers: input(s,len)=uppercase+Enter, inputl(s,len)=mixed+Enter,
 *           inputdat(msg,s,len,lc)=prompt+mpl+input1. */
int input1(char *s, int maxlen, int lc, int crend)
{
    int curpos=0, done=0, in_ansi=0;
    unsigned char ch;
    char s1[19];

    if(!okansi())
        bluein=0;

    if(bluein==1) {
        makeansi(31,s1,curatr);
        outstr(s1);
    }
    else if(bluein==2) {
        makeansi(15,s1,curatr);
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
                if (sess.input_extern) {
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
        makeansi(15,s1,curatr);
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
        makeansi(31,s1,curatr);
        outstr(s1);
    }
    else if(bluein==2) {
        makeansi(15,s1,curatr);
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
        makeansi(31,s1,curatr);
        outstr(s1);
    }
    else if(bluein==2) {
        makeansi(15,s1,curatr);
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

/* input — read string, forced uppercase, Enter to submit. */
void input(char *s, int len)
{
    input1(s, len, 0, 1);
}

/* inputl — read string, mixed case, Enter to submit. */
void inputl(char *s, int len)
{
    input1(s, len, 1, 1);
}

/* inputdat — prompt + blue input field + input1.  All-in-one for data entry forms. */
void inputdat(char msg[MAX_PATH_LEN],char *s, int len,int lc)
{
    npr("3%s\r\n5: ",msg);
    mpl(len);
    input1(s,len,lc,1);
}


/***********************************************************************
 * 5. CHOICE INPUT
 ***********************************************************************/

/* ynn — yes/no prompt with arrow-key selection.  pos=0 defaults No, pos=1 defaults Yes.
 * yn() and ny() are convenience wrappers. */
int ynn(int pos)
{
    char ch=0,len,i,done=0;

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

/* nek — wait for one key from allowed set s (case-insensitive).
 * f=1: echo the key and newline.  f=0: silent. */
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

/* onek — wait for one key from allowed set, always echo+newline. */
char onek(char *s)
{
    return(nek(s,1));
}
