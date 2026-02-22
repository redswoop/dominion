#ifndef _IO_STREAM_H_
#define _IO_STREAM_H_

#define IO_LOCAL  0
#define IO_REMOTE 1
#define IO_COUNT  2

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

#endif /* _IO_STREAM_H_ */
