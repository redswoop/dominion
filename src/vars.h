#ifndef _VARS_H_
#define _VARS_H_

#include "platform.h"

#include "fcns.h"
#include "io_stream.h"

#ifdef _DEFINE_GLOBALS_

/****************************************************************************/

/* --- Batch transfer --- */
batchrec batch;
int batchdir;
float batchsize;
float batchtime;

/* --- Quoting --- */
int bquote,equote,quoting;
char *quote;

/* --- Terminal/ANSI --- */
/* ansistr, endofline moved to io_session_t (Phase 3) */
/* charbuffer, curspeed, mciok moved to io_session_t (Phase 5) */
char cdir[MAX_PATH_LEN], chatreason[MAX_PATH_LEN], crttype,chatsoundon,
     dszlog[MAX_PATH_LEN], xdate[9], newprompt[161],
     cfilt[255],scfilt[255],blueinput,filelistformat[100],curconf,num_conf,
     filelistformat2[100],filelistformat3[100],
     readinvoting;

/* --- Session state --- */
/* Phases 3-5: ansiptr, change_color, change_ecolor, hungup, lecho, oldx, oldy,
 * screenlen, curatr, topline, screenbottom, screenlinest, defscreenbottom,
 * lines_listed, listing, charbufferpointer, chatcall, chatting, chat_file,
 * global_handle, hangup, x_only moved to io_session_t */
int already_on, arcling, async_irq, base, bchanged,confmode,
    checkit, configfile, curdir, curldir,
    curlsub, cursub, dlf, do_event, edlf,
    endday, express, expressabort, fsenttoday, fwaiting,
    gat_section, global_xx,
    input_extern, in_extern, live_user,
    ltime, mailcheck, msgreadlogon, noklevel, no_hangup, numbatch,
    numbatchdl, batchpoints, numextrn, numf, nummsgs, num_call_sys, num_dirs,
    num_listed, num_subs,
    oklevel, okmacro, okskey, ooneuser,
    restoring_shrink,
    smwcheck, statusfile, sysop_alert, tempio, topdata,
    usernum, useron, use_workspace, wfc,
    running_dv,msgr,umaxsubs,umaxdirs,
    ARC_NUMBER,MAX_BATCH,backdoor;

long hanguptime1, nscandate, this_date, timelastchar1;

double extratimecall, last_time, timeon, time_event, xtime;

unsigned char actsl;

/* --- Communication --- */
unsigned short com_speed, modem_speed;

protocolrec proto[20];

/* --- Config/system --- */
fnetrec fnet;
niftyrec nifty;
confrec conf[20];
userrec thisuser;
configrec syscfg;
statusrec status;
subboardrec *subboards;
directoryrec *directories;
usersubrec usub[MAX_SUBS],udir[MAX_DIRS];
screentype screensave;
messagerec menus[50];
xarcrec xarc[8];

/* --- TCP/network --- */
int X00port;
union REGS regs;

int tcp_port;
int listen_fd;
/* orig_termios, term_raw_mode moved to io_session_t (Phase 3) */

/* --- Misc --- */
char *xenviron[50];
int questused[20];
long last_time_c=0L;
/* scrn moved to io_session_t (Phase 4) */
char *sp;

JAMAPIREC JamRec;

/****************************************************************************/
#else
/****************************************************************************/

/* --- Batch transfer --- */
extern float batchtime;
extern float batchsize;
extern int numbatch,numbatchdl,batchpoints;
extern batchrec batch;
extern int batchdir;

/* --- Terminal/ANSI --- */
/* ansistr, endofline moved to io_session_t (Phase 3) */
/* charbuffer, curspeed, mciok moved to io_session_t (Phase 5) */
extern char cdir[MAX_PATH_LEN], chatreason[MAX_PATH_LEN],
     crttype,chatsoundon, dszlog[MAX_PATH_LEN],
     xdate[9], newprompt[161],
     cfilt[255],scfilt[255],blueinput,filelistformat[100],curconf,num_conf,
     filelistformat2[100],filelistformat3[100],
     readinvoting;

/* --- Session state --- */
/* Phases 3-5: ansiptr, change_color, change_ecolor, hungup, lecho, oldx, oldy,
 * screenlen, curatr, topline, screenbottom, screenlinest, defscreenbottom,
 * lines_listed, listing, charbufferpointer, chatcall, chatting, chat_file,
 * global_handle, hangup, x_only moved to io_session_t */
extern int already_on, arcling, async_irq, base, bchanged,confmode,
    checkit, configfile, curdir, curldir,
    curlsub, cursub, dlf, do_event, edlf,
    endday, express, expressabort, fsenttoday, fwaiting,
    gat_section, global_xx,
    input_extern, in_extern, live_user,
    ltime, mailcheck, msgreadlogon, noklevel, no_hangup, numbatch,
    numbatchdl, numextrn, numf, nummsgs, num_call_sys, num_dirs,
    num_listed, num_subs,
    oklevel, okmacro, okskey, ooneuser,
    restoring_shrink,
    smwcheck, statusfile, sysop_alert, tempio, topdata,
    usernum, useron, use_workspace, wfc,
    running_dv,msgr,umaxsubs,umaxdirs,
    ARC_NUMBER,MAX_BATCH,backdoor;

extern long hanguptime1, nscandate, this_date, timelastchar1;

extern double extratimecall, last_time, timeon, time_event, xtime;

extern unsigned char actsl;

/* --- Communication --- */
extern unsigned short com_speed, modem_speed;

extern protocolrec proto[20];

/* --- Config/system --- */
extern niftyrec nifty;
extern userrec thisuser;
extern fnetrec fnet;
extern configrec syscfg;
extern confrec conf[20];
extern statusrec status;
extern subboardrec *subboards;
extern directoryrec *directories;
extern usersubrec usub[MAX_SUBS],udir[MAX_DIRS];
extern screentype screensave;
extern messagerec menus[50];
extern xarcrec xarc[8];

/* --- TCP/network --- */
extern int X00port;
extern union REGS regs;

extern int tcp_port;
extern int listen_fd;
/* orig_termios, term_raw_mode moved to io_session_t (Phase 3) */

/* --- Misc --- */
extern char *xenviron[50];
extern int questused[20];
extern long last_time_c;
/* scrn moved to io_session_t (Phase 4) */
extern char *sp;

/* --- Quoting --- */
extern char *quote;
extern int bquote,equote,quoting;
/****************************************************************************/
#endif

/* from version.c */
extern char *wwiv_version;
extern char *wwiv_date;
extern unsigned int wwiv_num_version,CRCVal;


extern JAMAPIREC JamRec;

#endif
