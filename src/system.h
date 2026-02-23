/*
 * system.h — System-wide state struct (Phase B3)
 *
 * All system-wide globals live here. Per-session state stays in session_t.
 * Existing code compiles unchanged via compatibility macros in vars.h
 * (#define syscfg sys.cfg, etc.).
 *
 * Layer 3: depends on vardec headers, platform.h, jammb.h
 */

#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#include "vardec_types.h"
#include "vardec_config.h"
#include "vardec_msgfile.h"
#include "vardec_ui.h"
#include "platform.h"
#include "jammb.h"

typedef struct {
    /* Config (loaded from Config.dat) */
    configrec cfg;              /* was syscfg — renamed to avoid 'sys.syscfg' */
    niftyrec nifty;
    statusrec status;           /* NO MACRO — collides with postrec.status, mailrec.status */

    /* Board definitions */
    subboardrec *subboards;
    directoryrec *directories;
    protocolrec proto[20];
    confrec conf[20];           /* NO MACRO — collides with subboardrec.conf */
    messagerec menus[50];
    xarcrec xarc[8];

    /* Counts */
    int num_subs, num_dirs;
    int numextrn, nummsgs, num_call_sys;
    char num_conf;

    /* Process flags */
    int endday, ooneuser, no_hangup;
    int oklevel, noklevel;
    int configfile, statusfile;
    int restoring_shrink, tempio;

    /* Network / serial (vestigial) */
    int tcp_port, listen_fd;
    int X00port, async_irq, base;
    union REGS regs;

    /* Paths */
    char cdir[MAX_PATH_LEN];
    char cfilt[255], scfilt[255];

    /* Scheduled events (moved from session_t) */
    int do_event, wfc;
    double time_event, last_time, xtime;
    char xdate[9];

    /* Misc moved from session_t */
    int global_xx;
    int ARC_NUMBER, MAX_BATCH;

    /* Misc */
    char *xenviron[50];
    int questused[20];
    long last_time_c;
    long this_date;
    char *sp;
    JAMAPIREC JamRec;
} system_t;

extern system_t sys;
void system_init(system_t *s);

#endif /* _SYSTEM_H_ */
