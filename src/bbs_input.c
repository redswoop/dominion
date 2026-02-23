/*
 * bbs_input.c — BBS input multiplexer (Layer 5)
 *
 * How keys arrive from any source: local keyboard, remote TCP,
 * macro buffer, quote buffer.  Extracted from com.c.
 */

#include "io_ncurses.h"  /* MUST come before vars.h */
#include "vars.h"
#include "terminal_bridge.h"

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

/* empty — returns 1 if no input is available from ANY source.
 * Checks: local keyboard (kbhitb), remote TCP (comhit), macro buffer
 * (charbufferpointer), external program pipe (in_extern==2), and
 * quote buffer (bquote).  Used by getkey() to spin-wait for input. */
int empty()
{
    if(x_only) return 0;

    if (kbhitb() || (incom && comhit()) ||
        (charbufferpointer && charbuffer[charbufferpointer]) ||
        (in_extern == 2)||bquote)
        return(0);
    return(1);
}


/***********************************************************************
 * 3. KEY POST-PROCESSING
 ***********************************************************************/

/* skey1 — post-process a key after reading from any source.
 * Called by inkey() on every character.  Handles:
 *   - 127→8 mapping (DEL→BS, defense in depth)
 *   - Ctrl-A/D/F/Y → expand user macros from thisuser.macros[]
 *   - Ctrl-T → display time remaining (ptime)
 *   - Ctrl-R → reprint last line (reprint)
 * Writes result back through *ch pointer. */
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
                userdb_load(1,&u);
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
    case '\x15':
        if (okskey) break;
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
 *   1. Quote buffer (bquote)  — auto-quoting in message reply
 *   2. Macro buffer (charbufferpointer) — Ctrl-A/D/F/Y macro expansion
 *   3. Local keyboard (kbhitb → getchd1) — sysop console via ncurses
 *   4. Remote TCP (comhit → get1c) — telnet user
 *
 * Returns 0 if nothing available.  Calls skey1() to post-process
 * (macro expansion, 127→8 mapping).  Sets lastcon=1 for local, 0 for remote.
 *
 * For the scan-code two-byte sequence: first call returns 0 (NUL prefix),
 * getkey() loops on !ch, second call returns the scan code via getchd1()
 * which reads _pending_scancode, then skey() dispatches the function key. */
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
        ch = get1c();
        lastcon = 0;
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
    if (ch == 127) ch = 8;  /* macOS sends DEL for backspace */
    return(ch);
}
