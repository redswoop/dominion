#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#include "jammb.h"
#include "jamutil.h"

JAMAPIREC JamRec,Jr;

void JAMOpen(char *fn,JAMAPIREC *j)
{

    if( !JAMsysInitApiRec( j, fn, WORKBUFSIZE ))
    {
        puts( "Not enough memory" );
        return;
    }

    if( !JAMmbOpen( j ))
    {
        if( j->Errno != ENOENT )
        {
            perror( "Unable to open the JAM messagebase" );
            JAMsysDeinitApiRec( j );
            return;
        }

        if( !JAMmbCreate( j ))
        {
            perror( "Unable to create the JAM messagebase" );
            JAMsysDeinitApiRec( j );
            return;
        }
    }

}

int nummsgs;

int JamMsgInit( JAMAPIREC * pJam )
{
    memset( &pJam->Idx, '\0', sizeof( JAMIDXREC ));
    pJam->Idx.UserCRC = ( UINT32 ) -1L;

    memset( &pJam->Hdr, '\0', sizeof( JAMHDR ));

    pJam->Hdr.MsgIdCRC    = ( UINT32 ) -1L;
    pJam->Hdr.ReplyCRC    = ( UINT32 ) -1L;
    pJam->Hdr.PasswordCRC = ( UINT32 ) -1L;

    return( 1 );
}

int JamMsgDeinit( JAMAPIREC * pJam )
{
    memset( &pJam->Hdr, '\0', sizeof( JAMHDR ));

    return( 1 );
}

void main(int ac, char **ar)
{
    char basefn[81],s[81];
    int numtokeep=500,i;
    UINT32  MsgNo,WhatOffset,TxtLen,numu;
    char *b,*subfld;


    strcpy(basefn,ar[1]);
    if(ac>2)
        numtokeep=atoi(ar[2]);

    JAMOpen(basefn,&JamRec);
    nummsgs=filelength( JamRec.IdxHandle )/sizeof(JAMIDXREC);
    printf("%d messages\n",nummsgs);
    JAMOpen("temp",&Jr);

    if(nummsgs<numtokeep)
        return;

    i=nummsgs-numtokeep;
    if(i<1)
        i=1;

    for(;i<nummsgs;i++) {
        if(!JAMmbFetchMsgHdr(&JamRec,i,0)) {
            printf("Error fetching header: %d\n",JamRec.APImsg);
            perror("Hdr Fetch");
        }

        if(JamRec.Hdr.Attribute & MSG_DELETED)
            continue;

        TxtLen=JamRec.Hdr.TxtLen;
        b=malloc(TxtLen);
        lseek(JamRec.TxtHandle,JamRec.Hdr.TxtOffset,0);
        read(JamRec.TxtHandle,b,TxtLen);

        subfld=malloc(JamRec.Hdr.SubfieldLen);
        lseek(JamRec.HdrHandle,JamRec.Idx.HdrOffset+sizeof(JamRec.Hdr),0);
        read(JamRec.HdrHandle,subfld,JamRec.Hdr.SubfieldLen);

        JamMsgInit(&Jr);

        Jr.Hdr=JamRec.Hdr;

        MsgNo = ( filelength( Jr.IdxHandle ) / sizeof( JAMIDXREC ));

        Jr.Idx.HdrOffset = tell(Jr.HdrHandle);

        WhatOffset=(INT32)((MsgNo) * (INT32)sizeof(JAMIDXREC));
        lseek(Jr.IdxHandle,WhatOffset,0);
        write(Jr.IdxHandle,&Jr.Idx,sizeof(Jr.Idx));
        Jr.Hdr.TxtOffset=tell(Jr.TxtHandle);
        Jr.Hdr.TxtLen=TxtLen;
        write(Jr.TxtHandle,b,TxtLen);
        farfree(b);

        Jr.Hdr.MsgNum = MsgNo;
        strcpy(Jr.Hdr.Signature, HEADERSIGNATURE);
        Jr.Hdr.Revision=CURRENTREVLEV;
        Jr.Hdr.SubfieldLen=JAMsysAlign(Jr.Hdr.SubfieldLen);

        lseek(Jr.HdrHandle,Jr.Idx.HdrOffset,0);
        write(Jr.HdrHandle,&Jr.Hdr,sizeof(Jr.Hdr));
        write(Jr.HdrHandle,subfld,Jr.Hdr.SubfieldLen);
        farfree(subfld);

        Jr.HdrInfo.ActiveMsgs++;
    }

    close(Jr.HdrHandle);
    close(Jr.TxtHandle);
    close(Jr.IdxHandle);

    numu=filelength(JamRec.LrdHandle);
    lseek(JamRec.LrdHandle,0,0);
    lseek(Jr.LrdHandle,0,0);
    for(i=0;i<numu;i++) {
        read(JamRec.LrdHandle,&JamRec.LastRead,sizeof(JamRec.LastRead));
        JamRec.LastRead.LastReadMsg-=(nummsgs-numtokeep);
        write(Jr.LrdHandle,&JamRec.LastRead,sizeof(JamRec.LastRead));
    }    

    close(Jr.LrdHandle);

    JAMsysDeinitApiRec(&JamRec);
    JAMsysDeinitApiRec(&Jr);

    sprintf(s,"%s.jdt",basefn);
    unlink(s);
    rename("temp.jdt",s);

    sprintf(s,"%s.jdx",basefn);
    unlink(s);
    rename("temp.jdx",s);

    sprintf(s,"%s.jhr",basefn);
    unlink(s);
    rename("temp.jhr",s);

    sprintf(s,"%s.jlr",basefn);
    unlink(s);
    rename("temp.jlr",s);
}
