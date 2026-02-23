#include "file1.h"
#include "platform.h"
#include "fcns.h"
#include "conio.h"
#include "file.h"
#include "file2.h"
#include "file3.h"
#include "archive.h"
#include "filesys.h"
#include "bbsutl.h"
#include "disk.h"
#include "utility.h"
#include "timest.h"
#include "utility1.h"
#include "mm1.h"
#include "stringed.h"
#include "session.h"
#include "system.h"
#include "extrn.h"
#include "misccmd.h"
#include "sysopf.h"
#pragma hdrstop


#define SETREC(i)  lseek(sess.dlf,((long) (i))*((long)sizeof(uploadsrec)),SEEK_SET);


void addtobatch(uploadsrec u,int dn,int sending);

void batrec(int rw, int bnum)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    int f;
    char s[MAX_PATH_LEN];

    sprintf(s,"%sbatrec.dat",sys.cfg.datadir);
    f=open(s,O_RDWR | O_CREAT | O_BINARY,S_IREAD | S_IWRITE);
    lseek(f,(bnum)*(long)sizeof(batchrec),SEEK_SET);
    switch (rw) {
    case 0: 
        write(f,(void *)(&sess.batch),sizeof(batchrec));
        break;
    case 1: 
        read(f,(void *)(&sess.batch),sizeof(batchrec));
        break;
    }
    close(f);
}


void delbatch(int i)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    int f,i1;
    char s[MAX_PATH_LEN];

    if (i<sess.numbatch) {
        sess.batch.batchdesc[0]=0;
        batrec(1,i);
        sess.batchtime -= sess.batch.time;
        sess.batchsize -= sess.batch.size;
        if (sess.batch.sending)
            --sess.numbatchdl;
        --sess.numbatch;
        if(sess.batch.dir==-2) {
            sprintf(s,"%sTemp.%s",sys.cfg.tempdir,sys.xarc[sys.ARC_NUMBER].extension);
            unlink(s);
        }
        sprintf(s,"%sbatrec.dat",sys.cfg.datadir);
        f=open(s,O_RDWR | O_CREAT | O_BINARY,S_IREAD | S_IWRITE);
        for (i1=i; i1<=sess.numbatch; i1++) {
            lseek(f,(i1+1)*(long)sizeof(batchrec),SEEK_SET);
            read(f,(void *)(&sess.batch),sizeof(batchrec));
            lseek(f,(i1)*(long)sizeof(batchrec),SEEK_SET);
            write(f,(void *)(&sess.batch),sizeof(batchrec));
        }
        close(f);
    }
}

void downloaded(char *fn)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    int i,i1;
    uploadsrec u;
    userrec ur;
    char s[MAX_PATH_LEN];

    for (i1=0; i1<sess.numbatch; i1++) {
        sess.batch.batchdesc[0]=0;
        batrec(1,i1);
        if ((strcmp(fn,sess.batch.filename)==0) && (sess.batch.sending)) {
            dliscan1(sess.batch.dir);
            i=recno((sess.batch.filename));
            if (i>0) {
                SETREC(i);
                read(sess.dlf,(void *)&u,sizeof(uploadsrec));
                if(!(sys.directories[sess.batch.dir].mask & mask_no_ratio)) {
                    ++sess.user.downloaded;
                    sess.user.dk += (int) ((u.numbytes+1023)/1024);
                    sess.user.fpts -= (int) ((u.numbytes+1023)/10240)/sys.nifty.fptsratio;
                }
                ++u.numdloads;
                sys.status.dltoday++;
                SETREC(i);
                write(sess.dlf,(void *)&u,sizeof(uploadsrec));
                closedl();
                npr("2� 4%s 0Succesfully Transfered. 4%4ldK0, 2%3d Points\r\n",u.filename,((u.numbytes+1023)/1024),u.points);
                sprintf(s,"%s downloaded '%s' on %s",nam(&sess.user,sess.usernum), u.filename, date());
                ssm(u.ownerusr,0,s);
                userdb_load(u.ownerusr,&ur);
                ur.fpts+=sys.nifty.fcom;
                userdb_save(u.ownerusr,&ur);
                sprintf(s,"\tYou recieved %d commision points",sys.nifty.fcom);
                ssm(u.ownerusr,0,s);
                logtypes(4,"Downloaded %s from %s - %ldk",u.filename,sys.directories[sess.batch.dir].name,(u.numbytes+1023)/1024);
            }
            delbatch(i1);
            return;
        }
    }
    logtypes(3,"File '4%s0' was downloaded, but was not in Batch Queue!.",fn);
}


void upload_batch_file(int blind)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    int f,i,ok=1;
    uploadsrec u,u1;
    directoryrec d;
    char *ss,s[MAX_PATH_LEN],s1[MAX_PATH_LEN];
    long l,len;

    ss=NULL;

    outchr(12);
    npr("2� 0Processing: 4%s\r\n",sess.batch.filename);
    d=sys.directories[sess.batch.dir];
    dliscan1(sess.batch.dir);
    time(&l);
    u.daten=l;
    strcpy(u.filename,sess.batch.filename);
    strcpy(u.description,sess.batch.batchdesc);
    u.ownerusr=sess.usernum;
    u.ownersys=0;
    u.numdloads=0;
    u.filetype=0;
    u.mask=0;
    u.points=0;
    u.ats[0]=0;
    if(sess.batch.extdesc) u.mask |= mask_extended;

    sprintf(s,"%s%s",sys.cfg.batchdir,stripfn(u.filename));

    checkhangup();

    if(adddiz(s,&u)) {
        u.mask |= mask_extended;
    } 
    else if (!io.hangup&&!(u.mask & mask_extended)&&blind) {
        modify_extended_description(&ss);
        if (ss) {
            add_extended_description(u.filename,ss);
            u.mask |= mask_extended;
            free(ss);
        }
    }

    strcpy(u.upby,nam(&sess.user,sess.usernum));
    strcpy(u.date,date());


    pl(get_string2(37));
    if(finddup(u.filename,1)) {
        pl(get_string2(38));
        unlink(s);
        ok=0;
        logtypes(3,"File 4%s0 already exists, upload deleted.",u.filename);
    }

    dliscan1(sess.batch.dir);

    if(ok) {
        pl(get_string2(36));
        if(testarc(stripfn(u.filename),sys.cfg.batchdir)!=0) {
            printf("Failed\n");
            sprintf(s,"%s%s",sys.cfg.batchdir,stripfn(u.filename));
            if(exist(s)) {
                sess.batchdir=0;
                sess.batch.dir=0;
                dliscan1(sess.batch.dir);
                d=sys.directories[sess.batch.dir];
                logtypes(3,"File 4%s0 failed Integrity Test, Moving to SysOp Dir.",u.filename);
                pl("7� 0Error in Upload.  Sending to SysOp Directory");
                ok=2;
            } 
            else {
                logtypes(3,"File 4%s0 failed Integrity Test, deleted.",u.filename);
                pl("7� 0Error in upload, deleted by test.");
                ok=0;
            }
        }
    }

    if(ok) {
        if(ok==1) {
            pl(get_string2(33));
            comment_arc(stripfn(u.filename),sys.cfg.batchdir,d.upath);
            if(strstr(u.filename,".GIF")) {
                pl(get_string2(34));
                addgif(&u,sys.cfg.batchdir);
            }
        }

        f=open(s,O_RDONLY | O_BINARY);
        u.numbytes=filelength(f);
        close(f);

        for (i=sess.numf; i>=1; i--) {
            SETREC(i);
            read(sess.dlf,(void *)&u1,sizeof(uploadsrec));
            SETREC(i+1);
            write(sess.dlf,(void *)&u1,sizeof(uploadsrec));
        }
        SETREC(1);
        write(sess.dlf,(void *)&u,sizeof(uploadsrec));
        ++sess.numf;
        u1.numbytes=sess.numf;
        u1.daten=l;
        if(ok==1) {
            ++sess.user.uploaded;
            sess.user.uk += (int) ((u.numbytes+1023)/1024);
            if((d.mask & mask_autocredit)) {
                sess.user.fpts += (int) ((u.numbytes+1023)/10240);
                u.ats[0]=1;
                u.points=(int) ((u.numbytes+1023)/10240);
            }
        }
        SETREC(0);
        write(sess.dlf,(void *)&u1,sizeof(uploadsrec));

        ++sys.status.uptoday;
        save_status();
        pl(get_string2(35));
        copyupfile(stripfn(sess.batch.filename),d.dpath,sys.cfg.batchdir);
        logtypes(4,"Uploaded 9%s 0to 9%s 0- %ldk",u.filename,d.name,(u.numbytes+1023)/1024);
    }

    closedl();
    i=sess.curdir;
    if (sess.batchdir==0)
        sess.curdir=0;
}

void uploaded(char *fn)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    int i,i1,done=0;
    char *ss,s[MAX_PATH_LEN];

    dliscan1(sess.batchdir);
    for (i1=0; i1<sess.numbatch; i1++) {
        sess.batch.batchdesc[0]=0;
        batrec(1,i1);
        if ((strcmp(fn,sess.batch.filename)==0) && (!sess.batch.sending)) {
            i=recno(sess.batch.filename);
            if (i==-1)
                upload_batch_file(0);
            closedl();
            delbatch(i1);
            return;
        }
    }


    strcpy(sess.batch.filename,fn);
    sess.batch.batchdesc[0]=0;
    while ((!io.hangup) && (sess.batch.batchdesc[0]==0)) {
        sprintf(s,"Please Describe %s",fn);
        inputdat(s,sess.batch.batchdesc,39,1);
    }

    sess.batch.dir=0;

    if(!io.hangup)
    do {
        nl();
        dirlist(0);
        nl();
        npr("3Which area, (Enter) to send to SysOp: ");
        ss=smkey("?,",1,0,1,0);
        if(!ss[0]) {
            sess.batch.dir=0;
            done=1;
        }
        nl();
        nl();
        for(i=0;i<sys.num_dirs&&sess.udir[i].subnum>=0;i++)
            if(strcmp(ss,sess.udir[i].keys)==0) {
                if(!(sys.directories[sess.udir[i].subnum].mask & mask_no_uploads)) {
                    sess.batch.dir=sess.udir[i].subnum;
                    done=1;
                } 
                else {
                    nl();
                    pl(get_string2(32));
                    nl();
                }
            }
    } 
    while(!done&&!io.hangup);


    if (sess.batch.batchdesc[0]==0) {
        strcpy(sess.batch.batchdesc,"�������[User Hung Up]�������");
        sprintf(s,"%s needs a description!",fn);
        ssm(sess.usernum,0,s);
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
    auto& sess = Session::instance();
    FILE *f;
    char s[140];

    outchr(12);
    if ((f=fopen(sess.dszlog,"rt"))!=0) {
        while (fgets(s,138,f))
            handle_dszline(s);
        fclose(f);
    }
}

void batchul(int t)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    int f,i;
    char s[MAX_PATH_LEN],s1[MAX_PATH_LEN],s2[MAX_PATH_LEN],sx1[40],sx2[40],sx3[40];
    double ti;
    long l;
    directoryrec d;

    remove_from_temp("*.*",sys.cfg.batchdir,0);

    ti=timer();
    strcpy(s1,sys.proto[t].receivebatch);

    _chmod(sess.dszlog,1,0);
    unlink(sess.dszlog);
    unlink("batch.lst");

    ultoa(sess.com_speed,sx1,10);
    ultoa(sess.modem_speed,sx3,10);
    sx2[0]='0'+sys.cfg.primaryport;
    sx2[1]=0;
    sprintf(s2,"%s/BATCH.LST",sys.cdir);
    stuff_in(s,s1,sx1,sx2,"",sx3,s2);

    pl(get_string(70));
    cd_to(sys.cfg.batchdir);

    savescreen(&sess.screensave);
    clrscr();
    printf("[0;33;44;1m[K%s Is Uploading[0;1m\n\n",nam(&sess.user,sess.usernum));
    printf("%s\r\n",s);
    runprog(s,exist("lowlife"));
    restorescreen(&sess.screensave);
    process_dszlog();
    topscreen();
    ti=timer()-ti;
    if (ti<0) ti += 24.0*3600.0;
    sess.user.extratime += ti;
}

void batchdl(int t)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    int ok,f,i;
    char s[MAX_PATH_LEN],s1[MAX_PATH_LEN],sx1[40],sx2[40],sx3[40],s2[80];


    if (nsl()<=sess.batchtime&&!(sess.user.exempt & exempt_time)) {
        nl();
        pl(get_string(66));
        batchdled(1);
        if(nsl()<=sess.batchtime&&!(sess.user.exempt & exempt_time))
            return;
    }

    i=0;
    for(f=0;f<sess.numbatch;f++) {
        batrec(1,f);
        if(sess.batch.sending) i+=sess.batch.points;
    }

    if((sess.user.fpts<i)&&!(sess.user.exempt & exempt_ratio)&&(sys.nifty.nifstatus & nif_fpts)) {
        nl();
        pl(get_string(65));
        batchdled(1);
        i=0;
        for(f=0;f<sess.numbatch;f++) {
            batrec(1,f);
            if(sess.batch.sending) i+=sess.batch.points;
        }
        if((sess.user.fpts<i)&&!(sess.user.exempt & exempt_ratio)&&(sys.nifty.nifstatus & nif_fpts))
            return;
    }


    sprintf(s,"%s/BATCH.LST",sys.cdir);
    unlink(s);
    f=open(s,O_RDWR | O_BINARY| O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);
    if (f<0) {
        logtypes(3,"Unable to create transfer list.");
        pl("Cannot Transfer, Sorry");
        return;
    }

    for (i=0; i<sess.numbatch; i++) {
        sess.batch.batchdesc[0]=0;
        batrec(1,i);
        if (sess.batch.sending) {
            if(sess.batch.dir!=-2)
                sprintf(s1,"%s%s\r\n",sys.directories[sess.batch.dir].dpath,stripfn(sess.batch.filename));
            else
                sprintf(s1,"%s%s\r\n",sys.cfg.tempdir,stripfn(sess.batch.filename));
            write(f,s1,strlen(s1));
        }
    }

    close(f);
    strcpy(s1,sys.proto[t].sendbatch);

    ultoa(sess.com_speed,sx1,10);
    ultoa(sess.modem_speed,sx3,10);
    sx2[0]='0'+sys.cfg.primaryport;
    sx2[1]=0;
    sprintf(s2,"%s/BATCH.LST",sys.cdir);
    stuff_in(s,s1,sx1,sx2,"",sx3,s2);
    _chmod(sess.dszlog,1,0);
    pl(get_string(70));

    unlink(sess.dszlog);
    savescreen(&sess.screensave);
    clrscr();
    cd_to(sys.cfg.batchdir);
    printf("[0;44;1m[K%s Is Downloading\n[0;1m\n\n",nam(&sess.user,sess.usernum));
    printf("%s\r\n",s);
    cd_to(sys.cfg.batchdir);
    runprog(s,exist("Lowlife"));
    cd_to(sys.cdir);
    restorescreen(&sess.screensave);
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
    auto& sess = Session::instance();
    int i,i2;
    uploadsrec u;

    dliscan1(dn);
    if(atoi(s)) {
        SETREC(atoi(s));
        read(sess.dlf,(void *)&u,sizeof(uploadsrec));
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
        read(sess.dlf,(void *)&u,sizeof(uploadsrec));
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
    auto& sess = Session::instance();
    int protocol,had;

    if(sess.numbatch<1)
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
    auto& sess = Session::instance();
    char *p;
    char s[162],s1[162];
    int i,i1,i2,abort=0,next=0;
    uploadsrec u;


    sess.batchdir=sess.udir[dn].subnum;

    menubatch("download");

    pl(get_string(63));
    mpl(78);
    input(s,78);

    if (!s[0]&&sess.numbatch==0)
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
        i2=findfile(sess.batchdir,s);
        if(!i2) {
            pl(get_string2(26));
            for(i1=0;i1<sess.umaxdirs&&!abort;i1++) {
                checka(&abort,&next,0);
                findfile(sess.udir[i1].subnum,s);
            }
        }
        p=strtok(NULL," ,;");
    }

    if(sess.numbatch<1) return;
    printbatchstat(1);

    if(!mark)
        sendbatch();
}


void newul(int dn)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    char *p,*ss;
    char s[162],s1[162];
    int i,i2,ok=1;
    uploadsrec u;
    directoryrec d;
    double t;
    long l;

    if(dn==-1)
        dn=sess.udir[sess.curdir].subnum;
    sess.batchdir=dn;
    d=sys.directories[sess.batchdir];
    dliscan1(sess.batchdir);

    if (sess.numf>=d.maxfiles) {
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

    l=(long)freek1(sys.cfg.batchdir);
    sprintf(s,"Upload - %ldk free.",l);
    dtitle(s);
    nl();
    if(sess.user.helplevel==2) printfile("ulhelp");
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
                    if (sys.xarc[i].extension[0] && sys.xarc[i].extension[0]!=' ') {
                        if (s1[0])
                            strcat(s1,", ");
                        strcat(s1,sys.xarc[i].extension);
                        if (strcmp(s+9,sys.xarc[i].extension)==0)
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
                strcpy(sess.batch.filename,s);
                if(finddup(sess.batch.filename,0)) {
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
                    strcpy(sess.batch.batchdesc,s+1);
                    sess.batch.dir=0;
                } 
                else {
                    sess.batch.dir=sess.batchdir;
                    strcpy(sess.batch.batchdesc,s);
                }
                sess.batch.sending=0;
                modify_extended_description(&ss);
                if(ss) {
                    sess.batch.extdesc=1;
                    dliscan1(sess.batch.dir);
                    add_extended_description(sess.batch.filename,ss);
                    free(ss);
                    closedl();
                }
                batrec(0,sess.numbatch);
                sess.numbatch++;
                p=strtok(NULL," ,;");
            }
        }

        if(sess.numbatch==0) return;
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
    auto& sys = System::instance();
    auto& sess = Session::instance();
    double t;
    int i;

    for(i=0;i<sess.numbatch;i++) {
        sess.batch.batchdesc[0]=0;
        batrec(1,i);
        if(strcmp(u.filename,sess.batch.filename)==0) {
            if(sess.batch.dir==-2) {
                t=((double) (((sess.batch.len)+127)/128)) * (1620.0) /
                    ((double) (sess.modem_speed));
                sess.batchtime -= t;
                sess.batchsize -= ((sess.batch.len+1023L) /1024L);

                t=((double) (((u.numbytes)+127)/128)) * (1620.0) /
                    ((double) (sess.modem_speed));
                sess.batchtime += t;
                sess.batchsize += ((u.numbytes+1023L) /1024L);
                strcpy(sess.batch.filename,u.filename);
                sess.batch.dir=dn;
                sess.batch.time=t;
                sess.batch.sending=sending;
                sess.batch.len=u.numbytes;
                if(!(sys.directories[dn].mask & mask_no_ratio))
                    sess.batch.points=u.points;
                else
                    sess.batch.points=0;
                sess.batch.size=((u.numbytes+1023L) /1024L);
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
        ((double) (sess.modem_speed));
    sess.batchtime += t;
    sess.batchsize += ((u.numbytes+1023L) /1024L);
    strcpy(sess.batch.filename,u.filename);
    sess.batch.dir=dn;
    sess.batch.time=t;
    sess.batch.sending=sending;
    sess.batch.len=u.numbytes;
    if(!(sys.directories[dn].mask & mask_no_ratio))
        sess.batch.points=u.points;
    else
        sess.batch.points=0;
    sess.batch.size=((u.numbytes+1023L) /1024L);
    batrec(0,sess.numbatch);
    sess.numbatch++;
    sess.numbatchdl++;
}

void upload(char ms[41])
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    int dir;


    menubatch("upload");

    dir=-1;

    if(sys.cfg.newuploads==255); 
    else dir=sys.cfg.newuploads;

    if(sess.user.restrict & restrict_upload) dir=0;

    if(sys.cfg.sysconfig & sysconfig_all_sysop) dir=0;

    if(dir==-1) {
        if(ms[0]=='?') {
            if(!changefarea())
                return;
        }
    }

    newul(dir);
}
