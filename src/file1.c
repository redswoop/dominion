#include "vars.h"
#pragma hdrstop


#define SETREC(i)  lseek(dlf,((long) (i))*((long)sizeof(uploadsrec)),SEEK_SET);

void addtobatch(uploadsrec u,int dn,int sending);

void batrec(int rw, int bnum)
{
    int f;
    char s[MAX_PATH_LEN];

    sprintf(s,"%sBATREC.DAT",syscfg.datadir);
    f=open(s,O_RDWR | O_CREAT | O_BINARY,S_IREAD | S_IWRITE);
    lseek(f,(bnum)*(long)sizeof(batchrec),SEEK_SET);
    switch (rw) {
    case 0: 
        write(f,(void *)(&batch),sizeof(batchrec));
        break;
    case 1: 
        read(f,(void *)(&batch),sizeof(batchrec));
        break;
    }
    close(f);
}



void delbatch(int i)
{
    int f,i1;
    char s[MAX_PATH_LEN];

    if (i<numbatch) {
        batch.batchdesc[0]=0;
        batrec(1,i);
        batchtime -= batch.time;
        batchsize -= batch.size;
        if (batch.sending)
            --numbatchdl;
        --numbatch;
        if(batch.dir==-2) {
            sprintf(s,"%sTemp.%s",syscfg.tempdir,xarc[ARC_NUMBER].extension);
            unlink(s);
        }
        sprintf(s,"%sBATREC.DAT",syscfg.datadir);
        f=open(s,O_RDWR | O_CREAT | O_BINARY,S_IREAD | S_IWRITE);
        for (i1=i; i1<=numbatch; i1++) {
            lseek(f,(i1+1)*(long)sizeof(batchrec),SEEK_SET);
            read(f,(void *)(&batch),sizeof(batchrec));
            lseek(f,(i1)*(long)sizeof(batchrec),SEEK_SET);
            write(f,(void *)(&batch),sizeof(batchrec));
        }
        close(f);
    }
}

void downloaded(char *fn)
{
    int i,i1;
    uploadsrec u;
    userrec ur;
    char s[MAX_PATH_LEN];

    for (i1=0; i1<numbatch; i1++) {
        batch.batchdesc[0]=0;
        batrec(1,i1);
        if ((strcmp(fn,batch.filename)==0) && (batch.sending)) {
            dliscan1(batch.dir);
            i=recno((batch.filename));
            if (i>0) {
                SETREC(i);
                read(dlf,(void *)&u,sizeof(uploadsrec));
                if(!(directories[batch.dir].mask & mask_no_ratio)) {
                    ++thisuser.downloaded;
                    thisuser.dk += (int) ((u.numbytes+1023)/1024);
                    thisuser.fpts -= (int) ((u.numbytes+1023)/10240)/nifty.fptsratio;
                }
                ++u.numdloads;
                status.dltoday++;
                SETREC(i);
                write(dlf,(void *)&u,sizeof(uploadsrec));
                closedl();
                npr("2þ 4%s 0Succesfully Transfered. 4%4ldK0, 2%3d Points\r\n",u.filename,((u.numbytes+1023)/1024),u.points);
                sprintf(s,"%s downloaded '%s' on %s",nam(&thisuser,usernum), u.filename, date());
                ssm(u.ownerusr,0,s);
                read_user(u.ownerusr,&ur);
                ur.fpts+=nifty.fcom;
                write_user(u.ownerusr,&ur);
                sprintf(s,"\tYou recieved %d commision points",nifty.fcom);
                ssm(u.ownerusr,0,s);
                logtypes(4,"Downloaded %s from %s - %ldk",u.filename,directories[batch.dir].name,(u.numbytes+1023)/1024);
            }
            delbatch(i1);
            return;
        }
    }
    logtypes(3,"File '4%s0' was downloaded, but was not in Batch Queue!.",fn);
}


void upload_batch_file(int blind)
{
    int f,i,ok=1;
    uploadsrec u,u1;
    directoryrec d;
    char *ss,s[MAX_PATH_LEN],s1[MAX_PATH_LEN];
    long l,len;

    ss=NULL;

    outchr(12);
    npr("2þ 0Processing: 4%s\r\n",batch.filename);
    d=directories[batch.dir];
    dliscan1(batch.dir);
    time(&l);
    u.daten=l;
    strcpy(u.filename,batch.filename);
    strcpy(u.description,batch.batchdesc);
    u.ownerusr=usernum;
    u.ownersys=0;
    u.numdloads=0;
    u.filetype=0;
    u.mask=0;
    u.points=0;
    u.ats[0]=0;
    if(batch.extdesc) u.mask |= mask_extended;

    sprintf(s,"%s%s",syscfg.batchdir,stripfn(u.filename));

    checkhangup();

    if(adddiz(s,&u)) {
        u.mask |= mask_extended;
    } 
    else if (!hangup&&!(u.mask & mask_extended)&&blind) {
        modify_extended_description(&ss);
        if (ss) {
            add_extended_description(u.filename,ss);
            u.mask |= mask_extended;
            farfree(ss);
        }
    }

    strcpy(u.upby,nam(&thisuser,usernum));
    strcpy(u.date,date());


    pl(get_string2(37));
    if(finddup(u.filename,1)) {
        pl(get_string2(38));
        unlink(s);
        ok=0;
        logtypes(3,"File 4%s0 already exists, upload deleted.",u.filename);
    }

    dliscan1(batch.dir);

    if(ok) {
        pl(get_string2(36));
        if(testarc(stripfn(u.filename),syscfg.batchdir)!=0) {
            printf("Failed\n");
            sprintf(s,"%s%s",syscfg.batchdir,stripfn(u.filename));
            if(exist(s)) {
                batchdir=0;
                batch.dir=0;
                dliscan1(batch.dir);
                d=directories[batch.dir];
                logtypes(3,"File 4%s0 failed Integrity Test, Moving to SysOp Dir.",u.filename);
                pl("7þ 0Error in Upload.  Sending to SysOp Directory");
                ok=2;
            } 
            else {
                logtypes(3,"File 4%s0 failed Integrity Test, deleted.",u.filename);
                pl("7þ 0Error in upload, deleted by test.");
                ok=0;
            }
        }
    }

    if(ok) {
        if(ok==1) {
            pl(get_string2(33));
            comment_arc(stripfn(u.filename),syscfg.batchdir,d.upath);
            if(strstr(u.filename,".GIF")) {
                pl(get_string2(34));
                addgif(&u,syscfg.batchdir);
            }
        }

        f=open(s,O_RDONLY | O_BINARY);
        u.numbytes=filelength(f);
        close(f);

        for (i=numf; i>=1; i--) {
            SETREC(i);
            read(dlf,(void *)&u1,sizeof(uploadsrec));
            SETREC(i+1);
            write(dlf,(void *)&u1,sizeof(uploadsrec));
        }
        SETREC(1);
        write(dlf,(void *)&u,sizeof(uploadsrec));
        ++numf;
        u1.numbytes=numf;
        u1.daten=l;
        if(ok==1) {
            ++thisuser.uploaded;
            thisuser.uk += (int) ((u.numbytes+1023)/1024);
            if((d.mask & mask_autocredit)) {
                thisuser.fpts += (int) ((u.numbytes+1023)/10240);
                u.ats[0]=1;
                u.points=(int) ((u.numbytes+1023)/10240);
            }
        }
        SETREC(0);
        write(dlf,(void *)&u1,sizeof(uploadsrec));

        ++status.uptoday;
        save_status();
        pl(get_string2(35));
        copyupfile(stripfn(batch.filename),d.dpath,syscfg.batchdir);
        logtypes(4,"Uploaded 9%s 0to 9%s 0- %ldk",u.filename,d.name,(u.numbytes+1023)/1024);
    }

    closedl();
    i=curdir;
    if (batchdir==0)
        curdir=0;
}

void uploaded(char *fn)
{
    int i,i1,done=0;
    char *ss,s[MAX_PATH_LEN];

    dliscan1(batchdir);
    for (i1=0; i1<numbatch; i1++) {
        batch.batchdesc[0]=0;
        batrec(1,i1);
        if ((strcmp(fn,batch.filename)==0) && (!batch.sending)) {
            i=recno(batch.filename);
            if (i==-1)
                upload_batch_file(0);
            closedl();
            delbatch(i1);
            return;
        }
    }


    strcpy(batch.filename,fn);
    batch.batchdesc[0]=0;
    while ((!hangup) && (batch.batchdesc[0]==0)) {
        sprintf(s,"Please Describe %s",fn);
        inputdat(s,batch.batchdesc,39,1);
    }

    batch.dir=0;

    if(!hangup)
    do {
        nl();
        dirlist(0);
        nl();
        npr("3Which area, (Enter) to send to SysOp: ");
        ss=smkey("?,",1,0,1,0);
        if(!ss[0]) {
            batch.dir=0;
            done=1;
        }
        nl();
        nl();
        for(i=0;i<num_dirs&&udir[i].subnum>=0;i++)
            if(strcmp(ss,udir[i].keys)==0) {
                if(!(directories[udir[i].subnum].mask & mask_no_uploads)) {
                    batch.dir=udir[i].subnum;
                    done=1;
                } 
                else {
                    nl();
                    pl(get_string2(32));
                    nl();
                }
            }
    } 
    while(!done&&!hangup);



    if (batch.batchdesc[0]==0) {
        strcpy(batch.batchdesc,"ÄÄÄÄÄÄÄ[User Hung Up]ÄÄÄÄÄÄÄ");
        sprintf(s,"%s needs a description!",fn);
        ssm(usernum,0,s);
    }
    upload_batch_file(1);
}




void handle_dszline(char *l)
{
    char *ss;
    int i;
    char s[161];

    ss=strtok(l," \t");
    for (i=0; (i<10) && (ss); i++)
        ss=strtok(NULL," \t");
    if (ss) {
        strcpy(s,stripfn(ss));
        align(s);

        switch(*l) {
        case 'Z':
        case 'X':
        case 'R':
        case 'B':
        case 'H':
            uploaded(s);
            break;

        case 'z':
        case 'x':
        case 'b':
        case 'S':
        case 'Q':
        case 'h':
            downloaded(s);
            break;

        case 'E':
        case 'e':
        case 'L':
        case 'U':
            logtypes(3,"Error transferring '4%s0'",ss);
            lpr(get_string2(31),s);
            break;
        }
    }
}

void process_dszlog()
{
    FILE *f;
    char s[140];

    outchr(12);
    if ((f=fopen(dszlog,"rt"))!=0) {
        while (fgets(s,138,f))
            handle_dszline(s);
        fclose(f);
    }
}

void batchul(int t)
{
    int f,i;
    char s[MAX_PATH_LEN],s1[MAX_PATH_LEN],s2[MAX_PATH_LEN],sx1[40],sx2[40],sx3[40];
    double ti;
    long l;
    directoryrec d;

    remove_from_temp("*.*",syscfg.batchdir,0);

    ti=timer();
    strcpy(s1,proto[t].receivebatch);

    _chmod(dszlog,1,0);
    unlink(dszlog);
    unlink("batch.lst");

    ultoa(com_speed,sx1,10);
    ultoa(modem_speed,sx3,10);
    sx2[0]='0'+syscfg.primaryport;
    sx2[1]=0;
    sprintf(s2,"%s\\BATCH.LST",cdir);
    stuff_in(s,s1,sx1,sx2,"",sx3,s2);

    pl(get_string(70));
    cd_to(syscfg.batchdir);

    savescreen(&screensave);
    clrscr();
    printf("[0;33;44;1m[K%s Is Uploading[0;1m\n\n",nam(&thisuser,usernum));
    printf("%s\r\n",s);
    runprog(s,exist("lowlife"));
    restorescreen(&screensave);
    process_dszlog();
    topscreen();
    ti=timer()-ti;
    if (ti<0) ti += 24.0*3600.0;
    thisuser.extratime += ti;
}

void batchdl(int t)
{
    int ok,f,i;
    char s[MAX_PATH_LEN],s1[MAX_PATH_LEN],sx1[40],sx2[40],sx3[40],s2[80];


    if (nsl()<=batchtime&&!(thisuser.exempt & exempt_time)) {
        nl();
        pl(get_string(66));
        batchdled(1);
        if(nsl()<=batchtime&&!(thisuser.exempt & exempt_time))
            return;
    }

    i=0;
    for(f=0;f<numbatch;f++) {
        batrec(1,f);
        if(batch.sending) i+=batch.points;
    }

    if((thisuser.fpts<i)&&!(thisuser.exempt & exempt_ratio)&&(nifty.nifstatus & nif_fpts)) {
        nl();
        pl(get_string(65));
        batchdled(1);
        i=0;
        for(f=0;f<numbatch;f++) {
            batrec(1,f);
            if(batch.sending) i+=batch.points;
        }
        if((thisuser.fpts<i)&&!(thisuser.exempt & exempt_ratio)&&(nifty.nifstatus & nif_fpts))
            return;
    }



    sprintf(s,"%s\\BATCH.LST",cdir);
    unlink(s);
    f=open(s,O_RDWR | O_BINARY| O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);
    if (f<0) {
        logtypes(3,"Unable to create transfer list.");
        pl("Cannot Transfer, Sorry");
        return;
    }

    for (i=0; i<numbatch; i++) {
        batch.batchdesc[0]=0;
        batrec(1,i);
        if (batch.sending) {
            if(batch.dir!=-2)
                sprintf(s1,"%s%s\r\n",directories[batch.dir].dpath,stripfn(batch.filename));
            else
                sprintf(s1,"%s%s\r\n",syscfg.tempdir,stripfn(batch.filename));
            write(f,s1,strlen(s1));
        }
    }

    close(f);
    strcpy(s1,proto[t].sendbatch);

    ultoa(com_speed,sx1,10);
    ultoa(modem_speed,sx3,10);
    sx2[0]='0'+syscfg.primaryport;
    sx2[1]=0;
    sprintf(s2,"%s\\BATCH.LST",cdir);
    stuff_in(s,s1,sx1,sx2,"",sx3,s2);
    _chmod(dszlog,1,0);
    pl(get_string(70));

    unlink(dszlog);
    savescreen(&screensave);
    clrscr();
    cd_to(syscfg.batchdir);
    printf("[0;44;1m[K%s Is Downloading\n[0;1m\n\n",nam(&thisuser,usernum));
    printf("%s\r\n",s);
    cd_to(syscfg.batchdir);
    runprog(s,exist("Lowlife"));
    cd_to(cdir);
    restorescreen(&screensave);
    process_dszlog();
    topscreen();
}

void newdl(int dn)
{
    download(dn,0);
}

void mark(int dn)
{
    download(dn,1);
}

int findfile(int dn,char s[15])
{
    int i,i2;
    uploadsrec u;

    dliscan1(dn);
    if(atoi(s)) {
        SETREC(atoi(s));
        read(dlf,(void *)&u,sizeof(uploadsrec));
        if(checkdl(u,dn)) addtobatch(u,dn,1);
        else {
            npr("\r\n7%s0 Rejected\r\n\r\n",u.filename);
            logtypes(4,"Tried to dl 4%s0, but was rejected",u.filename);
        }
        closedl();
        return 1;
    }
    i=recno(s);
    i2=0;
    while(i!=-1) {
        i2=1;
        SETREC(i);
        read(dlf,(void *)&u,sizeof(uploadsrec));
        if(checkdl(u,dn)) addtobatch(u,dn,1);
        else {
            npr("\r\n7%s 0Rejected\r\n\r\n",u.filename);
            logtypes(3,"Tried to dl 4%s0, but was rejected",u.filename);
        }
        i=nrecno(s,i);
    }
    closedl();
    return i2;
}


void printbatchstat(int dl)
{
    if(dl) printfile("dstat"); 
    else printfile("dstat");
}


void sendbatch(void)
{
    int protocol,had;

    if(numbatch<1)
        return;
    nl();
    protocol=get_batchprotocol(1,&had);
    if(protocol==-1)
        return;
    else
        batchdl(protocol);
    if(had)
        verify_hangup();
}


void download(int dn,int mark)
{
    char *p;
    char s[162],s1[162];
    int i,i1,i2,abort=0,next=0;
    uploadsrec u;


    batchdir=udir[dn].subnum;

    menubatch("download");

    pl(get_string(63));
    mpl(78);
    input(s,78);

    if (!s[0]&&numbatch==0)
        return;
    strcpy(s1,s);
    p=strtok(s1," ,;");
    while (p) {
        i1=0;
        strcpy(s,p);
        if (!okfn(s))
            break;
        if (strchr(s,'.')==NULL)
            strcat(s,".*");
        align(s);
        i1=0;
        i2=findfile(batchdir,s);
        if(!i2) {
            pl(get_string2(26));
            for(i1=0;i1<umaxdirs&&!abort;i1++) {
                checka(&abort,&next,0);
                findfile(udir[i1].subnum,s);
            }
        }
        p=strtok(NULL," ,;");
    }

    if(numbatch<1) return;
    printbatchstat(1);

    if(!mark)
        sendbatch();
}



void newul(int dn)
{
    char *p,*ss;
    char s[162],s1[162];
    int i,i2,ok=1;
    uploadsrec u;
    directoryrec d;
    double t;
    long l;

    if(dn==-1)
        dn=udir[curdir].subnum;
    batchdir=dn;
    d=directories[batchdir];
    dliscan1(batchdir);

    if (numf>=d.maxfiles) {
        nl();
        pl("This directory is currently full.");
        nl();
        closedl();
        return;
    }

    if (d.mask & mask_no_uploads) {
        nl();
        pl("Uploads are not allowed to this directory.");
        nl();
        closedl();
        return;
    }

    closedl();

    l=(long)freek1(syscfg.batchdir);
    sprintf(s,"Upload - %ldk free.",l);
    dtitle(s);
    nl();
    if(thisuser.helplevel==2) printfile("Ulhelp");
    pl(get_string(62));
    mpl(78);
    input(s,78);

    if (s[0]) {

        strcpy(s1,s);

        p=strtok(s1," ,;");
        while (p) {
            strcpy(s,p);

            if(!okfn(s)) break;
            align(s);

            ok=1;

            if (d.mask & mask_archive) {
                ok=0;
                s1[0]=0;
                for (i=0; i<4; i++) {
                    if (xarc[i].extension[0] && xarc[i].extension[0]!=' ') {
                        if (s1[0])
                            strcat(s1,", ");
                        strcat(s1,xarc[i].extension);
                        if (strcmp(s+9,xarc[i].extension)==0)
                            ok=1;
                    }
                }

                if (!ok) {
                    nl();
                    pl("Sorry, all uploads to this directory must be Archived");
                    pl("Supported types are:");
                    pl(s1);
                    break;
                }
            }

            if(ok) {
                strcpy(batch.filename,s);
                if(finddup(batch.filename,0)) {
                    npr("8Sorry, you cannot upload that file\r\n");
                    nl();
                    break;
                }
                ss=NULL;
                pl(get_string(75));
                npr("3%s3: ",s);
                mpl(39);
                inputl(s,39);
                if(s[0]=='/') {
                    strcpy(batch.batchdesc,s+1);
                    batch.dir=0;
                } 
                else {
                    batch.dir=batchdir;
                    strcpy(batch.batchdesc,s);
                }
                batch.sending=0;
                modify_extended_description(&ss);
                if(ss) {
                    batch.extdesc=1;
                    dliscan1(batch.dir);
                    add_extended_description(batch.filename,ss);
                    farfree(ss);
                    closedl();
                }
                batrec(0,numbatch);
                numbatch++;
                p=strtok(NULL," ,;");
            }
        }

        if(numbatch==0) return;
    }

    printbatchstat(0);
    nl();
    i=get_batchprotocol(1,&i2);
    if(i==-1) return;
    else batchul(i);
    if(i2) verify_hangup();
}

void addtobatch(uploadsrec u,int dn,int sending)
{
    double t;
    int i;

    for(i=0;i<numbatch;i++) {
        batch.batchdesc[0]=0;
        batrec(1,i);
        if(strcmp(u.filename,batch.filename)==0) {
            if(batch.dir==-2) {
                t=((double) (((batch.len)+127)/128)) * (1620.0) /
                    ((double) (modem_speed));
                batchtime -= t;
                batchsize -= ((batch.len+1023L) /1024L);

                t=((double) (((u.numbytes)+127)/128)) * (1620.0) /
                    ((double) (modem_speed));
                batchtime += t;
                batchsize += ((u.numbytes+1023L) /1024L);
                strcpy(batch.filename,u.filename);
                batch.dir=dn;
                batch.time=t;
                batch.sending=sending;
                batch.len=u.numbytes;
                if(!(directories[dn].mask & mask_no_ratio))
                    batch.points=u.points;
                else
                    batch.points=0;
                batch.size=((u.numbytes+1023L) /1024L);
                batrec(0,i);
                return;
            } 
            else {
                pl("8File Already in batch queue!");
                return;
            }
        }
    }
    t=((double) (((u.numbytes)+127)/128)) * (1620.0) /
        ((double) (modem_speed));
    batchtime += t;
    batchsize += ((u.numbytes+1023L) /1024L);
    strcpy(batch.filename,u.filename);
    batch.dir=dn;
    batch.time=t;
    batch.sending=sending;
    batch.len=u.numbytes;
    if(!(directories[dn].mask & mask_no_ratio))
        batch.points=u.points;
    else
        batch.points=0;
    batch.size=((u.numbytes+1023L) /1024L);
    batrec(0,numbatch);
    numbatch++;
    numbatchdl++;
}

void upload(char ms[41])
{
    int dir;


    menubatch("upload");

    dir=-1;

    if(syscfg.newuploads==255); 
    else dir=syscfg.newuploads;

    if(thisuser.restrict & restrict_upload) dir=0;

    if(syscfg.sysconfig & sysconfig_all_sysop) dir=0;

    if(dir==-1) {
        if(ms[0]=='?') {
            if(!changefarea())
                return;
        }
    }

    newul(dir);
}
