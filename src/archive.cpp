#include "archive.h"
#include "platform.h"
#include "bbs_output.h"
#include "bbs_input.h"
#include "bbs_ui.h"
#include "conio.h"
#include "file.h"
#include "file1.h"
#include "file2.h"
#include "file3.h"
#include "filesys.h"
#include "bbsutl.h"
#include "disk.h"
#include "stringed.h"
#include "session.h"
#include "system.h"
#include "extrn.h"
#include "misccmd.h"
#include "sysopf.h"
#pragma hdrstop

#define SETREC(i)  lseek(sess.dlf,((long) (i))*((long)sizeof(uploadsrec)),SEEK_SET);


void selectarc(void)
{
    auto& sys = System::instance();
    char c;
    int done=0;
    int i;

    if(sys.ARC_NUMBER==-1) {
        nl();
        do {
            pl("Please Select Format");
            for(i=0;i<4&&sys.xarc[i].extension[0];i++)
                npr("5[5%d5]0 %s\r\n",i+1,sys.xarc[i].extension);
            nl();
            outstr("Select: ");
            c=onek("1234Q\r");
            switch(c) {
            case '1':
            case '\r': 
                sys.ARC_NUMBER=0; 
                done=1; 
                break;
            case '2':  
                sys.ARC_NUMBER=1; 
                done=1; 
                break;
            case '3':  
                sys.ARC_NUMBER=2; 
                done=1; 
                break;
            case '4':  
                sys.ARC_NUMBER=3; 
                done=1; 
                break;
            case 'Q': 
                return;
            }
        } 
        while(!done);
    }
}

int list_arc_out(char *fn, char *dir)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    char s[161],s1[MAX_PATH_LEN],s2[MAX_PATH_LEN],s3[MAX_PATH_LEN],c;
    int i,done=0,swapi;
    double p;

    strcpy(s1,dir);
    strcat(s1,fn);
    swapi=get_arc_cmd(s,s1,0,"");
    if (!okfn(fn)) {
        s[0]=0;
    }
    if (exist(s1) && (s[0]!=0)) {
        nl();
        npr("5Manipulating Archive 3%s\r\n",fn);
        sprintf(s1,"%s>%sarctemp",s,sys.cfg.gfilesdir);
        savescreen(&sess.screensave);
        system(s1);
        restorescreen(&sess.screensave);
        printfile("arctemp.");
        nl();
        do {
            npr(get_string(56));
            c=onek("\rQMRV");
            switch(c) {
            case '\r':
            case 'Q': 
                done=1; 
                break;
            case 'R': 
                printfile("arctemp."); 
                break;
            case 'V': 
                nl();
                inputdat("Filename to View",s3,12,0);
                if(!s3[0]) { 
                    pl("Bad Filename"); 
                    return 0; 
                }
                strcpy(s1,dir);
                strcat(s1,fn);
                swapi=get_arc_cmd(s,s1,1,s3);
                if(s[0]) {
                    topscreen();
                    cd_to(sys.cfg.tempdir);
                    savescreen(&sess.screensave);
                    runprog(s,swapi);
                    restorescreen(&sess.screensave);
                    cd_to(sys.cdir);
                } 
                else { 
                    npr("Bad Pathname, %s\r\n",s); 
                }
                sprintf(s,"%s%s",sys.cfg.tempdir,s3);
                if(exist(s)) {
                    s1[0]=sess.topdata; 
                    sess.topdata=0;
                    topscreen();
                    ascii_send(s,&i,&p);
                    unlink(s);
                    sess.topdata=s1[0];
                    topscreen();
                }
                break;
            case 'M': 
                strcpy(s1,dir);
                strcat(s1,fn);
                arcex(s1);
                nl();
                npr("7Your Files are now in Temp.%s, and have been added to your batch queue.\r\n",sys.xarc[sys.ARC_NUMBER].extension);
                nl();
                break;
            }
        } 
        while(!done&&!io.hangup);
    } 
    else if(exist(s1)) {
        npr("\r\nUnknown Archive Format: %s\r\n",fn);
        npr("5Would you like to text view the file? 0");
        if(yn())
            ascii_send(s1,&i,&p);
    }
    else if(!exist(s1))
        npr("\r\nFile %s is Offline\r\n",fn);
    return(0);
}


void add_arc(char *arc, char *fn)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    char s[255], s1[MAX_PATH_LEN],swapi;

    nl();
    dtitle("Dominion Demon Tasker - Compressing");
    if(sys.ARC_NUMBER==-1) sys.ARC_NUMBER=0;
    sprintf(s1,"%s.%s",arc, sys.xarc[sys.ARC_NUMBER].extension);
    swapi=get_arc_cmd(s,s1,2,fn);
    if (s[0]) {
        savescreen(&sess.screensave);
        runprog(s,swapi);
        restorescreen(&sess.screensave);
        logtypes(0,"Added 4%s0 to 4%s0",fn, s1);
    } 
    else
        pl("Unable to Add to archive");
}

void arcex(char *fn)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    char s[MAX_PATH_LEN],s1[MAX_PATH_LEN],s2[MAX_PATH_LEN],c,done;
    int i,swapi;
    uploadsrec u;

    nl();
    sys.ARC_NUMBER=-1;
    for(i=0;i<4;i++) {
        sprintf(s1,"%sTEMP.%s",sys.cfg.tempdir,sys.xarc[i].extension);
        if(exist(s1)) sys.ARC_NUMBER=i;
    }
    selectarc();
    npr("3Enter filename to add to temporary archive file\r\n");
    inputdat("May contain wildcards",s,12,0);
    if (!okfn(s))
        return;
    if (s[0]==0)
        return;
    if (strchr(s,'.')==NULL)
        strcat(s,".*");
    strcpy(s2,stripfn(s));
    for (i=0; i<strlen(s2); i++)
        if ((s2[i]=='|') || (s2[i]=='>') || (s2[i]=='<') || (s2[i]==';') || (s2[i]==' ') ||
            (s2[i]==':') || (s2[i]=='/') || (s2[i]=='\\'))
            return;

    if(!strcmp(s2,"*.*")) return;

    swapi=get_arc_cmd(s,fn,1,s2);
    cd_to(sys.cfg.tempdir);
    savescreen(&sess.screensave);
    runprog(s,swapi);
    restorescreen(&sess.screensave);
    cd_to(sys.cdir);


    sprintf(s1,"%s%s",sys.cfg.tempdir,s2);
    sprintf(s2,"%sTEMP",sys.cfg.tempdir);
    add_arc(s2, s1);
    sprintf(s2,"%sTEMP.%s",sys.cfg.tempdir,sys.xarc[sys.ARC_NUMBER].extension);
    if(exist(s2)) {
        i=open(s2,O_BINARY|O_RDWR);
        u.numbytes=filelength(i);
        u.points=(filelength(i)+1023L)/10240L;
        sprintf(u.filename,"TEMP    .%s",sys.xarc[sys.ARC_NUMBER].extension);
        addtobatch(u,-2,1);
        close(i);
    }
    remove_from_temp(s1,sys.cfg.tempdir,0);
}


void arc_cl(int type)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    char s[MAX_PATH_LEN],s1[MAX_PATH_LEN],s2[MAX_PATH_LEN];
    int i,abort,next,i1=0,num=0,l=0;
    uploadsrec u;

    nl();
    num=file_mask(s);
    if(num==-1&&type==1) return;
    dliscan();
    abort=0;
    next=0;
    if(num<1)
        i=recno(s);
    else {
        i=num;
        l=1;
    }

    do {
        if (i>0) {
            SETREC(i);
            read(sess.dlf,(void *)&u,sizeof(uploadsrec));
            if(type==1) {
                logtypes(0,"Viewed Archive 4%s",u.filename);
                i1=list_arc_out(stripfn(u.filename),sys.directories[sess.udir[sess.curdir].subnum].dpath);
            }
            else if(type==0) {
                npr("7Commenting: 0%s\r\n",u.filename);
                i1=comment_arc(stripfn(u.filename),sys.directories[sess.udir[sess.curdir].subnum].dpath,sys.directories[sess.udir[sess.curdir].subnum].upath);
            } 
            else if(type==2) {
                strcpy(s1,sys.directories[sess.udir[sess.curdir].subnum].dpath);
                addgif(&u,s1);
                SETREC(i);
                write(sess.dlf,(void *)&u,sizeof(uploadsrec));
                i1=0;
            } 
            else if(type==3) {
                strcpy(s1,sys.directories[sess.udir[sess.curdir].subnum].dpath);
                strcat(s1,stripfn(u.filename));
                adddiz(s1,&u);
                SETREC(i);
                write(sess.dlf,(void *)&u,sizeof(uploadsrec));
            }
            if (i1)
                abort=1;
            checka(&abort,&next,0);
            i=nrecno(s,i);
        }
    } 
    while ((i>0) && (!io.hangup) && (!abort) &&!l);
    closedl();
}


int testarc(char *fn, char *dir)
{
    auto& sess = Session::instance();
    char s[161],s1[MAX_PATH_LEN],s2[2];
    int i=0,swapi;

    strcpy(s1,dir);
    strcat(s1,fn);
    swapi=get_arc_cmd(s,s1,3,s2);
    if (!okfn(fn))
        s[0]=0;
    if ((s[0]!=0)) {
        set_protect(0);
        savescreen(&sess.screensave);
        i=runprog(s,swapi);
        restorescreen(&sess.screensave);
        topscreen();
    } 
    else {
        i=0;
    }
    return(i);
}


int comment_arc(char *fn, char *dir,char *cmntfn)
{
    auto& sess = Session::instance();
    char s[161],s1[MAX_PATH_LEN];
    int i=0,swapi;

    strcpy(s1,dir);
    strcat(s1,fn);
    swapi=get_arc_cmd(s,s1,4,cmntfn);
    if (!okfn(fn))
        s[0]=0;
    if(exist(s1) && (s[0]!=0)) {
        set_protect(0);
        printf("[0m");
        savescreen(&sess.screensave);
        runprog(s,swapi);
        restorescreen(&sess.screensave);
        topscreen();
    } 
    else {
        i=0;
    }
    return(i);
}

int get_arc_cmd(char *out, char *arcfn, int cmd, char *ofn)
{
    auto& sys = System::instance();
    char *ss,*ss1,s[161],s1[161];
    int i,swapi;

    out[0]=0;
    ss=strchr(arcfn,'.');
    if (ss==NULL)
        return 0;
    ++ss;
    ss1=strchr(ss,'.');
    while (ss1!=NULL) {
        ss=ss1;
        ++ss1;
        ss1=strchr(ss,'.');
    }
    if(!stricmp(ss,"QWK"))
        strcpy(ss,sys.xarc[sys.ARC_NUMBER].extension);
    for (i=0; i<8; i++)
        if (stricmp(ss,sys.xarc[i].extension)==0) {
            switch(cmd) {
            case 0: 
                strcpy(s,sys.xarc[i].arcl); 
                break;
            case 1: 
                strcpy(s,sys.xarc[i].arce); 
                break;
            case 2: 
                strcpy(s,sys.xarc[i].arca); 
                break;
            case 3: 
                strcpy(s,sys.xarc[i].arct); 
                break;
            case 4: 
                strcpy(s,sys.xarc[i].arcc); 
                break;
            }
            if (s[0]==0)
                return 0;
            if(cmd==4)
                sprintf(s1,"%s%s",sys.cfg.gfilesdir,ofn);
            stuff_in(out,s,arcfn,ofn,"","",s1);
            if(sys.xarc[i].attr & xarc_swap)
                return 1;
            else
                return 0;
        }
    return 0;
}

int adddiz(char *fn,uploadsrec *u)
{
    auto& sess = Session::instance();
    char s[MAX_PATH_LEN],s1[MAX_PATH_LEN],*ss;
    int i,i1;
    long l;
    uploadsrec u1;

    u1=(*u);
    if(u1.mask & mask_extended) {
        delete_extended_description(fn);
        u1.mask &= mask_extended;
        (*u)=u1;
    }
    unlink("file_id.diz");
    get_arc_cmd(s,fn,1,"file_id.diz");
    if(s[0]) {
        savescreen(&sess.screensave);
        runprog(s,0);
        restorescreen(&sess.screensave);
        if(!exist("file_id.diz")) return 0;
        npr("2�0 Description File Found in 7%s0, Adding\r\n",u1.filename);
        i=open("file_Id.diz",O_BINARY|O_RDWR);
        l=filelength(i);
        ss=(char *)malloca(l);
        read(i,ss,l);
        close(i);
        ss[l]='\0';
        add_extended_description(u1.filename,ss);
        u1.mask |= mask_extended;
        free(ss);
        (*u)=u1;
        unlink("file_id.diz");
        return 1;
    }
    return 0;
}


void unarc(char *arc, char *fn)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();

    char s[255], s1[MAX_PATH_LEN],swapi;

    sys.ARC_NUMBER=-1;
    selectarc();

    nl();
    dtitle("Dominion Demon Tasker - Extracting");
    if(sys.ARC_NUMBER==-1) sys.ARC_NUMBER=0;
    sprintf(s1,"%s.%s",arc, sys.xarc[sys.ARC_NUMBER].extension);
    stuff_in(s,sys.xarc[sys.ARC_NUMBER].arce,arc,fn,"","","");
    swapi=(sys.xarc[sys.ARC_NUMBER].attr & xarc_swap);
    if (s[0]) {
        savescreen(&sess.screensave);
        runprog(s,swapi);
        restorescreen(&sess.screensave);
        logtypes(0,"Extracted 4%s0 from 4%s0",fn, s1);
    } 
    else
        pl("Unable to Add to archive");
}
