#ifndef _VARDEC_H_
#define _VARDEC_H_
#define _X00_

#pragma warn -pro

#define NUV 1
#define BACK 1

#define EXTENDED 1

#define MAX_SUBS 200
#define MAX_DIRS 200

    typedef long                INT32;      /* 32 bits signed integer     */
    typedef unsigned long       UINT32;     /* 32 bits unsigned integer   */
    typedef short int           INT16;      /* 16 bits signed integer     */
    typedef unsigned short int  UINT16;     /* 16 bits unsigned integer   */
    typedef char                CHAR8;      /* 8 bits signed integer      */
    typedef unsigned char       UCHAR8;     /* 8 bits unsigned integer    */
    typedef int                 FHANDLE;    /* File handle                */


/* DATA FOR EVERY USER */

typedef struct {
    char    name[31],       // User's name
            realname[21],   // User's real name
            callsign[7],    // User's amateur callsign
            phone[21],      // User's Voice phone number
            dphone[21],     // User's Data phone number
            pw[21],         // User's password
            laston[9],      // last date on
            firston[9],     // first date on
            note[41],       // Sysop's note about user
            comment[41],    // User's comment
            street[41],     // User's street address
            city[41],       // User's City/St
            macros[4][81],  // Macros
            sex;            // User's sex

    unsigned char

            age,            // user's age
            inact,          // if deleted or inactive
            comp_type,      // computer type
            defprot,        // default transfer protocol
            defed,          // default editor
            flisttype,      // File List Format
            mlisttype,      // Message Header Format
            helplevel,      // User's Help Level
            lastsub,        // User's Last Message Area
            lastdir,        // User's Last File Area
            lastconf,       // User's Last Conference
            screenchars,    // Screen Length
            screenlines,    // screen Height
            sl,             // security level
            dsl,            // transfer security level
            exempt,         // exempt from ratios, etc
            colors[20],     // user's colors
            votes[20],      // user's votes
            illegal,        // illegal logons
            waiting,        // number mail waiting
            subop,          // sysop sub board number
            ontoday;        // number times on today
            forwardusr,     // User mail fowarded to
            msgpost,        // number messages posted
            emailsent,      // number of email sent
            feedbacksent,   // number of feedback sent
            posttoday,      // number posts today
            etoday,         // number emails today
            ar,             // board access
            dar,            // directory access
            restrict,       // restrictions on account
            month,
            day,
            year;           // user's birthday

    int
            fpts;           // Users File Points

    unsigned short
            uploaded,       // number files uploaded
            downloaded,     // number files downloaded
            logons,         // total number of logons
            fsenttoday1,    // feedbacks today
            emailnet,       // email sent into net
            postnet;        // posts sent into net

    unsigned long
            msgread,        // total num msgs read
            uk,             // number of k uploaded
            dk,             // number of k downloaded
            daten,          // numerical time last on
            sysstatus,      // status/defaults
            lastrate,       // last baud rate on
            nuv,            // Which NUV member
            timebank;       // Time in Bank

    float
            timeontoday,    // time on today
            extratime,      // time left today
            timeon,         // total time on system
            pcr,            // Specific PCR
            ratio,          // Specific K Ratio
            pos_account,    // $ credit
            neg_account;    // $ debit

// Reserved Bytes, in a few formats.

    char
            res[29];
    long
            resl[29];
    int
            resi[29];

    float
            resf[29];

    long
            qscn[200],
            nscn[200];
} userrec;

/*
typedef struct {
	char		name[31],		/* user's name */
			realname[21],		/* user's real name */
			callsign[7],		/* user's amateur callsign */
			phone[13],		/* user's phone number */
                        pw[20],                  /* user's password */
			laston[9],		/* last date on */
			firston[9],		/* first date on */
			note[41],		/* sysop's note about user */
                        comment[41],
                        street[41],
                        city[41],
                        state[2],
                        macros[4][81],          /* macro keys */
			sex;			/* user's sex */
	unsigned char	age,			/* user's age */
			inact,			/* if deleted or inactive */
			comp_type,		/* computer type */
			defprot,		/* deflt transfer protocol */
			defed,			/* default editor */
			screenchars,screenlines,/* screen size */
			sl,			/* security level */
			dsl,			/* transfer security level */
			exempt,			/* exempt from ratios, etc */
                        colors[20],              /* user's colors */
			votes[20],		/* user's votes */
			illegal,		/* illegal logons */
			waiting,		/* number mail waiting */
			sysopsub,		/* sysop sub board number */
			ontoday;		/* num times on today */
	unsigned short	homeuser,homesys,	/* where user can be found */
			forwardusr,forwardsys,	/* where to forward mail */
			msgpost,		/* number messages posted */
			emailsent,		/* number of email sent */
			feedbacksent,		/* number of f-back sent */
			posttoday,		/* number posts today */
			etoday,			/* number emails today */
			ar,			/* board access */
			dar,			/* directory access */
                        restrict;               /* restrictions on account */
        int             fpts;                   /* Users File Points */
        unsigned short  uploaded,               /* number files uploaded */
			downloaded,		/* number files downloaded */
			lastrate,		/* last baud rate on */
			logons;			/* total number of logons */
	unsigned long	msgread,		/* total num msgs read */
			uk,			/* number of k uploaded */
			dk,			/* number of k downloaded */
			qscn,			/* which subs to n-scan */
                        qscnptr[33],            /* q-scan pointers */
			nscn1,nscn2,		/* which dirs to n-scan */
			daten,			/* numerical time last on */
			sysstatus;		/* status/defaults */
	float		timeontoday,		/* time on today */
			extratime,		/* time left today */
			timeon,			/* total time on system */
			pos_account,		/* $ credit */
			neg_account,		/* $ debit */
                        ass_pts;                /* game money */
	unsigned char	bwcolors[8];		/* b&w colors */
	unsigned char	month,day,year;		/* user's birthday */
	unsigned int    emailnet,		/* email sent into net */
			postnet;		/* posts sent into net */
	unsigned short	fsenttoday1;		/* feedbacks today */
        unsigned char   num_extended;           /* num lines of ext desc */
        unsigned char   optional_val;           /* optional lines in msgs */
        unsigned long   timebank;
        char            res[29];                /* reserved bytes */
        unsigned long   qscn2;                  /* additional qscan ptr */
        unsigned long   qscnptr2[MAX_SUBS-32];  /* additional quickscan ptrs */
} olduserrec;
*/



/* SECLEV DATA FOR 1 SL */
typedef struct {
	unsigned short	time_per_day,		/* time allowed on per day */
			time_per_logon,		/* time allowed on per logon */
                        maxcalls,          /* messages allowed to read */
			emails,			/* number emails allowed */
			posts;			/* number posts allowed */
	unsigned long	ability;		/* bit mapped abilities */
} slrec;

/* AUTO-VALIDATION DATA */
typedef struct {
	unsigned char	sl,			/* SL */
			dsl;			/* DSL */
	unsigned short	ar,			/* AR */
			dar,			/* DAR */
			restrict;		/* restrictions */
} valrec;

typedef struct {
        char            extension[4],           /* extension for archive */
                        arca[32],
                        arce[32],
                        arcl[32];
} arcrec;


/* STATIC SYSTEM INFORMATION */
typedef struct {
	char		newuserpw[21],		/* new user password */
			systempw[21],		/* system password */
			msgsdir[81],		/* path for msgs directory */
			gfilesdir[81],		/* path for gfiles dir */
			datadir[81],		/* path for data directory */
			dloadsdir[81],		/* path for dloads dir */
			ramdrive,		/* drive for ramdisk */
			tempdir[81],		/* path for temporary directory */
                        resx[84],               /* reserved for never */
			bbs_init_modem[51],	/* modem initialization cmd */
			answer[21],		/* modem answer cmd */
                        menudir[105],
 /*                       connect_300[21],
                        connect_1200[21],
                        connect_2400[21],
			connect_9600[21],
                        connect_19200[21],*/
                        no_carrier[21],         /* modem disconnect */
			ring[21],		/* modem ring */
			terminal[21],		/* DOS cmd for run term prg */
			systemname[51],		/* BBS system name */
			systemphone[13],	/* BBS system phone number */
			sysopname[51],		/* sysop's name */
			executestr[51];		/* mail route path name */
	unsigned char	newusersl,		/* new user SL */
			newuserdsl,		/* new user DSL */
			maxwaiting,		/* max mail waiting */
			comport[5],		/* what connected to comm */
			com_ISR[5],		/* Com Interrupts */
			primaryport,		/* primary comm port */
			newuploads,		/* file dir new uploads go */
			closedsystem;		/* if system is closed */
	unsigned short	systemnumber,		/* BBS system number */
			baudrate[5],		/* Baud rate for com ports */
			com_base[5],		/* Com base addresses */
			maxusers,		/* max users on system */
			newuser_restrict,	/* new user restrictions */
			sysconfig,		/* System configuration */
			sysoplowtime,		/* Chat time on */
			sysophightime,		/* Chat time off */
			executetime;		/* time to run mail router */
	float		req_ratio,		/* required up/down ratio */
			newusergold;		/* new user gold */
	slrec		sl[256];		/* security level data */
	valrec		autoval[10];		/* sysop quik-validation data*/
	char		hangupphone[21],	/* string to hang up phone */
			pickupphone[21];	/* string to pick up phone */
	unsigned int    netlowtime,		/* net time on */
			nethightime;		/* net time off */
        char            connect_300_a[105];      /* alternate connect str's */
        arcrec          arcs[4];
        char            beginday_c[51],
                        logon_c[51];
        int             userreclen,
                        waitingoffset,
                        inactoffset;
        char            newuser_c[51];
        unsigned long   wwiv_reg_number;
        char            dial_prefix[21];
        float           post_call_ratio;
        char            upload_c[51];
        char            dszbatchdl[81];
        char            modem_type[9];
        char            batchdir[81];
        int             sysstatusoffset;
        char            network_type;
        char            res[36];               /* RESERVED */
} configrec;



/* DYNAMIC SYSTEM STATUS */
typedef struct {
	char		date1[9],		/* last date active */
			date2[9],		/* date before now */
			date3[9],		/* two days ago */
			log1[13],		/* yesterday's log */
			log2[13],		/* two days ago log */
                        dltoday,                // DLed today
                        log5[13],               // Log of 5 days ago
                        resx[4];                /* reserved for nothing */
	unsigned short	users,			/* Current number of users */
			callernum,		/* Current caller number */
			callstoday,		/* Number of calls today */
			msgposttoday,		/* Messages posted today*/
			emailtoday,		/* Email sent today */
			fbacktoday,		/* Feedback sent today */
			uptoday,		/* files uploaded today */
			activetoday;		/* Minutes active today */
	unsigned long	qscanptr;		/* Q-scan pointer value */
	char		amsganon;		/* auto-message anony stat */
	unsigned short	amsguser;		/* user who wrote a-msg */
	unsigned long	callernum1;		/* caller number */
        unsigned int    net_edit_stuff;         /* word for net editor */
        unsigned int    wwiv_version;           /* tell what version it is */
        unsigned int    net_version;            /* tell what version of net */
        float           net_bias;               /* network bias factor */
        long            last_connect,           /* date last connect.net */
                        last_bbslist;           /* date last bbslist.net */
        float           net_req_free;           /* net free factor def 3 */
        char            log3[18],               // Log of 3 days ago
                        log4[13],               // Log of 4 Days ago
                        lastuser[31];           // Last Caller
} statusrec;


// Subboardrec.anony
#define anony_enable_anony 0x01
#define anony_force_anony 0x04
#define anony_real_name 0x08

// Subboardrec.mattr
#define mattr_netmail 0x0001
#define mattr_ansi_only 0x0002
#define mattr_autoscan 0x0004
#define mattr_fidonet 0x0008
#define mattr_nomci 0x0010
#define mattr_private 0x0020
#define mattr_deleted 0x0040



// File Directory Record
typedef struct {
	char		name[41],		/* directory name */
			filename[9],		/* direct database filename */
                        dpath[81],               /* filename path */
                        upath[12],
                        vacs[21],
                        res[48],
                        acs[21];
	unsigned short	dar,			/* DAR for directory */
			maxfiles,		/* max files for directory */
			mask,			/* file type mask */
                        type,                   /* 4 digit directory type */
                        confnum;
} directoryrec;
		


// User Index
typedef struct {
	char		name[31];
	unsigned short	number;
} smalrec;



// Message Base Index
typedef struct {
        unsigned long   stored_as,              /* where it is stored */
            storage_type;           /* how it is stored */
} messagerec;



// Post Data
typedef struct {
	char		title[81];		/* title of post */
	unsigned char	anony,			/* anony-stat of message */
			status;			/* bit-mapped status */
	unsigned short	ownersys,owneruser;	/* who posted it */
	unsigned long	qscan,			/* qscan pointer */
			daten;			/* numerical date posted */
	messagerec	msg;			/* where to find it */
} postrec;



// Email Data
typedef struct {
	char		title[81];		/* E-mail title */
	unsigned char	anony,			/* anonymous mail? */
			status;			/* status for e-mail */
	unsigned short	fromsys,fromuser,	/* originating system,user */
			tosys,touser;		/* destination system,user */
	unsigned long	daten;			/* date it was sent */
	messagerec	msg;			/* where to find it */
} mailrec;



// Short Message Data
typedef struct {
	char		message[81];		/* short message to user */
	unsigned short	tosys,touser;		/* who it is to */
} shortmsgrec;



// Voting Responses
typedef struct {
	char		response[81];		/* Voting question response */
	unsigned short	numresponses;		/* number of responses */
} voting_response;



/* VOTING DATA INFORMATION */
typedef struct {
	char		question[81];		/* Question */
	unsigned char	numanswers;		/* number of responses */
	voting_response	responses[20];		/* actual responses */
} votingrec;

// Directory Entry Record
typedef struct {
    char            filename[13],       /* filename */
                    description[39],    /* file description */
                    date[9],            /* date u/l'ed */
                    upby[46];           /* name of upload user */
    unsigned char   filetype;           /* file type for apples */
    unsigned short  numdloads,          /* number times d/l'ed */
                    ownersys,ownerusr;  /* who uploaded it */
int                    points,
                    mask;               /* file type mask */
    int             ats[5];
    unsigned long   daten,              /* date uploaded */
                    numbytes;           /* number bytes long file is */
}  uploadsrec;



// History Log
typedef struct {
	char		date[9];		/* zlog for what date */
	unsigned short	active,			/* number minutes active */
			calls,			/* number calls */
			posts,			/* number posts */
			email,			/* number e-mail */
			fback,			/* number f-back */
                        up,                     /* number uploads */
                        dl;
} zlogrec;


/* DATA FOR EXTERNAL PROTOCOLS */
typedef struct {
        char        description[81],
                    receivefn[81],
                    sendfn[81],
                    sendbatch[81],
                    receivebatch[81];
        int         singleok;
    unsigned short  ok1,nok1,ok2,nok2;
       char         key;
} protocolrec;



/* DATA FOR CONVERSION OF MAIN MENU KEYS TO SUB-BOARD NUMBERS */
typedef struct {
	char		keys[3];
	int		subnum;
} usersubrec;


typedef struct {
        char            sending;
	char            filename[13];
	char            batchdesc[59];
        short           dir;
	float           time;
	float           size;
        short           extdesc;
        long            len;
        int             points;
} batchrec;

typedef struct {
        char            direction,
                        refresh,
                        replace,
                        verify,
                        delete,
                        unused,
                        fulldir,
                        subdir,
                        sourcepath[80],
                        destpath[80],
                        descr[80];
} bimodrec;

typedef struct {
        char            resv1[121],
                        uppath[80],
                        resv2[538];
} bicfgrec;



/* userrec.inact */
#define inact_deleted 0x01
#define inact_inactive 0x02
#define inact_lockedout 0x04

/* userrec.exempt */
#define exempt_ratio 0x01
#define exempt_time 0x02
#define exempt_userlist 0x04
#define exempt_post 0x08


/* userrec.restrict */
#define restrict_logon 0x0001
#define restrict_chat 0x0002
#define restrict_validate 0x0004
#define restrict_automessage 0x0008
#define restrict_anony 0x0010
#define restrict_post 0x0020
#define restrict_email 0x0040
#define restrict_vote 0x0080
#define restrict_auto_msg_delete 0x0100
#define restrict_net 0x0200
#define restrict_upload 0x0400
#define restrict_rumours 0x0800
#define restrict_timebank 0x1000
#define restrict_bbslist 0x2000
#define restrict_userlist 0x4000

#define restrict_string "LCMA*PEVKN!RTBU "

/* userrec.sysstatus */
#define sysstatus_ansi 0x0001
#define sysstatus_color 0x0002
#define sysstatus_fullline 0x0004
#define sysstatus_pause_on_page 0x0008
#define sysstatus_rip 0x0010
#define sysstatus_smw 0x0020
#define sysstatus_full_screen 0x0040
#define sysstatus_nscan_file_system 0x0080
#define sysstatus_regular 0x0100
#define sysstatus_clr_scrn 0x0200
#define sysstatus_avatar 0x0400

/* slrec.ability */
#define ability_post_anony 0x0001
#define ability_email_anony 0x0002
#define ability_read_post_anony 0x0004
#define ability_read_email_anony 0x0008
#define ability_limited_cosysop 0x0010
#define ability_cosysop 0x0020
#define ability_val_net 0x0040

/* subboardrec.anony */
#define anony_enable_anony 0x01
#define anony_enable_dear_abby 0x02
#define anony_force_anony 0x04
#define anony_real_name 0x08
#define anony_val_net 0x10
#define anony_ansi_only 0x20
#define anony_no_tag 0x40
#define anony_require_sv 0x80

/* postrec.anony, mailrec.anony */
#define anony_sender 0x01
#define anony_sender_da 0x02
#define anony_sender_pp 0x03
#define anony_receiver 0x10
#define anony_receiver_da 0x20
#define anony_receiver_pp 0x30

/* directoryrec.mask */
#define mask_PD 0x0001
#define mask_no_uploads 0x0004
#define mask_archive 0x0008
#define mask_unavail 0x0010
#define mask_no_ratio 0x0020
#define mask_autocredit 0x0040
#define mask_FDN  0x0080
#define mask_extended 0x8000

/* postrec.status */
#define status_unvalidated 0x01
#define status_delete 0x02
#define status_no_delete 0x04
#define status_pending_net 0x08
#define status_post_source_verified 0x10
#define status_pending_fido 0x20

/* mailrec.status */
#define status_multimail 0x01
#define status_source_verified 0x02

/* configrec.sysconfig */
#define sysconfig_no_local 0x0001
#define sysconfig_no_beep 0x0002
#define sysconfig_high_speed 0x0004
#define sysconfig_off_hook 0x0008
#define sysconfig_two_color 0x0010
#define sysconfig_flow_control 0x0020
#define sysconfig_printer 0x0040
#define sysconfig_list 0x0080
#define sysconfig_no_xfer 0x0100
#define sysconfig_2_way 0x0200
#define sysconfig_no_alias 0x0400
#define sysconfig_all_sysop 0x0800
#define sysconfig_shrink_term 0x1000
#define sysconfig_free_phone 0x2000
#define sysconfig_log_dl 0x4000

/* editorrec.config */
#define config_80_25 0x0001


#define PREV 1
#define NEXT 2
#define DONE 4
#define NUM_ONLY 1
#define UPPER_ONLY 2
#define ALL 4
#define SET 8


struct line {
	char		text[160];
	struct line 	*prev,*next;
};


typedef struct {
  int x1,y1,topline1,curatr1;
  char *scrn1;
} screentype;

typedef struct{
  char name[13];
  int len;
} ext_desc_type;


typedef struct {
        char            curspeed[31];           /* description of speed */
        char            return_code[41];        /* modem result code */
        unsigned short  modem_speed,            /* speed modems talk at */
                        com_speed,              /* speed com port runs at */
                        mode,
                        attr;
} resultrec;


#define max_buf 1024
#define MSG_COLOR 0


/****************************************************************************/
/* modem info structure */

#define mode_norm     1   /* normal status */
#define mode_ring     2   /* phone is ringing */
#define mode_dis      3   /* disconnected (no connection) */
#define mode_err      4   /* error encountered */
#define mode_ringing  5   /* remote phone is ringing */
#define mode_con      6   /* connection established */
#define mode_ndt      7   /* no dial tone */
#define mode_fax      8   /* fax connection */

#define flag_as       1   /* asymmetrical baud rates */
#define flag_ec       2   /* error correction in use */
#define flag_dc       4   /* data compression in use */
#define flag_fc       8   /* flow control should be used */
#define flag_append   16  /* description string should be appended */

/*
typedef struct {
  char result[41];
  char description[31];
  unsigned int main_mode;
  unsigned int flag_mask;
  unsigned int flag_value;
  unsigned int com_speed;
  unsigned int modem_speed;
} result_info;
*/

typedef struct {
  unsigned short ver;
  char name[81];
  char init[161];
  char setu[161];
  char ansr[81];
  char pick[81];
  char hang[81];
  char dial[81];

  char sepr[10];
} modem_info;

#define xarc_swap 0x001
#define xarc_euc 0x002

typedef struct {
        char extension[4];
        char arct[32],
             arcc[32],
             arca[32],
             arce[32],
             arcl[32];
        long attr;
         int ok1,ok2;
         int nk1,nk2;
} xarcrec;


typedef struct {
        char arct[32],
             arcc[32];
} exarc;

typedef struct {
    char desc[50],
         key[21],
         type[3],
         line[40],
         ms[80];
    char sl[21];
    int  attr;
} menurec;

typedef struct {
#ifdef PD
    char prompt[101],
         prompt2[91],
#else
    char prompt[192],
#endif
         helpfile[10],
         title1[101],
         title2[101],
         altmenu[9],
         format[9],
         slneed[31],
         pausefile[9],
         helplevel,
         columns,
         col[3],
         boarder,
         battr;
    int  attr;
} mmrec;

#define command_hidden 0x0001
#define command_unhidden 0x0002
#define command_title 0x0004
#define command_pulldown 0x0008
#define command_forced 0x0010
#define command_every 0x0020
#define command_default 0x0040

#define menu_extprompt 0x0001
#define menu_pulldown 0x0002
#define menu_format 0x0004
#define menu_promptappend 0x0008
#define menu_popup 0x0010
#define menu_noglobal 0x0020
#define menu_hideglobal 0x0040

typedef struct {
        char name[59],
             tag[20],
             type,
             flagstr[10],
             sl[10];
} confrec;

typedef struct {
     char      chatcolor,
               echochar,
               firstmenu[15],
               matrixtype,
               fcom,
               res,
               newusermenu[15],
               systemtype,
               menudir[30],
               fptsratio;
 unsigned long lockoutrate,
               nifstatus;
          char rotate[5];
         exarc arc[4];
          char matrix[31],
               lockoutpw[31];
          char nuvbad,
               nuvyes,
               nuvlevel,
               nuvsl[10],
               nuvinf[8],
               nuinf[8],
               nulevel,
               defaultcol[20],
               nuvaction,
               nuvbadlevel;
} niftyrec;

typedef struct {
        char epcr[31],
             eratio[31],
             efpts[31],
             etc[31],
             syspw[31],
             showpw[31],
             callcmd[31],
             readunval[31],
             cosysop[31],
             sysop[31],
             echat[31],
             dlunval[31],
             anyul[31],
             readanon[31],
             delmsg[31],
             zapmail[31];
} acsrec;


typedef struct {
    int un,
        sy;
} mmailrec;

#define nif_fpts 0x0001
#define nif_nuv 0x0002
#define nif_chattype 0x0004
#define nif_comment 0x0008
#define nif_mciok 0x0010
#define nif_autochat 0x0020
#define nif_ratio 0x0040
#define nif_forcevote 0x0080
#define nif_pcr 0x0100
#define nif_mpl 0x0200
#define nif_autocredit 0x0400
#define nif_automsg 0x0800
#define nif_lastfew 0x1000
#define nif_yourinfo 0x2000

#include "fido.h"

// Message Base Record
typedef struct {
            char        name[41],               // board name
                        filename[9],            // board database filename
                        nmpath[51],             // Netmail path
                        conf;                   // Which Conference
        unsigned char   readacs[21],            // Acs required to read
                        postacs[21],            // Acs required to post
                        anony,                  // Anonymous Type
                        age;                    // Minimum age for sub
        unsigned long   attr;                   // Attributes
        unsigned short  maxmsgs,                // max # of msgs
                        ar,                     // Ars
                        storage_type;           // how messages are stored
        addressrec      add;                    // Fido Alternate Address
        char            origin;                 // Which origin line
} subboardrec;

// Sturcture to pass msghdr information between functions

typedef struct {
    char subject[81],
    who_from[41],
    who_to[41],
    comment[41];
    UINT32 attr;
    UINT32 ReplyTo,Reply1st,ReplyNext,date;
    addressrec f;
    addressrec t;
    char msgid[41],replyid[41];
} hdrinfo;

#include "nuv.h"

/****************************************************************************/

#endif

