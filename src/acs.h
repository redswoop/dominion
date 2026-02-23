/*
 * acs.h — Access Control String evaluator.
 *
 * Evaluates ACS (Access Control String) expressions used throughout the
 * menu system to gate commands by security level, AR/DAR flags, age,
 * baud rate, user number, exempt flags, etc.
 *
 * ACS strings are '&'-delimited clauses.  Each clause starts with a
 * type letter optionally preceded by '!' for negation:
 *
 *   S200     — security level >= 200
 *   AA       — AR flag 'A' set
 *   !D50     — download SL < 50 (negated)
 *   S100&AA  — SL >= 100 AND AR 'A' set
 *
 * Clause types:
 *   A[A-Z]  AR flag          I[A-Z]  DAR flag
 *   S<n>    security level   D<n>    download SL
 *   G<n>    minimum age      B<n>    baud rate (hundreds)
 *   U<n>    user number      V       always denied
 *   C       can post         F       fast logon
 *   #       sysop            @<c>    conference flag
 *   H<n>    help level       E[RTUP] exempt flags
 *
 * Handler signature: all evaluation is pure — reads only from the
 * context struct.  acs_fill_context() bridges session globals.
 */

#ifndef _ACS_H_
#define _ACS_H_

typedef struct {
    unsigned char  sl;            /* active security level (actsl) */
    unsigned char  dsl;           /* download security level */
    unsigned char  age;
    unsigned char  exempt;        /* exempt flags bitmask */
    unsigned short ar;            /* AR flags */
    unsigned short dar;           /* DAR flags */
    int            usernum;
    int            modem_speed;
    int            is_sysop;      /* sysop2() result */
    int            is_backdoor;
    int            fastlogon;
    int            can_post;      /* postr_ok() result or caller override */
    unsigned char  helplevel;
    const char    *conf_flagstr;  /* sys.conf[curconf].flagstr */
} acs_context_t;

/* Fill context from current session/system state. */
void acs_fill_context(acs_context_t *ctx);

/* Evaluate an ACS string against a context.
 * Returns 1 if all clauses pass (or string is empty), 0 if any fails.
 * Unknown clause types pass silently (curok stays 1). */
int acs_check(const char *acs_string, const acs_context_t *ctx);

/* Thin wrapper: fill context from globals, call acs_check().
 * menu==0 or 3: execution mode ('C' always denies).
 * menu==1: display mode (respects 'C'). */
int slok(char val[31],char menu);

#endif /* _ACS_H_ */
