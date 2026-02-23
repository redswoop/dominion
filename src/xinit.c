#include "io_ncurses.h"
#include "vars.h"

#pragma hdrstop

#include <math.h>
#include <dirent.h>
#include "ansi.h"
#include "json_io.h"
#include "menudb.h"
#include "terminal_bridge.h"

extern char menuat[15];


#define GODOWN(x,y) gotoxy(y+1,x+1);

void bargraph1(int percent)
{
    int x;
    for(x=0;x<percent/2;x++)
        cprintf("%c",219);
}

void dotopinit(char fn[40],int per)
{
    int i;

    if(per);

    /*      GODOWN(1,3);
              for(i=0;i<48;i++) printf(" ");
              GODOWN(1,3);
              textattr(11);
              cprintf("Reading");
              textattr(15);
              cprintf(" %s",fn);
              GODOWN(2,3);
              bargraph1(per);*/
    textattr(1);
    cprintf("1 ");
    textattr(9);
    cprintf("Reading %s\r\n",fn);
}

void init(int show)
{
    char s[161],*buf,ch,*ss,s1[MAX_PATH_LEN],s2[10];
    int i,i1,i2,sm,cp,n,f;
    long l;
    union REGS r;
    struct date today;
    int resaveconfig=0;
    //  votingrec v;

    /* Allocate screen buffer (80x25x2 = 4000 bytes, char+attr pairs) */
    scrn=(char *)malloc(4000);
    if (!scrn) {
        printf("\n\nFailed to allocate screen buffer!\n\n");
        exit(1);
    }
    memset(scrn, 0, 4000);
    crttype=3; /* color text mode */
    defscreenbottom=24;
    screenbottom=defscreenbottom;
    screenlen=160*(screenbottom+1);

    /* Initialize default text attribute to white-on-black */
    curatr=0x07;

    /* Initialize ncurses via Terminal class */
    nc_active = term_init_local();
    if (scrn)
        term_set_screen_buffer(scrn);
    if (!nc_active)
        setvbuf(stdout, NULL, _IOLBF, 0);
    term_raw_mode = 1;

    /* Bind Terminal state pointers to BBS io_session_t fields.
     * After this, both sides share the same memory — no sync needed. */
    term_bind_state(&curatr, &topline, &screenbottom);

    if(!exist("exitdata.dom")) restoring_shrink=0; 
    else restoring_shrink=1;
    if (!restoring_shrink&&!show) {
        clrscr();
        memmove(scrn,ANSIHEADER,4000);
        term_render_scrn(0, 25);
        gotoxy(1,12);
    }
    getcwd(cdir, sizeof(cdir));
    configfile=-1;
    statusfile=-1;
    dlf=-1;
    curlsub=-1;
    curldir=-1;
    oldx=0;
    oldy=0;
    itimer();
    use_workspace=0;
    chat_file=0;
    do_event=0;
    sysop_alert=0;
    global_handle=0;


    getdate(&today);
    if (today.da_year<1993) {
        printf("\r\nYou need to set the date & time before running the BBS.\n");
        err(7,"","In Init()");
    }

    if(!restoring_shrink&&!show) {
        //      GODOWN(2,3);
        //      bargraph(0);
        dotopinit("config.json",10);
    }

    {
        cJSON *cfg_root = read_json_file("config.json");
        if (!cfg_root) {
            printf("\n\nconfig.json, Main Configuration File, Not Found!!.\n");
            err(1,"config.json","In Init()");
        }
        json_to_configrec(cfg_root, &syscfg, &nifty);
        cJSON_Delete(cfg_root);
    }

    if (!syscfg.primaryport && !tcp_port)
        ok_modem_stuff=0;

    if(!restoring_shrink&&!show)
        dotopinit("Checking Directories",15);


    for(i=0;i<6;i++) {
        switch(i) {
        case 0: 
            strcpy(s2,"Data"); 
            strcpy(s,syscfg.datadir); 
            break;
        case 1: 
            strcpy(s2,"Afiles"); 
            strcpy(s,syscfg.gfilesdir); 
            break;
        case 2: 
            strcpy(s2,"Temp"); 
            strcpy(s,syscfg.tempdir); 
            break;
        case 3: 
            strcpy(s2,"Msgs"); 
            strcpy(s,syscfg.msgsdir); 
            break;
        case 4: 
            strcpy(s2,"Batch"); 
            strcpy(s,syscfg.batchdir); 
            break;
        case 5: 
            strcpy(s2,"Menus"); 
            strcpy(s,syscfg.menudir); 
            break;
        }

        sprintf(s1,"%snul",s);

        if (!exist(s1)) {
            printf("\n\nYour %s directory isn't valid!  Now set to: %s\n",s2,s);
            printf("\nEnter new value: ");
            gets(s);
            printf("%s\n",s);
            if(s[strlen(s)-1]!='\\')
                strcat(s,"\\");
            printf("%s\n",s);
            sprintf(s1,"%snul",s);
            printf("%s\n",s1);
            if(!exist(s1)) {
                strcpy(s1,s);
                s1[strlen(s1)-1]=0;
                if(mkdir(s1))
                    err(1,s1,"In Init()");
            }
            switch(i) {
            case 0: 
                strcpy(syscfg.datadir,s); 
                break;
            case 1: 
                strcpy(syscfg.gfilesdir,s); 
                break;
            case 2: 
                strcpy(syscfg.tempdir,s); 
                break;
            case 3: 
                strcpy(syscfg.msgsdir,s); 
                break;
            case 4: 
                strcpy(syscfg.batchdir,s); 
                break;
            case 5: 
                strcpy(syscfg.menudir,s); 
                break;
            }
            resaveconfig=1;
        }
    }

    if(resaveconfig) {
        cJSON *cfg_root = configrec_to_json(&syscfg, &nifty);
        write_json_file("config.json", cfg_root);
        cJSON_Delete(cfg_root);
    }


    if(!restoring_shrink&&!show)
        dotopinit("Status.dat",20);

    sprintf(s,"%sstatus.json",syscfg.datadir);
    {
        cJSON *st_root = read_json_file(s);
        if (!st_root) {
            printf("\n\n\n%sstatus.json not found!\n\n",syscfg.datadir);
            err(1,s,"In Init()");
        }
        json_to_statusrec(st_root, &status);
        cJSON_Delete(st_root);
    }
    status.wwiv_version=wwiv_num_version;
    userdb_init(syscfg.datadir, syscfg.maxusers);
    menudb_init(syscfg.menudir);
    status.users = userdb_user_count();

    screensave.scrn1=(char *)mallocx(screenlen);

    read_in_file("mnudata.dat",(menus),50);

    subboards=(subboardrec *) mallocx(200*sizeof(subboardrec));
    directories=(directoryrec *)mallocx(200*sizeof(directoryrec));
    if(!restoring_shrink&&!show)
        dotopinit("config.dat",40);

    sprintf(s,"%ssubs.dat",syscfg.datadir);
    i=open(s,O_RDWR | O_BINARY);
    if (i<0) {
        printf("\n\n%ssubs.dat not found!",syscfg.datadir);
        err(1,s,"In Init()");
    }
    num_subs=(read(i,subboards, (200*sizeof(subboardrec))))/
        sizeof(subboardrec);
    if(subboards[0].postacs[0])
        subboards[0].postacs[0]=0;
    if(subboards[0].readacs[0])
        subboards[0].readacs[0]=0;
    if(!(subboards[0].attr & mattr_private)) {
        togglebit((long *)&subboards[0].attr,mattr_private);
        strcpy(subboards[0].name,"Private Mail");
        strcpy(subboards[0].filename,"email");
        subboards[0].conf='@';
    }
    lseek(i,0L,SEEK_SET);
    write(i,&subboards[0],sizeof(subboardrec));
    close(i);

    if(!restoring_shrink&&!show) {
        dotopinit("Subs.dat",50);
    }

    sprintf(s,"%sdirs.dat",syscfg.datadir);
    i=open(s,O_RDWR | O_BINARY);
    if (i<0) {
        printf("\n\n%sdirs.dat not found!\n\n",syscfg.datadir);
        err(1,s,"In Init()");
    }

    num_dirs=(read(i,directories, (200*sizeof(directoryrec))))/
        sizeof(directoryrec);
    close(i);

    if(!restoring_shrink&&!show)
        dotopinit("Protocol.dat",60);

    sprintf(s,"%sprotocol.dat",syscfg.datadir);
    i=open(s,O_RDWR | O_BINARY);
    if(i<0) {
        printf("\n\n%sProtocol.dat Not Found\n",syscfg.datadir);
        err(1,s,"In Init()");
    }
    numextrn=(read(i,(void *)&proto,20*sizeof(protocolrec)))/
        sizeof(protocolrec);
    close(i);


    if(!restoring_shrink&&!show) {
        dotopinit("Conf.dat",70);
    }

    sprintf(s,"%sconf.dat",syscfg.datadir);
    i=open(s,O_RDWR | O_BINARY);
    if(i<0) {
        printf("\n\n%sConf.dat Not Found!\n\n",syscfg.datadir);
        err(1,s,"In Init()");
    }
    num_conf=(read(i,(void *)&conf[0],20*sizeof(confrec)))/ sizeof(confrec);
    if(conf[0].sl[0])
        conf[0].sl[0]=0;
    close(i);

    sprintf(s,"%sarchive.dat",syscfg.datadir);
    i=open(s,O_BINARY|O_RDWR);
    if(i<0) {
        printf("\n\n%sArchive.dat Not Found\n",syscfg.datadir);
        err(1,s,"In Init()");
    }
    read(i,&xarc[0],8*sizeof(xarc[0]));
    close(i);



    /*  sprintf(s,"%sVOTING.DAT",syscfg.datadir);
          f=open(s,O_RDWR | O_BINARY);
          if (f>0) {
            n=(int) (filelength(f) / sizeof(votingrec)) -1;
            for (i=0; i<n; i++) {
              lseek(f,(long) i * sizeof(votingrec),SEEK_SET);
              read(f,(void *)&v,sizeof(votingrec));
              if (v.numanswers)
                questused[i]=1;
            }
            close(f);
          }*/

    userdb_load(1,&thisuser);
    cursub=0;
    fwaiting=numwaiting(&thisuser);

    sl1(2,status.date1);

    if (ok_modem_stuff)
        initport(syscfg.primaryport);
    if (syscfg.sysconfig & sysconfig_no_local)
        topdata=0;
    else
        topdata=status.net_edit_stuff;

    ss=getenv("PROMPT");
    strcpy(newprompt,"PROMPT=BBS: ");
    if (ss)
        strcat(newprompt,ss);
    else
        strcat(newprompt,"$P$G");
    sprintf(dszlog,"%s\\BBSDSZ.LOG",cdir);
    sprintf(s,"DSZLOG=%s",dszlog);
    i=i1=0;
    while (environ[i]!=NULL) {
        if (strncmp(environ[i],"PROMPT=",7)==0)
            xenviron[i1++]=newprompt;
        else
            if (strncmp(environ[i],"DSZLOG=",7)==0)
            xenviron[i1++]=strdup(s);
        else {
            if (strncmp(environ[i],"BBS=",4) && (strncmp(environ[i],"WWIV_FP=",8)))
                xenviron[i1++]=environ[i];
        }
        ++i;
    }
    if (!getenv("DSZLOG"))
        xenviron[i1++]=strdup(s);
    if (!ss)
        xenviron[i1++]=newprompt;

    sprintf(s,"BBS=%s",wwiv_version);
    xenviron[i1++]=s;

    xenviron[i1]=NULL;

    for (i=0; i<20; i++)
        questused[i]=0;

    if(!restoring_shrink&&!show)
        dotopinit("Final Data",95);


    time_event=((double)syscfg.executetime)*60.0;
    last_time=time_event-timer();
    if (last_time<0.0)
        last_time+=24.0*3600.0;
    do_event=0;
    if (status.callernum!=65535) {
        status.callernum1=(long)status.callernum;
        status.callernum=65535;
        save_status();
    }
    frequent_init();
    if (!restoring_shrink&&!show && !already_on) {
        remove_from_temp("*.*",syscfg.tempdir,0);
        remove_from_temp("*.*",syscfg.batchdir,0);
    }
    if(!restoring_shrink&&!show) menuat[0]=0;
    lecho=ok_local();
    quote=NULL;
    bquote=0;
    equote=0;

    daylight=0;

#ifdef MOUSE
    initpointer(1);
#endif

    if (!restoring_shrink&&!show) {
        /*      GODOWN(2,3);
                      bargraph(100);
                      GODOWN(1,3);*/
        cprintf("Completed Loading - ");
        textattr(10);
        cprintf("Entering WFC\n\n\n");
    }

}

void end_bbs(int lev)
{
    sl1(1,"");
    if (ok_modem_stuff) closeport();
    dtr(0);
    clrscrb();
    textattr(9);
    cprintf("\n ");
    textattr(15);
    cprintf("%s is outta here!\n\n",wwiv_version);
    _setcursortype(2);

    /* Restore terminal */
    term_shutdown();
    nc_active = 0;
    term_raw_mode = 0;

    exit(lev);
}


