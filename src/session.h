/*
 * session.h — Per-session state singleton (Phase C: kill vars.h)
 *
 * Every per-session global lives here. System-wide config/state stays
 * in System. Previously accessed via compatibility macros in vars.h
 * (#define thisuser session.user, etc.) — now accessed directly.
 *
 * Layer 3: depends on io_stream.h, vardec_user.h, vardec_ui.h
 */

#ifndef _SESSION_H_
#define _SESSION_H_

/* vardec headers MUST come before io_stream.h — io_stream.h macros
 * (incom, outcom, etc.) would corrupt struct member names */
#include "vardec_user.h"
#include "vardec_ui.h"
#include "io_stream.h"

class Session {
public:
    static Session& instance();

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
    double timeon, extratimecall;
    long hanguptime1, nscandate, timelastchar1;

    /* Session flags */
    int checkit, okmacro, okskey, mailcheck, smwcheck;
    int useron, backdoor, live_user, doinghelp;
    int in_extern, input_extern, use_workspace;
    int express, expressabort;
    int msgr, msgreadlogon;
    int fwaiting, fsenttoday;
    int topdata, sysop_alert, ltime;
    int already_on, arcling, bchanged;
    int dlf, edlf, numf, num_listed;
    int gat_section;

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
    char filelistformat[100];
    char filelistformat2[100];
    char filelistformat3[100];
    char blueinput;
    char readinvoting;
    screentype screensave;

    /* Misc */
    unsigned short com_speed, modem_speed;

private:
    Session();
};

/* io_stream.h computed macros (incom, outcom, etc.) chain through this:
 *   incom -> io.stream[IO_REMOTE].in_active -> Session::instance().io.stream[IO_REMOTE].in_active
 */
#define io Session::instance().io

#endif /* _SESSION_H_ */
