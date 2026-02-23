#include "platform.h"
#include "fcns.h"
#include "session.h"
#include "system.h"
#include "version.h"

#pragma hdrstop


void read_automessage()
{
    auto& sys = System::instance();
    int i;
    messagerec m;
    FILE *f;
    char s[161];

    sprintf(s,"%sauto.fmt",sys.cfg.gfilesdir);
    f=fopen(s,"rt");
    if (!f) return;
    fgets(s,161,f); 
    filter(s,'\n');
    plfmt(s);
    fgets(s,161,f); 
    filter(s,'\n');
    plfmt(s);
    printfile("auto.msg");
    fgets(s,161,f); 
    filter(s,'\n');
    plfmt(s);
    fclose(f);
}


void write_automessage()
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    hdrinfo hdr;
    int save,i;
    long len;
    char *b,s[MAX_PATH_LEN];

    if(sess.user.restrict & restrict_automessage) {
        pl("7You are restricted from changing the AutoMessage");
        pausescr();
        return;
    }

    upload_post();
    b=ninmsg(&hdr,&len,&save,0);
    if(save) {
        sys.status.amsganon=0;
        sys.status.amsguser=sess.usernum;
        logtypes(2,"Changed Automessage");
        sprintf(s,"%sauto.msg",sys.cfg.gfilesdir);
        unlink(s);
        i=open(s,O_BINARY|O_CREAT|O_TRUNC,S_IREAD|S_IWRITE);
        write(i,b,len);
        close(i);
        free(b);
    }
}


void addbbs(char *fn)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    int i,i1,i2,f,ok;
    char s[150],s1[150],s2[150],ch,ch1,*ss,s3[40],s4[40],final[150],form[40];
    long l,l1;
    FILE *format;

    if(sess.user.restrict & restrict_bbslist) {
        pl("7You are restricted from adding to the BBSlist");
        pausescr();
        return;
    }
    nl();
    npr("3Please enter phone number\r\n5: ");
    mpl(12);
    inputfone(s);
    nl();

    if ((s[3]!='-') || (s[7]!='-'))
        s[0]=0;

    if (strlen(s)==12) {
        ok=1;
        sprintf(s1,"%s%s",sys.cfg.gfilesdir,fn);

        f=open(s1,O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE);
        if (f>0) {
            lseek(f,0L,SEEK_SET);
            l=filelength(f);
            if ((ss=(char *)malloca(l+500L))==NULL) {
                close(f);
                return;
            }
            read(f,ss,(int)l);
            l1=0L;
            while ((l1<l) && (ok)) {
                i=0;
                do {
                    ch=ss[l1++]; 
                    s1[i]=ch;
                    if (ch==13) s1[i]=0;
                    ++i;
                } 
                while ((ch!=10) && (i<120) && (l1<l));
                if (strstr(s1,s)!=NULL)
                    ok=0;
                if (!strncmp(s1,s,12))
                    ok=0;
            }
            free(ss);
            close(f);
        }

        if(!ok) {
            nl();
            pl("7Sorry, but that number is already in the BBS list.");
            nl();
            return;
        }

        if(ok) {
            pl("Number not yet in BBS list.");
            nl();
            inputdat("Enter BBS name and comments",s1,40,1);
            inputdat("Enter maximum speed of the BBS",s2,5,0);
            inputdat("Enter BBS type (Dominion)",s3,8,1);
            nl();

            sprintf(form,"%sbbslist.fmt",sys.cfg.gfilesdir);
            format=fopen(form,"rt");
            fgets(form,41,format);
            fclose(format);
            filter(form,'\n');

            strcpy(final,s1);
            sprintf(s1,"%-40s",final);
            strcpy(final,s2);
            sprintf(s2,"%-5s",final);
            strcpy(final,s3);
            sprintf(s3,"%-8s",final);

            stuff_in(final,form,s,s1,s2,s3,"");
            pl(final);
            nl();
            prt(5,"Is this correct? ");
            if (yn()) {
                logtypes(2,"Added to BBSlist:0 %s, %s",s,s1);
                strcat(final,"\r\n");
                sprintf(s1,"%s%s",sys.cfg.gfilesdir,fn);
                f=open(s1,O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE);
                if (filelength(f)) {
                    lseek(f,-1L,SEEK_END);
                    read(f,((void *)&ch1),1);
                    if (ch1==26)
                        lseek(f,-1L,SEEK_END);
                }
                write(f,(void *)final,strlen(final));
                close(f);
                nl();
                pl("Added to BBS list.");
                nl();
            }
        }
    }
}

void searchbbs(char *fn)
{
    auto& sys = System::instance();
    FILE *f;
    char s[150],s1[20],s2[150];


    nl();
    /*    npr("3Enter Text to Search For\r\n5: ");
                mpl(20);
                input(s1,20);*/
    inputdat("Enter Text to Search For",s1,20,0);
    if(!s1[0]) return;

    sprintf(s,"%s%s",sys.cfg.gfilesdir,fn);
    f=fopen(s,"rt");
    while((fgets(s2,150,f))!=NULL) {
        filter(s2,'\n');
        strcpy(s,noc2(s2));
        strupr(s);
        if(strstr(s,s1)) {
            pl(s2);
            pausescr();
        }
    }
    nl();
    fclose(f);
}


void list_users()
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    subboardrec s;
    userrec u;
    int i,nu,abort,ok,num;
    char st[161];

    if (sess.usub[sess.cursub].subnum==-1||(sess.user.restrict & restrict_userlist)) {
        nl();
        pl("Sorry, you cannot currently view the user list");
        nl();
        return;
    }
    s=sys.subboards[sess.usub[sess.cursub].subnum];
    nl();
    pl("Users with access to current sub:");
    nl();
    npr("0%-30s 1[0%-40s1]\r\n","Users Name","Comment");
    abort=0;
    num=0;
    for (i=0; (i<userdb_user_count()) && (!abort) && (!io.hangup); i++) {
        smalrec sr;
        userdb_get_entry(i, &sr);
        userdb_load(sr.number,&u);
        ok=1;
        if (u.age<s.age)
            ok=0;
        if ((s.ar!=0) && ((u.ar & s.ar)==0))
            ok=0;
        if(u.inact & inact_lockedout)
            ok=0;
        if(u.exempt & exempt_userlist)
            ok=0;
        if (ok) {
            npr("3%-30s 1[0%-40s1]\r\n",nam(&u,sr.number),u.comment);
            ++num;
        }
    }
    if (!abort) {
        nl();
        npr("%d users.\r\n",num);
        nl();
    }
}


void yourinfo()
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    char s[MAX_PATH_LEN];
    originrec orig;

    outchr(12);
    dtitle("Your Information");

    npr("0Your Handle is 5%s0, Your Voice Phone Number is 5%s0\r\n",nam(&sess.user,sess.usernum),sess.user.phone);
    npr("Your Acting Security Level is 5%d0, And Your Transfer Level is 5%d0\r\n",sess.actsl,sess.user.dsl);
    nl();
    npr("You have Downloaded 5%d0 files, And Uploaded 5%d0 files\r\n",sess.user.downloaded,sess.user.uploaded);
    npr("You have posted 5%d0 times, and Called 5%d0 times\r\n",sess.user.msgpost,sess.user.logons);
    nl();
    npr("You have 5%d0 minutes left for this call\r\n",(int)((nsl()+30)/60.0));
    sprintf(s,"0, And have been on 5%d0 times today.",sess.user.ontoday);
    npr("You last called 5%s%s0\r\n",sess.user.laston,sess.user.ontoday?s:"");
    nl();
    npr("System is 5%s\r\n",wwiv_version);
    getorigin(sys.subboards[sess.usub[sess.cursub].subnum].origin,&orig);
    if(orig.add.zone)
        npr("Network is %s, Address %d:%d/%d\r\n",orig.netname,orig.add.zone,orig.add.net,orig.add.node);
    nl();
    if(sess.user.helplevel==2)
        pausescr();
}


void jumpconf(char ms[41])
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    char c,s[MAX_PATH_LEN];
    int i,ok,type=0;

    if(atoi(ms)) {
        if(slok(sys.conf[atoi(ms)].sl,0)) {
            sess.curconf=atoi(ms);
            changedsl();
            return;
        }
    } 
    else if(ms[0]) type=ms[0];

    dtitle("Conferences Available: ");
    for(c=0;c<sys.num_conf;c++) {
        ok=1;
        if(!slok(sys.conf[c].sl,0)) ok=0;
        if(type=='M'||type=='F')
            if(sys.conf[c].type!=type&&sys.conf[c].type)
                ok=0;
        if(ok)
            npr("0<%d> %s\r\n",c+1,sys.conf[c].name);
    }
    nl();
    outstr("Select: ");
    mpl(2);
    input(s,2);
    i=atoi(s);
    if(i==0) return;
    i--;
    if(i>sys.num_conf) return;
    if(slok(sys.conf[i].sl,0)) sess.curconf=i;
    else {
        pl("Unavailable Conference");
        sess.curconf=0;
        return;
    }
    changedsl();
}

void add_time(int limit)
{
    auto& sess = Session::instance();
    int minutes;
    char s[MAX_PATH_LEN];
    double nsln;

    nsln=nsl();
    npr("0Time in Bank: 5%d\r\n",sess.user.timebank);
    npr("0Bank Limit  : 5%d\r\n",limit);
    nl();
    npr("5%.0f0 minutes left online.\r\n",nsln/60.0);
    npr("How many minutes would you like to deposit? ");
    mpl(3);
    ansic(4);
    inputl(s,3);
    minutes = atoi(s);
    if (minutes<1) return;
    if (io.hangup) return;
    if (minutes > (int)((nsl() - 20.0)/60.0)) {
        nl();
        prt(7, "You do not have enough time left to deposit that much.");
        nl();
        pausescr();
        return;
    }
    if ((minutes + sess.user.timebank) > limit) {
        nl();
        npr("7You may only have up to %d minutes in your account at once.\r\n", limit);
        nl();
        pausescr();
        return;
    }
    sess.user.timebank += minutes;
    userdb_save(sess.usernum,&sess.user);
    logtypes(2,"Deposit 3%d0 minutes.", minutes);
    nl();
    npr("%d minute%c deposited.", minutes,((minutes > 1) ? 's' : 0));
    nl();

    if (sess.extratimecall > 0) {
        if (sess.extratimecall >= (double)(minutes * 60)) {
            sess.extratimecall -= (double)(minutes * 60);
            return;
        }
        minutes -= (int)(sess.extratimecall / 60.0);
        sess.extratimecall = 0.0;
    }
    sess.user.extratime -= (float)(minutes * 60);
    pausescr();
}

void remove_time()
{
    auto& sess = Session::instance();
    char s[MAX_PATH_LEN];
    int minutes;

    nl(); 
    nl();
    npr( "Time in account: %d minutes.\r\n", sess.user.timebank);
    nl();
    ansic(0);
    outstr("How many minutes would you like to withdraw? ");
    ansic(4);
    inputl(s,3);
    minutes = atoi(s);
    if (minutes<1) return;

    if (io.hangup) return;
    if (minutes > sess.user.timebank) {
        nl(); 
        nl();
        prt(7, "You don't have that much time in the account!");
        nl();
        nl();
        pausescr();
        return;
    }
    logtypes(2,"Withdrew 3%d0 minutes.", minutes);
    sess.user.extratime += (float)(minutes * 60);
    sess.user.timebank -= minutes;
    nl(); 
    nl();
    npr("4%d minute%c withdrawn.", minutes,((minutes > 1) ? 's' :0));
    nl();
    pausescr();
}

void bank2(int limit)
{
    auto& sess = Session::instance();
    int done = 0;
    char s[MAX_PATH_LEN], ch;

    logtypes(2,"Entered TimeBank");
    do {
        dtitle("Dominion Time Bank");
        nl();
        tleft(0);
        npr("0Time in Bank: 5%d\r\n",sess.user.timebank);
        npr("0Bank Limit  : 5%d\r\n",limit);
        nl();
        outstr(get_string(72));
        ch = onek("Q\rDW");
        switch (ch) {
        case '\r':
        case 'Q': 
            done = 1; 
            break;
        case 'D': 
            add_time(limit); 
            break;
        case 'W': 
            if(sess.user.restrict & restrict_timebank) {
                nl();
                pl(get_string(53));
                nl();
                logtypes(2,"Tried to withdraw time, but was dissallowed");
                break;
            }
            remove_time();  
            break;
        }
    } 
    while ((!io.hangup) && (!done));
    nl();
}

char *stl(long l)
{
    static char s[10];

    ltoa(l,s,10);
    return(s);
}

char *sti(int i)
{
    static char s[10];

    itoa(i,s,10);
    return(s);
}

#include "mci.h"

void updtopten(void)
{
    auto& sys = System::instance();
    userrec u;
    int i,i1,stop,num_users;
    char s1[161],s2[161],s3[10],s4[10],sname[31],s[5][10][31],ch;


    int low_posts_on_list,
    low_logons_on_list,
    posts_per_user[10],
    logons_per_user[10];

    unsigned long low_upk_on_list,
    low_dnk_on_list,
    up_k_per_user[10],
    dn_k_per_user[10];

    float low_time_on_list,
    timeon_per_user[10];


    low_posts_on_list  = 0;
    low_logons_on_list = 0;
    low_upk_on_list    = 0;
    low_dnk_on_list    = 0;
    low_time_on_list   = 0;

    for (i = 0; i < 10; i++) {
        posts_per_user[i]  = 0;
        logons_per_user[i] = 0;
        up_k_per_user[i]   = 0;
        dn_k_per_user[i]   = 0;
        timeon_per_user[i] = 0;
    }

    num_users = userdb_max_num();

    for (i = 0; i < 5; i++)
        for (i1 = 0; i1 < 10; i1++)
            strcpy(s[i][i1], "None");

    for (i = 1; i <= num_users; i++) {
        userdb_load(i, &u);
        if(u.inact & inact_deleted)
            continue;

        if (u.msgpost >= low_posts_on_list) {
            low_posts_on_list = posts_per_user[9];
            for (i1 = 0; i1 < 10; i1++)
                if (u.msgpost >= posts_per_user[i1]) {
                    stop = i1;
                    i1 = 10;
                }
            for (i1 = 9; i1 > stop; i1--) {
                posts_per_user[i1] =
                    posts_per_user[i1-1];
                strcpy(s[0][i1], s[0][i1-1]);
            }
            posts_per_user[stop] = u.msgpost;
            strcpy(s[0][stop], nam(&u,i));
        }

        if (u.logons >= low_logons_on_list) {
            low_logons_on_list = logons_per_user[9];
            for (i1 = 0; i1 < 10; i1++)
                if (u.logons >= logons_per_user[i1]) {
                    stop = i1;
                    i1 = 10;
                }
            for (i1 = 9; i1 > stop; i1--) {
                logons_per_user[i1] =
                    logons_per_user[i1-1];
                strcpy(s[1][i1], s[1][i1-1]);
            }
            logons_per_user[stop] = u.logons;
            strcpy(s[1][stop], nam(&u,i));
        }

        if (u.uk >= low_upk_on_list) {
            low_upk_on_list = up_k_per_user[9];
            for (i1 = 0; i1 < 10; i1++)
                if (u.uk >= up_k_per_user[i1]&&u.uk>0) {
                    stop = i1;
                    i1 = 10;
                }
            for (i1 = 9; i1 > stop; i1--) {
                up_k_per_user[i1] =
                    up_k_per_user[i1-1];
                strcpy(s[2][i1], s[2][i1-1]);
            }
            up_k_per_user[stop] = u.uk;
            strcpy(s[2][stop], nam(&u,i));
        }


        if (u.dk >= low_dnk_on_list) {
            low_dnk_on_list = dn_k_per_user[9];
            for (i1 = 0; i1 < 10; i1++)
                if (u.dk >= dn_k_per_user[i1]&&u.dk>0) {
                    stop = i1;
                    i1 = 10;
                }
            for (i1 = 9; i1 > stop; i1--) {
                dn_k_per_user[i1] =
                    dn_k_per_user[i1-1];
                strcpy(s[3][i1], s[3][i1-1]);
            }
            dn_k_per_user[stop] = u.dk;
            strcpy(s[3][stop], nam(&u,i));
        }

        if (u.timeon >= low_time_on_list) {
            low_time_on_list = timeon_per_user[9];
            for (i1 = 0; i1 < 10; i1++)
                if (u.timeon >= timeon_per_user[i1]) {
                    stop = i1;
                    i1 = 10;
                }
            for (i1 = 9; i1 > stop; i1--) {
                timeon_per_user[i1] =
                    timeon_per_user[i1-1];
                strcpy(s[4][i1], s[4][i1-1]);
            }
            timeon_per_user[stop] = u.timeon;
            strcpy(s[4][stop], nam(&u,i));
        }
    }

    sprintf(s1,"%stopten.dat",sys.cfg.datadir);
    i=open(s1,O_BINARY|O_RDWR|O_TRUNC|O_CREAT,S_IREAD|S_IWRITE);
    write(i,&s[0],5*10*31);
    write(i,&posts_per_user[0],10*sizeof(posts_per_user[0]));
    write(i,&logons_per_user[0],10*sizeof(logons_per_user[0]));
    write(i,&up_k_per_user[0],10*sizeof(up_k_per_user[0]));
    write(i,&dn_k_per_user[0],10*sizeof(dn_k_per_user[0]));
    write(i,&timeon_per_user[0],10*sizeof(timeon_per_user[0]));
    close(i);

}

char *topten(int type)
{
    auto& sys = System::instance();
    userrec u;
    int i,i1,stop,num_users,ok;
    char s1[161],s2[161],s3[10],s4[10],sname[31],s[5][10][31],ch;
    static char retstr[41];


    int low_posts_on_list,
    low_logons_on_list,
    posts_per_user[10],
    logons_per_user[10];

    unsigned long low_upk_on_list,
    low_dnk_on_list,
    up_k_per_user[10],
    dn_k_per_user[10];

    float low_time_on_list,
    timeon_per_user[10];


    sprintf(s1,"%stopten.dat",sys.cfg.datadir);
    i=open(s1,O_BINARY|O_RDWR);
    if(i<0) {
        updtopten();
        i=open(s1,O_BINARY|O_RDWR);
        if(i<0) return("Ouch");
    }

    read(i,&s[0],5*10*31);
    read(i,&posts_per_user[0],10*sizeof(posts_per_user[0]));
    read(i,&logons_per_user[0],10*sizeof(logons_per_user[0]));
    read(i,&up_k_per_user[0],10*sizeof(up_k_per_user[0]));
    read(i,&dn_k_per_user[0],10*sizeof(dn_k_per_user[0]));
    read(i,&timeon_per_user[0],10*sizeof(timeon_per_user[0]));
    close(i);


    i=mci_topten_which();

    switch(mci_topten_type()) {
    case 0:
        strcpy(sname,s[0][i]);
        sprintf(s3,"%d",posts_per_user[i]);
        break;
    case 1:
        strcpy(sname,s[1][i]);
        sprintf(s3,"%d",logons_per_user[i]);
        break;
    case 2:
        strcpy(sname,s[2][i]);
        sprintf(s3,"%ld",up_k_per_user[i]);
        break;
    case 3:
        strcpy(sname,s[3][i]);
        sprintf(s3,"%ld",dn_k_per_user[i]);
        break;
    case 4:
        strcpy(sname,s[4][i]);
        sprintf(s3,"%-6.0f",timeon_per_user[i]);
        break;
    }

    if(type)
        strcpy(s1,s3);
    else
        strcpy(s1,sname);

    if(s1[0])
        strcpy(retstr,s1);
    return retstr;

}

char *get_date_filename()
{
    auto& sys = System::instance();
    struct date today;
    char *dats[]={
        "jan","fab","mar","apr","may","jun","jul","aug","sep","oct","nov","dec"        };
    static char s[MAX_PATH_LEN];

    getdate(&today);

    sprintf(s,"%stoday.%s",sys.cfg.gfilesdir,dats[today.da_mon-1]);
    return(s);
}

void today_history()
{

    char s[181],s1[10],s2[MAX_PATH_LEN],*p;
    FILE *f;
    FILE *fbak;
    struct date today;
    int ok=0,day,h;
    char dfname[MAX_PATH_LEN];

    getdate(&today);
    strcpy(dfname,get_date_filename());

    outchr(12);
    npr("[� Famous Happenings for Today (%s) �]\r\n",date());

    /* PRINT BIRTHDAYS FOR CURRENT DAY */   
    npr("Birthdays\n\r");
    if ((f=fopen(dfname,"r"))!=NULL) {
        while ((fgets(s,80,f)) != NULL) {
            filter(s,'\n');

            sprintf(s1,"%c%c\n",s[3],s[4]);
            day=atoi(s1);

            if(day==today.da_day) ok=1;
            else ok=0;

            if(s[0]=='B' && ok==1) {
                p=strtok(&s[5]," ");
                npr("3%s ",p);
                p=strtok(NULL,"");
                npr("3%s\r\n",p);
            }
        }  
        fclose(f);
    }


    /* PRINT EVENTS FOR CURRENT DAY */   
    nl(); 
    nl();
    npr("Events\n\r");
    if ((f=fopen(dfname,"r"))!=NULL) {
        while ((fgets(s,80,f)) != NULL) {
            filter(s,'\n');

            sprintf(s1,"%c%c\n",s[3],s[4]);
            day=atoi(s1);

            if(day==today.da_day) ok=1;
            else ok=0; 

            if(s[0]=='S' && ok==1) {
                if(s[5]=='C') {
                    outstr("    ");
                    strcpy(p,&s[6]);
                } 
                else {
                    p=strtok(&s[5]," ");
                    npr("3%s ",p);
                    p=strtok(NULL,"");
                }
                npr("3%s\r\n",p);
            }
        }
        fclose(f);
    }
}

void dtitle(char msg[MAX_PATH_LEN])
{
    auto& sys = System::instance();
    FILE *f;
    char s[MAX_PATH_LEN],b1,b2,s1[MAX_PATH_LEN],bs2[MAX_PATH_LEN],bs1[MAX_PATH_LEN];
    char msg_stripped[MAX_PATH_LEN];
    int i;

    sprintf(s,"%sbox.fmt",sys.cfg.gfilesdir);
    f=fopen(s,"rt");
    if (!f) { pl(msg); return; }
    noc(msg_stripped,msg);
    fgets(s,4,f);
    b1=s[0];
    b2=s[1];
    for(i=0;i<strlenc(msg_stripped);i++) {
        bs1[i]=b1;
        bs2[i]=b2;
    }
    bs1[i]=0;
    bs2[i]=0;
    while(fgets(s,81,f)!=NULL) {
        filter(s,'\n');
        stuff_in(s1,s,msg,bs1,bs2,"","");
        plfmt(s1);
    }
    fclose(f);
}


void selfValidationCheck(char *param)
{
    char *p;
    char s[MAX_PATH_LEN];

    p=strtok(param,",");
    nl();
    inputdat("Enter Self-Validation password",s,31,0);
    if(!s[0])
        return;
    if(stricmp(p,s)==0) {
        p=strtok(NULL,"");
        set_autoval(atoi(p)-1);
        pl("Self-Validation was successful");
        logtypes(2,"User Self-Validated to level %d",atoi(p));
        changedsl();
    } 
    else {
        nl();
        pl("7Incorrect Password");
        logtypes(2,"Illegal Self-Validation attempt");
    }
    nl();
}

char *getComputerType(int which)
{
    auto& sys = System::instance();
    static char s[MAX_PATH_LEN];
    char s1[MAX_PATH_LEN];
    FILE *f;
    int i=0;

    sprintf(s1,"%sctype.txt",sys.cfg.gfilesdir);
    f=fopen(s1,"rt");
    if (!f) { strcpy(s, "Unknown"); return(s); }
    while(fgets(s,81,f)!=NULL&&i++<which);
    fclose(f);
    filter(s,'\n');
    return(s);
}

int numComputerTypes(void)
{
    auto& sys = System::instance();
    char s1[MAX_PATH_LEN];
    FILE *f;
    int i=0;

    sprintf(s1,"%sctype.txt",sys.cfg.gfilesdir);
    f=fopen(s1,"rt");
    if (!f) return(0);
    while(fgets(s1,81,f)!=NULL) i++;
    fclose(f);
    return(i);
}

