/*
 * session.c — Per-session state singleton implementation (Phase C)
 */

#include "session.h"
#include <string.h>

/* Undefine the io compatibility macro so we can access the member directly */
#undef io

Session& Session::instance()
{
    static Session s;
    return s;
}

Session::Session()
{
    memset(this, 0, sizeof(Session));

    /* Initialize the I/O subsystem */
    io_init(&this->io);

    /* Session defaults matching frequent_init() */
    curlsub = -1;
    okmacro = 1;
    okskey = 1;
    live_user = 1;
    msgr = 1;
    chatsoundon = 1;
}
