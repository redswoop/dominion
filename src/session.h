/*
 * session.h — Per-session state struct (Phase B)
 *
 * Every per-session global lives here. System-wide config/state stays
 * in vars.h. Existing code compiles unchanged via compatibility macros
 * in vars.h (#define thisuser session.user, etc.).
 *
 * Layer 3: depends on io_stream.h, vardec_user.h, vardec_ui.h
 */

#ifndef _SESSION_H_
#define _SESSION_H_

/* vardec headers MUST come before io_stream.h — io_stream.h defines macros
 * (#define curspeed, etc.) that would corrupt struct member names */
#include "vardec_user.h"
#include "vardec_ui.h"
#include "io_stream.h"

typedef struct {
    io_session_t io;            /* I/O, screen, parser, capabilities */

    /* User identity */
    userrec user;               /* current user record (was thisuser) */
    int usernum;                /* 1-based user index */
    unsigned char actsl;        /* active security level */
    usersubrec usub[MAX_SUBS];
    usersubrec udir[MAX_DIRS];
    int umaxsubs, umaxdirs;

    /* Navigation */
    int cursub, curdir;
    int curlsub, curldir;
    char curconf;
    int confmode;

    /* Menu state */
    menurec tg[50];
    mmrec pp;
    char menuat[15];
    char mstack[10][15];
    char mdepth;
    char maxcmd;

    /* Session timing */
    double timeon, extratimecall, last_time, xtime, time_event;
    long hanguptime1, nscandate, timelastchar1;

    /* Session flags */
    int checkit, okmacro, okskey, mailcheck, smwcheck;
    int useron, backdoor, live_user, doinghelp;
    int in_extern, input_extern, use_workspace;
    int express, expressabort;
    int msgr, msgreadlogon;
    int fwaiting, fsenttoday;
    int topdata, sysop_alert, ltime, do_event;
    int already_on, arcling, bchanged;
    int dlf, edlf, numf, num_listed;
    int gat_section, global_xx, wfc;
    int ARC_NUMBER, MAX_BATCH;

    /* Batch transfer */
    batchrec batch;
    int batchdir;
    float batchsize, batchtime;
    int numbatch, numbatchdl, batchpoints;

    /* Message quoting */
    int bquote, equote, quoting;
    char *quote;

    /* Display / strings */
    char chatreason[MAX_PATH_LEN];
    char chatsoundon;
    char newprompt[161];
    char dszlog[MAX_PATH_LEN];
    char xdate[9];
    char filelistformat[100];
    char filelistformat2[100];
    char filelistformat3[100];
    char blueinput;
    char readinvoting;
    screentype screensave;

    /* Misc */
    unsigned short com_speed, modem_speed;
} session_t;

extern session_t session;
void session_init(session_t *s);

/* Compatibility: 'io' now lives inside session.
 * io_stream.h macros like #define hangup io.hangup chain through this:
 *   hangup -> io.hangup -> session.io.hangup
 */
#define io session.io

#endif /* _SESSION_H_ */
