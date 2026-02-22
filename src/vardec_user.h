#ifndef _VARDEC_USER_H_
#define _VARDEC_USER_H_

#include "vardec_types.h"

/* User Index (name-to-number mapping) */
typedef struct {
	char		name[31];
	unsigned short	number;
} smalrec;

/* DATA FOR EVERY USER */

typedef struct {
    char    name[31],
            realname[21],
            callsign[7],
            phone[21],
            dphone[21],
            pw[21],
            laston[9],
            firston[9],
            note[41],
            comment[41],
            street[41],
            city[41],
            macros[4][MAX_PATH_LEN],
            sex;

    unsigned char

            age,
            inact,
            comp_type,
            defprot,
            defed,
            flisttype,
            mlisttype,
            helplevel,
            lastsub,
            lastdir,
            lastconf,
            screenchars,
            screenlines,
            sl,
            dsl,
            exempt,
            colors[20],
            votes[20],
            illegal,
            waiting,
            subop,
            ontoday;

    unsigned short
            forwardusr,
            msgpost,
            emailsent,
            feedbacksent,
            posttoday,
            etoday,
            ar,
            dar,
            restrict,
            month,
            day,
            year;

    int
            fpts;

    unsigned short
            uploaded,
            downloaded,
            logons,
            fsenttoday1,
            emailnet,
            postnet;

    unsigned long
            msgread,
            uk,
            dk,
            daten,
            sysstatus,
            lastrate,
            nuv,
            timebank;

    float
            timeontoday,
            extratime,
            timeon,
            pcr,
            ratio,
            pos_account,
            neg_account;

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


/* SECLEV DATA FOR 1 SL */
typedef struct {
	unsigned short	time_per_day,
			time_per_logon,
                        maxcalls,
			emails,
			posts;
	unsigned long	ability;
} slrec;

/* AUTO-VALIDATION DATA */
typedef struct {
	unsigned char	sl,
			dsl;
	unsigned short	ar,
			dar,
			restrict;
} valrec;


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

#endif /* _VARDEC_USER_H_ */
