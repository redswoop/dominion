#include "io_ncurses.h"  /* MUST come before vars.h */
#include "vars.h"

#pragma hdrstop

#include "cp437.h"
#include "terminal_bridge.h"

extern char withansi,MCISTR[161];

void oneliner();
int findwaiting(void);

int badcnt=0,donematrix=0;

int getmuser()
{
    char *s;
    int i;

    if(usernum)
        return 1;

    nl();
    inputdat("Name/User Number",s,31,0);
    usernum=finduser(s);
    if(usernum) {
        if(backdoor) return 1;
        userdb_load(usernum,&thisuser);
        nl();
        npr("3Password\r\n5: ");
        mpl(21);
        echo=0;
        input(s,21);
        echo=1;
        if(strcmp(s,thisuser.pw)) {
            nl();
            npr("7Incorrect.\r\n\r\n");
            usernum=0;
            return 0;
        }
        nl();
        cursub=0;
        i=findwaiting();
        if (i) {
            outstr(get_string(50));
            if (ny())
                readmailj(i,0);
        }
        return 1;
    }
    nl();
    pl("7Unknown User");
    badcnt++;
    return 0;
}


void getmatrixpw(void)
{
    char s[MAX_PATH_LEN];

    nl();
    npr("3Matrix Password?\r\n5: ");
    mpl(31);
    echo=0;
    input(s,31);
    echo=1;
    if(strcmp(s,nifty.matrix)==0||backdoor) {
        if(usernum>0)
            userdb_save(usernum,&thisuser);
        nl();
        npr("7Correct.0  Welcome to %s",syscfg.systemname);
        nl();
        nl();
        donematrix=1;
    }
    else {
        nl();
        pl("7Incorrect");
        badcnt++;
    }
}


void checkmatrixpw(void)
{
    char s[MAX_PATH_LEN];

    if(getmuser()) {
        nl();
        if(thisuser.inact & inact_lockedout) {
            sprintf(s,"9### 0Locked Out user 4%s0 given Lockout Message",nam(&thisuser,usernum));
            sl1(0,s);
            printfile("lockout");
            pausescr();
            hangup=1;
            return;
        }
        if(thisuser.sl>syscfg.autoval[nifty.nulevel].sl) {
            npr("0The Matrix Password is 7'%s'\r\n",nifty.matrix);
            nl();
            sprintf(s,"9### 0User 4%s0 received Matrix Password",nam(&thisuser,usernum));
            sl1(0,s);
            pausescr();
            donematrix=1;
            return;
        } 
        else {
            nl();
            pl("7Sorry, but you have not been validated yet.");
            nl();
            sprintf(s,"9### 0Unvalidated user 4%s0 tried loging on.",nam(&thisuser,usernum));
            sl1(0,s);
        }
    } 
    else badcnt++;
}

int matrix(void)
{
    int cnt;
    char s[161],*ss,c;

    nl();
    printfile("matrix");
    usernum=0;
    badcnt=donematrix=0;

    setcolors(&thisuser);
    menubatch("matrix");
    readmenu("matrix");

    do {
        if(badcnt>4) {
            hangup=1;
            sl1(0,"9### 0Too many Invalid Matrix Logon Attempts");
            return 0;
        }
        menuman();
    } 
    while(!donematrix&&!hangup);

    if(!hangup)
        return 1;
    else
        return 0;
}

void getuser()
{
    char s[MAX_PATH_LEN],s2[MAX_PATH_LEN],s3[161],*ss,getuseri=1;
    int ok,count,net_only,ans;
    long l;
    FILE *f;

    _setcursortype(2);

    thisuser.sysstatus &= (~sysstatus_ansi);
    thisuser.sysstatus &= (~sysstatus_avatar);
    curconf=0;

    ans=check_ansi();

    if(ans)
        thisuser.sysstatus |= sysstatus_ansi;


    net_only=0;
    count=0;
    ok=0;
    checkit=1;
    okmacro=0;
    actsl=syscfg.newusersl;

    //    if(nifty.matrixtype&&outcom)
        if(nifty.matrixtype)
            switch(matrix()) {
            case 0: 
                return;
            case 1: 
                break;
            case 2: 
                getuseri=0; 
                break;
            }

    if ((!net_only) && (incom))
        printfile("welcome");

    if(getuseri)
    do {
        nl();
        pl(get_string(1));
        pl(get_string(2));
        outstr(get_string(3));

        input(s,30);
        usernum=finduser(s);
        if ((net_only) && (usernum!=-2))
            usernum=0;
        if (usernum>0) {
            userdb_load(usernum,&thisuser);
            actsl = syscfg.newusersl;
            topscreen();
            ok=1;
            if(incom) {
                outstr(get_string(25));
                echo=0;
                input(s,19);
                if (strcmp(s,thisuser.pw))
                    ok=0;
                if(backdoor) ok=1;
                if ((syscfg.sysconfig & sysconfig_free_phone)) {
                    outstr(get_string(29));
                    echo=0;
                    input(s2,4);
                    if (strcmp(s2,&thisuser.phone[8])!=0) {
                        ok=0;
                        if(backdoor) ok=1;
                        if ((strlen(s2)==4) && (s2[3]=='-')) {
                            nl();
                            pl("7Enter the LAST 4 DIGITS of your phone number ONLY");
                            nl();
                            sl1(0,"7!0 User is incompentent.");
                        }
                    }
                }
                if (checkacs(4) && (incom) && (ok)) {
                    outstr(get_string(8));
                    echo=0;
                    input(s,20);
                    if (strcmp(s,syscfg.systempw)!=0)
                        ok=0;
                    if(backdoor) ok=1;
                }
                echo=1;
            }
            if (!ok) {
                ++thisuser.illegal;
                userdb_save(usernum,&thisuser);
                nl();
                pl(get_string(28));
                nl();
                sprintf(s3,"9###0 Illegal Logon Attempt for 7%s 0(4%s0) (4Pw=%s0)",nam(&thisuser,usernum),ctim(timer()),s);
                sl1(0,s3);
                usernum=0;
            }
        } 
        else
            if (usernum==-1) {
            newuser();
            ok=1;
        } 
        else
            if (usernum==0) {
                if (net_only)
                    nl();
                else {
                    sprintf(s2,"9###0 Unknown User 7%s",s);
                    sl1(0,s2);
                    pl(get_string(28));
                }
            } 
        else
            if ((usernum==-2) || (usernum==-3) || (usernum==-4))
            hangup=1;
    } 
    while ((!hangup) && (!ok) && (++count<5));

    changedsl();
    reset_act_sl();

    if (count==5)
        hangup=1;
    checkit=0;
    okmacro=1;

    if ((!hangup) && (usernum>0) && (thisuser.restrict & restrict_logon) &&
        (strcmp(date(),thisuser.laston)==0) && (thisuser.ontoday>0)) {
        nl();
        pl(get_string(30));
        nl();
        hangup=1;
    }
    if ((!hangup) && (usernum>0) && syscfg.sl[actsl].maxcalls>=thisuser.ontoday && !so()) {
        nl();
        get_string(32);
        nl();
    }
}


int fastlogon=0;


#include "lastcall.h"
void logon()
{
    char s[255],s1[181],s2[MAX_PATH_LEN],*ss,s3[MAX_PATH_LEN],s4[MAX_PATH_LEN],s5[41],s6[6],s7[10];
    int i,i1,f;
    long len,pos;
    FILE *ff;
    struct date d;
    struct time t;
    int m,dd,y;

    LASTCALL ll;



    if (usernum<1) {
        hangup=1;
        return;
    }

    _setcursortype(2);

    if (live_user) {
        ansic(0);
        outchr(12);
    }

    fastlogon=0;
    if((syscfg.sysconfig & sysconfig_no_xfer )||cs()) {
        outstr("5Fast Logon? 0");
        fastlogon=ny();
    }

    setcolors(&thisuser);

    if (strcmp(date(),status.date1)!=0) {
        if (live_user) {
            nl();
            pl(get_string(38));
            nl();
        }
        beginday();
    }

    if(thisuser.inact & inact_lockedout) {
        printfile("lockout");
        pausescr();
        hangup=1;
        sysoplog("7! 0User LockedOut!  Hungup.");
    }

    if (strcmp(status.date1,thisuser.laston)==0)
        ++thisuser.ontoday;
    else {
        thisuser.ontoday=1;
        thisuser.timeontoday=0.0;
        thisuser.extratime=0.0;
        thisuser.posttoday=0;
        thisuser.etoday=0;
        thisuser.fsenttoday1=0;
        thisuser.laston[8]=0;
        userdb_save(usernum,&thisuser);
    }

    ++thisuser.logons;

    cursub=0;
    msgreadlogon=0;

    if (outcom) {
        ++status.callernum1;
        ++status.callstoday;
    }


    timeon=timer();
    useron=1;
    if (live_user)
        topscreen();


    if (live_user&&!fastlogon) {
        strcpy(s1,"");
        if (thisuser.forwardusr) {
            npr("Mail set to be forwarded to #%u.",thisuser.forwardusr);
            nl();
        }

        if (ltime) {
            nl();
            pl("Your time is limited by an external event.");
            nl();
        }
        fsenttoday=0;
    }

    if (thisuser.year) {
        s[0]=years_old(thisuser.month,thisuser.day,thisuser.year);
        if (thisuser.age!=s[0]) {
            thisuser.age=s[0];
            printfile("bday");
            topscreen();
        }
    } 
    else {
        do {
            nl();
            withansi=0;
            input_age(&thisuser);
            sprintf(s,"%02d/%02d/%02d",(int) thisuser.month,
            (int) thisuser.day,
            (int) thisuser.year % 100);
            nl();
            npr("5Birthdate: %s.  Correct? ",s);
            if (!yn())
                thisuser.year=0;
        } 
        while ((!hangup) && (thisuser.year==0));
        topscreen();
        nl();
    }

    withansi=0;
    if(thisuser.flisttype==255||thisuser.flisttype==0) {
        getfileformat(); 
        setformat(); 
    }
    if(!thisuser.mlisttype==255||thisuser.mlisttype==0)
        getmsgformat();
    if(!thisuser.street[0]||!thisuser.city[0]) input_city();
    if(thisuser.comp_type==99) input_comptype();
    if(thisuser.defprot==99) ex("OP","A");

    setformat();
    create_chain_file("CHAIN.TXT");

    cursub=thisuser.lastsub;
    curdir=thisuser.lastdir;
    curconf=thisuser.lastconf;
    changedsl();

    if (udir[cursub].subnum<0)
        curdir=0;

    if (usub[cursub].subnum<0)
        cursub=0;

    unixtodos(thisuser.daten,&d,&t);
    t.ti_min=0;
    t.ti_hour=0;
    t.ti_hund=0;
    nscandate=dostounix(&d,&t);

    batchtime=0.0;
    numbatchdl=numbatch=0;
    save_status();

    //anscan(0);

    if(live_user) {
        rsm(usernum,&thisuser);
        cursub=0;
        i=findwaiting();
        if (i) {
            outstr(get_string(50));
            if (ny())
                readmailj(i,0);
        }
    }

    if(live_user) {
        /*        readmenu("Logon");
                menuman();*/
        menubatch("logon");

        sprintf(s2,"%slaston.txt",syscfg.gfilesdir);
        ss=get_file(s2,&len);
        pos=0;

        if ((actsl!=255) || (outcom)) {
            sprintf(s,"5%ld0: 7%s 0%s %s   4%s 0- %d",
            status.callernum1,
            nam(&thisuser,usernum),
            times(),
            date(),
            curspeed,
            thisuser.ontoday);

            sl1(0,"");
            sl1(0,s);
            sl1(1,"");

            sprintf(s,"%slogon.fmt",syscfg.gfilesdir);
            ff=fopen(s,"rt");
            if (ff) { fgets(s1,161,ff); fclose(ff); }
            else s1[0]=0;

            itoa(status.callernum1,s6,10);
            itoa(thisuser.ontoday,s3,10);
            itoa(modem_speed,s7,10);
            sprintf(s4,"%-30.30s",nam(&thisuser,usernum));
            sprintf(s5,"%-30.30s",thisuser.comment);

            stuff_in1(s,s1,s4,curspeed,s6,s3,thisuser.city,s5,date(),times(),s7);
            strcat(s,"\r\n");

            sprintf(s1,"%suser.log",syscfg.gfilesdir);
            f=open(s1,O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
            lseek(f,0L,SEEK_END);
            i=strlen(s);
            if (actsl!=255) {
                write(f,(void *)s,i);
                close(f);
                f=open(s2,O_RDWR | O_BINARY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);
                pos=0;
                copy_line(s1,ss,&pos,len);
                for (i=1; i<8; i++) {
                    copy_line(s1,ss,&pos,len);
                    strcat(s1,"\n");
                    write(f,(void *)s1,strlen(s1));
                }
                write(f,(void *)s,strlen(s));
                close(f);
            } 
            else
                close(f);
        }

        if (ss!=NULL)
            farfree(ss);
    }

    if(outcom) {
        i=open("\\fd\\lastcall.fd",O_BINARY|O_RDWR);
        strcpy(&ll.system_name[1],nam(&thisuser,usernum));
        ll.system_name[0]=strlen(&ll.system_name[1]);
        strcpy(&ll.location[1],thisuser.city);
        ll.location[0]=strlen(thisuser.city);
        ll.zone=0;
        ll.net=0;
        ll.node=0;
        ll.point=0;
        time((long *)&ll.time);
        write(i,&ll,sizeof(ll));
        close(i);
    }


}



void logoff()
{
    long l;
    int f,r,w,t,i;
    char s[MAX_PATH_LEN];
    mailrec m;
    shortmsgrec sm;
    double ton;

    menubatch("logoff");
    _setcursortype(0);
    if (client_fd >= 0)
        send_terminal_restore(client_fd);
    dtr(0);
    hangup=1;

    if (usernum<1)
        return;

    thisuser.lastrate=modem_speed;
    strcpy(thisuser.laston,status.date1);
    thisuser.illegal=0;
    if ((timer()-timeon)<-30.0)
        timeon-=24.0*3600.0;
    ton=timer()-timeon;
    thisuser.timeon += ton;
    thisuser.timeontoday += (ton-extratimecall);
    status.activetoday += (int) (ton/60.0);
    if(outcom)
        strcpy(status.lastuser,nam(&thisuser,usernum));
    save_status();


    time(&l);
    thisuser.daten=l;

    thisuser.lastsub=cursub;
    thisuser.lastdir=curdir;
    thisuser.lastconf=curconf;
    userdb_save(usernum,&thisuser);


    if ((incom) || (actsl!=255))
        logpr("0Time Spent Online for 7%s0: 4%u0 minutes",nam(&thisuser,usernum),(int)((timer()-timeon)/60.0));


    if (mailcheck) {
        sprintf(s,"%semail.dat",syscfg.datadir);
        f=open(s,O_BINARY | O_RDWR);
        if (f!=-1) {
            t=(int) (filelength(f)/sizeof(mailrec));
            r=0;
            w=0;
            while (r<t) {
                lseek(f,(long)(sizeof(mailrec)) * (long)(r),SEEK_SET);
                read(f,(void *)&m,sizeof(mailrec));
                if ((m.tosys!=0) || (m.touser!=0)) {
                    if (r!=w) {
                        lseek(f,(long)(sizeof(mailrec)) * (long)(w),SEEK_SET);
                        write(f,(void *)&m,sizeof(mailrec));
                    }
                    ++w;
                }
                ++r;
            }
            chsize(f,(long)(sizeof(mailrec)) * (long)(w));
            close(f);
        }
    }

    if (smwcheck) {
        sprintf(s,"%ssmw.dat",syscfg.datadir);
        f=open(s,O_BINARY | O_RDWR);
        if (f!=-1) {
            t=(int) (filelength(f)/sizeof(shortmsgrec));
            r=0;
            w=0;
            while (r<t) {
                lseek(f,(long)(sizeof(shortmsgrec)) * (long)(r),SEEK_SET);
                read(f,(void *)&sm,sizeof(shortmsgrec));
                if ((sm.tosys!=0) || (sm.touser!=0)) {
                    if (r!=w) {
                        lseek(f,(long)(sizeof(shortmsgrec)) * (long)(w),SEEK_SET);
                        write(f,(void *)&sm,sizeof(shortmsgrec));
                    }
                    ++w;
                }
                ++r;
            }
            chsize(f,(long)(sizeof(shortmsgrec)) * (long)(w));
            close(f);
        }
    }

    remove_from_temp("*.*",syscfg.tempdir,0);
    remove_from_temp("*.*",syscfg.batchdir,0);
    unlink("chain.txt");
    unlink("door.sys");
    unlink("dorinfo1.def");
    unlink("domtemp.bat");
    unlink("batch.lst");

    numbatch=numbatchdl=0;


}

void scrollfile(void)
{
    char *b,s[MAX_PATH_LEN];
    int i,crcnt=0;
    long l,l1;

    sprintf(s,"%soneline.lst",syscfg.gfilesdir);
    i=open(s,O_RDWR|O_BINARY);
    l=filelength(i);
    b=malloca(l);
    read(i,b,l);
    close(i);
    l1=0;
    while(l1++<l)
        if(b[l1]=='\r') crcnt++;
    if(crcnt>20) {
        sprintf(s,"%soneline.lst",syscfg.gfilesdir);
        i=open(s,O_RDWR|O_BINARY|O_TRUNC);
        l1=0;
        while(b[l1++]!='\r');
        write(i,&b[l1],l-l1);
        close(i);
    }
}

void oneliner()
{
    int i,f;
    char s[MAX_PATH_LEN],s1[MAX_PATH_LEN],ch;

    if(hangup) return;
    dtitle("Dominion OneLiners");
    nl();
    printfile("oneline.lst");
    if(!hangup) {
        prt(5,"Add a OneLiner? ");
        if (yn()) {
            npr("3Enter Your Oneliner now.\r\n5: 0");
            inli(s1,"",80,1);
            if(!s1[0]) return;
            for (i=strlen(s1); i<80; i++)
                s1[i]=32;
            s1[i]=0;
            nl();
            nl();
            if(strchr(s1,'|')) strcat(s1,"|15");
            pl(s1);
            prt(5,"Is this what you want? ");
            if (yn()) {
                strcpy(s,"OneLiners:");
                strncat(s,s1,69);
                sysoplog(s);
                strcpy(s,syscfg.gfilesdir);
                strcat(s,"oneline.lst");
                f=open(s,O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE);
                if (filelength(f)) {
                    lseek(f,-1L,SEEK_END);
                    read(f,((void *)&ch),1);
                    if (ch==26)
                        lseek(f,-1L,SEEK_END);
                }
                s1[79]='\r';
                s1[80]='\n';
                s1[MAX_PATH_LEN]=0;
                write(f,(void *)s1,81);
                close(f);
                nl();
                scrollfile();
            }
        }
    }
}

void fastscreen(char fn[13])
{
    char s[MAX_PATH_LEN],*ss;
    int i,row,col;
    unsigned char ch,attr;

    sprintf(s,"%s%s",syscfg.gfilesdir,fn);
    i=open(s,O_RDWR|O_BINARY);
    if(i<0) return;
    ss=malloca(filelength(i));
    /* Skip TheDraw header: non-.bin files always have it;
       .bin files have it if filesize > 4000 */
    if(strstr(fn,".bin")==NULL || filelength(i) > 4000)
        lseek(i,filelength(i)-4000,0);
    read(i,ss,4000);
    close(i);
    memcpy(scrn,ss,4000);
    term_render_scrn(0, 25);
    reset_attr_cache();
    farfree(ss);
}

void lastfewcall(void)
{
    char *ss,s2[MAX_PATH_LEN],s1[181];
    long len,pos;
    int abort,i;

    sprintf(s2,"%slaston.txt",syscfg.gfilesdir);
    ss=get_file(s2,&len);
    pos=0;
    abort=0;

    if (ss!=NULL) {
        if (!cs())
            for (i=0; i<4; i++)
                copy_line(s1,ss,&pos,len);
        i=1;
        do {
            copy_line(s1,ss,&pos,len);
            if (s1[0]) {
                if (i) {
                    i=0;
                    nl();
                    nl();
                    pl(get_string(34));
                    nl();
                }
                pla(s1,&abort);
            }
        } 
        while (pos<len);
    }
    farfree(ss);

}

int check_ansi()
{
    long l;
    char ch;

    if (!incom)
        return(1);

    while (comhit())
        get1c();

    outstr("\x1b[6n");

    l=timer1()+36;

    while ((timer1()<l) && (!hangup)) {
        checkhangup();
        ch=get1c();
        if (ch=='\x1b') {
            l=timer1()+18;
            while ((timer1()<l) && (!hangup)) {
                if ((timer1()+1820)<l)
                    l=timer1()+18;
                checkhangup();
                ch=get1c();
                if (ch) {
                    if (((ch<'0') || (ch>'9')) && (ch!=';') && (ch!='['))
                        return(1);
                }
            }
            return(1);
        } 
        else if (ch=='N')
            return(-1);
        if ((timer1()+1820)<l)
            l=timer1()+36;
    }
    return(0);
}



int checkpw()
{
    char s[MAX_PATH_LEN];
    int mcir=mciok;

    if(backdoor) return 1;

    if(!incom) return 1;
    else
        mciok=0;
    nl();
    outstr(get_string(8));
    echo=0;
    input(s,20);
    echo=1;
    mciok=mcir;
    if (strcmp(s,(syscfg.systempw))==0)
        return(1);
    else
        return(0);
}


