/*
 * io_ncurses.c — ncurses display layer for local sysop console
 *
 * Manages ncurses initialization, color pair mapping, and CP437 rendering.
 * Remote (TCP) output is unaffected — ncurses only drives the sysop terminal.
 *
 * When stdout is not a tty (e.g. BBS launched by test harness with pipes),
 * ncurses is skipped entirely. nc_active == 0 in that case, and all display
 * functions become no-ops. A minimal raw termios setup is done instead so
 * the BBS can still function headlessly.
 */

#include "io_ncurses.h"
#include "cp437.h"
#include "vars.h"
#include <unistd.h>
#include <locale.h>
#include <signal.h>

int nc_active = 0;

/* DOS color index → ncurses color constant
 * DOS:     0=black 1=blue 2=green 3=cyan 4=red 5=magenta 6=brown 7=white
 * ncurses: 0=black 1=red  2=green 3=yellow 4=blue 5=magenta 6=cyan 7=white */
static const int _dos2nc[8] = {
    COLOR_BLACK,    /* DOS 0 = black */
    COLOR_BLUE,     /* DOS 1 = blue */
    COLOR_GREEN,    /* DOS 2 = green */
    COLOR_CYAN,     /* DOS 3 = cyan */
    COLOR_RED,      /* DOS 4 = red */
    COLOR_MAGENTA,  /* DOS 5 = magenta */
    COLOR_YELLOW,   /* DOS 6 = brown/yellow */
    COLOR_WHITE     /* DOS 7 = white */
};


/* Crash handler — restore terminal before dying */
static void _crash_handler(int sig)
{
    if (nc_active)
        endwin();
    /* Re-raise with default handler so we get the normal exit (core dump, etc.) */
    signal(sig, SIG_DFL);
    raise(sig);
}

void ncurses_init(void)
{
    int fg, bg;

    if (!isatty(STDOUT_FILENO)) {
        /* No terminal — headless mode (test harness, daemon, etc.).
         * Set up minimal raw termios so the BBS still works. */
        nc_active = 0;
        if (isatty(STDIN_FILENO)) {
            struct termios raw;
            tcgetattr(STDIN_FILENO, &orig_termios);
            raw = orig_termios;
            raw.c_lflag &= ~(ICANON | ECHO | ISIG);
            raw.c_iflag &= ~(IXON | ICRNL);
            raw.c_cc[VMIN] = 0;
            raw.c_cc[VTIME] = 0;
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
        }
        return;
    }

    setlocale(LC_ALL, "");  /* Enable UTF-8 support in ncurses */
    initscr();
    nc_active = 1;

    if (has_colors()) {
        start_color();
        /* 64 color pairs: pair_number = bg*8 + fg + 1
         * Pair 0 is the default (reserved by ncurses). */
        for (bg = 0; bg < 8; bg++)
            for (fg = 0; fg < 8; fg++)
                init_pair(bg * 8 + fg + 1, _dos2nc[fg], _dos2nc[bg]);
    }

    raw();                      /* Raw mode — pass all keys, no signals */
    noecho();                   /* BBS handles echo itself */
    keypad(stdscr, TRUE);       /* Enable function key detection */
    nodelay(stdscr, TRUE);      /* Non-blocking by default for kbhit */
    scrollok(stdscr, FALSE);    /* We handle scrolling manually */
    idlok(stdscr, TRUE);        /* Allow hardware insert/delete line */

    /* BBS assumes 80x25 (screenbottom=24). Constrain ncurses to match,
     * regardless of actual terminal size. Without this, content below
     * row 24 is unmanaged and becomes garbled during scrolling. */
    erase();
    refresh();
    resizeterm(25, 80);

    /* Restore terminal on crash so the user doesn't get a broken shell */
    signal(SIGSEGV, _crash_handler);
    signal(SIGABRT, _crash_handler);
    signal(SIGBUS,  _crash_handler);
    signal(SIGFPE,  _crash_handler);
    signal(SIGTERM, _crash_handler);
    signal(SIGINT,  _crash_handler);
}


void ncurses_shutdown(void)
{
    if (nc_active)
        endwin();
    else if (isatty(STDIN_FILENO))
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}


int nc_attr(int dos_attr)
{
    int fg = dos_attr & 7;
    int bg = (dos_attr >> 4) & 7;
    int pair = bg * 8 + fg + 1;
    int bold = (dos_attr & 0x08) ? A_BOLD : 0;
    int blink = (dos_attr & 0x80) ? A_BLINK : 0;
    return COLOR_PAIR(pair) | bold | blink;
}


void nc_put_cp437(unsigned char ch)
{
    if (!nc_active) return;
    addstr(cp437_to_utf8[ch]);
}


void nc_render_scrn(int start_row, int num_rows)
{
    int row, col;
    extern char far *scrn;

    if (!scrn || !nc_active) return;

    for (row = start_row; row < start_row + num_rows && row < 25; row++) {
        move(row, 0);
        for (col = 0; col < 80; col++) {
            int off = (row * 80 + col) * 2;
            unsigned char ch = (unsigned char)scrn[off];
            unsigned char at = (unsigned char)scrn[off + 1];
            attrset(nc_attr(at));
            addstr(cp437_to_utf8[ch ? ch : ' ']);
        }
    }
    refresh();
}
