#include "io_stream.h"

/* Undefine compat macros so we can access struct members directly */
#undef echo
#undef lastcon
#undef using_modem
#undef incom
#undef outcom
#undef ok_modem_stuff
#undef client_fd

io_session_t io;

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
}
