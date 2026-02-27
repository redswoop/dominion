/*
 * console.h — Sysop console multiplexer
 *
 * Provides a unified ncurses console for the sysop that shows system
 * status, active nodes, and can mirror any session's screen. Uses PTYs
 * to isolate each BBS session while proxying TCP and local I/O.
 *
 * Entry point: console_run() — called from main() when -P without -Q.
 */

#ifndef CONSOLE_H_
#define CONSOLE_H_

/* Main console event loop. Does not return until shutdown. */
void console_run(void);

#endif /* CONSOLE_H_ */
