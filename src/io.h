#ifndef _IO_H_
#define _IO_H_

/*
 * io.h — Dominion I/O layer umbrella header
 *
 * Handles ncurses setup (macro conflict cleanup) which must happen
 * before platform.h / vars.h are included.
 *
 * io_stream.h (session state + compat macros) is pulled in by vars.h
 * and cannot be included here — the curspeed #define macro would collide
 * with a struct field in vardec_ui.h if active before vardec.h is parsed.
 *
 * Usage:
 *     #include "io.h"              // ncurses setup — before vars.h
 *     #define _DEFINE_GLOBALS_     // if this TU owns globals
 *     #include "vars.h"           // platform.h + fcns.h + io_stream.h
 *
 * After these includes you have:
 *   - io_session_t io (session state struct + compat macros)
 *   - ncurses functions (ncurses_init, nc_attr, nc_put_cp437, etc.)
 *   - All IO function prototypes (via fcns_io.h in fcns.h)
 */

#include "io_ncurses.h"

#endif /* _IO_H_ */
