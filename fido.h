typedef struct {
        int zone,net,node,point;
} addressrec;

#define fdr_connect 0x0001
#define fdr_mail    0x0002
#define fdr_local   0x0004
#define fdr_cmdtype 0x0008

typedef struct {
        char  desc[51],                 // Description of ErrorLevel
              fn[81];                   // Filename to run on mail
 unsigned int speed;                    // Modem Speed returned
        int   level,                    // Actual Error Level
              attr;                     // Action Type
} fdrrec;

typedef struct {
    char mailer[81],
         fdpath[81];
    int nlev,retlev;
    unsigned long attr;
} fnetrec;

//FIDO .msg Format
typedef struct {
        char from[36],
             to[36],
             title[72],
             date[20];
             int
             timesread,
             destnode,
             orignode,
             cost,
             orignet,
             destnet;
             char
             res[8];
             int
             replyto,
             attrib,
             nextreply;
} fmsgrec;

typedef struct {
        char origin[81];
        char tear[81];
        char netname[41];
        addressrec add;
} originrec;

#ifdef DFIDO

#else
extern fnetrec fnet;
#endif
