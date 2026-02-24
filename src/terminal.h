/*
 * terminal.h — Clean terminal I/O layer
 *
 * LEAF COMPONENT: depends on ncurses, TCP sockets, and nothing else.
 * No BBS headers, no vardec.h, no vars.h, no globals.
 *
 * Provides dual-stream output (local ncurses + remote TCP), ANSI color
 * generation/parsing, CP437→UTF-8 conversion, screen buffer management,
 * and keyboard/telnet input. Everything a BBS (or any terminal app)
 * needs for character-mode I/O, without knowing anything about BBS
 * data structures, user records, or configuration.
 */

#ifndef TERMINAL_H_
#define TERMINAL_H_

#include <string>
#include <termios.h>

class Terminal {
public:
    Terminal();
    ~Terminal();

    /* -- Lifecycle -- */
    bool initLocal();               /* ncurses setup; false if no tty */
    void shutdown();

    /* -- Remote TCP stream -- */
    void setRemote(int fd);
    void closeRemote();
    void detachRemote();            /* reset state without closing fd */
    bool remoteConnected() const;
    void sendTelnetNegotiation();
    void sendTerminalInit();        /* alt screen + black bg */
    void sendTerminalRestore();     /* restore primary screen */
    void remotePutch(unsigned char c);   /* single char, CP437→UTF-8 */
    void remoteWriteRaw(const char *s);  /* raw bytes to TCP */
    bool remoteDataReady();
    unsigned char remoteGetKey();         /* with IAC filtering */

    /* -- Output (writes to all active streams) -- */
    void putch(unsigned char c);
    void puts(const char *s);
    void newline();                 /* \r\n */
    void printf(const char *fmt, ...) __attribute__((format(printf, 2, 3)));

    /* -- Color -- */
    void setAttr(unsigned char attr);
    unsigned char attr() const { return (unsigned char)*pCuratr_; }
    void setCurAttr(unsigned char a) { *pCuratr_ = a; }
    int makeAnsi(unsigned char attr, char *buf);
    void emitAttr(int attr);
    int ncAttr(int dosAttr);
    void ncPutCp437(unsigned char ch);
    void injectTrueColor(unsigned char cga_attr);  /* send true-color fg+bg for CGA attr */

    /* -- Input -- */
    bool keyReady();                /* any input from any source? */
    unsigned char getKey();         /* blocking */
    unsigned char getKeyNB();       /* non-blocking, 0 if nothing */
    bool localKeyReady();
    unsigned char localGetKey();         /* blocking */
    unsigned char localGetKeyNB();       /* non-blocking, 255 if nothing */

    /* -- Screen primitives -- */
    void clearScreen();
    void clearToEol();
    void moveCursor(int x, int y);
    void gotoXY(int x, int y);          /* cursor position: local + TCP */
    void scrollUp(int top, int bottom, int lines);
    void out1chx(unsigned char ch);      /* char + attr to screen buf + ncurses */
    void out1ch(unsigned char ch);       /* dispatch CR/LF/BS/FF/printable */
    void cr();
    void lf();
    void bs();

    /* -- Screen state -- */
    int cursorX() const { return cx_; }
    int cursorY() const { return cy_ - *pTopLine_; }
    int cursorYabs() const { return cy_; }
    int screenBottom() const { return *pScreenBottom_; }
    int topLine() const { return *pTopLine_; }
    void setTopLine(int t) { *pTopLine_ = t; }
    void setScreenBottom(int b) { *pScreenBottom_ = b; }
    void setCursorPos(int x, int yabs) { cx_ = x; cy_ = yabs; }
    bool localActive() const { return ncActive_; }

    /* -- State binding (Phase 3: share memory with BBS io_session_t) -- */
    void bindState(int *curatr, int *topLine, int *screenBottom);

    /* -- Screen buffer -- */
    char *screenBuffer() { return scrn_; }
    void setScreenBuffer(char *buf) { scrn_ = buf; ownsScrn_ = false; }
    void scrnPut(int x, int y, unsigned char ch, unsigned char attr);
    void renderScrn(int startRow, int numRows);

    /* -- ncurses ownership transfer (console mirroring) -- */
    void adoptLocal(Terminal& from);   /* take ncurses ownership from another Terminal */
    void releaseLocal(Terminal& to);   /* give ncurses ownership back */

    /* -- ANSI art file -- */
    void sendAnsiFile(const std::string& path);  /* CP437 + CGA true-color */

    /* -- Helpers for input UI -- */
    void backspace();               /* BS, space, BS */

private:
    bool telnetFilter(unsigned char *ch);

    /* -- Screen buffer internal -- */
    void scrnScroll(int top, int bottom, int n);

    /* -- ANSI -- */
    void executeAnsi();
    void addAnsiParam(char *s, int val);

    /* -- Keyboard -- */
    int ncToScancode(int key);

    /* -- Stream state -- */
    struct Stream {
        int fd = -1;
        bool active = false;
        bool needs_iac = false;
    };
    Stream remote_;

    /* -- Screen state -- */
    char *scrn_ = nullptr;
    bool ownsScrn_ = true;
    int cx_ = 0, cy_ = 0;
    int lastAttr_ = -1;

    /* State fields with pointer indirection.
     * Pointers default to internal storage (standalone/iotest).
     * bindState() rebinds them to BBS io_session_t fields so both
     * sides share the same memory — no sync needed. */
    int curatrVal_ = 0x07;
    int topLineVal_ = 0;
    int screenBottomVal_ = 24;
    int *pCuratr_ = &curatrVal_;
    int *pTopLine_ = &topLineVal_;
    int *pScreenBottom_ = &screenBottomVal_;

    /* -- ANSI parser -- */
    char ansiStr_[81] = {};
    int ansiPtr_ = 0;
    int savedX_ = 0, savedY_ = 0;

    /* -- Telnet -- */
    int iacState_ = 0;

    /* -- Keyboard -- */
    int pendingScancode_ = -1;

    /* -- Terminal -- */
    bool ncActive_ = false;
    bool use16colors_ = false;
    struct termios origTermios_ = {};
};

#endif /* TERMINAL_H_ */
