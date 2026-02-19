#define QWKIMPORT
#define QWKEXPORT
#define USEFLOATS

typedef struct {
    char status;
    char msgnum[7];
    char date[8];
    char time[5];
    char to[25];
    char from[25];
    char subj[25];
    char pword[12];
    char repnum[8];
    char numchunks[6];
    char live;
    int  confnum;
    char junk[3];
} QWKHDRREADER;

typedef struct {
    char   status;
    long   msgnum;
    time_t date;
    char   to[26];
    char   from[26];
    char   subj[26];
    char   pword[13];
    long   repnum;
    long   numchunks;
    char   live;
    int    confnum;
} QWKHDR;

#ifdef USEFLOATS
 typedef struct {
     float recnum;
     char  junk;
 } QWKIDX;
#endif

typedef struct __qwkarea__ {
    char               *name;
    int                confnum;
    struct __qwkarea__ *next;
    struct __qwkarea__ *prev;
} QWKAREAS;

#define QWK_ISALIVE(a)  ((a) == 0xe1)
#define QWKBLKSIZE      128
#define QWKPUBUNREAD    ' '
#define QWKPUBREAD      '-'
#define QWKPRIVATE      '*'
#define QWKSYSUNREAD    '~'
#define QWKSYSREAD      '`'
#define QWKPROTUNREAD   '%'
#define QWKPROTREAD     '^'
#define QWKGRPUNREAD    '!'
#define QWKGRPREAD      '#'
#define QWKPROTALL      '$'

QWKAREAS * qwkarearead    (FILE *fp,long *numareas);
void       qwkfreeareas   (QWKAREAS *head);
#ifdef QWKIMPORT
 size_t     qwkreadhdr     (FILE *fp,QWKHDR *hdr,int rep);
 size_t     qwkreadblk     (FILE *fp,char *blk);
 size_t     qwkreadblks    (FILE *fp,char *blk,size_t c);
 #ifdef USEFLOATS
 #endif
#endif

#ifdef QWKEXPORT
 size_t     qwkwritehdr    (FILE *fp,QWKHDR *hdr,int rep);
 size_t     qwkwriteblk    (FILE *fp,char *blk);
 size_t     qwkwriteblks   (FILE *fp,char *blk);
 #ifdef USEFLOATS
 #endif
#endif

#ifdef USEFLOATS
 float      MSBINToIEEE    (float f);
 float      IEEEToMSBIN    (float f);
#endif
