#ifndef _VARDEC_CONFIG_H_
#define _VARDEC_CONFIG_H_

#include "vardec_types.h"
#include "vardec_user.h"

typedef struct {
        char            extension[4],
                        arca[32],
                        arce[32],
                        arcl[32];
} arcrec;


/* STATIC SYSTEM INFORMATION */
typedef struct {
	char		newuserpw[21],
			systempw[21],
			msgsdir[MAX_PATH_LEN],
			gfilesdir[MAX_PATH_LEN],
			datadir[MAX_PATH_LEN],
			dloadsdir[MAX_PATH_LEN],
			ramdrive,
			tempdir[MAX_PATH_LEN],
                        resx[84],
			bbs_init_modem[51],
			answer[21],
                        menudir[105],
                        no_carrier[21],
			ring[21],
			terminal[21],
			systemname[51],
			systemphone[13],
			sysopname[51],
			executestr[51];
	unsigned char	newusersl,
			newuserdsl,
			maxwaiting,
			comport[5],
			com_ISR[5],
			primaryport,
			newuploads,
			closedsystem;
	unsigned short	systemnumber,
			baudrate[5],
			com_base[5],
			maxusers,
			newuser_restrict,
			sysconfig,
			sysoplowtime,
			sysophightime,
			executetime;
	float		req_ratio,
			newusergold;
	slrec		sl[256];
	valrec		autoval[10];
	char		hangupphone[21],
			pickupphone[21];
	unsigned int    netlowtime,
			nethightime;
        char            connect_300_a[105];
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
        char            dszbatchdl[MAX_PATH_LEN];
        char            modem_type[9];
        char            batchdir[MAX_PATH_LEN];
        int             sysstatusoffset;
        char            network_type;
        char            res[36];
} configrec;



/* DYNAMIC SYSTEM STATUS */
typedef struct {
	char		date1[9],
			date2[9],
			date3[9],
			log1[13],
			log2[13],
                        dltoday,
                        log5[13],
                        resx[4];
	unsigned short	users,
			callernum,
			callstoday,
			msgposttoday,
			emailtoday,
			fbacktoday,
			uptoday,
			activetoday;
	unsigned long	qscanptr;
	char		amsganon;
	unsigned short	amsguser;
	unsigned long	callernum1;
        unsigned int    net_edit_stuff;
        unsigned int    wwiv_version;
        unsigned int    net_version;
        float           net_bias;
        long            last_connect,
                        last_bbslist;
        float           net_req_free;
        char            log3[18],
                        log4[13],
                        lastuser[31];
} statusrec;


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

typedef struct {
        char arct[32],
             arcc[32];
} exarc;

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

#endif /* _VARDEC_CONFIG_H_ */
