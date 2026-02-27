/*
 * terminal.cpp — Clean terminal I/O implementation
 *
 * Extracted and reorganized from com.c, tcpio.c, conio.c, io_ncurses.c.
 * Zero BBS dependencies. This file includes only system headers and cp437.h.
 */

#include "terminal.h"
#include "cp437.h"

#include <ncurses.h>
#undef getch    /* ncurses macro conflicts */
#undef echo     /* ncurses macro conflicts */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <cctype>
#include <csignal>
#include <vector>
#include <unistd.h>
#include <fcntl.h>
#include <locale.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <errno.h>

/* ================================================================== */
/*  CP437 color index → ncurses color constant                        */
/*  DOS:     0=black 1=blue 2=green 3=cyan 4=red 5=magenta 6=brown 7=white */
/*  ncurses: 0=black 1=red  2=green 3=yellow 4=blue 5=magenta 6=cyan 7=white */
/* ================================================================== */

static const int dos2nc[8] = {
    COLOR_BLACK, COLOR_BLUE, COLOR_GREEN, COLOR_CYAN,
    COLOR_RED, COLOR_MAGENTA, COLOR_YELLOW, COLOR_WHITE
};

/* CGA RGB values for all 16 DOS colors.
 * DOS order: 0=black 1=blue 2=green 3=cyan 4=red 5=magenta 6=brown 7=white
 *            8-15 = bright versions of 0-7 */
static const int cgaRGB[16][3] = {
    {  0,   0,   0},  /* 0:  black */
    {  0,   0, 170},  /* 1:  blue */
    {  0, 170,   0},  /* 2:  green */
    {  0, 170, 170},  /* 3:  cyan */
    {170,   0,   0},  /* 4:  red */
    {170,   0, 170},  /* 5:  magenta */
    {170,  85,   0},  /* 6:  brown */
    {170, 170, 170},  /* 7:  light grey */
    { 85,  85,  85},  /* 8:  dark grey */
    { 85,  85, 255},  /* 9:  light blue */
    { 85, 255,  85},  /* 10: light green */
    { 85, 255, 255},  /* 11: light cyan */
    {255,  85,  85},  /* 12: light red */
    {255,  85, 255},  /* 13: light magenta */
    {255, 255,  85},  /* 14: yellow */
    {255, 255, 255},  /* 15: white */
};


/* ================================================================== */
/*  Crash handler — restore terminal before dying                     */
/* ================================================================== */

static Terminal *g_crash_term = nullptr;

static void crash_handler(int sig)
{
    if (g_crash_term)
        g_crash_term->shutdown();
    signal(sig, SIG_DFL);
    raise(sig);
}


/* ================================================================== */
/*  Constructor / Destructor                                          */
/* ================================================================== */

Terminal::Terminal()
{
    scrn_ = (char *)std::calloc(4000, 1);  /* 80×25×2 */
}

Terminal::~Terminal()
{
    shutdown();
    if (ownsScrn_)
        std::free(scrn_);
    scrn_ = nullptr;
}


/* ================================================================== */
/*  Lifecycle                                                          */
/* ================================================================== */

bool Terminal::initLocal()
{
    /* Always save original terminal state for belt-and-suspenders restore */
    if (isatty(STDIN_FILENO))
        tcgetattr(STDIN_FILENO, &origTermios_);

    if (!isatty(STDOUT_FILENO)) {
        ncActive_ = false;
        if (isatty(STDIN_FILENO)) {
            struct termios raw = origTermios_;
            raw.c_lflag &= ~(ICANON | ECHO | ISIG);
            raw.c_iflag &= ~(IXON | ICRNL);
            raw.c_cc[VMIN] = 0;
            raw.c_cc[VTIME] = 0;
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
        }
        return false;
    }

    setlocale(LC_ALL, "");
    initscr();
    ncActive_ = true;

    if (has_colors()) {
        start_color();
        /* Normal 8-color pairs: 1..64 (terminal palette) */
        for (int bg = 0; bg < 8; bg++)
            for (int fg = 0; fg < 8; fg++)
                init_pair(bg * 8 + fg + 1, dos2nc[fg], dos2nc[bg]);
        /* Bright foreground pairs: 65..128 (explicit bright, no A_BOLD) */
        if (COLORS >= 16 && COLOR_PAIRS > 128) {
            use16colors_ = true;
            /* Define custom bright colors with exact CGA RGB.
             * Use color indices 16-23 so we don't disturb the
             * terminal's built-in palette (0-15). */
            int brightOff = 8;
            if (can_change_color() && COLORS >= 24) {
                for (int i = 0; i < 8; i++)
                    init_color(dos2nc[i] + 16,
                               cgaRGB[i+8][0] * 1000 / 255,
                               cgaRGB[i+8][1] * 1000 / 255,
                               cgaRGB[i+8][2] * 1000 / 255);
                brightOff = 16;
            }
            for (int bg = 0; bg < 8; bg++)
                for (int fg = 0; fg < 8; fg++)
                    init_pair(64 + bg * 8 + fg + 1,
                              dos2nc[fg] + brightOff, dos2nc[bg]);
        }
    }

    raw();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    scrollok(stdscr, FALSE);
    idlok(stdscr, TRUE);
    erase();
    refresh();

    g_crash_term = this;
    signal(SIGSEGV, crash_handler);
    signal(SIGABRT, crash_handler);
    signal(SIGBUS,  crash_handler);
    signal(SIGFPE,  crash_handler);
    signal(SIGTERM, crash_handler);
    signal(SIGINT,  crash_handler);

    return true;
}

int Terminal::ncLines() const { return ncActive_ ? LINES : 0; }
int Terminal::ncCols()  const { return ncActive_ ? COLS  : 0; }

void Terminal::resize(int rows, int cols)
{
    if (!ncActive_) return;
    resizeterm(rows, cols);
}

void Terminal::shutdown()
{
    if (ncActive_) {
        endwin();
        ncActive_ = false;
    }
    /* Always restore terminal state (belt-and-suspenders after endwin) */
    if (isatty(STDIN_FILENO))
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &origTermios_);
    if (g_crash_term == this)
        g_crash_term = nullptr;
}


/* ================================================================== */
/*  Remote TCP stream                                                  */
/* ================================================================== */

void Terminal::setRemote(int fd)
{
    remote_.fd = fd;
    remote_.active = true;
    remote_.needs_iac = true;
    iacState_ = 0;
}

void Terminal::setRemoteNoIac(int fd)
{
    remote_.fd = fd;
    remote_.active = true;
    remote_.needs_iac = false;
    remote_.is_pty = true;
    iacState_ = 0;
}

void Terminal::closeRemote()
{
    if (remote_.fd >= 0) {
        close(remote_.fd);
        remote_.fd = -1;
    }
    remote_.active = false;
}

void Terminal::detachRemote()
{
    remote_.fd = -1;
    remote_.active = false;
}

bool Terminal::remoteConnected() const
{
    if (remote_.fd < 0) return false;

    /* PTY fds don't support recv() — check if fd is still valid */
    if (remote_.is_pty)
        return fcntl(remote_.fd, F_GETFL) >= 0;

    char buf;
    int n = recv(remote_.fd, &buf, 1, MSG_PEEK | MSG_DONTWAIT);
    if (n == 0) return false;
    if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) return false;
    return true;
}

/* Telnet protocol */
#define IAC   255
#define WILL  251
#define WONT  252
#define DO    253
#define DONT  254
#define IAC_SB  250
#define IAC_SE  240
#define TELOPT_ECHO   1
#define TELOPT_SGA    3
#define TELOPT_NAWS   31

void Terminal::sendTelnetNegotiation()
{
    unsigned char neg[] = {
        IAC, WILL, TELOPT_ECHO,
        IAC, WILL, TELOPT_SGA,
        IAC, DO,   TELOPT_NAWS,
    };
    write(remote_.fd, neg, sizeof(neg));
    iacState_ = 0;
}

void Terminal::sendTerminalInit()
{
    const char *seq = "\033[?1049h\033[0m\033[40m\033[2J\033[H";
    write(remote_.fd, seq, std::strlen(seq));
}

void Terminal::sendTerminalRestore()
{
    const char *seq = "\033[0m\033[?1049l";
    write(remote_.fd, seq, std::strlen(seq));
}

void Terminal::remotePutch(unsigned char c)
{
    if (remote_.fd < 0) return;
    if (c >= 0x80) {
        const char *utf8 = cp437_to_utf8[c];
        write(remote_.fd, utf8, std::strlen(utf8));
    } else {
        write(remote_.fd, &c, 1);
    }
}

void Terminal::remoteWriteRaw(const char *s)
{
    if (remote_.fd < 0) return;
    write(remote_.fd, s, std::strlen(s));
}

bool Terminal::remoteDataReady()
{
    if (remote_.fd < 0) return false;
    struct timeval tv = {0, 0};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(remote_.fd, &fds);
    return (select(remote_.fd + 1, &fds, NULL, NULL, &tv) > 0);
}

unsigned char Terminal::remoteGetKey()
{
    unsigned char ch;
    struct timeval tv = {0, 0};
    fd_set fds;

    if (remote_.fd < 0) return 0;

    FD_ZERO(&fds);
    FD_SET(remote_.fd, &fds);
    if (select(remote_.fd + 1, &fds, NULL, NULL, &tv) <= 0)
        return 0;

    while (1) {
        int n = read(remote_.fd, &ch, 1);
        if (n <= 0) {
            if (n == 0) detachRemote();  /* EOF — mark inactive, BBS owns fd close */
            return 0;
        }
        if (telnetFilter(&ch))
            return ch;
        FD_ZERO(&fds);
        FD_SET(remote_.fd, &fds);
        if (select(remote_.fd + 1, &fds, NULL, NULL, &tv) <= 0)
            return 0;
    }
}

bool Terminal::telnetFilter(unsigned char *ch)
{
    unsigned char c = *ch;
    switch (iacState_) {
    case 0:
        if (c == IAC) { iacState_ = 1; return false; }
        return true;
    case 1:
        if (c == IAC) { iacState_ = 0; *ch = 0xFF; return true; }
        if (c == WILL || c == WONT || c == DO || c == DONT) { iacState_ = 2; return false; }
        if (c == IAC_SB) { iacState_ = 3; return false; }
        iacState_ = 0; return false;
    case 2:
        iacState_ = 0; return false;
    case 3:
        if (c == IAC) iacState_ = 4;
        return false;
    case 4:
        iacState_ = (c == IAC_SE) ? 0 : 3;
        return false;
    }
    iacState_ = 0;
    return false;
}


/* ================================================================== */
/*  ncurses helpers                                                    */
/* ================================================================== */

int Terminal::ncAttr(int dosAttr)
{
    int fg = dosAttr & 7;
    int bg = (dosAttr >> 4) & 7;
    int blink = (dosAttr & 0x80) ? A_BLINK : 0;

    if ((dosAttr & 0x08) && use16colors_) {
        /* Use explicit bright-color pair instead of A_BOLD */
        int pair = 64 + bg * 8 + fg + 1;
        return COLOR_PAIR(pair) | blink;
    }

    int pair = bg * 8 + fg + 1;
    int bold = (dosAttr & 0x08) ? A_BOLD : 0;
    return COLOR_PAIR(pair) | bold | blink;
}

void Terminal::emitAttr(int attr)
{
    if (attr < 0) { lastAttr_ = -1; return; }  /* cache invalidation only */
    if (attr == lastAttr_) return;
    if (ncActive_) attrset(ncAttr(attr));
    lastAttr_ = attr;
}


/* ================================================================== */
/*  Screen buffer                                                      */
/* ================================================================== */

void Terminal::scrnPut(int x, int y, unsigned char ch, unsigned char attr)
{
    if (!scrn_) return;
    if (x < 0 || x >= 80 || y < 0 || y > *pScreenBottom_) return;
    int off = (y * 80 + x) * 2;
    if (off >= 0 && off < 4000) {
        scrn_[off] = ch;
        scrn_[off + 1] = attr;
    }
}

void Terminal::scrnScroll(int t, int b, int n)
{
    if (!scrn_) return;
    if (n <= 0) {
        for (int row = t; row <= b; row++)
            std::memset(&scrn_[row * 160], 0, 160);
        return;
    }
    for (int row = t; row <= b - n; row++)
        std::memmove(&scrn_[row * 160], &scrn_[(row + n) * 160], 160);
    for (int row = b - n + 1; row <= b; row++)
        std::memset(&scrn_[row * 160], 0, 160);
}

void Terminal::renderScrn(int startRow, int numRows)
{
    if (!scrn_ || !ncActive_) return;
    for (int row = startRow; row < startRow + numRows && row < 25; row++) {
        move(row, 0);
        for (int col = 0; col < 80; col++) {
            int off = (row * 80 + col) * 2;
            unsigned char ch = (unsigned char)scrn_[off];
            unsigned char at = (unsigned char)scrn_[off + 1];
            attrset(ncAttr(at));
            addstr(cp437_to_utf8[ch ? ch : ' ']);
        }
    }
    refresh();
}


void Terminal::renderBuffer(const char *buf, int startRow, int numRows)
{
    if (!buf || !ncActive_) return;
    for (int row = startRow; row < startRow + numRows; row++) {
        move(row, 0);
        for (int col = 0; col < 80; col++) {
            int off = (row * 80 + col) * 2;
            unsigned char ch = (unsigned char)buf[off];
            unsigned char at = (unsigned char)buf[off + 1];
            /* Render zero-attr cells as normal white-on-black (0x07),
             * not black-on-black which is invisible. */
            attrset(ncAttr(at ? at : 0x07));
            addstr(cp437_to_utf8[ch ? ch : ' ']);
        }
    }
    refresh();
}

void Terminal::drawStatusLine(int row, const char *text, int attr)
{
    if (!ncActive_) return;
    move(row, 0);
    attrset(ncAttr(attr));
    clrtoeol();
    if (text) addstr(text);
    refresh();
}

void Terminal::clearArea(int startRow, int numRows)
{
    if (!ncActive_) return;
    attrset(ncAttr(0x07));
    for (int r = startRow; r < startRow + numRows; r++) {
        move(r, 0);
        clrtoeol();
    }
    refresh();
}


/* ================================================================== */
/*  Screen primitives                                                  */
/* ================================================================== */

void Terminal::scrollUp(int t, int b, int l)
{
    if (l == 0) {
        if (ncActive_) {
            /* Sync ncurses attr to curatr before clearing.
             * setAttr()/emitAttr() may have set a non-black background;
             * clrtoeol fills with the current ncurses background. */
            attrset(ncAttr(*pCuratr_));
            for (int row = t; row <= b; row++) {
                move(row, 0);
                clrtoeol();
            }
        }
        scrnScroll(t, b, 0);
    } else {
        if (ncActive_) {
            attrset(ncAttr(*pCuratr_));
            setscrreg(t, b);
            scrollok(stdscr, TRUE);
            wscrl(stdscr, l);
            scrollok(stdscr, FALSE);
            setscrreg(0, *pScreenBottom_);
        }
        scrnScroll(t, b, l);
    }
    lastAttr_ = -1;
    if (ncActive_) refresh();
}

void Terminal::moveCursor(int x, int y)
{
    if (x < 0) x = 0;
    if (x > 79) x = 79;
    if (y < 0) y = 0;
    y += *pTopLine_;
    if (y > *pScreenBottom_) y = *pScreenBottom_;
    cx_ = x;
    cy_ = y;
    if (ncActive_) { move(y, x); refresh(); }
}

void Terminal::gotoXY(int x, int y)
{
    /* Send ANSI cursor position to remote */
    if (remote_.active) {
        int absY = y;
        if (absY < 0) absY = 0;
        absY += *pTopLine_;
        char buf[20];
        std::snprintf(buf, sizeof(buf), "\x1b[%d;%dH", absY + 1, x + 1);
        remoteWriteRaw(buf);
    }
    /* Position locally (moveCursor handles clamping + ncurses) */
    moveCursor(x, y);
}

void Terminal::cr()
{
    cx_ = 0;
    if (ncActive_) { move(cy_, 0); refresh(); }
}

void Terminal::lf()
{
    if (cy_ == *pScreenBottom_) {
        scrollUp(*pTopLine_, *pScreenBottom_, 1);
        if (ncActive_) { move(cy_, cx_); refresh(); }
    } else {
        cy_++;
        if (ncActive_) { move(cy_, cx_); refresh(); }
    }
}

void Terminal::bs()
{
    if (cx_ == 0) {
        if (cy_ != *pTopLine_) {
            cx_ = 79;
            cy_--;
            if (ncActive_) { move(cy_, cx_); refresh(); }
        }
    } else {
        cx_--;
        if (ncActive_) { move(cy_, cx_); refresh(); }
    }
}

void Terminal::clearScreen()
{
    lastAttr_ = -1;
    scrollUp(*pTopLine_, *pScreenBottom_, 0);
    moveCursor(0, 0);

    /* Clear remote terminal: reset attributes, force black bg, clear, home */
    if (remote_.active)
        remoteWriteRaw("\033[0m\033[40m\033[2J\033[H");
}

void Terminal::clearToEol()
{
    int ox = cursorX();
    int oy = cursorY();

    if (ncActive_) { attrset(ncAttr(*pCuratr_)); clrtoeol(); refresh(); }

    if (scrn_) {
        int row = oy + *pTopLine_;
        for (int col = ox; col < 80; col++) {
            int off = (row * 80 + col) * 2;
            if (off >= 0 && off < 4000) {
                scrn_[off] = ' ';
                scrn_[off + 1] = *pCuratr_;
            }
        }
    }

    moveCursor(ox, oy);
}

void Terminal::out1chx(unsigned char ch)
{
    emitAttr(*pCuratr_);
    scrnPut(cx_, cy_, ch, *pCuratr_);
    if (ncActive_) {
        move(cy_, cx_);
        addstr(cp437_to_utf8[ch]);
    }
    cx_++;
    if (cx_ >= 80) {
        cx_ = 0;
        if (cy_ == *pScreenBottom_)
            scrollUp(*pTopLine_, *pScreenBottom_, 1);
        else
            cy_++;
        if (ncActive_) move(cy_, cx_);
    }
    if (ncActive_) refresh();
}

void Terminal::out1ch(unsigned char ch)
{
    if (ch > 31)
        out1chx(ch);
    else if (ch == 13)
        cr();
    else if (ch == 10)
        lf();
    else if (ch == 12)
        clearScreen();
    else if (ch == 8)
        bs();
    else if (ch == 7) {
        if (ncActive_) { beep(); }
    }
}


/* ================================================================== */
/*  ANSI generation                                                    */
/* ================================================================== */

void Terminal::addAnsiParam(char *s, int val)
{
    char tmp[20];
    if (s[0])
        std::strcat(s, ";");
    else
        std::strcpy(s, "\x1B[");
    std::snprintf(tmp, sizeof(tmp), "%d", val);
    std::strcat(s, tmp);
}

int Terminal::makeAnsi(unsigned char attr, char *s)
{
    static const char *temp = "04261537";
    unsigned char catr = *pCuratr_;
    s[0] = 0;

    if (attr != catr) {
        if ((catr & 0x88) ^ (attr & 0x88)) {
            addAnsiParam(s, 0);
            addAnsiParam(s, 30 + temp[attr & 0x07] - '0');
            addAnsiParam(s, 40 + temp[(attr & 0x70) >> 4] - '0');
            catr = attr & 0x77;
        }
        if ((catr & 0x07) != (attr & 0x07))
            addAnsiParam(s, 30 + temp[attr & 0x07] - '0');
        /* Always include explicit background when emitting color changes */
        if (s[0] || (catr & 0x70) != (attr & 0x70))
            addAnsiParam(s, 40 + temp[(attr & 0x70) >> 4] - '0');
        if ((catr & 0x08) ^ (attr & 0x08))
            addAnsiParam(s, 1);
        if ((catr & 0x80) ^ (attr & 0x80))
            addAnsiParam(s, 5);
    }
    if (s[0])
        std::strcat(s, "m");
    return (int)std::strlen(s);
}


/* ================================================================== */
/*  ANSI parsing (for incoming ANSI art / data)                       */
/* ================================================================== */

void Terminal::executeAnsi()
{
    int args[11], argptr, count, ptr, tempptr, ox, oy;
    char cmd, tmp[11];
    static const char *clrlst = "04261537";

    if (ansiStr_[1] != '[') {
        /* Not a CSI sequence — ignore */
    } else {
        argptr = tempptr = 0;
        ptr = 2;
        for (count = 0; count < 10; count++)
            args[count] = tmp[count] = 0;
        cmd = ansiStr_[ansiPtr_ - 1];
        ansiStr_[ansiPtr_ - 1] = 0;

        while (ansiStr_[ptr] && argptr < 10 && tempptr < 10) {
            if (ansiStr_[ptr] == ';') {
                tmp[tempptr] = 0;
                tempptr = 0;
                args[argptr++] = std::atoi(tmp);
            } else {
                tmp[tempptr++] = ansiStr_[ptr];
            }
            ++ptr;
        }
        if (tempptr && argptr < 10) {
            tmp[tempptr] = 0;
            args[argptr++] = std::atoi(tmp);
        }
        if (cmd >= 'A' && cmd <= 'D' && !args[0])
            args[0] = 1;

        switch (cmd) {
        case 'f':
        case 'H':
            moveCursor(args[1] - 1, args[0] - 1);
            break;
        case 'A':
            moveCursor(cursorX(), cursorY() - args[0]);
            break;
        case 'B':
            moveCursor(cursorX(), cursorY() + args[0]);
            break;
        case 'C':
            moveCursor(cursorX() + args[0], cursorY());
            break;
        case 'D':
            moveCursor(cursorX() - args[0], cursorY());
            break;
        case 's':
            savedX_ = cursorX();
            savedY_ = cursorY();
            break;
        case 'u':
            moveCursor(savedX_, savedY_);
            break;
        case 'J':
            if (args[0] == 2)
                clearScreen();
            break;
        case 'k':
        case 'K':
            ox = cursorX();
            oy = cursorY();
            if (ncActive_) { clrtoeol(); refresh(); }
            if (scrn_) {
                int row = oy + *pTopLine_;
                for (int col = ox; col < 80; col++) {
                    int off = (row * 80 + col) * 2;
                    if (off >= 0 && off < 4000) {
                        scrn_[off] = ' ';
                        scrn_[off + 1] = *pCuratr_;
                    }
                }
            }
            moveCursor(ox, oy);
            break;
        case 'm':
            if (!argptr) { argptr = 1; args[0] = 0; }
            for (count = 0; count < argptr; count++) {
                switch (args[count]) {
                case 0: *pCuratr_ = 0x07; break;
                case 1: *pCuratr_ |= 0x08; break;
                case 4: break;
                case 5: *pCuratr_ |= 0x80; break;
                case 7: {
                    int p = *pCuratr_ & 0x77;
                    *pCuratr_ = (*pCuratr_ & 0x88) | (p << 4) | (p >> 4);
                    break;
                }
                case 8: *pCuratr_ = 0; break;
                default:
                    if (args[count] >= 30 && args[count] <= 37)
                        *pCuratr_ = (*pCuratr_ & 0xf8) | (clrlst[args[count] - 30] - '0');
                    else if (args[count] >= 40 && args[count] <= 47)
                        *pCuratr_ = (*pCuratr_ & 0x8f) | ((clrlst[args[count] - 40] - '0') << 4);
                }
            }
            /* Inject true-color override after SGR updates curatr */
            if (remote_.active)
                injectTrueColor(*pCuratr_);
            break;
        }
    }
    ansiPtr_ = 0;
}


/* ================================================================== */
/*  Color                                                              */
/* ================================================================== */

void Terminal::setAttr(unsigned char attr)
{
    if (attr == *pCuratr_) return;

    char buf[30];
    makeAnsi(attr, buf);

    if (buf[0]) {
        /* Send ANSI sequence to remote */
        if (remote_.active) {
            remoteWriteRaw(buf);
            /* Inject true color fg+bg to bypass terminal palette.
             * Ensures CGA-accurate colors regardless of terminal profile. */
            injectTrueColor(attr);
        }

        /* Update curatr only — do NOT call emitAttr() here.
         * ncurses attribute is set by out1chx() when characters are
         * actually rendered, and by explicit attrset() in clear ops.
         * Calling emitAttr here pollutes lastAttr_, causing stale
         * cache hits after session transitions (WFC → login). */
        *pCuratr_ = attr;
    }
}

void Terminal::injectTrueColor(unsigned char cga_attr)
{
    if (!remote_.active) return;
    int fgi = cga_attr & 0x0F;
    int bgi = (cga_attr >> 4) & 0x07;
    char buf[64];
    std::snprintf(buf, sizeof(buf),
                  "\x1b[38;2;%d;%d;%dm\x1b[48;2;%d;%d;%dm",
                  cgaRGB[fgi][0], cgaRGB[fgi][1], cgaRGB[fgi][2],
                  cgaRGB[bgi][0], cgaRGB[bgi][1], cgaRGB[bgi][2]);
    remoteWriteRaw(buf);
}


/* ================================================================== */
/*  ncurses ownership transfer                                         */
/* ================================================================== */

void Terminal::adoptLocal(Terminal& from)
{
    ncActive_ = from.ncActive_;
    use16colors_ = from.use16colors_;
    lastAttr_ = -1;  /* force re-sync on next emitAttr */
    from.ncActive_ = false;
    from.lastAttr_ = -1;
}

void Terminal::releaseLocal(Terminal& to)
{
    to.ncActive_ = ncActive_;
    to.use16colors_ = use16colors_;
    to.lastAttr_ = -1;
    ncActive_ = false;
    lastAttr_ = -1;
}


/* ================================================================== */
/*  ANSI art file sender                                               */
/* ================================================================== */

void Terminal::sendAnsiFile(const std::string& path)
{
    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) return;

    struct stat st;
    if (fstat(fd, &st) < 0) { close(fd); return; }

    /* Read entire file and send through putch() which handles everything:
     * - Remote output via remotePutch() (CP437→UTF-8)
     * - ANSI accumulation → executeAnsi() → updates *pCuratr_ + ncurses
     * - True-color injection (executeAnsi injects after SGR processing)
     * - Printable chars → out1chx() → ncurses + screen buffer */
    size_t sz = (size_t)st.st_size;
    std::vector<unsigned char> buf(sz);
    ssize_t n = read(fd, buf.data(), sz);
    close(fd);
    if (n <= 0) return;

    for (ssize_t i = 0; i < n; i++)
        putch(buf[i]);
}


/* ================================================================== */
/*  Output                                                             */
/* ================================================================== */

void Terminal::putch(unsigned char c)
{
    /* Remote output (skip tabs and form feed — handled specially below) */
    if (remote_.active && c != 9 && c != 12)
        remotePutch(c);

    /* ANSI escape accumulation */
    if (ansiPtr_ > 0) {
        ansiStr_[ansiPtr_++] = c;
        ansiStr_[ansiPtr_] = 0;
        if (((c < '0' || c > '9') && c != '[' && c != ';') ||
            ansiStr_[1] != '[' || ansiPtr_ > 75)
            executeAnsi();
    }
    else if (c == 27) {
        ansiStr_[0] = 27;
        ansiPtr_ = 1;
        ansiStr_[ansiPtr_] = 0;
    }
    else if (c == 9) {
        /* Tab expansion */
        int x = cursorX();
        int stop = ((x / 8) + 1) * 8;
        for (int i = x; i < stop; i++)
            putch(' ');
    }
    else {
        /* Form feed (12) → clearScreen() which handles both local + remote */
        out1ch(c);
    }
}

void Terminal::puts(const char *s)
{
    while (*s)
        putch((unsigned char)*s++);
}

void Terminal::newline()
{
    putch('\r');
    putch('\n');
}

void Terminal::printf(const char *fmt, ...)
{
    va_list ap;
    char buf[512];
    va_start(ap, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    puts(buf);
}

void Terminal::backspace()
{
    puts("\b \b");
}


/* ================================================================== */
/*  Keyboard input                                                     */
/* ================================================================== */

int Terminal::ncToScancode(int key)
{
    switch (key) {
    case KEY_F(1):  return 59;
    case KEY_F(2):  return 60;
    case KEY_F(3):  return 61;
    case KEY_F(4):  return 62;
    case KEY_F(5):  return 63;
    case KEY_F(6):  return 64;
    case KEY_F(7):  return 65;
    case KEY_F(8):  return 66;
    case KEY_F(9):  return 67;
    case KEY_F(10): return 68;
    case KEY_F(13): return 84;  /* Shift-F1 */
    case KEY_F(14): return 85;  /* Shift-F2 */
    case KEY_F(15): return 86;  /* Shift-F3 */
    case KEY_F(16): return 87;  /* Shift-F4 */
    case KEY_F(17): return 88;  /* Shift-F5 */
    case KEY_F(18): return 89;  /* Shift-F6 */
    case KEY_F(19): return 90;  /* Shift-F7 */
    case KEY_F(20): return 91;  /* Shift-F8 */
    case KEY_F(21): return 92;  /* Shift-F9 */
    case KEY_F(22): return 93;  /* Shift-F10 */
    case KEY_F(25): return 94;  /* Ctrl-F1 */
    case KEY_F(26): return 95;  /* Ctrl-F2 */
    case KEY_F(27): return 96;  /* Ctrl-F3 */
    case KEY_F(28): return 97;  /* Ctrl-F4 */
    case KEY_F(29): return 98;  /* Ctrl-F5 */
    case KEY_F(30): return 99;  /* Ctrl-F6 */
    case KEY_F(31): return 100; /* Ctrl-F7 */
    case KEY_F(32): return 101; /* Ctrl-F8 */
    case KEY_F(33): return 102; /* Ctrl-F9 */
    case KEY_F(34): return 103; /* Ctrl-F10 */
    case KEY_UP:    return 72;
    case KEY_DOWN:  return 80;
    case KEY_LEFT:  return 75;
    case KEY_RIGHT: return 77;
    case KEY_HOME:  return 71;
    case KEY_END:   return 79;
    case KEY_BTAB:  return 15;  /* Shift-Tab */
    default:        return 0;
    }
}

bool Terminal::localKeyReady()
{
    if (pendingScancode_ >= 0) return true;
    if (!ncActive_) return false;
    nodelay(stdscr, TRUE);
    int ch = wgetch(stdscr);
    if (ch == ERR) return false;
    ungetch(ch);
    return true;
}

unsigned char Terminal::localGetKey()
{
    if (pendingScancode_ >= 0) {
        int sc = pendingScancode_;
        pendingScancode_ = -1;
        return (unsigned char)sc;
    }
    if (!ncActive_) { usleep(100000); return 0; }
    nodelay(stdscr, FALSE);
    int ch;
    while ((ch = wgetch(stdscr)) == ERR)
        usleep(1000);
    nodelay(stdscr, TRUE);

    if (ch == KEY_ENTER || ch == '\n') return '\r';
    if (ch == KEY_BACKSPACE || ch == 127) return 8;
    if (ch >= KEY_MIN) {
        pendingScancode_ = ncToScancode(ch);
        return 0;
    }
    return (unsigned char)ch;
}

unsigned char Terminal::localGetKeyNB()
{
    if (pendingScancode_ >= 0) {
        int sc = pendingScancode_;
        pendingScancode_ = -1;
        return (unsigned char)sc;
    }
    if (!ncActive_) return 255;
    nodelay(stdscr, TRUE);
    int ch = wgetch(stdscr);
    if (ch == ERR) return 255;
    if (ch == KEY_ENTER || ch == '\n') return '\r';
    if (ch == KEY_BACKSPACE || ch == 127) return 8;
    if (ch >= KEY_MIN) {
        pendingScancode_ = ncToScancode(ch);
        return 0;
    }
    return (unsigned char)ch;
}


/* ================================================================== */
/*  Public input API                                                   */
/* ================================================================== */

bool Terminal::keyReady()
{
    return localKeyReady() || (remote_.active && remoteDataReady());
}

unsigned char Terminal::getKeyNB()
{
    /* Local keyboard first */
    if (localKeyReady()) {
        unsigned char ch = localGetKeyNB();
        if (ch != 255) return ch;
    }
    /* Remote TCP */
    if (remote_.active && remoteDataReady())
        return remoteGetKey();
    return 0;
}

unsigned char Terminal::getKey()
{
    unsigned char ch;
    do {
        while (!keyReady())
            usleep(10000);  /* 10ms poll */
        ch = getKeyNB();
    } while (!ch);
    if (ch == 127) ch = 8;  /* DEL → BS */
    return ch;
}


/* ================================================================== */
/*  State binding                                                      */
/* ================================================================== */

void Terminal::bindState(int *curatr, int *topLine, int *screenBottom)
{
    pCuratr_ = curatr;
    pTopLine_ = topLine;
    pScreenBottom_ = screenBottom;
}


/* ================================================================== */
/*  ncurses CP437 output                                               */
/* ================================================================== */

void Terminal::ncPutCp437(unsigned char ch)
{
    if (!ncActive_) return;
    addstr(cp437_to_utf8[ch]);
}
