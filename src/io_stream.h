#ifndef _IO_STREAM_H_
#define _IO_STREAM_H_

#include <termios.h>
#include "session_caps.h"

#define IO_LOCAL  0
#define IO_REMOTE 1
#define IO_COUNT  2

#define MAX_ANSISTR 81

#ifndef MAX_PATH_LEN
#define MAX_PATH_LEN 261
#endif

typedef struct {
    int fd_in;           /* -1 = inactive */
    int fd_out;          /* -1 = inactive */
    int in_active;       /* accepting input */
    int out_active;      /* sending output */
    int available;       /* stream configured/usable */
    int needs_iac;       /* telnet IAC filter/escape */
    int cols, rows;      /* terminal dimensions */
} io_stream_t;

typedef struct {
    io_stream_t stream[IO_COUNT];
    int echo;            /* 1=normal, 0=masked */
    int session_type;    /* 0=local, 1=remote, -1=fast-login */
    int lastcon;         /* 0=remote, 1=local */
    char echo_char;      /* mask char (usually '*') */

    /* com.c parser state (Phase 2) */
    char bluein;            /* input field: 0=none, 1=blue, 2=alt */
    int  colblock;          /* color override */
    int  pending_scancode;  /* ncurses→DOS bridge */

    /* IO-internal globals from vars.h (Phase 3) */
    int  hungup;            /* connection lost flag */
    int  change_color;      /* ctrl-3 color change pending */
    int  change_ecolor;     /* ctrl-14 extended color pending */
    char ansistr[MAX_ANSISTR]; /* ANSI escape sequence accumulator */
    int  ansiptr;           /* current position in ansistr */
    char endofline[MAX_ANSISTR]; /* deferred end-of-line color */
    int  oldx, oldy;        /* saved cursor position (ANSI ESC[s/u) */
    int  screenlen;         /* screen buffer size in bytes */
    int  lecho;             /* local echo override */
    struct termios orig_termios; /* saved terminal state */
    int  term_raw_mode;     /* terminal in raw mode flag */

    /* Screen state (Phase 4) */
    int  curatr;            /* current DOS color attribute byte */
    int  topline;           /* top line of user area (below status bar) */
    int  screenbottom;      /* bottom line of user area */
    int  screenlinest;      /* user's screen lines (from thisuser) */
    int  defscreenbottom;   /* default screen bottom (usually 24) */
    int  lines_listed;      /* lines since last pause */
    int  listing;           /* in file listing mode (suppresses pause) */
    char *scrn;             /* shadow screen buffer (80*25*2 bytes) */

    /* Session interface globals (Phase 5) */
    int  hangup;            /* end-session flag */
    char mciok;             /* MCI code expansion enabled */
    char charbuffer[161];   /* keyboard type-ahead buffer */
    int  charbufferpointer; /* position in charbuffer */
    int  chatcall;          /* sysop chat request pending */
    int  chatting;          /* in chat mode */
    int  chat_file;         /* chat log file descriptor */
    int  x_only;            /* transfer-only mode */
    char curspeed[MAX_PATH_LEN]; /* current connection speed string */
    int  global_handle;     /* global file handle */

    /* Session capabilities (Phase 6) */
    session_caps_t caps;    /* granular ANSI capability control */
} io_session_t;

extern io_session_t io;
void io_init(io_session_t *s);

/* Compatibility macros -- existing code compiles unchanged */
#define incom           io.stream[IO_REMOTE].in_active
#define outcom          io.stream[IO_REMOTE].out_active
#define ok_modem_stuff  io.stream[IO_REMOTE].available
#define using_modem     io.session_type
#define echo            io.echo
#define lastcon         io.lastcon
#define client_fd       io.stream[IO_REMOTE].fd_out

/* Phase 2 — com.c parser state */
#define colblock        io.colblock

/* Phase 3 — IO-internal globals */
#define hungup          io.hungup
#define change_color    io.change_color
#define change_ecolor   io.change_ecolor
#define ansistr         io.ansistr
#define ansiptr         io.ansiptr
#define endofline       io.endofline
#define oldx            io.oldx
#define oldy            io.oldy
#define screenlen       io.screenlen
#define lecho           io.lecho
#define orig_termios    io.orig_termios
#define term_raw_mode   io.term_raw_mode

/* Phase 4 — screen state */
#define curatr          io.curatr
#define topline         io.topline
#define screenbottom    io.screenbottom
#define screenlinest    io.screenlinest
#define defscreenbottom io.defscreenbottom
#define lines_listed    io.lines_listed
#define listing         io.listing
#define scrn            io.scrn

/* Phase 5 — session interface globals */
#define hangup          io.hangup
#define mciok           io.mciok
#define charbuffer      io.charbuffer
#define charbufferpointer io.charbufferpointer
#define chatcall        io.chatcall
#define chatting        io.chatting
#define chat_file       io.chat_file
#define x_only          io.x_only
#define curspeed        io.curspeed
#define global_handle   io.global_handle

#endif /* _IO_STREAM_H_ */
