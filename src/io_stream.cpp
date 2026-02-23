#include "io_stream.h"
#include <string.h>

/* Undefine computed macros so we can access struct members directly */
#undef using_modem
#undef incom
#undef outcom
#undef ok_modem_stuff
#undef client_fd

/* io_session_t io; — removed Phase B, now lives in session_t */

void io_init(io_session_t *s)
{
    int i;
    for (i = 0; i < IO_COUNT; i++) {
        s->stream[i].fd_in = -1;
        s->stream[i].fd_out = -1;
        s->stream[i].in_active = 0;
        s->stream[i].out_active = 0;
        s->stream[i].available = 0;
        s->stream[i].needs_iac = 0;
        s->stream[i].cols = 80;
        s->stream[i].rows = 25;
    }
    /* Local stream uses stdin/stdout, always active */
    s->stream[IO_LOCAL].fd_in = 0;  /* STDIN_FILENO */
    s->stream[IO_LOCAL].fd_out = 1; /* STDOUT_FILENO */
    s->stream[IO_LOCAL].in_active = 1;
    s->stream[IO_LOCAL].out_active = 1;
    s->stream[IO_LOCAL].available = 1;

    s->echo = 1;
    s->session_type = 0;
    s->lastcon = 0;
    s->echo_char = '*';

    /* com.c parser state (Phase 2) */
    s->bluein = 0;
    s->colblock = 0;
    s->pending_scancode = -1;

    /* IO-internal globals (Phase 3) */
    s->hungup = 0;
    s->change_color = 0;
    s->change_ecolor = 0;
    memset(s->ansistr, 0, sizeof(s->ansistr));
    s->ansiptr = 0;
    memset(s->endofline, 0, sizeof(s->endofline));
    s->oldx = 0;
    s->oldy = 0;
    s->screenlen = 0;
    s->lecho = 0;
    memset(&s->orig_termios, 0, sizeof(s->orig_termios));
    s->term_raw_mode = 0;

    /* Screen state (Phase 4) */
    s->curatr = 0x07;     /* white on black */
    s->topline = 0;
    s->screenbottom = 24;
    s->screenlinest = 25;
    s->defscreenbottom = 24;
    s->lines_listed = 0;
    s->listing = 0;
    s->scrn = 0;         /* allocated by caller */

    /* Session interface globals (Phase 5) */
    s->hangup = 0;
    s->mciok = 0;
    memset(s->charbuffer, 0, sizeof(s->charbuffer));
    s->charbufferpointer = 0;
    s->chatcall = 0;
    s->chatting = 0;
    s->chat_file = 0;
    s->x_only = 0;
    memset(s->curspeed, 0, sizeof(s->curspeed));
    s->global_handle = 0;

    /* Session capabilities (Phase 6) — default all ON for existing sessions */
    s->caps.color = CAP_ON;
    s->caps.cursor = CAP_ON;
    s->caps.fullscreen = CAP_ON;
    s->caps.cp437 = CAP_ON;
}
