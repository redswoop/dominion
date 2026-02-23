/*
 * bbs_output.c — BBS output pipeline (Layer 5)
 *
 * Everything that sends characters to the user.  Extracted from com.c.
 * Depends on BBS globals (vars.h), stream_processor, terminal_bridge.
 */

#include "io_ncurses.h"  /* MUST come before vars.h */
#include "platform.h"
#include "fcns.h"
#include "tcpio.h"
#include "conio.h"
#include "bbsutl.h"
#include "bbsutl2.h"
#include "disk.h"
#include "utility.h"
#include "session.h"
#include "system.h"
#include "terminal_bridge.h"
#include "stream_processor.h"
#include "ansi_attr.h"
#include "misccmd.h"

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
    io.curatr = (io.curatr & 0xf8) | i;
}

void setbgc(int i)
{
    io.curatr = (io.curatr & 0x8f) | (i << 4);
}

void setc(unsigned char ch)
{
    term_set_attr(ch);
}

void ansic(int n)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    char c,s[10];

    if(io.colblock)
        c=sys.nifty.defaultcol[n];
    else
        c = sess.user.colors[n];

    if (c == io.curatr) return;

    setc(c);

    if (okansi())
        makeansi(sess.user.colors[0],io.endofline, io.curatr);
    else
        io.endofline[0] = 0;
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
    auto& sys = System::instance();
    auto& sess = Session::instance();
    int i, i1;

    /* TCP output (skip tabs — they expand locally) */
    if (outcom && (c != 9)) {
        if (c == 12 && io.echo) {
            /* Form feed: ANSI clear + home for TCP.
             * Raw 0x0C is unreliable on modern terminals. */
            term_remote_write_raw("\x1b[2J\x1b[H");
        } else {
            outcomch(io.echo ? c : sys.nifty.echochar);
        }
    }

    if (c == 9) {
        /* Tab expansion */
        i1 = wherex();
        for (i = i1; i < (((i1 / 8) + 1) * 8); i++)
            stream_emit_char(32);
    }
    else if (io.echo || io.lecho) {
        out1ch(c);
        if (c == 12 && okansi()) outstrm("\x1b[0;1m");
        if (c == 10) {
            ++io.lines_listed;
            if (((sysstatus_pause_on_page & sess.user.sysstatus)) &&
                (io.lines_listed >= io.screenlinest - 1) && !io.listing) {
                pausescr();
                io.lines_listed = 0;
            }
            else if (((sysstatus_pause_on_message & sess.user.sysstatus)) &&
                (io.lines_listed >= io.screenlinest - 1) && readms) {
                pausescr();
                io.lines_listed = 0;
            }
        }
    }
    else
        out1ch(sys.nifty.echochar);
}


void outchr(unsigned char c)
{
    auto& sys = System::instance();
    if (io.global_handle && io.echo)
        global_char(c);

    if (io.chatcall && !io.x_only && !(sys.cfg.sysconfig & sysconfig_no_beep))
        setbeep(1);

    stream_putch(c);

    if (io.chatcall)
        setbeep(0);
}


void outstr(char *s)
{
    int i=0;
    int slen,x;


    checkhangup();

    if(io.hangup) return;

    if(s[0]=='\x96') {
        i++;
        slen=strlenc(s);
        x=79;
        x-=slen;
        x/=2;
        io.curatr = 0xFF;  /* invalidate so setc always emits ANSI */
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

    while (s[i]&&!io.hangup)
        outchr(s[i++]);

}

void outstrm(char *s)
{
    int i=0;

    if(!outcom||!incom||!strcmp(io.curspeed,"KB")) return;
    checkhangup();
    while (s[i]&&!io.hangup)
        outcomch(s[i++]);
}

void nl()
{
    if (io.endofline[0]) {
        outstr(io.endofline);
        io.endofline[0] = 0;
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

    *cc = io.curatr;
    strcpy(xl, io.endofline);
    i = ((wherey() + io.topline) * 80) * 2;
    for (i1 = 0; i1 < wherex(); i1++) {
        cl[i1]  = io.scrn[i + (i1 * 2)];
        atr[i1] = io.scrn[i + (i1 * 2) + 1];
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
    strcpy(io.endofline, xl);
}
