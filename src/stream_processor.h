#ifndef STREAM_PROCESSOR_H_
#define STREAM_PROCESSOR_H_

/* Stream processor — interprets markup in the BBS output byte stream.
 *
 * Handles: pipe color (|nn), easy color (char 6), change_color (char 3),
 * change_ecolor (char 14), avatar protocol (chars 5/22/151), MCI expansion
 * (backtick), ANSI escape accumulation/parsing.
 *
 * For markup bytes: consumed internally, calls term_set_attr() etc.
 * For ANSI sequences: raw bytes go to TCP, parsed locally via term_* calls.
 * For normal characters: calls stream_emit_char() (implemented by bbs_output.c).
 */

void stream_putch(unsigned char c);

/* Reset all parser state (call on session init) */
void stream_reset(void);

#endif /* STREAM_PROCESSOR_H_ */
