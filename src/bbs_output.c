/*
 * bbs_output.c — BBS output pipeline (Layer 5)
 *
 * Everything that sends characters to the user.  Extracted from com.c.
 * Depends on BBS globals (vars.h), stream_processor, terminal_bridge.
 */

#include "io_ncurses.h"  /* MUST come before vars.h */
#include "vars.h"
#include "terminal_bridge.h"
#include "stream_processor.h"
#include "ansi_attr.h"

#pragma hdrstop

#include <stdarg.h>

#define sysstatus_pause_on_message 0x0400

/* com.c file-scoped state → io_session_t (Phase 2) */
#define bluein      io.bluein

#include "mci.h"

extern int readms;


/***********************************************************************
 * 1. STRING LENGTH
 ***********************************************************************/

int strlenc(char *s)
{
    return mci_strlen(s);
}


/***********************************************************************
 * 2. ANSI / COLOR
 ***********************************************************************/

void setfgc(int i)
{
    curatr = (curatr & 0xf8) | i;
}

void setbgc(int i)
{
    curatr = (curatr & 0x8f) | (i << 4);
}

void setc(unsigned char ch)
{
    term_set_attr(ch);
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

    if (okansi())
        makeansi(thisuser.colors[0],endofline, curatr);
    else
        endofline[0] = 0;
}


/***********************************************************************
 * 3. CORE OUTPUT
 ***********************************************************************/

void outstrm(char *s);

/* stream_emit_char — called by stream_processor when a normal character
 * emerges after all markup processing.  Handles TCP output, local
 * rendering, tab expansion, line counting, and pause. */
void stream_emit_char(unsigned char c)
{
    int i, i1;

    /* TCP output (skip tabs — they expand locally) */
    if (outcom && (c != 9)) {
        if (c == 12 && echo) {
            /* Form feed: ANSI clear + home for TCP.
             * Raw 0x0C is unreliable on modern terminals. */
            term_remote_write_raw("\x1b[2J\x1b[H");
        } else {
            outcomch(echo ? c : nifty.echochar);
        }
    }

    if (c == 9) {
        /* Tab expansion */
        i1 = wherex();
        for (i = i1; i < (((i1 / 8) + 1) * 8); i++)
            stream_emit_char(32);
    }
    else if (echo || lecho) {
        out1ch(c);
        if (c == 12 && okansi()) outstrm("\x1b[0;1m");
        if (c == 10) {
            ++lines_listed;
            if (((sysstatus_pause_on_page & thisuser.sysstatus)) &&
                (lines_listed >= screenlinest - 1) && !listing) {
                pausescr();
                lines_listed = 0;
            }
            else if (((sysstatus_pause_on_message & thisuser.sysstatus)) &&
                (lines_listed >= screenlinest - 1) && readms) {
                pausescr();
                lines_listed = 0;
            }
        }
    }
    else
        out1ch(nifty.echochar);
}


void outchr(unsigned char c)
{
    if (global_handle && echo)
        global_char(c);

    if (chatcall && !x_only && !(syscfg.sysconfig & sysconfig_no_beep))
        setbeep(1);

    stream_putch(c);

    if (chatcall)
        setbeep(0);
}


void outstr(char *s)
{
    int i=0;
    int slen,x;


    checkhangup();

    if(hangup) return;

    if(s[0]=='\x96') {
        i++;
        slen=strlenc(s);
        x=79;
        x-=slen;
        x/=2;
        curatr = 0xFF;  /* invalidate so setc always emits ANSI */
        setc(0x07);     /* explicit white on black for centering spaces */
        for(slen=0;slen<x-2;slen++)
            outchr(' ');
    }

    if(s[0]=='\xF1') {
            dtitle(&s[1]);
        return;
    }

    if(s[0]=='\x98') {
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

void pl(char *s)
{
    outstr(s);
    nl();
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

void prt(int i, char *s)
{
    ansic(i);
    outstr(s);
    ansic(0);
}


/***********************************************************************
 * 4. OUTPUT HELPERS
 ***********************************************************************/

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
