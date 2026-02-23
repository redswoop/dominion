#include "io_ncurses.h"
#include "xinit.h"
#include "platform.h"
#include "fcns.h"
#include "bbsutl.h"
#include "timest.h"
#include "disk.h"
#include "utility.h"
#include "jam_bbs.h"
#include "config.h"
#include "wfc.h"
#include "session.h"
#include "system.h"
#include "version.h"
#include "error.h"

#pragma hdrstop

#include <math.h>
#include <dirent.h>
#include "ansi.h"
#include "json_io.h"
#include "menudb.h"
#include "terminal_bridge.h"

/* sess.menuat now in vars.h (Phase B0) */


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
    auto& sys = System::instance();
    auto& sess = Session::instance();
    char s[161],*buf,ch,*ss,s1[MAX_PATH_LEN],s2[10];
    int i,i1,i2,sm,cp,n,f;
    long l;
    struct date today;
    int resaveconfig=0;
    //  votingrec v;

    /* Allocate screen buffer (80x25x2 = 4000 bytes, char+attr pairs) */
    io.scrn=(char *)malloc(4000);
    if (!io.scrn) {
        printf("\n\nFailed to allocate screen buffer!\n\n");
        exit(1);
    }
    memset(io.scrn, 0, 4000);

    io.defscreenbottom=24;
    io.screenbottom=io.defscreenbottom;
    io.screenlen=160*(io.screenbottom+1);

    /* Initialize default text attribute to white-on-black */
    io.curatr=0x07;

    /* Initialize ncurses via Terminal class */
    nc_active = term_init_local();
    if (io.scrn)
        term_set_screen_buffer(io.scrn);
    if (!nc_active)
        setvbuf(stdout, NULL, _IOLBF, 0);
    io.term_raw_mode = 1;

    /* Bind Terminal state pointers to BBS io_session_t fields.
     * After this, both sides share the same memory — no sync needed. */
    term_bind_state(&io.curatr, &io.topline, &io.screenbottom);

    if(!exist("exitdata.dom")) sys.restoring_shrink=0; 
    else sys.restoring_shrink=1;
    if (!sys.restoring_shrink&&!show) {
        clrscr();
        memmove(io.scrn,ANSIHEADER,4000);
        term_render_scrn(0, 25);
        gotoxy(1,12);
    }
    getcwd(sys.cdir, sizeof(sys.cdir));
    sys.configfile=-1;
    sys.statusfile=-1;
    sess.dlf=-1;
    sess.curlsub=-1;
    sess.curldir=-1;
    io.oldx=0;
    io.oldy=0;
    itimer();
    sess.use_workspace=0;
    io.chat_file=0;
    sys.do_event=0;
    sess.sysop_alert=0;
    io.global_handle=0;


    getdate(&today);
    if (today.da_year<1993) {
        printf("\r\nYou need to set the date & time before running the BBS.\n");
        err(7,"","In Init()");
    }

    if(!sys.restoring_shrink&&!show) {
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
        json_to_configrec(cfg_root, &sys.cfg, &sys.nifty);
        cJSON_Delete(cfg_root);
    }

    if (!sys.cfg.primaryport && !sys.tcp_port)
        ok_modem_stuff=0;

    if(!sys.restoring_shrink&&!show)
        dotopinit("Checking Directories",15);


    for(i=0;i<6;i++) {
        switch(i) {
        case 0: 
            strcpy(s2,"Data"); 
            strcpy(s,sys.cfg.datadir); 
            break;
        case 1: 
            strcpy(s2,"Afiles"); 
            strcpy(s,sys.cfg.gfilesdir); 
            break;
        case 2: 
            strcpy(s2,"Temp"); 
            strcpy(s,sys.cfg.tempdir); 
            break;
        case 3: 
            strcpy(s2,"Msgs"); 
            strcpy(s,sys.cfg.msgsdir); 
            break;
        case 4: 
            strcpy(s2,"Batch"); 
            strcpy(s,sys.cfg.batchdir); 
            break;
        case 5: 
            strcpy(s2,"Menus"); 
            strcpy(s,sys.cfg.menudir); 
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
                strcpy(sys.cfg.datadir,s); 
                break;
            case 1: 
                strcpy(sys.cfg.gfilesdir,s); 
                break;
            case 2: 
                strcpy(sys.cfg.tempdir,s); 
                break;
            case 3: 
                strcpy(sys.cfg.msgsdir,s); 
                break;
            case 4: 
                strcpy(sys.cfg.batchdir,s); 
                break;
            case 5: 
                strcpy(sys.cfg.menudir,s); 
                break;
            }
            resaveconfig=1;
        }
    }

    if(resaveconfig) {
        cJSON *cfg_root = configrec_to_json(&sys.cfg, &sys.nifty);
        write_json_file("config.json", cfg_root);
        cJSON_Delete(cfg_root);
    }


    if(!sys.restoring_shrink&&!show)
        dotopinit("Status.dat",20);

    sprintf(s,"%sstatus.json",sys.cfg.datadir);
    {
        cJSON *st_root = read_json_file(s);
        if (!st_root) {
            printf("\n\n\n%sstatus.json not found!\n\n",sys.cfg.datadir);
            err(1,s,"In Init()");
        }
        json_to_statusrec(st_root, &sys.status);
        cJSON_Delete(st_root);
    }
    sys.status.wwiv_version=wwiv_num_version;
    userdb_init(sys.cfg.datadir, sys.cfg.maxusers);
    menudb_init(sys.cfg.menudir);
    sys.status.users = userdb_user_count();

    sess.screensave.scrn1=(char *)mallocx(io.screenlen);

    read_in_file("mnudata.dat",(sys.menus),50);

    sys.subboards=(subboardrec *) mallocx(200*sizeof(subboardrec));
    sys.directories=(directoryrec *)mallocx(200*sizeof(directoryrec));
    if(!sys.restoring_shrink&&!show)
        dotopinit("config.dat",40);

    sprintf(s,"%ssubs.dat",sys.cfg.datadir);
    i=open(s,O_RDWR | O_BINARY);
    if (i<0) {
        printf("\n\n%ssubs.dat not found!",sys.cfg.datadir);
        err(1,s,"In Init()");
    }
    sys.num_subs=(read(i,sys.subboards, (200*sizeof(subboardrec))))/
        sizeof(subboardrec);
    if(sys.subboards[0].postacs[0])
        sys.subboards[0].postacs[0]=0;
    if(sys.subboards[0].readacs[0])
        sys.subboards[0].readacs[0]=0;
    if(!(sys.subboards[0].attr & mattr_private)) {
        togglebit((long *)&sys.subboards[0].attr,mattr_private);
        strcpy(sys.subboards[0].name,"Private Mail");
        strcpy(sys.subboards[0].filename,"email");
        sys.subboards[0].conf='@';
    }
    lseek(i,0L,SEEK_SET);
    write(i,&sys.subboards[0],sizeof(subboardrec));
    close(i);

    if(!sys.restoring_shrink&&!show) {
        dotopinit("Subs.dat",50);
    }

    sprintf(s,"%sdirs.dat",sys.cfg.datadir);
    i=open(s,O_RDWR | O_BINARY);
    if (i<0) {
        printf("\n\n%sdirs.dat not found!\n\n",sys.cfg.datadir);
        err(1,s,"In Init()");
    }

    sys.num_dirs=(read(i,sys.directories, (200*sizeof(directoryrec))))/
        sizeof(directoryrec);
    close(i);

    if(!sys.restoring_shrink&&!show)
        dotopinit("Protocol.dat",60);

    sprintf(s,"%sprotocol.dat",sys.cfg.datadir);
    i=open(s,O_RDWR | O_BINARY);
    if(i<0) {
        printf("\n\n%sProtocol.dat Not Found\n",sys.cfg.datadir);
        err(1,s,"In Init()");
    }
    sys.numextrn=(read(i,(void *)&sys.proto,20*sizeof(protocolrec)))/
        sizeof(protocolrec);
    close(i);


    if(!sys.restoring_shrink&&!show) {
        dotopinit("Conf.dat",70);
    }

    sprintf(s,"%sconf.dat",sys.cfg.datadir);
    i=open(s,O_RDWR | O_BINARY);
    if(i<0) {
        printf("\n\n%sConf.dat Not Found!\n\n",sys.cfg.datadir);
        err(1,s,"In Init()");
    }
    sys.num_conf=(read(i,(void *)&sys.conf[0],20*sizeof(confrec)))/ sizeof(confrec);
    if(sys.conf[0].sl[0])
        sys.conf[0].sl[0]=0;
    close(i);

    sprintf(s,"%sarchive.dat",sys.cfg.datadir);
    i=open(s,O_BINARY|O_RDWR);
    if(i<0) {
        printf("\n\n%sArchive.dat Not Found\n",sys.cfg.datadir);
        err(1,s,"In Init()");
    }
    read(i,&sys.xarc[0],8*sizeof(sys.xarc[0]));
    close(i);


    /*  sprintf(s,"%sVOTING.DAT",sys.cfg.datadir);
          f=open(s,O_RDWR | O_BINARY);
          if (f>0) {
            n=(int) (filelength(f) / sizeof(votingrec)) -1;
            for (i=0; i<n; i++) {
              lseek(f,(long) i * sizeof(votingrec),SEEK_SET);
              read(f,(void *)&v,sizeof(votingrec));
              if (v.numanswers)
                sys.questused[i]=1;
            }
            close(f);
          }*/

    userdb_load(1,&sess.user);
    sess.cursub=0;
    sess.fwaiting=numwaiting(&sess.user);

    sl1(2,sys.status.date1);

    if (ok_modem_stuff)
        initport(sys.cfg.primaryport);
    if (sys.cfg.sysconfig & sysconfig_no_local)
        sess.topdata=0;
    else
        sess.topdata=sys.status.net_edit_stuff;

    ss=getenv("PROMPT");
    strcpy(sess.newprompt,"PROMPT=BBS: ");
    if (ss)
        strcat(sess.newprompt,ss);
    else
        strcat(sess.newprompt,"$P$G");
    sprintf(sess.dszlog,"%s/BBSDSZ.LOG",sys.cdir);
    sprintf(s,"DSZLOG=%s",sess.dszlog);
    i=i1=0;
    while (environ[i]!=NULL) {
        if (strncmp(environ[i],"PROMPT=",7)==0)
            sys.xenviron[i1++]=sess.newprompt;
        else
            if (strncmp(environ[i],"DSZLOG=",7)==0)
            sys.xenviron[i1++]=strdup(s);
        else {
            if (strncmp(environ[i],"BBS=",4) && (strncmp(environ[i],"WWIV_FP=",8)))
                sys.xenviron[i1++]=environ[i];
        }
        ++i;
    }
    if (!getenv("DSZLOG"))
        sys.xenviron[i1++]=strdup(s);
    if (!ss)
        sys.xenviron[i1++]=sess.newprompt;

    sprintf(s,"BBS=%s",wwiv_version);
    sys.xenviron[i1++]=s;

    sys.xenviron[i1]=NULL;

    for (i=0; i<20; i++)
        sys.questused[i]=0;

    if(!sys.restoring_shrink&&!show)
        dotopinit("Final Data",95);


    sys.time_event=((double)sys.cfg.executetime)*60.0;
    sys.last_time=sys.time_event-timer();
    if (sys.last_time<0.0)
        sys.last_time+=24.0*3600.0;
    sys.do_event=0;
    if (sys.status.callernum!=65535) {
        sys.status.callernum1=(long)sys.status.callernum;
        sys.status.callernum=65535;
        save_status();
    }
    frequent_init();
    if (!sys.restoring_shrink&&!show && !sess.already_on) {
        remove_from_temp("*.*",sys.cfg.tempdir,0);
        remove_from_temp("*.*",sys.cfg.batchdir,0);
    }
    if(!sys.restoring_shrink&&!show) sess.menuat[0]=0;
    io.lecho=ok_local();
    sess.quote=NULL;
    sess.bquote=0;
    sess.equote=0;

    daylight=0;


    if (!sys.restoring_shrink&&!show) {
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
    io.term_raw_mode = 0;

    exit(lev);
}

