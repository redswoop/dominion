#ifndef _VARS_H_
#define _VARS_H_

#include "platform.h"

#include "fcns.h"
#include "session.h"   /* includes io_stream.h; provides session_t + #define io session.io */
#include "system.h"    /* system_t + #define syscfg sys.cfg, etc. */

#ifdef _DEFINE_GLOBALS_

/* All system-wide globals now live in system_t (Phase B3).
 * The global instance 'sys' is defined in system.c.
 * Compatibility macros below let existing code compile unchanged. */

/****************************************************************************/
#else
/****************************************************************************/

/* Nothing — all externs are in system.h and session.h */

/****************************************************************************/
#endif

/* from version.c */
extern const char *wwiv_version;
extern const char *wwiv_date;
extern unsigned int wwiv_num_version,CRCVal;

/*=========================================================================*/
/* Phase B3: System-wide compatibility macros                               */
/* System globals now live in system_t. These macros let existing code      */
/* compile unchanged: syscfg -> sys.cfg, subboards -> sys.subboards, etc.   */
/* NO MACRO for 'status' (collides with postrec.status, mailrec.status)    */
/* NO MACRO for 'conf' (collides with subboardrec.conf)                    */
/*=========================================================================*/

#define syscfg          sys.cfg
#define nifty           sys.nifty
/* status: no macro — collides with postrec.status, mailrec.status */
#define subboards       sys.subboards
#define directories     sys.directories
#define proto           sys.proto
/* conf: no macro — collides with subboardrec.conf */
#define menus           sys.menus
#define xarc            sys.xarc
#define num_subs        sys.num_subs
#define num_dirs        sys.num_dirs
#define numextrn        sys.numextrn
#define nummsgs         sys.nummsgs
#define num_call_sys    sys.num_call_sys
#define num_conf        sys.num_conf
#define endday          sys.endday
#define ooneuser        sys.ooneuser
#define no_hangup       sys.no_hangup
#define oklevel         sys.oklevel
#define noklevel        sys.noklevel
#define configfile      sys.configfile
#define statusfile      sys.statusfile
#define restoring_shrink sys.restoring_shrink
#define tempio          sys.tempio
#define tcp_port        sys.tcp_port
#define listen_fd       sys.listen_fd
#define X00port         sys.X00port
#define async_irq       sys.async_irq
#define base            sys.base
#define regs            sys.regs
#define cdir            sys.cdir
#define cfilt           sys.cfilt
#define scfilt          sys.scfilt
#define xenviron        sys.xenviron
#define questused       sys.questused
#define last_time_c     sys.last_time_c
#define this_date       sys.this_date
#define sp              sys.sp
#define JamRec          sys.JamRec

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
#define already_on      session.already_on
#define arcling         session.arcling
#define bchanged        session.bchanged
#define dlf             session.dlf
#define edlf            session.edlf
#define numf            session.numf
#define num_listed      session.num_listed
#define gat_section     session.gat_section

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
#define filelistformat  session.filelistformat
#define filelistformat2 session.filelistformat2
#define filelistformat3 session.filelistformat3
#define blueinput       session.blueinput
#define readinvoting    session.readinvoting
#define screensave      session.screensave

#define com_speed       session.com_speed
#define modem_speed     session.modem_speed

/* Moved from session_t to system_t (Phase B3) */
#define do_event        sys.do_event
#define wfc             sys.wfc
#define time_event      sys.time_event
#define last_time       sys.last_time
#define xtime           sys.xtime
#define xdate           sys.xdate
#define global_xx       sys.global_xx
#define ARC_NUMBER      sys.ARC_NUMBER
#define MAX_BATCH       sys.MAX_BATCH

#endif /* _VARS_H_ */
