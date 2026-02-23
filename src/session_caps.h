/*
 * session_caps.h — Session capability flags (Layer 2 — pure types)
 *
 * Replaces the monolithic okansi() single-flag check with granular
 * per-capability control.  CAP_TRANSFORM is defined for future MCP/AI
 * use but only OFF/ON are wired today.
 */

#ifndef _SESSION_CAPS_H_
#define _SESSION_CAPS_H_

typedef enum { CAP_OFF = 0, CAP_ON = 1, CAP_TRANSFORM = 2 } cap_mode_t;

typedef struct {
    cap_mode_t color;       /* SGR color sequences */
    cap_mode_t cursor;      /* cursor positioning (CUP, CUD, etc.) */
    cap_mode_t fullscreen;  /* full-screen UI (needs cursor + dimensions) */
    cap_mode_t cp437;       /* CP437 glyph rendering */
} session_caps_t;

#endif /* _SESSION_CAPS_H_ */
