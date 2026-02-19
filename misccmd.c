#include "vars.h"

#pragma hdrstop

void read_automessage()
{
    int i;
    messagerec m;
    FILE *f;
    char s[161];

    sprintf(s,"%sauto.fmt",syscfg.gfilesdir);
    f=fopen(s,"rt");
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
    hdrinfo hdr;
    int save,i;
    long len;
    char *b,s[81];

    if(thisuser.restrict & restrict_automessage) {
        pl("7You are restricted from changing the AutoMessage");
        pausescr();
        return;
    }

    upload_post();
    b=ninmsg(&hdr,&len,&save,0);
    if(save) {
        status.amsganon=0;
        status.amsguser=usernum;
        logtypes(2,"Changed Automessage");
        sprintf(s,"%sauto.msg",syscfg.gfilesdir);
        unlink(s);
        i=open(s,O_BINARY|O_CREAT|O_TRUNC,S_IREAD|S_IWRITE);
        write(i,b,len);
        close(i);
        farfree(b);
    }
}


void addbbs(char *fn)
{
    int i,i1,i2,f,ok;
    char s[150],s1[150],s2[150],ch,ch1,*ss,s3[40],s4[40],final[150],form[40];
    long l,l1;
    FILE *format;

    if(thisuser.restrict & restrict_bbslist) {
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
        sprintf(s1,"%s%s",syscfg.gfilesdir,fn);

        f=open(s1,O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE);
        if (f>0) {
            lseek(f,0L,SEEK_SET);
            l=filelength(f);
            if ((ss=malloca(l+500L))==NULL) {
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
            farfree(ss);
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

            sprintf(form,"%sbbslist.fmt",syscfg.gfilesdir);
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
                sprintf(s1,"%s%s",syscfg.gfilesdir,fn);
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
    FILE *f;
    char s[150],s1[20],s2[150];


    nl();
    /*    npr("3Enter Text to Search For\r\n5: ");
                mpl(20);
                input(s1,20);*/
    inputdat("Enter Text to Search For",s1,20,0);
    if(!s1[0]) return;

    sprintf(s,"%s%s",syscfg.gfilesdir,fn);
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
    subboardrec s;
    userrec u;
    int i,nu,abort,ok,num;
    char st[161];

    if (usub[cursub].subnum==-1||(thisuser.restrict & restrict_userlist)) {
        nl();
        pl("Sorry, you cannot currently view the user list");
        nl();
        return;
    }
    s=subboards[usub[cursub].subnum];
    nl();
    pl("Users with access to current sub:");
    nl();
    npr("0%-30s 1[0%-40s1]\r\n","Users Name","Comment");
    abort=0;
    num=0;
    for (i=0; (i<status.users) && (!abort) && (!hangup); i++) {
        read_user(smallist[i].number,&u);
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
            npr("3%-30s 1[0%-40s1]\r\n",nam(&u,smallist[i].number),u.comment);
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
    char s[81];
    originrec or;

    outchr(12);
    dtitle("Your Information");

    npr("0Your Handle is 5%s0, Your Voice Phone Number is 5%s0\r\n",nam(&thisuser,usernum),thisuser.phone);
    npr("Your Acting Security Level is 5%d0, And Your Transfer Level is 5%d0\r\n",actsl,thisuser.dsl);
    nl();
    npr("You have Downloaded 5%d0 files, And Uploaded 5%d0 files\r\n",thisuser.downloaded,thisuser.uploaded);
    npr("You have posted 5%d0 times, and Called 5%d0 times\r\n",thisuser.msgpost,thisuser.logons);
    nl();
    npr("You have 5%d0 minutes left for this call\r\n",(int)((nsl()+30)/60.0));
    sprintf(s,"0, And have been on 5%d0 times today.",thisuser.ontoday);
    npr("You last called 5%s%s0\r\n",thisuser.laston,thisuser.ontoday?s:"");
    nl();
    npr("System is 5%s\r\n",wwiv_version);
    getorigin(subboards[usub[cursub].subnum].origin,&or);
    if(or.add.zone)
        npr("Network is %s, Address %d:%d/%d\r\n",or.netname,or.add.zone,or.add.net,or.add.node);
    nl();
    if(thisuser.helplevel==2)
        pausescr();
}


void jumpconf(char ms[41])
{
    char c,s[81];
    int i,ok,type=0;

    if(atoi(ms)) {
        if(slok(conf[atoi(ms)].sl,0)) {
            curconf=atoi(ms);
            changedsl();
            return;
        }
    } 
    else if(ms[0]) type=ms[0];

    dtitle("Conferences Available: ");
    for(c=0;c<num_conf;c++) {
        ok=1;
        if(!slok(conf[c].sl,0)) ok=0;
        if(type=='M'||type=='F')
            if(conf[c].type!=type&&conf[c].type)
                ok=0;
        if(ok)
            npr("0<%d> %s\r\n",c+1,conf[c].name);
    }
    nl();
    outstr("Select: ");
    mpl(2);
    input(s,2);
    i=atoi(s);
    if(i==0) return;
    i--;
    if(i>num_conf) return;
    if(slok(conf[i].sl,0)) curconf=i;
    else {
        pl("Unavailable Conference");
        curconf=0;
        return;
    }
    changedsl();
}

void add_time(int limit)
{
    int minutes;
    char s[81];
    double nsln;

    nsln=nsl();
    npr("0Time in Bank: 5%d\r\n",thisuser.timebank);
    npr("0Bank Limit  : 5%d\r\n",limit);
    nl();
    npr("5%.0f0 minutes left online.\r\n",nsln/60.0);
    npr("How many minutes would you like to deposit? ");
    mpl(3);
    ansic(4);
    inputl(s,3);
    minutes = atoi(s);
    if (minutes<1) return;
    if (hangup) return;
    if (minutes > (int)((nsl() - 20.0)/60.0)) {
        nl();
        prt(7, "You do not have enough time left to deposit that much.");
        nl();
        pausescr();
        return;
    }
    if ((minutes + thisuser.timebank) > limit) {
        nl();
        npr("7You may only have up to %d minutes in your account at once.\r\n", limit);
        nl();
        pausescr();
        return;
    }
    thisuser.timebank += minutes;
    write_user(usernum,&thisuser);
    logtypes(2,"Deposit 3%d0 minutes.", minutes);
    nl();
    npr("%d minute%c deposited.", minutes,((minutes > 1) ? 's' : 0));
    nl();

    if (extratimecall > 0) {
        if (extratimecall >= (double)(minutes * 60)) {
            extratimecall -= (double)(minutes * 60);
            return;
        }
        minutes -= (int)(extratimecall / 60.0);
        extratimecall = 0.0;
    }
    thisuser.extratime -= (float)(minutes * 60);
    pausescr();
}

void remove_time()
{
    char s[81];
    int minutes;

    nl(); 
    nl();
    npr( "Time in account: %d minutes.\r\n", thisuser.timebank);
    nl();
    ansic(0);
    outstr("How many minutes would you like to withdraw? ");
    ansic(4);
    inputl(s,3);
    minutes = atoi(s);
    if (minutes<1) return;

    if (hangup) return;
    if (minutes > thisuser.timebank) {
        nl(); 
        nl();
        prt(7, "You don't have that much time in the account!");
        nl();
        nl();
        pausescr();
        return;
    }
    logtypes(2,"Withdrew 3%d0 minutes.", minutes);
    thisuser.extratime += (float)(minutes * 60);
    thisuser.timebank -= minutes;
    nl(); 
    nl();
    npr("4%d minute%c withdrawn.", minutes,((minutes > 1) ? 's' :0));
    nl();
    pausescr();
}

void bank2(int limit)
{
    int done = 0;
    char s[81], ch;

    logtypes(2,"Entered TimeBank");
    do {
        dtitle("Dominion Time Bank");
        nl();
        tleft(0);
        npr("0Time in Bank: 5%d\r\n",thisuser.timebank);
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
            if(thisuser.restrict & restrict_timebank) {
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
    while ((!hangup) && (!done));
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

extern int whichten,toptentype;

void updtopten(void)
{
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

    num_users = number_userrecs();

    for (i = 0; i < 5; i++)
        for (i1 = 0; i1 < 10; i1++)
            strcpy(s[i][i1], "None");

    for (i = 1; i <= num_users; i++) {
        read_user(i, &u);
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

    sprintf(s1,"%stopten.dat",syscfg.datadir);
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


    sprintf(s1,"%stopten.dat",syscfg.datadir);
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


    i=whichten;

    switch(toptentype) {
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
    struct date today;
    char *dats[]={
        "jan","fab","mar","apr","may","jun","jul","aug","sep","oct","nov","dec"        };
    static char s[81];

    getdate(&today);

    sprintf(s,"%stoday.%s",syscfg.gfilesdir,dats[today.da_mon-1]);
    return(s);
}

void today_history()
{

    char s[181],s1[10],s2[81],*p;
    FILE *f;
    FILE *fbak;
    struct date today;
    int ok=0,day,h;
    char dfname[81];

    getdate(&today);
    strcpy(dfname,get_date_filename());

    outchr(12);
    npr("[ð Famous Happenings for Today (%s) ð]\r\n",date());

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

void dtitle(char msg[81])
{
    FILE *f;
    char s[81],b1,b2,s1[81],bs2[81],bs1[81];
    int i;

    sprintf(s,"%sbox.fmt",syscfg.gfilesdir);
    f=fopen(s,"rt");
    strcpy(s,msg);
    noc(msg,s);
    fgets(s,4,f);
    b1=s[0];
    b2=s[1];
    for(i=0;i<strlenc(msg);i++) {
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


void selfval(char *param)
{
    char *p;
    char s[81];

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

char *ctype(int which)
{
    static char s[81];
    char s1[81];
    FILE *f;
    int i=0;

    sprintf(s1,"%sctype.txt",syscfg.gfilesdir);
    f=fopen(s1,"rt");
    while(fgets(s,81,f)!=NULL&&i++<which);
    fclose(f);
    filter(s,'\n');
    return(s);
}

int numctypes(void)
{
    char s1[81];
    FILE *f;
    int i=0;

    sprintf(s1,"%sctype.txt",syscfg.gfilesdir);
    f=fopen(s1,"rt");
    while(fgets(s1,81,f)!=NULL) i++;
    fclose(f);
    return(i);
}

