#include "vars.h"

#pragma hdrstop

#include "json_io.h"

extern char commstr[41];


void reset_files(int show)
{
    (void)show;
    userdb_rebuild_index();
    status.users = userdb_user_count();
    save_status();
}


void get_status()
{
    char s[MAX_PATH_LEN];
    cJSON *st_root;

    sprintf(s,"%sstatus.json",syscfg.datadir);
    st_root = read_json_file(s);
    if (st_root) {
        json_to_statusrec(st_root, &status);
        cJSON_Delete(st_root);
    } 
    else
        save_status();
}

void read_new_stuff()
{
    int i;
    char s[MAX_PATH_LEN];

    read_in_file("mnudata.dat",(menus),50);
}


void chuser()
{
    char s[MAX_PATH_LEN];
    int i;

    if (!checkpw())
        return;
    prt(2,"User to change to? ");
    input(s,30);
    i=finduser1(s);
    if (i>0) {
        userdb_save(usernum,&thisuser);
        userdb_load(i,&thisuser);
        usernum=i;
        actsl=255;
        logtypes(3,"Changed to 4%s",nam(&thisuser,usernum));
        topscreen();
    } 
    else
        pl("Unknown user.");
}

void zlog()
{
    zlogrec z;
    char s[MAX_PATH_LEN];
    int abort,f,i,i1;

    sprintf(s,"%shistory.dat",syscfg.datadir);
    f=open(s,O_RDWR | O_BINARY);
    if (f<0)
        return;
    i=0;
    abort=0;
    read(f,(void *)&z,sizeof(zlogrec));
    pla("0  Date     Calls  Active   Posts   Email   Fback    U/L   D/L   %Act   T/user",&abort);
    pla("7��������   �����  ������   �����   �����   �����    ���   ���   �����  ������0",&abort);
    while ((i<230) && (!abort) && (!hangup) && (z.date[0]!=0)) {
        if (z.calls)
            i1=z.active/z.calls;
        else
            i1=0;
        sprintf(s,"%s    %4d    %4d    %4d    %4d     %4d    %3d   %3d     %3d     %3d",
        z.date,z.calls,z.active,z.posts,z.email,z.fback,z.up,z.dl,10*z.active/144,i1);
        pla(s,&abort);
        ++i;
        if (i<230) {
            lseek(f,((long) i) * sizeof(zlogrec),SEEK_SET);
            read(f,(void *)&z,sizeof(zlogrec));
        }
    }
    close(f);
}

void beginday()
{
    char s[255],s1[MAX_PATH_LEN];
    zlogrec z,z1;
    int f,i,i1;

    double fk;
    int    nus;


    pl("Updating Logs");
    logpr("");
    logpr("1�1>0Totals for 7%s1<1�",status.date1); 
    logpr("   0Calls             1: 7%d",status.callstoday); 
    logpr("   0Message Posted    1: 7%d",status.msgposttoday);
    logpr("   0FeedBack          1: 7%d",status.fbacktoday);
    logpr("   0Uploads           1: 7%d",status.uptoday);
    logpr("   0Downloads         1: 7%d",status.dltoday);
    logpr("   0Total Time Active 1: 7%d",status.activetoday);
    logpr("");
    strcpy(z.date,status.date1);
    z.active=status.activetoday;
    z.calls=status.callstoday;
    z.posts=status.msgposttoday;
    z.email=status.emailtoday;
    z.fback=status.fbacktoday;
    z.up=status.uptoday;
    z.dl=status.dltoday;
    sprintf(s,"%shistory.dat",syscfg.datadir);
    f=open(s,O_RDWR|O_BINARY);
    lseek(f,(nifty.systemtype-1)*sizeof(zlogrec),SEEK_SET);
    read(f,&z1,sizeof(z1));
    close(f);
    s[0]=z1.date[6];
    s[1]=z1.date[7];
    s[2]=z1.date[0];
    s[3]=z1.date[1];
    s[4]=z1.date[3];
    s[5]=z1.date[4];
    s[6]=0;
    sprintf(s1,"%s%s.log",syscfg.gfilesdir,s);
    unlink(s1);

    status.callstoday=0;
    status.msgposttoday=0;
    status.emailtoday=0;
    status.fbacktoday=0;
    status.uptoday=0;
    status.dltoday=0;
    status.activetoday=0;
    strcpy(status.date1,date());
    sl1(2,date());
    sprintf(s,"%suser.log",syscfg.gfilesdir);
    unlink(s);
    save_status();
    sprintf(s,"%shistory.dat",syscfg.datadir);
    f=open(s,O_RDWR | O_BINARY);
    if (f<0) {
        f=open(s,O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
        z1.date[0]=0;
        z1.active=0;
        z1.calls=0;
        z1.posts=0;
        z1.email=0;
        z1.fback=0;
        z1.up=0;
        z1.dl=0;
        for (i=0; i<230; i++)
            write(f,(void *)&z1,sizeof(zlogrec));
    } 
    else {
        for (i=230; i>=1; i--) {
            lseek(f,(long) ((i-1) * sizeof(zlogrec)),SEEK_SET);
            read(f,(void *)&z1,sizeof(zlogrec));
            lseek(f,(long) (i * sizeof(zlogrec)),SEEK_SET);
            write(f,(void *)&z1,sizeof(zlogrec));
        }
    }
    lseek(f,0L,SEEK_SET);
    write(f,(void *)&z,sizeof(zlogrec));
    close(f);
    pl("Updating Top Ten");
    updtopten();
    pl("Sorting File Areas");
    sort_all("GA");

    if (syscfg.beginday_c[0]) {
        stuff_in(s,syscfg.beginday_c,create_chain_file("CHAIN.TXT"),"","","","");
        runprog(s,0);
    }

    fk=freek1(syscfg.datadir);
    nus=syscfg.maxusers-userdb_user_count();

    if (fk<512.0) {
        sprintf(s,"2! 0Only %dk free in data directory.",(int) fk);
        ssm(1,0,s);
        logpr(s);
    }

    fk=freek1(syscfg.batchdir);

    if (fk<2048.0) {
        sprintf(s,"2! 0Only %dk free in batch directory.",(int) fk);
        ssm(1,0,s);
        logpr(s);
    }

    if ((!syscfg.closedsystem) && (nus<15)){
        sprintf(s,"7!!>0Only %d new user slots left.",nus);
        ssm(1,0,s);
        logpr(s);
    }
}




void print_local_file(char ss[MAX_PATH_LEN])
{
    char s[MAX_PATH_LEN];

    if ((syscfg.sysconfig & sysconfig_list) &&!incom) {
        sprintf(s,"LST %s%s",syscfg.gfilesdir,ss);
        if(searchpath("LST.EXE")!=NULL) {
            runprog(s,0);
            if(!wfc)
                topscreen();
            return;
        }
    }

    printfile(ss);
    nl();
    pausescr();
}

void text_edit()
{
    /*  char s[MAX_PATH_LEN],s1[MAX_PATH_LEN];
        
          nl();
          npr("3Filename\r\n5: ");
          mpl(71);
          input(s,71);
          if (strstr(s,".log")!=NULL)
            s[0]=0;
          if (s[0]) {
            logpr("5@0 Edited 4%s",s);
              tedit(s);
          }*/
}

void viewlog()
{
    char s[15],s1[MAX_PATH_LEN];
    int i,f;
    zlogrec z;

    inputdat("View Logs from How Many Days Ago? (Enter=Today)",s,4,0);
    i=atoi(s);
    if(!i) {
        sl1(3,s);
        strcpy(s1,s);
        print_local_file(s1);
    } 
    else {
        sprintf(s1,"%shistory.dat",syscfg.datadir);
        f=open(s1,O_BINARY|O_RDWR);
        i-=1;
        lseek(f,(i*sizeof(zlogrec)),SEEK_SET);
        read(f,&z,sizeof(zlogrec));
        close(f);
        s[0]=z.date[6];
        s[1]=z.date[7];
        s[2]=z.date[0];
        s[3]=z.date[1];
        s[4]=z.date[3];
        s[5]=z.date[4];
        s[6]=0;
        sprintf(s1,"%s.log",s);
        print_local_file(s1);
    }
}


void glocolor(void)
{
    userrec u;
    int x,uu,i;
    char s[MAX_PATH_LEN];

    int sl=-1,dsl=-1,color=-1,prot=-1,format=-1,address=0,resetpc=0,fixneg;
    int resetptr=-1;

    outstr("Global Sl: "); 
    input(s,3); 
    if(s[0]) sl=atoi(s);
    outstr("Global Dsl: "); 
    input(s,3); 
    if(s[0]) dsl=atoi(s);
    outstr("5Do Colors? "); 
    color=yn();
    outstr("5Global Protocol? "); 
    prot=yn();
    outstr("5Reset File Format? "); 
    format=yn();
    outstr("5Reset Address? "); 
    address=yn();
    outstr("5Reset Computer Types? "); 
    resetpc=yn();
    outstr("5Reset Message Pointers? "); 
    resetptr=yn();
    outstr("5Fix negative DL/UL/Fpts? "); 
    fixneg=yn();
    outstr("5Continue? "); 
    if(!yn()) return;

    userdb_save(usernum,&thisuser);

    dtitle("Setting Users to: ");
    if(sl!=-1)     npr("Sl    : %d\r\n",sl);
    if(dsl!=-1)    npr("Dsl   : %d\r\n",dsl);
    if(prot)       pl("Reseting Protocol");
    if(format)     pl("Reseting Format");
    if(address)    pl("Reseting Addresses");
    if(color)      pl("Reseting Colors");
    if(resetptr)   pl("Reseting Message Ponters");
    if(fixneg)     pl("Fixing negative DL/UL/Fpts");

    for(x=1;x<=userdb_user_count();x++) {
        userdb_load(x,&u);
        if(fixneg) {
            if((long)u.dk<0)
                u.dk=0;
            if((long)u.uk<0)
                u.uk=0;
            if(u.fpts<0)
                u.fpts=0;
        }
        if(sl!=-1) u.sl=sl;
        if(dsl!=-1) u.dsl=dsl;
        if(color) {
            setcolors(&u);
            u.sysstatus ^= sysstatus_ansi;
            u.sysstatus ^= sysstatus_color;
            u.sysstatus ^= sysstatus_avatar;
        }
        if(format) u.flisttype=99;
        if(prot) u.defprot=99;
        if(resetpc) u.comp_type=99;
        u.lastsub=0;
        u.lastdir=0;
        u.lastconf=1;
        if(address) {
            u.city[0]=0;
            u.street[0]=0;
        }
        if(resetptr) {
            for(i=0;i<200;i++)
                u.qscn[i]=0;
        }
        userdb_save(x,&u);
    }

    userdb_load(usernum,&thisuser);

    reset_files(1);
}



