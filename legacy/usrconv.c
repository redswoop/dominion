#include <fcntl.h>
#include <sys/stat.h>

#define MAX_SUBS 64
#define MAX_DIRS 64


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
} nuserrec;

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
} userrec;


userrec u;
nuserrec n;


void cdata()
{
	int i;

    strcpy(n.name,u.name);
    strcpy(n.realname,u.realname);
    strcpy(n.phone,u.phone);
    strcpy(n.pw,u.pw);
    strcpy(n.laston,u.laston);
    strcpy(n.firston,u.firston);
    strcpy(n.note,u.note);
    strcpy(n.comment,u.comment);
    strcpy(n.street,u.street);
    strcpy(n.city,u.city);
    strcpy(n.colors,u.colors);

    for(i=0;i<4;i++)
       strcpy(n.macros[i],u.macros[i]);

    n.sex        =u.sex;
    n.age        =u.age;
    n.inact      =u.inact;
    n.comp_type  =u.comp_type;
    n.defprot    =u.defprot;
    n.defed      =u.defed;
    n.screenchars=u.screenchars;
    n.screenlines=u.screenlines;
    n.sl         =u.sl;
    n.dsl        =u.dsl;
    n.exempt     =u.exempt;
    n.illegal    =u.illegal;
    n.waiting    =u.waiting;
    n.subop      =u.sysopsub;
    n.ontoday    =u.ontoday;
    n.forwardusr =u.forwardusr;
    n.msgpost    =u.msgpost;
    n.emailsent  =u.emailsent;
    n.feedbacksent=u.feedbacksent;
    n.posttoday  =u.posttoday;
    n.etoday     =u.etoday;
    n.ar         =u.ar;
    n.dar        =u.dar;
    n.restrict   =u.restrict;
    n.fpts       =u.fpts;
    n.uploaded   =u.uploaded;
    n.downloaded =u.downloaded;
    n.lastrate   =u.lastrate;
    n.logons     =u.logons;
    n.msgread    =u.msgread;
    n.uk         =u.uk;
    n.dk         =u.dk;
    n.daten      =u.daten;
    n.sysstatus  =u.sysstatus;
    n.timeontoday=u.timeontoday;
    n.extratime  =u.extratime;
    n.timeon     =u.timeon;
    n.month      =u.month;
    n.day        =u.day;
    n.year       =u.year;
    n.emailnet   =u.emailnet;
    n.postnet    =u.postnet;
    n.fsenttoday1=u.fsenttoday1;
    n.timebank   =u.timebank;

	for(i=0;i<200;i++)
        n.qscn[i]=0;

	for(i=0;i<200;i++)
        n.nscn[i]=0;

    n.flisttype=n.mlisttype=n.ratio=n.pcr=0;
    n.helplevel=u.res[3];
    n.lastsub=n.lastdir=0;
    n.lastconf=1;
    strcpy(n.dphone,"");

    memset(n.res,0,29);
    memset(n.resl,0,29);
    memset(n.resi,0,29);
    memset(n.resf,0,29);
}

void main(void)
{
    int i,f,g,num;

    f=open("uuser.dat",O_BINARY|O_RDWR);
    g=open("Newuser.dat",O_BINARY|O_RDWR|O_CREAT,S_IREAD|S_IWRITE);

    num=filelength(f)/sizeof(u);

    printf("num=%d",num);
    for(i=0;i<num;i++) {
        read(f,&u,sizeof(u));
        cdata();
        write(g,&n,sizeof(n));
    }

    close(f);
    close(g);
}
