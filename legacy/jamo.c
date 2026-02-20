#include "platform.h"
#include "jammb.h"
#include "jamutil.h"

JAMAPIREC JamRec,Jr;

int InitJAMAPIREC( JAMAPIREC * pJam, CHAR8 * pFile );
int DisplayMsgHdr( INT32 MsgNo );
int DisplayMsgSubFld( void );
int DisplayMsgTxt( void );
int DisplayHdrInfo( void );
CHAR8 * AttrToStr( UINT32 Attr );
CHAR8 * GetSubFldName( JAMSUBFIELD * pSubFld );
CHAR8 * GetSubFldStr( JAMSUBFIELD * pSubFld );
void readmailj(int msgnum);
CHAR8 * DispTime( UINT32 * pt );

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

int JamMsgWrite( JAMAPIREC * pJam, CHAR8 * pMsgTxt )
{
    int     LockTryCnt = 0;
    UINT32  MsgNo;

    while( !JAMmbLockMsgBase( pJam, 1 )) {
        if( ++LockTryCnt >= 15 )
        {
            puts( "Unable to get lock on messagebase" );
            exit( 3 );
        }

        sleep( 1 );
    }

    MsgNo = ( filelength( pJam->IdxHandle ) / sizeof( JAMIDXREC )) + pJam->HdrInfo.BaseMsgNum;
    pJam->Hdr.MsgNum = MsgNo;


    pJam->Idx.HdrOffset = filelength( pJam->HdrHandle );

    pJam->Hdr.TxtOffset = filelength( pJam->TxtHandle );

    if( !JAMmbStoreMsgIdx( pJam, MsgNo )) {
        printf( "Error writing JAMIDXREC: %d\n", pJam->APImsg );
        exit( 4 );
    }

    if( !JAMmbStoreMsgHdr( pJam, MsgNo )) {
        printf( "Error writing JAMHDR: %d\n", pJam->APImsg );
        exit( 4 );
    }


    if( JAMsysWrite( NULL, pJam->HdrHandle, pJam->WorkBuf, ( int ) pJam->Hdr.SubfieldLen ) != ( int ) pJam->Hdr.SubfieldLen ) {
        printf( "Error writing SubFields\n" );
        exit( 4 );
    }

    if( !JAMmbStoreMsgTxtBuf( pJam, pMsgTxt, pJam->Hdr.TxtLen, 1 )) {
        printf( "Error writing message text: %d\n", pJam->APImsg );
        exit( 4 );
    }

    pJam->HdrInfo.ActiveMsgs++;
    JAMmbUnLockMsgBase( pJam, 1 );

    return( 1 );
}

int JamMsgDeinit( JAMAPIREC * pJam )
{
    memset( &pJam->Hdr, '\0', sizeof( JAMHDR ));

    return( 1 );
}

void main(int ac, char **ar)
{
    char fn[81];
    int numtokeep=500,i;
    UINT32  MsgNo;
    char *b;

    perror("for fun");

    JAMOpen("test",&JamRec);
    printf("opened?\n");
    nummsgs=filelength( JamRec.IdxHandle )/sizeof(JAMIDXREC);
    printf("%d messages\n",nummsgs);
    JAMOpen("temp",&Jr);
    printf("opened?\n");

    i=nummsgs-numtokeep;
    if(i<1)
        i=1;

    JAMmbLockMsgBase( &Jr, 1 );
    for(;i<nummsgs;i++) {
        printf("doing %d\n",i);
        if(!JAMmbFetchMsgHdr(&JamRec,i,0)) {
            printf("Error fetching header: %d\n",JamRec.APImsg);
        }
        printf("fetched hdr\n");
        if(!JAMmbFetchMsgTxt(&JamRec,1)) {
            printf("Error fetching text: %d\n",JamRec.APImsg);
        }
        printf("fetched txt,sending to JamMsgWrite\n");
        Jr.Hdr.TxtLen=JamRec.Hdr.TxtLen;

        MsgNo = ( filelength( Jr.IdxHandle ) / sizeof( JAMIDXREC )) + Jr.HdrInfo.BaseMsgNum;
        Jr.Hdr.MsgNum = MsgNo;

        Jr.Idx.HdrOffset = filelength( Jr.HdrHandle );
        Jr.Hdr.TxtOffset = filelength( Jr.TxtHandle );

        if( !JAMmbStoreMsgIdx( &Jr, MsgNo )) {
            printf( "Error writing JAMIDXREC: %d\n", Jr.APImsg );
            perror("Storemsgidx");
        }

        if( !JAMmbStoreMsgHdr( &Jr, MsgNo )) {
            printf( "Error writing JAMHDR: %d\n", Jr.APImsg );
            perror("Storemsghdr");
        }

/*        if( JAMsysWrite( NULL, Jr.HdrHandle,Jr.WorkBuf, ( int ) Jr.Hdr.SubfieldLen ) != ( int ) Jr.Hdr.SubfieldLen ) {
            printf( "Error writing SubFields\n" );
            exit( 4 );
        }*/

        Jr.WorkBuf=JamRec.WorkBuf;
/*        if( !JAMmbStoreMsgTxt( &Jr)) {
            printf( "Error writing message text: %d\n", Jr.APImsg );
            perror("storemsgtxt");
        }*/
        write(Jr.TxtHandle,JamRec.WorkBuf,JamRec.Hdr.TxtLen);

        Jr.HdrInfo.ActiveMsgs++;

//        farfree(b);
     /*   JamMsgDeinit(&JamRec);
        JamMsgDeinit(&Jr);*/
    }

    JAMmbUnLockMsgBase( &Jr, 1 );
    JAMsysDeinitApiRec(&JamRec);
    JAMsysDeinitApiRec(&Jr);
}
