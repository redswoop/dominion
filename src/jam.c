#include "vars.h"
#pragma hdrstop


#define WORKBUFSIZE       0x8000

#define hdr_private 1
#define hdr_netmail 2

int readms=0;

int InitJAMAPIREC( JAMAPIREC * pJam, CHAR8 * pFile );
int DisplayMsgHdr( INT32 MsgNo );
int DisplayMsgSubFld( void );
int DisplayMsgTxt( void );
int DisplayHdrInfo( void );
CHAR8 * AttrToStr( UINT32 Attr );
CHAR8 * GetSubFldName( JAMSUBFIELD * pSubFld );
CHAR8 * GetSubFldStr( JAMSUBFIELD * pSubFld );
void readmailj(int msgnum,int sb);
CHAR8 * DispTime( UINT32 * pt );

void JAMOpen(char *fn);
void JAMClose(void);
void postjam(int sb,hdrinfo *hdr1, int usehdr);
void replyj(int sb,int msgnum);
void addLastRead(int num);
void saveLastRead(void);
void post(int sb);
int findnextwaiting(int msgnum,int old,userrec *u);
int findnextthread(int msgnum);
int findlastthread(int msgnum);
void quote_jam(char *buf,long len,hdrinfo *hdr);

void errorjam(void)
{
    switch(JamRec.APImsg) {
    case JAMAPIMSG_ISNOTOPEN: 
        pl("Files not open"); 
        break;
    case JAMAPIMSG_ISNOTLOCKED: 
        pl("Files not locked"); 
        break;
    default: 
        npr("Error type %d\r\n",JamRec.APImsg); 
        break;
    }
    pausescr();
}

void getjamhdr(hdrinfo *hdr1)
{
    hdrinfo hdr;

    UINT32  SubFldLen = JamRec.Hdr.SubfieldLen,
    Len;

    memset(&hdr.subject[0],0L,sizeof(hdrinfo));

    if( !JAMmbFetchMsgHdr( &JamRec, JamRec.Hdr.MsgNum, 1 ))
    {
        printf( "Error reading message header, code: %d, errno: %d\n", JamRec.APImsg, JamRec.Errno );
        return;
    }


    if( SubFldLen > JamRec.WorkLen )
        SubFldLen = JamRec.WorkLen;


    for(  JamRec.SubFieldPtr = ( JAMSUBFIELD * ) JamRec.WorkBuf;

        JamRec.SubFieldPtr->DatLen + sizeof( JAMBINSUBFIELD ) <= SubFldLen;

        Len = JAMsysAlign( JamRec.SubFieldPtr->DatLen + sizeof( JAMBINSUBFIELD )),
        SubFldLen -= Len,
        JamRec.SubFieldPtr = JAMsysAddPtr( JamRec.SubFieldPtr, Len ))
        {
        switch(JamRec.SubFieldPtr->LoID) {
        case 2: 
            strcpy(hdr.who_from,GetSubFldStr(JamRec.SubFieldPtr));
            break;
        case 3: 
            strcpy(hdr.who_to,GetSubFldStr(JamRec.SubFieldPtr));
            break;
        case 4:
            strcpy(hdr.msgid,GetSubFldStr(JamRec.SubFieldPtr));
            break;
        case 5:
            strcpy(hdr.replyid,GetSubFldStr(JamRec.SubFieldPtr));
            break;
        case 6:
            strcpy(hdr.subject,GetSubFldStr(JamRec.SubFieldPtr));
            break;
        case 666:
            strcpy(hdr.comment,GetSubFldStr(JamRec.SubFieldPtr));
            break;
        }
    }

    hdr.attr=JamRec.Hdr.Attribute;
    hdr.date=JamRec.Hdr.DateWritten;

    *hdr1=hdr;
}

void show_message(int *next,int abort,char *buf,UINT32 len)
{
    unsigned char n[MAX_PATH_LEN],d[MAX_PATH_LEN],s[161],s1[MAX_PATH_LEN],s2[MAX_PATH_LEN],s3[MAX_PATH_LEN],ch;
    int f,done,end,cur,p,p1,p2,printit,ctrla,centre,i,i1,ansi,cabort=0;
    char a,a1,a2;
    long l1;

    ansi=0;
    *next=0;
    done=0;
    abort=0;

    p=0;
    p1=0;
    done=0;
    printit=0;
    ctrla=0;
    centre=0;
    l1=0;


    while ((!done) && (!abort) && (!hangup)) {
        ch=buf[l1];
        if (l1>=len)
            ch=26;

        if (ch==26) done=1;
        else if (ch!=10) {
            if ((ch==13) || (!ch) || ch==141) {
                printit=1;
            } 
            else if (ch==1)
                ctrla=1;
            else if (ch==2)
                centre=1;
            else {
                if(!ctrla) {
                    if (ch==27) {
                        if ((topline) && (screenbottom==24) && (!ansi))
                            set_protect(0);
                        ansi=1;
                        lines_listed=0;
                    }
                    s[p++]=ch;
                    if ((ch==3) || (ch==8))
                        --p1;
                    else
                        ++p1;
                    if ((ch==32) && (!centre))
                        printit=1;
                }
            }

            if ((printit) || (ansi) || (p>=80)) {
                printit=0;
                if (centre ) {
                    i1=(thisuser.screenchars-wherex()-p1)/2;
                    for (i=0; (i<i1) && (!abort) && (!hangup); i++)
                        osan(" ",&abort,next);
                }
                if (p) {
                    if ((wherex() + p1 >= thisuser.screenchars) && (!centre) && (!ansi))
                        nl();
                    s[p]=0;
                    outstr(s);
                    p1=0;
                    p=0;
                }
                centre=0;
            }

            cabort++;
            if(cabort>=50) {
                checka(&abort,next,1);
                cabort=0;
            }
            if (ch==13)
                if (ctrla==0) {
                    nl();
                } 
                else
                    ctrla=0;
        } 

        ++l1;
    }

    if ((!abort) && (p)) {
        s[p]=0;
        pl(s);
    }

    nl();
    if ((ansi) && (topdata) && (useron))
        topscreen();

    mciok=1;
}

void read_msg(long recnr,int *next)
{
    int abort=0;
    hdrinfo hdr;

    JAMmbFetchMsgHdr( &JamRec, recnr, 0 );
    getjamhdr(&hdr);
    JAMmbFetchMsgTxt(&JamRec,1);

    readms=1;
    showmsgheader(0,hdr.subject,hdr.who_from,ctime(&hdr.date),hdr.who_to,msgr,nummsgs,hdr.comment,0,&abort);
    if(!abort)
        show_message(next,abort,JamRec.WorkBuf,JamRec.Hdr.TxtLen);
    if(abort)
        *next=0;
    else
        *next=1;
    readms=0;
}



int DisplayMsgSubFld( void )
{
    UINT32  SubFldLen = JamRec.Hdr.SubfieldLen,
    Len;

    printf( "Message number: %lu\n\n", JamRec.Hdr.MsgNum );


    if( !JAMmbFetchMsgHdr( &JamRec, JamRec.Hdr.MsgNum, 1 ))
    {
        printf( "Error reading message header, code: %d, errno: %d\n", JamRec.APImsg, JamRec.Errno );
        return( 0 );
    }


    if( SubFldLen > JamRec.WorkLen )
        SubFldLen = JamRec.WorkLen;



    puts( " HiID LoID Name             Len Data\n"
        " ---- ---- ----             --- ----" );

    for(  JamRec.SubFieldPtr = ( JAMSUBFIELD * ) JamRec.WorkBuf;

        JamRec.SubFieldPtr->DatLen + sizeof( JAMBINSUBFIELD ) <= SubFldLen;

        Len = JAMsysAlign( JamRec.SubFieldPtr->DatLen + sizeof( JAMBINSUBFIELD )),
        SubFldLen -= Len,
        JamRec.SubFieldPtr = JAMsysAddPtr( JamRec.SubFieldPtr, Len ))
        {
        printf( "%5u%5u %6lu \"%s\"\n",
        JamRec.SubFieldPtr->HiID, JamRec.SubFieldPtr->LoID,
        //GetSubFldName( JamRec.SubFieldPtr ),
        JamRec.SubFieldPtr->DatLen,
        GetSubFldStr( JamRec.SubFieldPtr ));
    }

    return( 1 );
}




void scanj(int msgnum,int *nextsub,int sb, int private)
{
    long recnr,l,len;
    UINT32 ucrc,hcrc;
    int done=0,next=1,i,ok,abort=0,disp=1,board,f=0,titleorigin=0,fd=0;
    char s[161],*b,*ss1,s1[MAX_PATH_LEN],findtitle[MAX_PATH_LEN];
    hdrinfo hdr;

    findtitle[0]=0;
    sprintf(s,"%s%s",syscfg.msgsdir,subboards[usub[sb].subnum].filename);

    JAMOpen(s);

    if(!JAMmbFetchLastRead(&JamRec,usernum)) {
        if(JamRec.APImsg==JAMAPIMSG_CANTFINDUSER) {
            addLastRead(usernum);
            if(!JAMmbFetchLastRead(&JamRec,usernum))
                errorjam();
        }
    }

    curlsub=usub[sb].subnum;

    strcpy(s,thisuser.realname);
    strlwr(s);
    ucrc=JAMsysCrc32( s,strlen(s), ( UINT32 ) -1L );

    strcpy(s,pnam(&thisuser));
    strlwr(s);
    hcrc=JAMsysCrc32( s,strlen(s), ( UINT32 ) -1L );

    while(!done&&!hangup) {
        if(fd)
            disp=0;

        if(msgnum>=nummsgs) {
            if(titleorigin) {
                findtitle[0]=0;
                titleorigin=0;
                npr("5Return to point before search? ");
                if(ny())
                    msgnum=titleorigin;
            }

            if(msgnum>nummsgs) {
                done=1;
                continue;
            }
        }

        msgr=msgnum;

        if(private&&disp) {
            f=0;
            JAMmbFetchMsgIdx(&JamRec,msgnum);
            if(ucrc==JamRec.Idx.UserCRC||hcrc==JamRec.Idx.UserCRC)
                f=1;
            if(!f)
                disp=0;
        }

        if(findtitle[0]) {
            if(disp) {
                JAMmbFetchMsgHdr(&JamRec, msgnum,0);
                getjamhdr(&hdr);
                if(stristr(hdr.subject,findtitle)==NULL)
                    disp=0;
                else
                    disp=1;
            }
        }

        if(disp) {
            if(subboards[curlsub].attr & mattr_nomci)
                mciok=0;
            JAMmbFetchMsgHdr(&JamRec,msgnum,0);

            if(JamRec.Hdr.Attribute & MSG_DELETED)
                pl("7- 0Message Deleted");
            else
                read_msg(msgnum,&next);

            if(private) {
                JAMmbLockMsgBase(&JamRec,1);
                JamRec.Hdr.Attribute |= MSG_READ;
                JAMmbStoreMsgHdr(&JamRec,msgnum);
                JAMmbUnLockMsgBase(&JamRec,1);
            }

            JamRec.LastRead.UserID=usernum;
            JamRec.LastRead.LastReadMsg=msgnum;
            if(JamRec.LastRead.LastReadMsg>JamRec.LastRead.HighReadMsg)
                JamRec.LastRead.HighReadMsg=msgnum;

            saveLastRead();
            disp=0;
        } 
        else if(findtitle[0]) {
            msgnum++;
            disp=1;
            continue;
        }


        nl();
        JAMmbFetchMsgHdr(&JamRec, msgnum,0);
        getjamhdr(&hdr);
        strcpy(s,get_string(14));
        outstr(s);
        strcpy(s,"Q[]FRP!VALFB?-CDH@$Z");
        if(cs())
            strcat(s,"UKXBME");
        if(msgnum<nummsgs)
            strcat(s,"T");
        ss1=smkey(s,1,1,1,0);
        strcpy(s,ss1);

        if(!s[0]) {
            if(!private)
                msgnum++;
            else
                msgnum=findnextwaiting(msgnum,1,&thisuser);
            if(!msgnum)
                done=1;
            disp=1;
        } 
        else if(atoi(s)) {
            i=atoi(s);
            if(i<=nummsgs) {
                msgnum=i;
                disp=1;
            }
        } 
        else if(s[0]==']') {
            i=findnextthread(msgnum);
            if(i) {
                disp=1;
                msgnum=i;
            } 
            else
                pl("No more messages found in this thread.");
        }
        else if(s[0]=='!') {
            if(!(subboards[sb].attr & mattr_private)||cs())
                private=opp(private);
        }
        else if(s[0]=='Q') {
            done=1;
            *nextsub=0;
        }
        else if(s[0]=='Z') {
            i=inscan(sb,&thisuser);
            if(i) {
                pl("Removing `B from Newscan");
                togglenws(sb,&thisuser,0);
            } else {
                pl("Adding `B to Newscan");
                togglenws(sb,&thisuser,1);
            }
        } 
        else if(s[0]=='@') {
            DisplayMsgSubFld();
        } 
        else if(s[0]=='$') {
            fd=opp(fd);
        }
        else if(s[0]=='P'||s[0]=='R') {
            if(s[0]=='R') {
                f=1;
                if(private) {
                    JAMmbFetchMsgIdx(&JamRec,msgnum);
                        if(ucrc!=JamRec.Idx.UserCRC&&hcrc!=JamRec.Idx.UserCRC)
                            f=0;
                }
                if(f)
                    replyj(sb,msgnum);
            }
            else
                post(sb);
            if(!JamRec.isOpen) {
                sprintf(s,"%s%s",syscfg.msgsdir,subboards[usub[sb].subnum].filename);
                JAMOpen(s);
            }
        }
        else if(s[0]=='X') {
            JAMmbFetchMsgTxt(&JamRec,0);
            getjamhdr(&hdr);
            extract_out(JamRec.WorkBuf,JamRec.Hdr.TxtLen,&hdr);
        } 
        else if(s[0]=='E') {
            JAMmbFetchMsgHdr(&JamRec,msgnum,0);
            editpost(&JamRec.Hdr.Attribute);

            JAMmbLockMsgBase(&JamRec,1);
            JAMmbStoreMsgHdr(&JamRec,msgnum);
            JAMmbUnLockMsgBase(&JamRec,1);
        }
        else if(s[0]=='A')
            disp=1;
        else if(s[0]=='S') {
     /*       nl();
            done=1;
            npr("5Kill %s from Newscan? ",subboards[curlsub].name);
            if(yn())
                thisuser.qscn[curlsub]=-1;*/
            pl("This command is temporarily disabled cause glenn keeps crashing my board");
        }
        else if(s[0]=='K') {
            JAMmbFetchMsgHdr(&JamRec,msgnum,0);
            if(JamRec.Hdr.Attribute & MSG_DELETED)
                togglebit((long *)&JamRec.Hdr.Attribute,MSG_DELETED);
            else {
                nl();
                npr("5Kill this Message? ");
                if(yn())
                    togglebit((long *)&JamRec.Hdr.Attribute,MSG_DELETED);
            }
            JamRec.Hdr.Attribute |= MSG_DELETED;
            JAMmbLockMsgBase(&JamRec,1);
            JAMmbStoreMsgHdr(&JamRec,msgnum);
            JAMmbUnLockMsgBase(&JamRec,1);
        }
        else if(s[0]=='?')
            printmenu(1);
        else if(s[0]=='B')
            boardedit();
        else if(s[0]=='-') {
            if(msgnum>JamRec.HdrInfo.BaseMsgNum)
                msgnum--;
            disp=1;
        } 
        else if(s[0]=='L') {
            msgnum=JamRec.HdrInfo.BaseMsgNum+JamRec.HdrInfo.ActiveMsgs;
            msgnum--;
            disp=1;
        } 
        else if(s[0]=='F') {
            if(findtitle[0]) {
                pl("Title Scan Disabled");
                findtitle[0]=0;
                titleorigin=0;
            } 
            else {
                inputdat("Text to Search for in Titles",findtitle,51,0);
                if(findtitle[0])
                    titleorigin=msgnum;
            }
        }
        else if(s[0]=='T'||(s[0]=='/'&&s[1]=='T')) {
            abort=0;
            for(i=0;i<10&&!hangup&&!abort&&msgnum<nummsgs;i++) {
                msgnum++;
                JAMmbFetchMsgHdr(&JamRec, msgnum,0);
                getjamhdr(&hdr);

                if(!stricmp(thisuser.name,hdr.who_from)||!stricmp(thisuser.realname,hdr.who_from))
                    sprintf(s,"3[3%d3] ",msgnum);
                else if(!stricmp(thisuser.name,hdr.who_to)||!stricmp(thisuser.realname,hdr.who_to))
                    sprintf(s,"6[2%d6] ",msgnum);
                else
                    sprintf(s,"1(1%d1) ",msgnum);

                if(s[0]=='/'||private)
                    sprintf(s1,"0%-30.30s By: %-21.21s To: %s",hdr.subject,hdr.who_from,hdr.who_to);
                else
                    sprintf(s1,"0%-40.40s By: %-21.21s",hdr.subject,hdr.who_from);

                strcat(s,s1);
                pla(s,&abort);
            }
        }
        else if(!stricmp(s,"/C"))
            outchr(12);
        else if(!stricmp(s,"CHAT"))
            reqchat("Chat Reason");
        else if(!stricmp(s,"**"))
            getcmdtype();
    }

    if(!private) {
        nl();
        outstr(get_string(60));
        if(yn())
            post(sb);
    }

    JAMClose();
}


void rscanj(void)
{
    int i,board,next,new;
    char s[MAX_PATH_LEN];

    if(subboards[usub[cursub].subnum].attr & mattr_private) {
        readmailj(1,cursub);
        return;
    }

    sprintf(s,"%s%s",syscfg.msgsdir,subboards[usub[cursub].subnum].filename);
    JAMOpen(s);

    outstr(get_string2(9));
    input(s,4);
    if(s[0]=='Q') {
        JAMClose();
        return;
    }
    i=atoi(s);
    if(!i)
        i=1;
    if (i>=nummsgs)
        i=nummsgs;
    nl();
    JAMClose();
    scanj(i,&next,cursub,0);
}

#define LEN 161


char *ninmsg(hdrinfo *hdr1,long *len,int *save,int sb)
{
    char s[LEN],s1[LEN],s2[LEN],ro[MAX_PATH_LEN],fnx[MAX_PATH_LEN],chx,*ss,*ss1,*p;
    int maxli,curli,done,savel,i,i1,i2,i3,i4,i5,f,setanon,result,abort=0;
    messagerec m;
    long ll,l1;
    char *lin, *b;
    int real_name=0,fsed=0,anony=0;
    hdrinfo hdr;
    userrec u;
    addressrec add;
    originrec o;


    hdr= *hdr1;
    if ((fsed!=0) && (!okfsed()))
        fsed=0;
    sprintf(fnx,"%sMSGtmp",syscfg.tempdir);
    if (fsed)
        fsed=1;
    if (use_workspace) {
        if (!exist(fnx))
            use_workspace=0;
        else
            fsed=2;
    }
    done=0;
    setanon=0;
    *save=0;
    curli=0;
    b=NULL;

    if (actsl<45)
        maxli=30;
    else
        if (actsl<60)
        maxli=50;
    else
        if (actsl<80)
        maxli=60;
    else
        maxli=80;


    if (!fsed) {
        if ((lin=malloca((long)(maxli+10)*LEN))==NULL) {
            return NULL;
        }
        for (i=0; i<maxli; i++)
            lin[i*LEN]=0;
        ro[0]=0;
    }


    nl();

    if (!fsed) {
        nl();
        pl(get_string(11));
        pl(get_string(12));
        pl(get_string(41));

        while (!done&&!hangup&&!abort) {

            while((result=ainli(s,ro,160,1,1,curli?1:0))>0) {
                if(result==1||result==2&&curli) {
                    --curli;
                    if(result==2)
                        outstr("[A\r");
                    strcpy(ro,&(lin[(curli)*LEN]));
                    if(strlen(ro)>thisuser.screenchars-1)
                        ro[thisuser.screenchars-2]=0;
                }
            }

            if(result==-1) strcpy(s,"/S");
            if(result==-2) strcpy(s,"/A");
            if(result==-3) strcpy(s,"/L");
            if(result==-4) strcpy(s,"/C");
            if(result==-5) strcpy(s,"/Q");
            if(result==-6) strcpy(s,"/M");

            if (hangup) done=1;
            savel=1;
            if (s[0]=='/') {
                if (stricmp(s,"/Q")==0) {
                    savel=0;
                    if (quote!=NULL)
                        get_quote(0);
                }
                if (!stricmp(s,"/M")) {
                    savel=0;
                    printmenu(15);
                }
                if (stricmp(s,"/?")==0) {
                    savel=0;
                    printmenu(2);
                }
                if (stricmp(s,"/L")==0) {
                    savel=0;
                    i2=0;
                    for (i=0; (i<curli) && (!i2); i++) {
                        strcpy(s1,&(lin[i*LEN]));
                        i3=strlen(s1);
                        if (s1[i3-1]==1)
                            s1[i3-1]=0;
                        if (s1[0]==2) {
                            strcpy(s1,&(s1[1]));
                            i5=0;
                            for(i4=0; i4<strlen(s1); i4++)
                                if ((s1[i4]==8) || (s1[i4]==3))
                                    --i5;
                                else
                                    ++i5;
                            for (i4=0; (i4<(thisuser.screenchars-i5)/2) && (!i2); i4++)
                                osan(" ",&i2,&i1);
                        }
                        pla(s1,&i2);
                    }
                    nl();
                    pl("Continue...");
                }
                if (!stricmp(s,"/S")) {
                    *save=1;
                    done=1;
                    savel=0;
                }
                if (stricmp(s,"/A")==0) {
                    npr("5Abort this message? ");
                    if(yn()) {
                        done=1;
                        savel=0;
                        abort=1;
                        *save=0;
                    }
                }
                if (stricmp(s,"/C")==0) {
                    npr("5Clear message? ");
                    if(yn()) {
                        savel=0;
                        curli=0;
                        pl("Message cleared.");
                        nl();
                    }
                }
                strcpy(s1,s);
                s1[3]=0;
            }
            if (savel) {
                strcpy(&(lin[(curli++)*LEN]),s);
                if (curli==(maxli+1)) {
                    nl();
                    pl("No more lines... Please Save or back up.");
                    nl();
                    --curli;
                } 
                else if (curli==maxli) {
                    pl("þ Message limit reached, /S to save þ");
                } 
                else if ((curli+5)==maxli) {
                    pl("þ> 5 lines left <þ");
                }
            }
        }

        if (curli==0)
            *save=0;
    } 
    else {
        if (fsed==1) {
            *save=external_edit("MSGtmp",syscfg.tempdir,maxli);
        } 
        else {
            *save=exist(fnx);
            use_workspace=0;
        }
    }

    if(quote!=NULL)
        farfree(quote);
    quote=NULL;

    if(abort||!(*save)) {
        farfree(lin);
        if (fsed)
            unlink(fnx);
        pl("Aborted.");
        *save=0;
        return NULL;
    }

    anony=subboards[usub[sb].subnum].anony;

    switch(anony) {
    case 0:
        anony=0;
        break;
    case anony_enable_anony:
        if (setanon) {
            if (setanon==1)
                anony=anony_sender;
            else
                anony=0;
        }
        else {
            prt(5,"Anonymous? ");
            if (yn())
                anony=anony_sender;
            else
                anony=0;
        }
        break;
    case anony_force_anony:
        anony=anony_sender;
        break;
    case anony_real_name:
        real_name=1;
        anony=0;
        break;
    }
    outstr(get_string(61));
    if (fsed) {
        i5=open(fnx,O_RDONLY | O_BINARY);
        l1=filelength(i5);
    }
    else {
        l1=0;
        for (i5=0; i5<curli; i5++) {
            l1 += strlen(&(lin[i5*LEN]));
            l1 += 2;
        }
    }

    l1 += 1024;
    if ((b=malloca(l1))==NULL) {
        farfree(lin);
        pl("Out of memory.");
        return NULL;
    }

    l1=0;

    if (fsed) {
        ll=filelength(i5);
        read(i5, (void *) (& (b[l1]) ),ll);
        l1 += ll;
        close(i5);
    }
    else {
        for (i5=0; i5<curli; i5++)
            addline(b,&(lin[i5*LEN]),&l1);
    }


    if(real_name)
        strcpy(hdr.who_from,thisuser.realname);
    else if(anony)
        strcpy(hdr.who_from,"Anonymous");
    else
        strcpy(hdr.who_from,pnam(&thisuser));

    if(!anony)
        strcpy(hdr.comment,thisuser.comment);
    else
        strcpy(hdr.comment,"I am Ambiguous");

    *hdr1=hdr;
    *len=l1;

    if (!fsed)
        farfree((void *)lin);

    charbufferpointer=0;
    charbuffer[0]=0;

    return b;
}


#include <errno.h>

void replyj(int sb,int msgnum)
{
    hdrinfo hdr;
    int i;
    char *buf;
    long len;

    JAMmbFetchMsgHdr( &JamRec, msgnum, 0 );
    getjamhdr(&hdr);

    JAMmbFetchMsgTxt( &JamRec, 1 );
    quote_jam(JamRec.WorkBuf,JamRec.Hdr.TxtLen,&hdr);

    postjam(sb,&hdr,1);
    if(quote!=NULL)
        farfree(quote);
}

void post(int sb)
{
    hdrinfo hdr;

    memset(&hdr.subject[0],0,sizeof(hdrinfo));
    postjam(sb,&hdr,0);
}

int okpost(void)
{
    if (freek1(syscfg.msgsdir)<10.0) {
        nl();
        pl("Sorry, not enough disk space left.");
        nl();
        return 0;
    }

    if ((restrict_post & thisuser.restrict) || (thisuser.posttoday>=syscfg.sl[actsl].posts)) {
        nl();
        pl("Too many messages posted today.");
        nl();
        return 0;
    }

    if (!slok(subboards[curlsub].postacs,1)) {
        nl();
        pl("You can't post here.");
        nl();
        return 0;
    }

    if (subboards[curlsub].attr & mattr_fidonet) {
        if (thisuser.restrict & restrict_net) {
            nl();
            pl("You can't post on networked message areas.");
            nl();
            return 0;
        }
        nl();
        pl(get_string(23));
        nl();
    }

    return 1;
}

void SaveJamMsg(hdrinfo *hdr,long len, char *b,int sb)
{
    char s[MAX_PATH_LEN];
    UINT32 SubFldPos=0;

    sprintf(s,"%s%s",syscfg.msgsdir,subboards[usub[sb].subnum].filename);
    JAMOpen(s);


    JamMsgInit(&JamRec);

    JamMsgAddSFldStr(&JamRec, JAMSFLD_SUBJECT, hdr->subject, &SubFldPos );
    JamMsgAddSFldStr(&JamRec, JAMSFLD_SENDERNAME, hdr->who_from, &SubFldPos );
    JamMsgAddSFldStr(&JamRec, JAMSFLD_RECVRNAME, hdr->who_to, &SubFldPos );
    JamMsgAddSFldStr(&JamRec, 666, hdr->comment, &SubFldPos );

    if(subboards[curlsub].attr & mattr_netmail) {
        sprintf(s,"%d:%d/%d.%d",hdr->f.zone,hdr->f.net,hdr->f.node,hdr->f.point);
        JamMsgAddSFldStr(&JamRec, JAMSFLD_OADDRESS, s, &SubFldPos );

        sprintf(s,"%d:%d/%d.%d",hdr->t.zone,hdr->t.net,hdr->t.node,hdr->t.point);
        JamMsgAddSFldStr(&JamRec, JAMSFLD_DADDRESS, s, &SubFldPos );
    }

    JamRec.Hdr.SubfieldLen=SubFldPos;
    JamRec.Hdr.TxtLen=len;
    JamRec.Hdr.Attribute |= MSG_LOCAL;
    time((long *)&JamRec.Hdr.DateWritten);

    JamMsgWrite(&JamRec,b);
    JamMsgDeinit(&JamRec);

    JAMClose();
}

int get_receiver(hdrinfo *hdr)
{
    unsigned short i,i1;
    char s[MAX_PATH_LEN];
    userrec u;

    inputdat("Receiver",s,41,1);

    if((hdr->attr & hdr_private))
    {
        if(!s[0])
            return 0;

        if((hdr->attr & hdr_netmail)) {
            strcpy(hdr->who_to,s);
            nl();
            inputdat("Destination Address",s,21,0);
            if(!s[0])
                return 0;
            parseadd(s,&hdr->t);
        } 
        else {
            i=atoi(s);
            if(!i) {
                parse_email_info(s,&i);
                if(!i)
                    return 1;
            }
            userdb_load(i,&u);
            strcpy(hdr->who_to,pnam(&u));
        }
    } 
    else {
        if(!s[0])
            strcpy(hdr->who_to,"All");
        else
            strcpy(hdr->who_to,s);
    }

    return 1;
}



int inputhdr(hdrinfo *hdr,int usehdr,int sb)
{
    addressrec add;
    originrec o;

    char s[MAX_PATH_LEN];

    if(!hdr->subject[0]&&!usehdr) {
        inputdat("Subject",s,60,1);

        if (!s[0])
            return 0;
        else
            strcpy(hdr->subject,s);
    }

    nl();
    if(usehdr) {
        strcpy(hdr->who_to,hdr->who_from);

        hdr->t=hdr->f;
        getorigin(sb,&o);
        hdr->f=o.add;
    } 
    else
        if(!get_receiver(hdr))
            return 0;

    return 1;
}


void postjam(int sb,hdrinfo *hdr1,int usehdr)
{
    hdrinfo hdr;
    char *b,s[MAX_PATH_LEN];
    long len;
    int i,save;
    userrec u;

    curlsub=usub[sb].subnum;

    if(!okpost())
        return;

    hdr= *hdr1;

    hdr.attr=0;

    if(subboards[curlsub].attr & mattr_private)
        hdr.attr |= hdr_private;

    if(subboards[curlsub].attr & mattr_netmail)
        hdr.attr |= hdr_netmail;

    upload_post();
    save=0;

    if(!inputhdr(&hdr,usehdr,sb)) {
        pl("Aborted.");
        return;
    }

    if((hdr.attr & hdr_private)&&!hdr.who_to[0]) {
        pl("error in address");
        return;
    }

    b=ninmsg(&hdr,&len,&save,sb);

    if(quote!=NULL)
        farfree(quote);

    if(!save)
        return;

    if(b==NULL)
        return;

    SaveJamMsg(&hdr,len,b,sb);

    farfree(b);

    ++thisuser.msgpost;
    ++thisuser.posttoday;
    ++status.msgposttoday;
    save_status();
    topscreen();

    logtypes(0,"Posted on 4%s 2[2%s2]",subboards[curlsub].name,hdr.subject);
    npr("Posted on %s\r\n",subboards[curlsub].name);

    save_status();
    if ((subboards[curlsub].attr & mattr_fidonet)|| (subboards[curlsub].attr & mattr_netmail) ) {
        ++thisuser.postnet;
        i = open("DMRESCAN.NOW", O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
        close(i);
    }
}


CHAR8 * GetSubFldStr( JAMSUBFIELD * pSubFld )
{
    static CHAR8    Buffer [256],
    * pBuf;
    UINT32          i;
    int             BufPos;

    for(  pBuf = Buffer, i = BufPos = 0;
        i < pSubFld->DatLen && BufPos + 5 < sizeof( Buffer );
        pBuf++, i++, BufPos++ )
        {
        if( pSubFld->Buffer [( int ) i] < ' ' )
        {
            sprintf( pBuf, "<%02x>", pSubFld->Buffer [( int ) i] );
            BufPos += 3;
            pBuf += 3;
        }
        else
            *pBuf = pSubFld->Buffer [( int ) i];
    }

    *pBuf = '\0';

    return( Buffer );
}

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


/* ---------------------------------------------------------------------- *
 *
 *  JamMsgDeinit
 *
 * ---------------------------------------------------------------------- */

int JamMsgDeinit( JAMAPIREC * pJam )
{
    memset( &pJam->Hdr, '\0', sizeof( JAMHDR ));

    return( 1 );
}

int JamMsgWrite( JAMAPIREC * pJam, CHAR8 * pMsgTxt )
{
    int     LockTryCnt = 0;
    UINT32  MsgNo;

    /*
            **  Lock the messagebase
            */

    while( !JAMmbLockMsgBase( pJam, 1 ))
    {
        if( ++LockTryCnt >= 15 )
        {
            puts( "Unable to get lock on messagebase" );
            exit( 3 );
        }

        sleep( 1 );
    }



    MsgNo = ( filelength( pJam->IdxHandle ) / sizeof( JAMIDXREC )) + pJam->HdrInfo.BaseMsgNum;
    pJam->Hdr.MsgNum = MsgNo;


    /*
            **  Get the offset in the header file for the next message header
            */

    pJam->Idx.HdrOffset = filelength( pJam->HdrHandle );


    /*
            **  And get the offset in the text file for the next text
            */


    pJam->Hdr.TxtOffset = filelength( pJam->TxtHandle );


    /*
            **  Store the index record
            */

    if( !JAMmbStoreMsgIdx( pJam, MsgNo ))
    {
        printf( "Error writing JAMIDXREC: %d\n", pJam->APImsg );
        exit( 4 );
    }


    /*
            **  And the header record
            */

    if( !JAMmbStoreMsgHdr( pJam, MsgNo ))
    {
        printf( "Error writing JAMHDR: %d\n", pJam->APImsg );
        exit( 4 );
    }


    /*
            **  Write all the subfields
            */

    if( JAMsysWrite( NULL, pJam->HdrHandle, pJam->WorkBuf, ( int ) pJam->Hdr.SubfieldLen ) != ( int ) pJam->Hdr.SubfieldLen )
    {
        printf( "Error writing SubFields\n" );
        exit( 4 );
    }


    /*
            **  Write the message text
            */


    if( !JAMmbStoreMsgTxtBuf( pJam, pMsgTxt, pJam->Hdr.TxtLen, 1 ))
    {
        printf( "Error writing message text: %d\n", pJam->APImsg );
        exit( 4 );
    }


    /*
            **  Unlock the messagebase
            */

    pJam->HdrInfo.ActiveMsgs++;
    JAMmbUnLockMsgBase( pJam, 1 );

    return( 1 );
}

int JamMsgAddSFldStr( JAMAPIREC * pJam, UINT16 SubFld, CHAR8 * Str, UINT32 * pSubFldPos )
{
    int Len = strlen( Str );

    if( !JAMmbAddField( pJam, SubFld, 1, Len, pSubFldPos, Str ))
        puts( "WARNING: Work buffer for subfields to small" );

    switch( SubFld )
    {
    case JAMSFLD_RECVRNAME :
        strlwr( Str );
        pJam->Idx.UserCRC = JAMsysCrc32( Str, Len, ( UINT32 ) -1L );
        break;

    case JAMSFLD_MSGID :
        strlwr( Str );
        pJam->Hdr.MsgIdCRC = JAMsysCrc32( Str, Len, ( UINT32 ) -1L );
        break;

    case JAMSFLD_REPLYID :
        strlwr( Str );
        pJam->Hdr.ReplyCRC = JAMsysCrc32( Str, Len, ( UINT32 ) -1L );
        break;
    }

    return( 1 );
}

void JAMOpen(char *fn)
{

    if(JamRec.isOpen)
        JAMClose();



    if( !JAMsysInitApiRec( &JamRec, fn, WORKBUFSIZE ))
    {
        puts( "Not enough memory" );
        return;
    }

    if( !JAMmbOpen( &JamRec ))
    {
        if( JamRec.Errno != ENOENT )
        {
            perror( "Unable to open the JAM messagebase" );
            JAMsysDeinitApiRec( &JamRec );
            return;
        }

        if( !JAMmbCreate( &JamRec ))
        {
            perror( "Unable to create the JAM messagebase" );
            JAMsysDeinitApiRec( &JamRec );
            return;
        }
    }

    nummsgs=filelength( JamRec.IdxHandle )/sizeof(JAMIDXREC);
}


void JAMClose(void)
{
    if(!JAMsysDeinitApiRec(&JamRec))
        pl("Deinit error");
    if(JamRec.isOpen)
        pl("still open");
}

void readmailj(int msgnum,int sb)
{
    int i;

    if(!msgnum)
        msgnum++;

    msgnum=findnextwaiting(msgnum,1,&thisuser);

    scanj(msgnum,&i,sb,1);
}

void addLastRead(int num)
{
    int i;

    JAMmbLockMsgBase(&JamRec, 1 );
    lseek(JamRec.LrdHandle,0L,SEEK_END);
    JamRec.LastRead.HighReadMsg=0;
    JamRec.LastRead.LastReadMsg=0;
    JamRec.LastRead.UserID=num;
    write(JamRec.LrdHandle,&JamRec.LastRead,sizeof(JAMLREAD));
    JAMmbUnLockMsgBase(&JamRec, 1 );
    JamRec.LastLRDnum=filelength(JamRec.LrdHandle)/sizeof(JAMLREAD);
}

void saveLastRead(void)
{
    JAMmbLockMsgBase(&JamRec, 1 );
    JAMmbStoreLastRead(&JamRec,1);
    JAMmbUnLockMsgBase(&JamRec, 1 );
}

void nscan(int sb,int *next)
{
    int num;
    char s[MAX_PATH_LEN];

    sprintf(s,"%s%s",syscfg.msgsdir,subboards[usub[sb].subnum].filename);

    JAMOpen(s);

    if(!JAMmbFetchLastRead(&JamRec,usernum))
        num=1;
    else
        num=JamRec.LastRead.HighReadMsg;

    JAMClose();

    if(nummsgs>num) {
        pl(get_string((18)));
        num++;
        if(subboards[usub[sb].subnum].attr & mattr_private)
            readmailj(num,sb);
        else
            scanj(num,next,sb,0);
        nl();
        pl(get_string(19));
    } 
    else
        pl(get_string(35));
}



void gnscan(void)
{
    int i,next=1;

    pl(get_string(15));
    for(i=0;i<200&&usub[i].subnum!=-1&&i<umaxsubs&&!hangup&&next;i++) {
        if(inscan(i,&thisuser)&&!(subboards[usub[i].subnum].attr & mattr_private))
            nscan(cursub=usub[i].subnum,&next);
    }
    pl(get_string(16));
}

void email(int u,char subject[MAX_PATH_LEN],int ask)
{
    hdrinfo hdr;
    userrec ur;

    userdb_load(u,&ur);
    if ((ur.inact & inact_deleted))
        return;

    if(ask) {
        npr("5Send Mail to %s? ",nam(&ur,u));
        if(!ny())
            return;
    }

    if(subject[0])
        strcpy(hdr.subject,subject);
    else
        hdr.subject[0]=0;
    strcpy(hdr.who_from,pnam(&ur));

    postjam(cursub,&hdr,1);
}

void smail(char ms[MAX_PATH_LEN])
{
    char *p, s[MAX_PATH_LEN],subject[MAX_PATH_LEN];
    int un=0;

    p=strtok(ms,";");

    while(p!=NULL) {
        strcpy(s,p);
        if(atoi(s))
            un=atoi(s);
        else
            strcpy(subject,s);
        p=strtok(NULL,";");
    }

    if(un)
        email(un,subject,1);
    else
        post(cursub);
}

int findnextwaiting(int msgnum,int old,userrec *u)
{
    int found=0;
    UINT32 ucrc,hcrc;
    char s[MAX_PATH_LEN];

    strcpy(s,u->realname);
    strlwr(s);
    ucrc=JAMsysCrc32( s,strlen(s), ( UINT32 ) -1L );

    strcpy(s,pnam(u));
    strlwr(s);
    hcrc=JAMsysCrc32( s,strlen(s), ( UINT32 ) -1L );

    msgnum++;
    while(msgnum<=nummsgs&&!found) {
        JAMmbFetchMsgIdx(&JamRec,msgnum);
        if(ucrc==JamRec.Idx.UserCRC||hcrc==JamRec.Idx.UserCRC) {
            if(old)
                found=1;
            else {
                JAMmbFetchMsgHdr(&JamRec,msgnum,0);
                if(!(JamRec.Hdr.Attribute & MSG_READ))
                    found=1;
            }
        }
        if(!found)
            msgnum++;
    }

    if(!found)
        msgnum=0;

    return(msgnum);
}

int findwaiting(void)
{
    char s[MAX_PATH_LEN];
    int i;

    sprintf(s,"%s%s",syscfg.msgsdir,subboards[usub[cursub].subnum].filename);

    JAMOpen(s);
    i=findnextwaiting(0,0,&thisuser);
    JAMClose();
    return i;
}

int numwaiting(userrec *u)
{
    char s[MAX_PATH_LEN];
    int i=0,i1=0;

    if(u->inact & inact_deleted)
        return 0;

    sprintf(s,"%s%s",syscfg.msgsdir,subboards[usub[cursub].subnum].filename);

    JAMOpen(s);

    i1=findnextwaiting(i1,0,u);
    while(i1) {
        i++;
        i1=findnextwaiting(i1,0,u);
    }

    JAMClose();

    return i;
}


int getnextword(char *buf,long len, long *pos,char *s)
{
    long i,i1=0;
    int ret=0;

    i= *pos;

    while(i<len&&buf[i]!=32&&buf[i]!=13&&buf[i]!=10&&i1<81)
        s[i1++]=buf[i++];

    if(buf[i]==13||buf[i]==10)
        ret=1;

    *pos=i+1;
    s[i1]=0;

    return ret;
}


void quote_jam(char *buf,long len,hdrinfo *hdr)
{
    char *nb, word[MAX_PATH_LEN],thisline[MAX_PATH_LEN],from[MAX_PATH_LEN];
    long pos=0,nlen=0;
    int ret,justwrap=0,added=0,done=0;

    if(quote!=NULL)
        farfree(quote);

    nb=malloca(len+1000L);

    thisline[0]=0;
    word[0]=0;

    strcpy(from,ini(hdr->who_from));


    sprintf(thisline,"%s said this to %s",hdr->who_from,hdr->who_to);
    addline(nb,thisline,&nlen);
    addline(nb,"",&nlen);

    sprintf(thisline," %s> ",from);

    while(pos<=len&&!done) {
        ret=getnextword(buf,len,&pos,word);
        if(justwrap) {
            ret=0;
            justwrap=0;
        }
        if((strlen(thisline)+strlen(word)>78)/*||ret*/) {
            addline(nb,thisline,&nlen);
            if(!strchr(word,'>'))
                sprintf(thisline," %s> ",from);
            justwrap=0;
        }
        if(!strcmp(word,"---"))
            done=1;
        strcat(thisline,word);
        strcat(thisline," ");
    }

    addline(nb,thisline,&nlen);

    nb[nlen+1]=0;

    quote=nb;
}


int findnextthread(int msgnum)
{
    int i=msgnum+1;
    long msgid,replyid;
    hdrinfo h,r;

    JAMmbFetchMsgHdr(&JamRec,msgnum,0);
    getjamhdr(&h);
    npr("m=%s\r\n",h.msgid);
    pausescr();

    /*    msgid=JamRec.Hdr.MsgIdCRC;
        if(msgid==0)
            return 0;*/
    if(!h.msgid[0])
        return 0;

    while(i<nummsgs) {
        JAMmbFetchMsgHdr(&JamRec,i,0);
        getjamhdr(&r);
        npr("r=%s\r\n",r.replyid);
        if(!stricmp(h.msgid,r.replyid))
            return i;
        i++;
    }

    return 0;
}

void editpost(UINT32 *attr)
{
    int i,done=0;
    char s[MAX_PATH_LEN],c;

    do {
        dtitle("Message Status");
        nl();
        bitset("1. Sent",*attr,MSG_SENT);
        bitset("2. Read",*attr,MSG_READ);
        bitset("3. Deleted",*attr,MSG_DELETED);
        bitset("4. Private",*attr,MSG_PRIVATE);
        bitset("5. Hold",*attr,MSG_HOLD);
        bitset("6. Locked",*attr,MSG_LOCKED);
        nl();
        npr("5PSelect (Q=Quit) ");
        switch(onek("123456Q\r")) {
        case '\r':
        case 'Q': 
            done=1; 
            break;
        case '1':
            togglebit((long *)attr,MSG_SENT);
            break;
        case '2':
            togglebit((long *)attr,MSG_READ);
            break;
        case '3':
            togglebit((long *)attr,MSG_DELETED);
            break;
        case '4':
            togglebit((long *)attr,MSG_PRIVATE);
            break;
        case '5':
            togglebit((long *)attr,MSG_HOLD);
            break;
        case '6':
            togglebit((long *)attr,MSG_LOCKED);
            break;
        }
    } 
    while(!done);
}
