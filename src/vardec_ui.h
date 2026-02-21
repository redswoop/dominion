#ifndef _VARDEC_UI_H_
#define _VARDEC_UI_H_

#include "vardec_types.h"

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
        char            curspeed[31];
        char            return_code[41];
        unsigned short  modem_speed,
                        com_speed,
                        mode,
                        attr;
} resultrec;


#define max_buf 1024
#define MSG_COLOR 0


/****************************************************************************/
/* modem info structure */

#define mode_norm     1
#define mode_ring     2
#define mode_dis      3
#define mode_err      4
#define mode_ringing  5
#define mode_con      6
#define mode_ndt      7
#define mode_fax      8

#define flag_as       1
#define flag_ec       2
#define flag_dc       4
#define flag_fc       8
#define flag_append   16

typedef struct {
  unsigned short ver;
  char name[MAX_PATH_LEN];
  char init[161];
  char setu[161];
  char ansr[MAX_PATH_LEN];
  char pick[MAX_PATH_LEN];
  char hang[MAX_PATH_LEN];
  char dial[MAX_PATH_LEN];

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


/* DATA FOR EXTERNAL PROTOCOLS */
typedef struct {
        char        description[MAX_PATH_LEN],
                    receivefn[MAX_PATH_LEN],
                    sendfn[MAX_PATH_LEN],
                    sendbatch[MAX_PATH_LEN],
                    receivebatch[MAX_PATH_LEN];
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


#pragma pack(push, 1)
typedef struct {
    char desc[50],
         key[21],
         type[3],
         line[40],
         ms[80];
    char sl[21];
    INT16 attr;
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
    INT16 attr;
} mmrec;
#pragma pack(pop)

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

#endif /* _VARDEC_UI_H_ */
