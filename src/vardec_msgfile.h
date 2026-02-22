#ifndef _VARDEC_MSGFILE_H_
#define _VARDEC_MSGFILE_H_

#include "vardec_types.h"
#include "fido.h"


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

/* subboardrec.mattr */
#define mattr_netmail 0x0001
#define mattr_ansi_only 0x0002
#define mattr_autoscan 0x0004
#define mattr_fidonet 0x0008
#define mattr_nomci 0x0010
#define mattr_private 0x0020
#define mattr_deleted 0x0040

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


/* File Directory Record */
typedef struct {
	char		name[41],
			filename[9],
                        dpath[MAX_PATH_LEN],
                        upath[12],
                        vacs[21],
                        res[48],
                        acs[21];
	unsigned short	dar,
			maxfiles,
			mask,
                        type,
                        confnum;
} directoryrec;


/* smalrec (User Index) moved to vardec_user.h */

/* Message Base Index */
typedef struct {
        unsigned long   stored_as,
            storage_type;
} messagerec;


/* Post Data */
typedef struct {
	char		title[MAX_PATH_LEN];
	unsigned char	anony,
			status;
	unsigned short	ownersys,owneruser;
	unsigned long	qscan,
			daten;
	messagerec	msg;
} postrec;


/* Email Data */
typedef struct {
	char		title[MAX_PATH_LEN];
	unsigned char	anony,
			status;
	unsigned short	fromsys,fromuser,
			tosys,touser;
	unsigned long	daten;
	messagerec	msg;
} mailrec;


/* Short Message Data */
typedef struct {
	char		message[MAX_PATH_LEN];
	unsigned short	tosys,touser;
} shortmsgrec;


/* Voting Responses */
typedef struct {
	char		response[MAX_PATH_LEN];
	unsigned short	numresponses;
} voting_response;


/* VOTING DATA INFORMATION */
typedef struct {
	char		question[MAX_PATH_LEN];
	unsigned char	numanswers;
	voting_response	responses[20];
} votingrec;

/* Directory Entry Record */
typedef struct {
    char            filename[13],
                    description[39],
                    date[9],
                    upby[46];
    unsigned char   filetype;
    unsigned short  numdloads,
                    ownersys,ownerusr;
int                    points,
                    mask;
    int             ats[5];
    unsigned long   daten,
                    numbytes;
}  uploadsrec;


/* History Log */
typedef struct {
	char		date[9];
	unsigned short	active,
			calls,
			posts,
			email,
			fback,
                        up,
                        dl;
} zlogrec;


/* Message Base Record */
typedef struct {
            char        name[41],
                        filename[9],
                        nmpath[51],
                        conf;
        unsigned char   readacs[21],
                        postacs[21],
                        anony,
                        age;
        unsigned long   attr;
        unsigned short  maxmsgs,
                        ar,
                        storage_type;
        addressrec      add;
        char            origin;
} subboardrec;

/* Structure to pass msghdr information between functions */
typedef struct {
    char subject[MAX_PATH_LEN],
    who_from[41],
    who_to[41],
    comment[41];
    UINT32 attr;
    UINT32 ReplyTo,Reply1st,ReplyNext,date;
    addressrec f;
    addressrec t;
    char msgid[41],replyid[41];
} hdrinfo;

typedef struct {
    int un,
        sy;
} mmailrec;

#include "nuv.h"

#endif /* _VARDEC_MSGFILE_H_ */
