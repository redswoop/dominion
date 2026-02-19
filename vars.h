#ifndef _VARS_H_
#define _VARS_H_

#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys\stat.h>
#include <dos.h>
#include <alloc.h>
#include <time.h>


#include "fcns.h"

#ifdef _DEFINE_GLOBALS_

/****************************************************************************/

int numbatch,numbatchdl,batchpoints;
batchrec batch;

int batchdir;
int bquote,equote,quoting;
char *quote;

float batchtime;
float batchsize;

char ansistr[81], cdir[81], charbuffer[161], chatreason[81], crttype,chatsoundon,
     curspeed[81],  dszlog[81], endofline[81],xdate[9], newprompt[161],
     mciok,cfilt[255],scfilt[255],blueinput,filelistformat[100],curconf,num_conf,
     filelistformat2[100],filelistformat3[100];

int already_on, ansiptr,  arcling, async_irq, base, bchanged,confmode,
    change_color, charbufferpointer, chatcall, chatting, chat_file,
    change_ecolor, checkit, configfile, curatr, curdir, curldir,
    curlsub, cursub,  defscreenbottom, dlf, do_event, echo, edlf,
    endday, express, expressabort, flow_control, fsenttoday, fwaiting,
    gat_section, global_handle, global_xx, hangup, hungup, incom,
    input_extern, in_extern, lastcon, lecho, lines_listed, live_user,
    ltime, mailcheck, msgreadlogon, noklevel, no_hangup, numbatch,
    numbatchdl, numextrn, numf, nummsgs, num_call_sys, num_dirs,
    num_listed, num_subs,
    oklevel, okmacro, okskey, ok_modem_stuff, oldx, oldy, ooneuser,
    outcom, restoring_shrink, screenbottom, screenlen, screenlinest,
    smwcheck, statusfile, sysop_alert, tempio, topdata, topline,
    userfile, usernum, useron, use_workspace, using_modem, wfc,
    x_only,listing,running_dv,msgr,umaxsubs,umaxdirs,
    ARC_NUMBER,MAX_BATCH,backdoor;

long hanguptime1, nscandate, this_date, timelastchar1;

float batchtime;

double extratimecall, last_time, timeon, time_event, xtime;

unsigned char actsl, andwith;


unsigned short com_speed,modem_flag, modem_mode,
               modem_speed;

protocolrec proto[20];
resultrec result_codes[30];
int num_result_codes;
fnetrec fnet;
niftyrec nifty;
confrec conf[20];
userrec thisuser;
configrec syscfg;
statusrec status;
smalrec *smallist;
subboardrec *subboards;
directoryrec *directories;
usersubrec usub[MAX_SUBS],udir[MAX_DIRS];
screentype screensave;
messagerec menus[50];
xarcrec xarc[8];
modem_info *modem_i;

int X00port;
union REGS regs;

char *xenviron[50];
int questused[20];
long last_time_c=0L;
char far *scrn;
char *sp;


JAMAPIREC JamRec;

/****************************************************************************/
#else
/****************************************************************************/

extern float batchtime;
extern float batchsize;
extern int numbatch,numbatchdl,batchpoints;
extern batchrec batch;
extern int batchdir;

extern char ansistr[81], cdir[81], charbuffer[161], chatreason[81],
     crttype,chatsoundon, curspeed[81],  dszlog[81],
     endofline[81], xdate[9], newprompt[161],
     mciok,cfilt[255],scfilt[255],blueinput,filelistformat[60],curconf,num_conf,
     filelistformat2[60],filelistformat3[60],
     readinvoting;

extern int already_on, ansiptr,  arcling, async_irq, base, bchanged,confmode,
    change_color, charbufferpointer, chatcall, chatting, chat_file,
    change_ecolor, checkit, configfile, curatr, curdir, curldir,
    curlsub, cursub,  defscreenbottom, dlf, do_event, echo, edlf,
    endday, express, expressabort, flow_control, fsenttoday, fwaiting,
    gat_section, global_handle, global_xx, hangup, hungup, incom,
    input_extern, in_extern, lastcon, lecho, lines_listed, live_user,
    ltime, mailcheck, msgreadlogon, noklevel, no_hangup, numbatch,
    numbatchdl, numextrn, numf, nummsgs, num_call_sys, num_dirs,
    num_listed, num_subs,
    oklevel, okmacro, okskey, ok_modem_stuff, oldx, oldy, ooneuser,
    outcom, restoring_shrink, screenbottom, screenlen, screenlinest,
    smwcheck, statusfile, sysop_alert, tempio, topdata, topline,
    userfile, usernum, useron, use_workspace, using_modem, wfc,
    x_only,listing,running_dv,msgr,umaxsubs,umaxdirs,
    ARC_NUMBER,MAX_BATCH,backdoor;


                                                    
extern long hanguptime1, nscandate, this_date, timelastchar1;

extern float batchtime;

extern double extratimecall, last_time, timeon, time_event, xtime;

extern unsigned char actsl,andwith;


extern unsigned short com_speed, modem_flag, modem_mode,
               modem_speed;


extern protocolrec proto[20];
extern niftyrec nifty;
extern userrec thisuser;
extern fnetrec fnet;
extern configrec syscfg;
extern confrec conf[20];
extern statusrec status;
extern smalrec *smallist;
extern subboardrec *subboards;
extern directoryrec *directories;
extern usersubrec usub[MAX_SUBS],udir[MAX_DIRS];
extern screentype screensave;
extern messagerec menus[50];
extern modem_info *modem_i;
extern xarcrec xarc[8];

extern int X00port;
extern union REGS regs;

extern char *xenviron[50];
extern resultrec result_codes[30];
extern int num_result_codes;
extern int questused[20];
extern long last_time_c;
extern char far *scrn;
extern char *sp;


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
