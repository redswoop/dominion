/*
**  lastcall.h (FrontDoor)
**
**  Copyright 1991 Joaquim H. Homrighausen. All rights reserved.
**
**  LASTCALL.FD definitions for FrontDoor 2.00+
**
**  Last revision:  91-10-02
**
**  -------------------------------------------------------------------------
**  This information is not necessarily final and is subject to change at any
**  given time without further notice
**  -------------------------------------------------------------------------
*/



/*
**  Recent activity (statistics) definitions.
*/
typedef struct
    {
    unsigned int inbound,                                  /* Inbound mail calls */
            outbound,                                /* Outbound mail calls */
            humans,                                  /* Inbound BBS callers */
            filesin,                                       /* Inbound files */
            filesout,                                     /* Outbound files */
            goodsess,                                      /* Good sessions */
            badsess,                                     /* Failed sessions */
            requests;                              /* Inbound file requests */
   long   date,                                   /* UNIX-style timestamp */
            bytesin,                                /* Inbound (rcvd) bytes */
            bytesout;                              /* Outbound (sent) bytes */
    }
    STATREC, *STATRECPTR;

/*
**  Note that the two char[] fields are in Pascal fashion. The first byte is
**  the length byte. The actual string starts at [1] and the string is NOT
**  NUL terminated. In pascal they would be string[30] and string[40].
*/
typedef struct
    {
    char    system_name[31],
            location[41];
    unsigned int    zone,net,node,point;
    long   time;                                   /* UNIX-style timestamp */
    }
    LASTCALL, *LASTCALLPTR;

/*
**  The LASTCALL.FD file contains four records, two STATREC and two
**  LASTCALL. Today's activity is moved to Yesterday's activity as soon
**  as FD runs its 'past-midnight' internal event.
*/
typedef struct
    {
    LASTCALL    lastin,                           /* Last inbound mail call */
                lastout;                         /* Last outbound mail call */
    STATREC     today_act,                              /* Today's activity */
                yesterday_act;                      /* Yesterday's activity */
    }
    ACTIVITY, *ACTIVITYPTR;

/* end of file "lastcall.h" */
