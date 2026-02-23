#ifndef _VARS_H_
#define _VARS_H_

#include "platform.h"

#include "fcns.h"
#include "session.h"   /* includes io_stream.h; provides session_t + #define io session.io */

#ifdef _DEFINE_GLOBALS_

/****************************************************************************/

/* --- System-wide strings --- */
char cdir[MAX_PATH_LEN], cfilt[255], scfilt[255], num_conf;

/* --- System-wide state --- */
/* Per-session globals moved to session_t (Phase B) */
int async_irq, base, configfile,
    endday, noklevel, no_hangup,
    numextrn, nummsgs, num_call_sys, num_dirs, num_subs,
    oklevel, ooneuser,
    restoring_shrink, statusfile, tempio;

long this_date;

protocolrec proto[20];

/* --- Config/system --- */
niftyrec nifty;
confrec conf[20];
configrec syscfg;
statusrec status;
subboardrec *subboards;
directoryrec *directories;
messagerec menus[50];
xarcrec xarc[8];

/* --- TCP/network --- */
int X00port;
union REGS regs;

int tcp_port;
int listen_fd;

/* --- Misc --- */
char *xenviron[50];
int questused[20];
long last_time_c=0L;
char *sp;

JAMAPIREC JamRec;

/****************************************************************************/
#else
/****************************************************************************/

/* --- System-wide strings --- */
extern char cdir[MAX_PATH_LEN], cfilt[255], scfilt[255], num_conf;

/* --- System-wide state --- */
extern int async_irq, base, configfile,
    endday, noklevel, no_hangup,
    numextrn, nummsgs, num_call_sys, num_dirs, num_subs,
    oklevel, ooneuser,
    restoring_shrink, statusfile, tempio;

extern long this_date;

extern protocolrec proto[20];

/* --- Config/system --- */
extern niftyrec nifty;
extern configrec syscfg;
extern confrec conf[20];
extern statusrec status;
extern subboardrec *subboards;
extern directoryrec *directories;
extern messagerec menus[50];
extern xarcrec xarc[8];

/* --- TCP/network --- */
extern int X00port;
extern union REGS regs;

extern int tcp_port;
extern int listen_fd;

/* --- Misc --- */
extern char *xenviron[50];
extern int questused[20];
extern long last_time_c;
extern char *sp;

/****************************************************************************/
#endif

/* from version.c */
extern const char *wwiv_version;
extern const char *wwiv_date;
extern unsigned int wwiv_num_version,CRCVal;

extern JAMAPIREC JamRec;

/*=========================================================================*/
/* Phase B compatibility macros                                             */
/* Per-session globals now live in session_t. These macros let existing     */
/* code compile unchanged: thisuser -> session.user, etc.                   */
/* The 'io' macro is defined in session.h: #define io session.io            */
/* io_stream.h macros chain through: hangup -> io.hangup -> session.io.hangup */
/*=========================================================================*/

#define thisuser        session.user
#define usernum         session.usernum
#define actsl           session.actsl
#define usub            session.usub
#define udir            session.udir
#define umaxsubs        session.umaxsubs
#define umaxdirs        session.umaxdirs

#define cursub          session.cursub
#define curdir          session.curdir
#define curlsub         session.curlsub
#define curldir         session.curldir
#define curconf         session.curconf
#define confmode        session.confmode

#define tg              session.tg
#define pp              session.pp
#define menuat          session.menuat
#define mstack          session.mstack
#define mdepth          session.mdepth
#define maxcmd          session.maxcmd

/* timeon: no macro — collides with userrec.timeon member.
 * Use session.timeon explicitly. */
#define extratimecall   session.extratimecall
#define last_time       session.last_time
#define xtime           session.xtime
#define time_event      session.time_event
#define hanguptime1     session.hanguptime1
#define nscandate       session.nscandate
#define timelastchar1   session.timelastchar1

#define checkit         session.checkit
#define okmacro         session.okmacro
#define okskey          session.okskey
#define mailcheck       session.mailcheck
#define smwcheck        session.smwcheck
#define useron          session.useron
#define backdoor        session.backdoor
#define live_user       session.live_user
#define doinghelp       session.doinghelp
#define in_extern       session.in_extern
#define input_extern    session.input_extern
#define use_workspace   session.use_workspace
#define express         session.express
#define expressabort    session.expressabort
#define msgr            session.msgr
#define msgreadlogon    session.msgreadlogon
#define fwaiting        session.fwaiting
#define fsenttoday      session.fsenttoday
#define topdata         session.topdata
#define sysop_alert     session.sysop_alert
#define ltime           session.ltime
#define do_event        session.do_event
#define already_on      session.already_on
#define arcling         session.arcling
#define bchanged        session.bchanged
#define dlf             session.dlf
#define edlf            session.edlf
#define numf            session.numf
#define num_listed      session.num_listed
#define gat_section     session.gat_section
#define global_xx       session.global_xx
#define wfc             session.wfc
#define ARC_NUMBER      session.ARC_NUMBER
#define MAX_BATCH       session.MAX_BATCH

#define batch           session.batch
/* batchdir: no macro — collides with configrec.batchdir member.
 * Use session.batchdir explicitly. */
#define batchsize       session.batchsize
#define batchtime       session.batchtime
#define numbatch        session.numbatch
#define numbatchdl      session.numbatchdl
#define batchpoints     session.batchpoints

#define bquote          session.bquote
#define equote          session.equote
#define quoting         session.quoting
#define quote           session.quote

#define chatreason      session.chatreason
#define chatsoundon     session.chatsoundon
#define newprompt       session.newprompt
#define dszlog          session.dszlog
#define xdate           session.xdate
#define filelistformat  session.filelistformat
#define filelistformat2 session.filelistformat2
#define filelistformat3 session.filelistformat3
#define blueinput       session.blueinput
#define readinvoting    session.readinvoting
#define screensave      session.screensave

#define com_speed       session.com_speed
#define modem_speed     session.modem_speed

#endif /* _VARS_H_ */
