#include "platform.h"
#include "fcns.h"
#include "session.h"
#include "system.h"
#include "acs.h"
#include "cmd_registry.h"
#pragma hdrstop

#define FDEC

#define SETREC(i)  lseek(sess.dlf,((long) (i))*((long)sizeof(uploadsrec)),SEEK_SET);


int INDENTION=30;
#define MAX_LINES 15

char *getfhead(int bot)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    static char s[161];
    FILE *ff;

    sprintf(s,"%sfile%d.fmt",sys.cfg.gfilesdir,sess.user.flisttype);
    ff=fopen(s,"rt");
    if (!ff) { strcpy(s,""); return(s); }

    fgets(s,161,ff);
    fgets(s,161,ff);
    fgets(s,161,ff);

    fgets(s,161,ff);
    if(bot)
        fgets(s,161,ff);
    filter(s,'\n');

    fclose(ff);
    return(s);
}

int ratio_ok()
{
    auto& sess = Session::instance();
    int ok=1;
    char s[101];

    if (!(sess.user.exempt & exempt_ratio)&&!checkacs(1))
        ok=filer_ok();

    if(ok)
        if (!(sess.user.exempt & exempt_post)&&checkacs(0))
            ok=postr_ok();

    return(ok);
}

int postr_ok()
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    int ok=1;
    char s[101];

    if(!sess.user.pcr) {

        if (!(sess.user.exempt & exempt_post))
            if ((sys.cfg.post_call_ratio>0.0001) && (post_ratio()<sys.cfg.post_call_ratio)) {
                ok=0;
                nl();
                sprintf(s,"Your post/call ratio is %-5.3f.  You need a ratio of %-5.3f to download.",
                post_ratio(), sys.cfg.post_call_ratio);
                pl(s);
                nl();
            }
    } 
    else {
        if (!(sess.user.exempt & exempt_post))
            if ((sess.user.pcr>0.0001) && (post_ratio()<sess.user.pcr)) {
                ok=0;
                nl();
                sprintf(s,"Your post/call ratio is %-5.3f.  You need a ratio of %-5.3f to download.",
                post_ratio(), sess.user.pcr);
                pl(s);
                nl();
            }
    }

    return(ok);
}

int filer_ok()
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    if(!sess.user.ratio) {
        if ((sys.cfg.req_ratio>0.0001) && (ratio()<sys.cfg.req_ratio)) {
            nl();
            npr("Your File ratio is %-5.3f.  You need a ratio of %-5.3f to download.\r\n",
            ratio(), sys.cfg.req_ratio);
            nl();
            return 0;
        }
        return 1;
    } 
    else {
        if ((sess.user.ratio>0.0001) && (ratio()<sess.user.ratio)) {
            nl();
            npr("Your File ratio is %-5.3f.  You need a ratio of %-5.3f to download.\r\n",
            ratio(), sess.user.ratio);
            nl();
            return 0;
        }
        return 1;
    }
}


int dcs() {
    auto& sess = Session::instance();
    if (sess.user.dsl>=100)
        return(1);
    else
        return(0);
}


void dliscan1(int dn)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    char s[MAX_PATH_LEN];
    int i;
    uploadsrec u;

    closedl();
    if(sys.directories[dn].type==99)
        sprintf(s,"%sTEMPDIR.dir",sys.directories[dn].dpath);
    else
        sprintf(s,"%sdir/%s.DIR",sys.cfg.datadir,sys.directories[dn].filename);
    sess.dlf=open(s,O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
    i=filelength(sess.dlf)/sizeof(uploadsrec);
    if (i==0) {
        u.numbytes=0;
        SETREC(0);
        write(sess.dlf,(void *)&u,sizeof(uploadsrec));
    } 
    else {
        SETREC(0);
        read(sess.dlf,(void *)&u,sizeof(uploadsrec));
    }
    sess.numf=u.numbytes;

    sprintf(s,"%sdir/%s.EXT",sys.cfg.datadir,sys.directories[dn].filename);
    sess.edlf=open(s,O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
}


void dliscan()
{
    auto& sess = Session::instance();
    dliscan1(sess.udir[sess.curdir].subnum);
}


void closedl()
{
    auto& sess = Session::instance();
    if (sess.dlf>0) {
        close(sess.dlf);
        sess.dlf=-1;
    }
    if (sess.edlf>0) {
        close(sess.edlf);
        sess.edlf=-1;
    }
}


void align(char *s)
{
    char f[40],e[40],s1[20],*s2;
    int i,i1,i2;

    i1=0;
    if (s[0]=='.')
        i1=1;
    for (i=0; i<strlen(s); i++)
        if ((s[i]=='\\') || (s[i]=='/') || (s[i]==':') || (s[i]=='<') ||
            (s[i]=='>') || (s[i]=='|'))
            i1=1;
    if (i1) {
        strcpy(s,"        .   ");
        return;
    }
    s2=strchr(s,'.');
    if (s2==NULL) {
        e[0]=0;
    } 
    else {
        strcpy(e,&(s2[1]));
        e[3]=0;
        s2[0]=0;
    }
    strcpy(f,s);

    for (i=strlen(f); i<8; i++)
        f[i]=32;
    f[8]=0;
    i1=0;
    i2=0;
    for (i=0; i<8; i++) {
        if (f[i]=='*')
            i1=1;
        if (f[i]==' ')
            i2=1;
        if (i2)
            f[i]=' ';
        if (i1)
            f[i]='?';
    }

    for (i=strlen(e); i<3; i++)
        e[i]=32;
    e[3]=0;
    i1=0;
    for (i=0; i<3; i++) {
        if (e[i]=='*')
            i1=1;
        if (i1)
            e[i]='?';
    }

    for (i=0; i<12; i++)
        s1[i]=32;
    strcpy(s1,f);
    s1[8]='.';
    strcpy(&(s1[9]),e);
    strcpy(s,s1);
    for (i=0; i<12; i++)
        s[i]=toupper(s[i]);
}


int compare(char s1[15], char s2[15])
{
    int ok,i;

    ok=1;
    for (i=0; i<12; i++)
        if ((s1[i]!=s2[i]) && (s1[i]!='?') && (s2[i]!='?'))
            ok=0;
    return(ok);
}

int printinfo(uploadsrec *u, int *abort,int number)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    char s[161],ss[39],s1[10],s2[5],s3[5],s4[5],f[MAX_PATH_LEN],fn[20];
    int i,i1,i2;
    double t;

    strcpy(fn,u->filename);
    //  strrev(fn);

    if(sess.modem_speed)
        t=((double) (((u->numbytes)+127)/128)) * (1620.0)/((double) (sess.modem_speed));

    i=(u->numbytes+1023)/1024;
    sprintf(s1,"%4d",i);
    sprintf(s2,"%4d",u->points);
    sprintf(s3,"%3d",number);
    sprintf(ss,"%-39.39s",u->description);
    sprintf(s4,"%3d",u->numdloads);

    sprintf(f,"%s%s",sys.directories[sess.udir[sess.curdir].subnum].dpath,u->filename);

    if(!(u->ats[0]))
        stuff_in1(s,sess.filelistformat3,fn,ss,s1,s2,s3,u->upby,u->date,s4,ctim(t),"");

    else if(exist(f))
        stuff_in1(s,sess.filelistformat,fn,ss,s1,s2,s3,u->upby,u->date,s4,ctim(t),"");
    else
        stuff_in1(s,sess.filelistformat2,fn,ss,s1,s2,s3,u->upby,u->date,s4,ctim(t),"");

    outstr(s);

    if ((!*abort) && (u->mask & mask_extended))
        sess.num_listed+=print_extended(u->filename,abort,15,1);
    else nl();
    if (!(*abort))
        ++sess.num_listed;
    if(strstr(s,"`M")&&!(*abort)) ++sess.num_listed;
    return(1);
}


void printtitle()
{
    int x;

    outchr(12);
    pl(get_string(22));
    displayformat();
    /*    ansic(5);
                    for(x=0;x<79;x++) outchr(196);
                    nl();*/
    pl(getfhead(0));
}


int file_mask(char *s)
{
    int i=0;

    nl();
    outstr(get_string(47));
    mpl(12);
    input(s,12);

    if (s[0]==0) {
        strcpy(s,"*.*");
        i=-1;
    } 
    else if(atoi(s))
        return(atoi(s));

    if (strchr(s,'.')==NULL)
        strcat(s,".*");

    align(s);
    nl();
    return(i);
}

int nonstop;

int pauseline(int line,int *abort)
{
    auto& sess = Session::instance();
    int cont=0,i;
    char c;
    char s[40];

    if(line)
        pl(getfhead(1));

    if(nonstop) return 0;
    do {
        outstr(get_string(39));
        strcpy(s,"I?VWMDSNBR\r");
        if(cs())
            strcat(s,"!@#$");
        c=onek(s);
        if(okansi()) outstr("[A[K");
        checkhangup();
        switch(c) {
        case '!': 
            ex("F3",""); 
            dliscan(); 
            break;
        case '@': 
            ex("FR",""); 
            dliscan(); 
            break;
        case '#': 
            ex("F2",""); 
            dliscan(); 
            break;
        case '$': 
            ex("F5","");
            dliscan(); 
            break;
        case 'D': 
            newdl(sess.curdir); 
            dliscan(); 
            break;
        case 'V': 
            ex("FV",""); 
            dliscan(); 
            break;
        case 'B': 
            ex("FB",""); 
            dliscan(); 
            break;
        case 'N': 
            nonstop=1; 
            cont=1; 
            break;
        case 'S': 
            *abort=1; 
            cont=1; 
            break;
        case 'M': 
            mark(sess.curdir); 
            dliscan(); 
            break;
        case '\r': 
            cont=1; 
            break;
        case 'I': 
            getfileinfo(); 
            dliscan(); 
            break;
        case 'W': 
            getfileformat(); 
            setformat(); 
            break;
        case '?': 
            printmenu(22); 
            if(cs()) printmenu(17); 
            break;
        case 'R': 
            return 1;
        }
    } 
    while(!cont&&!io.hangup);
    return 0;
}


int lfs(char spec[12],char ss[MAX_PATH_LEN],int *abort,long *bytes,int isnew)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    char s[MAX_PATH_LEN],s1[MAX_PATH_LEN],c,*b;
    int i,next,x,prtitle,listed=0,topofpage=0,ok=1,i2,val=1;
    uploadsrec u;


    dliscan();
    if(!slok(sys.directories[sess.udir[sess.curdir].subnum].vacs,0)) {
        pl(get_string(82));
        return 0;
    }
    io.listing=1;
    sess.num_listed=0;
    prtitle=0;

    setformat();
    topofpage=1;

    for (i=val; (i<=sess.numf) && (! (*abort)) && (!io.hangup); i++) {
        SETREC(i);
        read(sess.dlf,(void *)&u,sizeof(uploadsrec));
        ok=1;

        if(isnew)
            if ((u.daten>=sess.nscandate)||(!u.ats[0]));
        else ok=0;
        if(u.ats[1]==100&&isnew) ok=1;

        if(ss[0]) {
            strcpy(s,u.description);
            for (i2=0; i2<strlen(s); i2++)
                s[i2]=toupper(s[i2]);
            s[i2]=0;
            if (strstr(s,ss)==NULL) {
                if(u.mask & mask_extended) {
                    b=read_extended_description(u.filename);
                    for (i2=0; i2<strlen(b); i2++)
                        b[i2]=toupper(b[i2]);
                    if (strstr(b,ss)==NULL)
                        ok=0;
                    free(b);
                } 
                else
                    ok=0;
            }
        }

        if(ok)
            if((sess.num_listed+count_extended(u.filename))>sess.user.screenlines-6) {
                if(!nonstop) {
                    if(!pauseline(1,abort))
                        topofpage=i+1;
                    else i=topofpage-1;
                    sess.num_listed=0;
                    prtitle=0;
                }
            }

        if (compare(spec,u.filename)&&ok) {
            if(!prtitle) {
                printtitle();
                prtitle=1;
            }
            if(printinfo(&u,abort,i)) {
                listed++;
                (*bytes)+=(u.numbytes+1023)/1024;
            } 
            else if(!nonstop) {
                if(!pauseline(1,abort))
                    topofpage=i+1;
                else i=topofpage-1;
                sess.num_listed=0;
                prtitle=0;
            }
        }

        if(!nonstop)
        if(sess.num_listed>sess.user.screenlines-6) {
            if(!pauseline(1,abort))
                topofpage=i+1;
            else
                i=topofpage-1;
            sess.num_listed=0;
            prtitle=0;
        }
    }

    if (!(* abort)&&prtitle) {
        pauseline(1,abort);
    }

    closedl();
    io.listing=0;
    return listed;
}

int changefarea(void)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    int done=0,i;
    char s[41];

    do {
        inputdat("Select area, ?=List, [CR] to abort",s,31,0);
        if(s[0]=='?')
            dirlist(0);
        else if(s[0]) {
            for(i=0;sess.udir[i].subnum>=0&&i<sys.num_dirs;i++) {
                if(strcmp(s,sess.udir[i].keys)==0)
                    sess.curdir=sess.udir[i].subnum;
            }
            done=1;
        } 
        else if(!s[0]) return 0;
    } 
    while(!done);
    return 1;
}

void listfiles(char ms[40])
{
    auto& sess = Session::instance();
    char s[MAX_PATH_LEN];
    int abort=0,num,dn=0;
    long len=0;

    if(ms[0]=='?')
        if(!changefarea()) return;
    else if(ms[0])
        dn=atoi(ms);
    if(dn) sess.curdir=dn;
    file_mask(s);
    nonstop=0;
    nl();
    num=lfs(s,"",&abort,&len,0);
    npr("\r\n\r\n5%d 0Files Listed, 5%ld0 KB Total",num,len);
    nl();
}

int nscandir(int d, int *abort, int title,int *next)
{
    auto& sess = Session::instance();
    int i,od,did=0,x,num;
    uploadsrec u;
    char s[MAX_PATH_LEN],s1[MAX_PATH_LEN];
    long len=0;

    od=sess.curdir;
    sess.curdir=d;
    nonstop=0;
    io.listing=1;
    *next=0;
    if(title) {
        pl(get_string(44));
    }
    num=lfs("????????.???","",abort,&len,1);
    sess.curdir=od;
    if(did&&!(*abort)&&!(*next)&&!io.hangup) pauseline(1,abort);
    if(*next) *abort=0;
    io.listing=0;
    return(num);
}


void nscanall()
{
    auto& sess = Session::instance();
    int abort,i,i1,next,save,num=0;
    char s[MAX_PATH_LEN];

    abort=0;
    sess.num_listed=0;
    save=sess.curdir;
    setformat();
    for (sess.curdir=0; (sess.curdir<200) && (!abort) && (sess.udir[sess.curdir].subnum!=-1) && !io.hangup; sess.curdir++) {
        i1=sess.udir[sess.curdir].subnum;
        if (sess.user.nscn[i1]>=0)
            num+=nscandir(sess.curdir,&abort,1,&next);
    }
    if (!abort) {
        nl();
        nl();
        npr("%d Files listed",num);
        nl();
    }
    sess.curdir=save;
}


int recno(char s[15])
{
    auto& sess = Session::instance();
    int i;
    uploadsrec u;

    i=1;
    if (sess.numf<1)
        return(-1);
    SETREC(i);
    read(sess.dlf,(void *)&u,sizeof(uploadsrec));
    while ((i<sess.numf) && (compare(s,u.filename)==0)) {
        ++i;
        SETREC(i);
        read(sess.dlf,(void *)&u,sizeof(uploadsrec));
    }
    if (compare(s,u.filename))
        return(i);
    else
        return(-1);
}


int nrecno(char s[15],int i1)
{
    auto& sess = Session::instance();
    int i;
    uploadsrec u;

    i=i1+1;
    if ((sess.numf<1) || (i1>=sess.numf))
        return(-1);

    SETREC(i);
    read(sess.dlf,(void *)&u,sizeof(uploadsrec));
    while ((i<sess.numf) && (compare(s,u.filename)==0)) {
        ++i;
        SETREC(i);
        read(sess.dlf,(void *)&u,sizeof(uploadsrec));
    }
    if (compare(s,u.filename))
        return(i);
    else
        return(-1);
}


int checkdl(uploadsrec u,int dn)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    double t;
    char s[MAX_PATH_LEN];
    userrec us;

    tleft(1);

    if(u.ownersys!=sess.usernum&&u.ownersys!=0) {
        pl(get_string(86));
        return(0);
    }

    if(sess.modem_speed)
        t=((double) (((u.numbytes)+127)/128)) * (1620.0)/((double) (sess.modem_speed));

    if(t>nsl()&&!dcs()&&!(sess.user.exempt & exempt_time)&&!checkacs(3)) {
        pl(get_string(87));
        printfile("nodl");
        return 0;
    }

    sprintf(s,"%s%s",sys.directories[dn].dpath,u.filename);
    if(!exist(s)) {
        pl("File is OffLine");
        if(printfile("offline"))
            email(1,"Offline File Request",1);
        return 0;
    }

    if(!u.ats[0] && !dcs() /*&& !(sess.user.exempt & exempt_dlvalfile)*/&&!checkacs(11)) {
        npr("%s",get_string(43));
        return 0;
    }

    if(u.mask & mask_unavail&&!dcs()) {
        printfile("unavail");
        return 0;
    }


    if(!(sys.directories[dn].mask & mask_no_ratio)) {
        if(!dcs()&&!(sess.user.exempt & exempt_ratio)&&!checkacs(2)) {
            if(sys.nifty.nifstatus & nif_fpts)
                if(sess.user.fpts*sys.nifty.fptsratio<u.points) {
                    nl();
                    npr("%s",get_string(27));
                    printfile("nodl");
                    return 0;
                }
        }

        if(!dcs())  {
            if(!ratio_ok()) {
                printfile("nodl");
                return 0;
            }
        }

    }

    return(1);
}


void finddescription(char ms[41])
{
    auto& sess = Session::instance();
    uploadsrec u;
    int i,i2,abort,d,ocd,num=0,next=0,sp_found=0;
    long len=0;

    char s[MAX_PATH_LEN],s1[MAX_PATH_LEN],s2[MAX_PATH_LEN],wh[41],*p;

    nl();

    setformat();
    dtitle("Text Search");
    strcpy(s,"????????.???");
    s1[0]=0;

    if(ms[0]!='F')
        inputdat("Enter string to search for in file bases",s1,58,0);

    if(ms[0]!='T')
        file_mask(s);

    if(!(strcmp(s,"????????.???"))&&!s1[0]) return;

    logtypes(4,"Scanned for %s in %s",s1,s);
    ocd=sess.curdir;
    abort=0;
    sess.num_listed=0;

    wh[0]=0;
    do {
        inputdat("Scan which areas? (C=Current, A=All, ?=List)",wh,41,0);
        if(wh[0]=='?')
            dirlist(0);
    } 
    while(wh[0]=='?');

    p=strtok(wh," ,;");
    while (p&&!abort&&!io.hangup) {
        checka(&abort,&next,0);
        if(strcmp(p,"A")==0) {
            for (i=0; (i<64) && (!abort) && (!io.hangup) && (sess.udir[i].subnum!=-1); i++) {
                sess.curdir=i;
                num+=lfs(s,s1,&abort,&len,0);
            }
            break;
        }
        if(strcmp(p,"C")==0) {
            num+=lfs(s,s1,&abort,&len,0);
            break;
        }
        sp_found=0;
        i=0;
        while(!sp_found&&i<64 && !abort && !io.hangup &&sess.udir[i].subnum!=-1) {
            if(strcmp(p,sess.udir[i].keys)==0)
                sp_found=1;
            else
                i++;
        }
        if(sp_found) {
            sess.curdir=i;
            num+=lfs(s,s1,&abort,&len,0);
        }
        p=strtok(NULL," ,;");
    }

    sess.curdir=ocd;
    if (num && (!abort)) {
        nl();
        npr("5%d0 Files Found, 5%ld0 bytes\r\n",num,len);
        nl();
    }
}


void setformat()
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    FILE *ff;
    char s[161],s1[161];
    int i=0;

    sprintf(s,"%sfile%d.fmt",sys.cfg.gfilesdir,sess.user.flisttype);
    ff=fopen(s,"rt");
    fgets(sess.filelistformat,99,ff);
    filter(sess.filelistformat,'\n');

    fgets(sess.filelistformat2,99,ff);
    filter(sess.filelistformat2,'\n');

    fgets(sess.filelistformat3,99,ff);
    filter(sess.filelistformat3,'\n');


    stuff_in1(s,sess.filelistformat,"Dom30   .Zip","\xAE"," 300","   0","  1","Fallen Angel","01/01/92","100","","");
    strcpy(s1,noc2(s));
    for(i=0;i<strlen(s1);i++) {
        if(s1[i]==(char)0xAE) INDENTION=i;
    }
    if(INDENTION>39) INDENTION=39;
    fclose(ff);
}


int okfn(char *s)
{
    int i,l,ok;
    unsigned char ch;

    ok=1;
    l=strlen(s);
    for (i=0; i<l; i++) {
        ch=s[i];
        if ((ch<=' ') || (ch=='/') || (ch=='\\') || (ch==':') ||
            (ch=='>') || (ch=='<') || (ch=='|')  || (ch=='+') ||
            (ch==',') || (ch==';') || (ch>126))
            ok=0;
    }
    return(ok);
}

void nnscan(char ms[41])
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    char *p,s[MAX_PATH_LEN],ch;
    int ok=1,i=0,global=0,allconf=0,ask=0,pdate=0,done=0,abort=0;

    s[0]=0;

    if(ms[0])
        while(ms[i]&&!done) {
            ch=ms[i];

            switch(ch) {
            case '?': 
                ask=1;
                break;
            case 'S': 
                pdate=1;
                break;
            case 'G': 
                global=1;
                break;
            case 'A': 
                allconf=1;
                break;
            case 'C': 
                global=0;
                break;
            case ';': 
                strcpy(s,&ms[i+1]);
                done=1;
                break;
            }
            i++;
        } 
    else {
        ask=0;
        allconf=0;
        global=0;
    }

    if(ask) {
        if(s[0])
            outstr(s);
        else if(!allconf)
            outstr(get_string2(8));
        else
            outstr(get_string2(27));

        if(!allconf)
            ok=yn();
        else {
            ch=onek("\rYNA");
            if(ch=='N'||ch=='\r')
                ok=0;
            if(ch=='Y'||ch=='A') {
                ok=1;
                allconf=(ch=='A');
            }
        }
    }

    if(!ok)
        return;

    setformat();
    abort=0;

    if(pdate)
        ex("FP","");

    if(global) {
        strcpy(s,sys.conf[sess.curconf].flagstr);
        strcpy(sys.conf[sess.curconf].flagstr,"");
        changedsl();
        nscanall();
        strcpy(sys.conf[sess.curconf].flagstr,s);
        changedsl();
    } 
    else
        nscandir(sess.curdir,&abort,0,&i);
}
