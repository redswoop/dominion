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
char ansistr[MAX_PATH_LEN], cdir[MAX_PATH_LEN], charbuffer[161], chatreason[MAX_PATH_LEN], crttype,chatsoundon,
     curspeed[MAX_PATH_LEN],  dszlog[MAX_PATH_LEN], endofline[MAX_PATH_LEN],xdate[9], newprompt[161],
     mciok,cfilt[255],scfilt[255],blueinput,filelistformat[100],curconf,num_conf,
     filelistformat2[100],filelistformat3[100],
     readinvoting;

/* --- Session state --- */
int already_on, ansiptr,  arcling, async_irq, base, bchanged,confmode,
    change_color, charbufferpointer, chatcall, chatting, chat_file,
    change_ecolor, checkit, configfile, curatr, curdir, curldir,
    curlsub, cursub,  defscreenbottom, dlf, do_event, edlf,
    endday, express, expressabort, fsenttoday, fwaiting,
    gat_section, global_handle, global_xx, hangup, hungup,
    input_extern, in_extern, lecho, lines_listed, live_user,
    ltime, mailcheck, msgreadlogon, noklevel, no_hangup, numbatch,
    numbatchdl, batchpoints, numextrn, numf, nummsgs, num_call_sys, num_dirs,
    num_listed, num_subs,
    oklevel, okmacro, okskey, oldx, oldy, ooneuser,
    restoring_shrink, screenbottom, screenlen, screenlinest,
    smwcheck, statusfile, sysop_alert, tempio, topdata, topline,
    usernum, useron, use_workspace, wfc,
    x_only,listing,running_dv,msgr,umaxsubs,umaxdirs,
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
struct termios orig_termios;
int term_raw_mode;

/* --- Misc --- */
char *xenviron[50];
int questused[20];
long last_time_c=0L;
char far *scrn;
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
extern char ansistr[MAX_PATH_LEN], cdir[MAX_PATH_LEN], charbuffer[161], chatreason[MAX_PATH_LEN],
     crttype,chatsoundon, curspeed[MAX_PATH_LEN],  dszlog[MAX_PATH_LEN],
     endofline[MAX_PATH_LEN], xdate[9], newprompt[161],
     mciok,cfilt[255],scfilt[255],blueinput,filelistformat[100],curconf,num_conf,
     filelistformat2[100],filelistformat3[100],
     readinvoting;

/* --- Session state --- */
extern int already_on, ansiptr,  arcling, async_irq, base, bchanged,confmode,
    change_color, charbufferpointer, chatcall, chatting, chat_file,
    change_ecolor, checkit, configfile, curatr, curdir, curldir,
    curlsub, cursub,  defscreenbottom, dlf, do_event, edlf,
    endday, express, expressabort, fsenttoday, fwaiting,
    gat_section, global_handle, global_xx, hangup, hungup,
    input_extern, in_extern, lecho, lines_listed, live_user,
    ltime, mailcheck, msgreadlogon, noklevel, no_hangup, numbatch,
    numbatchdl, numextrn, numf, nummsgs, num_call_sys, num_dirs,
    num_listed, num_subs,
    oklevel, okmacro, okskey, oldx, oldy, ooneuser,
    restoring_shrink, screenbottom, screenlen, screenlinest,
    smwcheck, statusfile, sysop_alert, tempio, topdata, topline,
    usernum, useron, use_workspace, wfc,
    x_only,listing,running_dv,msgr,umaxsubs,umaxdirs,
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
extern struct termios orig_termios;
extern int term_raw_mode;

/* --- Misc --- */
extern char *xenviron[50];
extern int questused[20];
extern long last_time_c;
extern char far *scrn;
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
