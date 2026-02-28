/*
 * system.h — System-wide state singleton (Phase C: kill vars.h)
 *
 * All system-wide globals live here. Per-session state stays in Session.
 * Previously accessed via compatibility macros in vars.h
 * (#define syscfg sys.cfg, etc.) — now accessed directly.
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
#include "jam/jammb.h"

class System {
public:
    static System& instance();

    /* Config (loaded from Config.dat) */
    configrec cfg;              /* was syscfg */
    niftyrec nifty;
    statusrec status;

    /* Board definitions */
    subboardrec *subboards;
    directoryrec *directories;
    protocolrec proto[20];
    confrec conf[20];
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
    int is_child_process;  /* Set in forked children, 0 in supervisor */

    /* Network */
    int tcp_port, listen_fd;

    /* Paths */
    char cdir[MAX_PATH_LEN];
    char cfilt[255], scfilt[255];

    /* Scheduled events */
    int do_event, wfc;
    double time_event, last_time, xtime;
    char xdate[9];

    /* Misc */
    int global_xx;
    int ARC_NUMBER, MAX_BATCH;
    char *xenviron[50];
    int questused[20];
    long last_time_c;
    long this_date;
    char *sp;
    JAMAPIREC JamRec;

private:
    System();
};

/* System state persistence */
void save_status();
void read_in_file(char *fn, messagerec *m, int maxary);

#endif /* _SYSTEM_H_ */
