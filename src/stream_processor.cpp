/*
 * stream_processor.c — Markup interpretation for BBS output byte stream
 *
 * Owns parser state for pipe color, easy color, avatar, MCI.
 * ANSI accumulator and saved cursor remain in io_session_t because
 * bbsutl2.c (reprint) and conio.c (set_protect) access them.
 *
 * NOT a leaf component — depends on BBS globals for MCI expansion,
 * io.echo/outcom flags, user color prefs, line counting, pausescr.
 * The point is to separate markup interpretation from Terminal rendering.
 *
 * io.change_color / io.change_ecolor remain in io_session_t because outs()
 * in conio.c uses them directly.
 */

#include "io_ncurses.h"  /* MUST come before vars.h */
#include "platform.h"
#include "fcns.h"
#include "session.h"
#include "system.h"
#include "terminal_bridge.h"
#include "stream_processor.h"

extern char MCISTR[161];


/***********************************************************************
 * Parser state — file-scoped statics
 *
 * Only state that is exclusively used by the output markup pipeline.
 * io.ansistr/io.ansiptr/io.oldx/io.oldy stay in io_session_t (shared with
 * bbsutl2.c reprint() and conio.c set_protect()).
 ***********************************************************************/

/* Pipe color: |nn */


static int sp_pipe = 0;
static char sp_pipestr[5];

/* Easy color: char 6 + index */
static int sp_easycolor = 0;

/* Avatar protocol */
static unsigned char sp_ac = 0;
static unsigned char sp_ac2 = 0;

/* MCI expansion */
static int sp_mci = 0;


/***********************************************************************
 * ANSI parser (extracted from execute_ansi in com.c)
 *
 * Uses io.ansistr/io.ansiptr from io_session_t (via macros in io_stream.h).
 * Uses io.oldx/io.oldy from io_session_t for cursor save/restore (ESC[s/u),
 * shared with set_protect() in conio.c.
 ***********************************************************************/

static void sp_execute_ansi(void)
{
    int args[11], argptr, count, ptr, tempptr;
    char cmd, temp[11];
    static const char *clrlst = "04261537";

    if (io.ansistr[1] != '[') {
        /* Not a CSI sequence — ignore */
    }
    else {
        argptr = tempptr = 0;
        ptr = 2;
        for (count = 0; count < 10; count++)
            args[count] = temp[count] = 0;
        cmd = io.ansistr[io.ansiptr - 1];
        io.ansistr[io.ansiptr - 1] = 0;
        while ((io.ansistr[ptr]) && (argptr<10) && (tempptr<10)) {
            if (io.ansistr[ptr] == ';') {
                temp[tempptr] = 0;
                tempptr = 0;
                args[argptr++] = atoi(temp);
            }
            else
                temp[tempptr++] = io.ansistr[ptr];
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
            io.oldx = wherex();
            io.oldy = wherey();
            break;
        case 'u':
            movecsr(io.oldx, io.oldy);
            break;
        case 'J':
            if (args[0] == 2) {
                clrscrb();
                /* ESC[2J doesn't move the cursor on most modern terminals.
                 * clrscrb() homes locally; sync TCP cursor to match. */
                if (outcom)
                    term_remote_write_raw("\x1b[H");
            }
            break;
        case 'k':
        case 'K':
            term_clear_to_eol();
            break;
        case 'm':
            {
                int had_reset = 0;
                if (!argptr) {
                    argptr = 1;
                    args[0] = 0;
                }
                for (count = 0; count < argptr; count++)
                    switch (args[count]) {
                    case 0:
                        io.curatr = 0x07;
                        had_reset = 1;
                        break;
                    case 1:
                        io.curatr = io.curatr | 0x08;
                        break;
                    case 4:
                        break;
                    case 5:
                        io.curatr = io.curatr | 0x80;
                        break;
                    case 7:
                        ptr = io.curatr & 0x77;
                        io.curatr = (io.curatr & 0x88) | (ptr << 4) | (ptr >> 4);
                        break;
                    case 8:
                        io.curatr = 0;
                        break;
                    default:
                        if ((args[count] >= 30) && (args[count] <= 37))
                            setfgc(clrlst[args[count] - 30] - '0');
                        else if ((args[count] >= 40) && (args[count] <= 47))
                            setbgc(clrlst[args[count] - 40] - '0');
                    }
                /* Force black background on TCP after SGR reset.
                 * ESC[0m on modern terminals resets to the terminal's
                 * default background, which may not be black. */
                if (had_reset && outcom && (io.curatr & 0x70) == 0)
                    term_remote_write_raw("\x1b[40m");

                /* Inject true color foreground to bypass terminal palette.
                 * Ensures CGA-accurate colors regardless of terminal profile. */
                if (outcom) {
                    static const int cga_rgb[16][3] = {
                        {  0,   0,   0}, {  0,   0, 170},
                        {  0, 170,   0}, {  0, 170, 170},
                        {170,   0,   0}, {170,   0, 170},
                        {170,  85,   0}, {170, 170, 170},
                        { 85,  85,  85}, { 85,  85, 255},
                        { 85, 255,  85}, { 85, 255, 255},
                        {255,  85,  85}, {255,  85, 255},
                        {255, 255,  85}, {255, 255, 255},
                    };
                    int fg = io.curatr & 0x0F;
                    char tc[24];
                    sprintf(tc, "\x1b[38;2;%d;%d;%dm",
                            cga_rgb[fg][0], cga_rgb[fg][1], cga_rgb[fg][2]);
                    term_remote_write_raw(tc);
                }
            }
            break;
        }
    }
    io.ansiptr = 0;
}


/***********************************************************************
 * stream_putch — process one byte of BBS output
 ***********************************************************************/

void stream_putch(unsigned char c)
{
    auto& sys = System::instance();
    int i;

    /* --- Pipe color: | followed by digits --- */
    if (sp_pipe) {
        if (c >= '0' && c <= '9') {
            sp_pipestr[sp_pipe - 1] = c;
            sp_pipe++;
            return;
        }
        else {
            sp_pipestr[sp_pipe - 1] = 0;
            sp_pipe = 0;
            if (io.caps.color != CAP_OFF)
                term_set_attr((unsigned char)atoi(sp_pipestr));
        }
        /* fall through: current char c still needs processing */
    }

    /* --- Easy color: char 6 + index --- */
    if (sp_easycolor) {
        sp_easycolor = 0;
        if (c > 0 && c <= 20)
            ansic(c - 1);
        return;
    }

    /* --- Avatar: char 5 = set color (next byte is attr) --- */
    if (c == 5 && sp_ac == 0) {
        sp_ac = 10;
        return;
    }

    if (sp_ac == 10) {
        sp_ac = 0;
        if (io.caps.color != CAP_OFF)
            term_set_attr(c);
        return;
    }

    /* --- Avatar: char 151 = repeat (next two bytes: char, count) --- */
    if (sp_ac == 101) {
        sp_ac = 0;
        for (i = 0; i < c; i++)
            outchr(sp_ac2);
        return;
    }

    if (sp_ac == 100) {
        sp_ac2 = c;
        sp_ac = 101;
        return;
    }

    /* --- Avatar: char 22 = escape sequence (2 data bytes follow) --- */
    if (c == 22 && sp_ac == 0) {
        if (outcom)
            outcomch(c);
        sp_ac = 1;
        return;
    }

    if (sp_ac == 1) {
        sp_ac = 2;
        if (outcom)
            outcomch(c);
        return;
    }

    if (sp_ac == 2) {
        io.curatr = c;
        if (outcom)
            outcomch(c);
        sp_ac = 0;
        return;
    }

    /* --- MCI expansion: backtick + letter --- */
    if (sp_mci) {
        sp_mci = 0;
        if (io.mciok) {
            setmci(c);
            outstr(MCISTR);
            return;
        }
    }

    /* --- io.change_color / io.change_ecolor (state in io_session_t) --- */
    if (io.change_color) {
        io.change_color = 0;
        if ((c >= '0') && (c <= '9'))
            ansic(c - '0');
        return;
    }

    if (io.change_ecolor) {
        io.change_ecolor = 0;
        if ((c >= '0') && (c <= '9'))
            ansic(c - '0' + 10);
        return;
    }

    /* --- Trigger characters --- */
    if (c == 3) {
        io.change_color = 1;
        return;
    }

    if (c == 14) {
        io.change_ecolor = 1;
        return;
    }

    if (c == 6) {
        sp_easycolor = 1;
        return;
    }

    if (c == '`') {
        if (io.mciok && !sp_mci) {
            sp_mci = 1;
            return;
        }
    }

    if (c == '|') {
        if (io.mciok) {
            sp_pipe = 1;
            return;
        }
    }

    if (c == 151) {
        if (io.mciok) {
            sp_ac = 100;
            return;
        }
    }

    /* --- End-of-line color reset --- */
    if ((c == 10) && io.endofline[0]) {
        outstr(io.endofline);
        io.endofline[0] = 0;
    }

    /* --- ANSI accumulation (io.ansistr/io.ansiptr in io_session_t) --- */
    if (io.ansiptr) {
        if (outcom && c != 9)
            outcomch(io.echo ? c : sys.nifty.echochar);
        io.ansistr[io.ansiptr++] = c;
        io.ansistr[io.ansiptr] = 0;
        if (((c < '0' || c > '9') && c != '[' && c != ';') ||
            io.ansistr[1] != '[' || io.ansiptr > 75)
            sp_execute_ansi();
        return;
    }

    if (c == 27) {
        if (outcom)
            outcomch(io.echo ? c : sys.nifty.echochar);
        io.ansistr[0] = 27;
        io.ansiptr = 1;
        io.ansistr[io.ansiptr] = 0;
        return;
    }

    /* --- Normal character — hand off to bbs_output.c --- */
    stream_emit_char(c);
}


/***********************************************************************
 * stream_reset — zero all parser state
 ***********************************************************************/

void stream_reset(void)
{
    sp_pipe = 0;
    memset(sp_pipestr, 0, sizeof(sp_pipestr));
    sp_easycolor = 0;
    sp_ac = 0;
    sp_ac2 = 0;
    sp_mci = 0;

    /* These live in io_session_t (shared with other modules) */
    io.ansiptr = 0;
    io.change_color = 0;
    io.change_ecolor = 0;
}
