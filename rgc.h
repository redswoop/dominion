typedef unsigned word;
typedef unsigned char byte;
typedef unsigned char boolean;
typedef unsigned int bit;


typedef byte acrq[4];           /*  AR flags                            */

typedef struct {                /*  Access condition flags              */
    bit rlogon      : 1;        /*  L - Restricted to one call a day    */
    bit rchat       : 1;        /*  C - Can't page the SysOp            */
    bit rvalidate   : 1;        /*  V - Posts marked unvalidated        */
    bit rbackspace  : 1;        /*  B - Can't do ^B/^N/etc in messages  */
    bit ramsg       : 1;        /*  A - Can't change the AutoMessage    */
    bit rpostan     : 1;        /*  * - Can't post anonymously          */
    bit rpost       : 1;        /*  P - Can't post at all               */
    bit remail      : 1;        /*  E - Can't send any e-mail           */
    bit rvoting     : 1;        /*  K - Can't vote                      */
    bit rmsg        : 1;        /*  M - Force e-mail deletion           */
    bit spsr        : 1;        /*  RESERVED                            */
    bit onekey      : 1;        /*  onekey input mode                   */
    bit avatar      : 1;        /*  user has AVATAR                     */
    bit pause       : 1;        /*  pause                               */
    bit novice      : 1;        /*  user is at novice help level        */
    bit ansi        : 1;        /*  user has ANSI                       */
    bit color       : 1;        /*  user has color                      */
    bit alert       : 1;        /*  alert SysOp when user logs on       */
    bit smw         : 1;        /*  short-message waiting for user      */
    bit nomail      : 1;        /*  user mail-box is closed             */
    bit fnodlratio  : 1;        /*  1 - No UL/DL ratio                  */
    bit fnopostratio: 1;        /*  2 - No post/call ratio              */
    bit fnofilepts  : 1;        /*  3 - No file points checking         */
    bit fnodeletion : 1;        /*  4 - Protection from deletion        */
} user_type;

typedef struct {
    bit lockedout   : 1;        /*  locked out                          */
    bit udeleted    : 1;        /*  deleted                             */
    bit trapactivity: 1;        /*  trapping users activity             */
    bit trapseperate: 1;        /*  trap to seperate TRAP file          */
    bit chatauto    : 1;        /*  auto chat trapping                  */
    bit chatseperate: 1;        /*  seperate chat log                   */
    bit slogseperate: 1;        /*  seperate log file                   */
    bit clsmsg      : 1;        /*  clear-screen before messages        */
    bit avadjust    : 1;        /*  avatar color adjust                 */
    bit fseditor    : 1;        /*  Full Screen Editor                  */
} user_flags;

typedef struct {                /*  anonymous message types             */
    bit atno        : 1;        /*  No anonymous posts allowed          */
    bit atyes       : 1;        /*  Anonymous posts are allowed         */
    bit atforced    : 1;        /*  ALL posts are forced anonymous      */
    bit atdearabby  : 1;        /*  "Dear Abby" message base            */
    bit atanyname   : 1;        /*  Users can post any name they want   */
} anon_type;


typedef byte mzscanr[32];/*  message base scan flags             */
typedef byte fzscanr[32];/* file base scan flags                */

typedef byte clrs[2][10];       /*  color records                       */
#define MAXBOARDS 250
typedef char mhireadr[MAXBOARDS][6];/* message last scan date           */

typedef struct {                /*  USER.DAT : User account records     */
    char name[37];              /*  user name                           */
    char realname[37];          /*  real name                           */
    char pw[21];                /*  user password                       */
    char ph[13];                /*  user phone #                        */
    char bday[9];               /*  user birthdate                      */
    char firston[9];            /*  firston date                        */
    char laston[9];             /*  laston date                         */
    char street[31];            /*  mailing address                     */
    char citystate[31];         /*  city, state                         */
    char zipcode[11];           /*  zipcode                             */
    char computer[36];          /*  type of computer                    */
    char occupation[36];        /*  occupation                          */
    char wherebbs[36];          /*  BBS reference                       */
    char note[36];              /*  SysOp note                          */
    char userstartmenu[9];	/*  Menu to start user out at		*/
    char lockedfile[9];         /*  if locked out                       */
    user_type ac;               /*  user flags                          */
    user_flags flags;           /*  User flags                          */
    acrq ar;                    /*  AR flags                            */
    byte vote[25];              /*  voting data                         */
    char sex;                   /*  user sex                            */
    long ttimeon;               /*  total mins spent on                 */
    long uk;                    /*  UL k                                */
    long dk;                    /*  DL k                                */
    int tltoday;                /*  number of minutes left today        */
    int forusr;           	/*  Forward mail to user #		*/
    int filepoints;		/*  number of filepoints		*/
    word uploads;               /*  number of uploads                   */
    word downloads;             /*  number of downloads                 */
    word loggedon;              /*  number of times logged on           */
    word msgpost;               /*  number of public posts              */
    word emailsent;             /*  number of private posts             */
    word feedback;              /*  number of feedback posts            */
    word timebank;              /*  # mins in Time Bank                 */
    word timebankadd;           /*  time added to timebank TODAY        */
    word dlktoday;		/*  # of k downloaded today		*/
    word dltoday;		/*  # of files downloaded today		*/
    byte waiting;               /*  mail waiting                        */
    byte linelen;               /*  number of columns                   */
    byte pagelen;               /*  number of rows                      */
    byte ontoday;               /*  number times on today               */
    byte illegal;               /*  number of illegal login attempts    */
    byte flistopt;              /*  type of file list type to use       */
    byte lastmbase;             /*  last message base                   */
    byte lastfbase;             /*  last file base                      */
    byte sl;                    /*  SL                                  */
    byte dsl;                   /*  DSL                                 */
    mhireadr mhiread;           /*  NewScan high message pointers       */
    mzscanr mzscan;             /*  NewScan message bases               */
    fzscanr fzscan;             /*  NewScan file bases                  */
    clrs cols;                  /*  user colors                         */
    
    byte garbage;
    word timebankwith;                /* amount of time withdrawn today*/
    word passwordchanged;             /* last day password changed */
    byte defarctype;                  /* default QWK archive type */
    char lastconf;                    /* last conference they were in */
    long lastqwkt;                  /* date/time of last qwk packet */
    boolean getownqwk,                        /* add own messages to qwk packet? */
    scanfilesqwk,                     /* scan file bases for qwk packets? */
    privateqwk;               /* get private mail in qwk packets? */

    long credit,                           /* Amount of credit a user has */
    debit;                    /* Amount of debit a user has */
    long expiration;               /* Expiration date of this user */
    char expireto;                    /* Subscription level to expire to */
    byte ColorScheme;                 /* User's color scheme # */
    boolean TeleConfEcho,                     /* echo Teleconf lines? */
    TeleConfInt;              /* interrupt during typing? */
} user_data;


