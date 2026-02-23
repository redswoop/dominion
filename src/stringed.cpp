#include "platform.h"
#include "fcns.h"
#include "session.h"
#include "system.h"
#include "error.h"
#pragma hdrstop


typedef struct {
    char thestring[161];
}
stringrec;

int opp(int i)
{
    if(i) i=0;
    else i=1;

    return i;
}


char *disk_string(int whichstring,char *fn)
{
    auto& sys = System::instance();
    static char s[161],s1[MAX_PATH_LEN];
    int i;
    stringrec astring;

    sprintf(s1,"%s%s",sys.cfg.datadir,fn);
    i=open(s1,O_RDONLY|O_BINARY);

    if (i<0) {
        err(1,s1,"In Disk_String");
    }

    lseek(i,((long)(whichstring-1))*(sizeof(astring)),SEEK_SET);
    read(i,(void *)&astring,sizeof(astring));
    sprintf(s,"%s",&astring.thestring); 
    close(i);

    return(s);
}


typedef struct {
    char str[181];
    int which;
} strcacherec;

strcacherec strcac[10];
int whichcac=0;


char *get_string(int whichstring)
{
    static char s[200];
    int i;

    for(i=0;i<10;i++) {
        if(strcac[i].which==whichstring)
            return (strcac[i].str);
    }

    strcpy(s,disk_string(whichstring,"strings.dat"));
    if(s[0]=='@'&&s[1]=='@') {
        printfile(&s[2]);
        s[0]=0;
    }

    whichcac++;
    if(whichcac>9)
        whichcac=0;
    strcpy(strcac[whichcac].str,s);

    return(s);
}


char *get_string2(int whichstring)
{
    return(disk_string(whichstring,"sysstr.dat"));
}


char *getdesc(int whichstring)
{
    return(disk_string(whichstring,"sdesc.dat"));
}


typedef struct {
    char rumour[MAX_PATH_LEN],
    by[31],
    an;
} 
rumourrec;


char *get_say(int which)
{
    auto& sys = System::instance();
    static char s[161],s1[MAX_PATH_LEN];
    int i,tot=0,i1;
    long l;
    rumourrec arum;
    userrec u;

    sprintf(s1,"%srumours.dat",sys.cfg.datadir);
    if(!exist(s1)) return("Sorry, I have Nothing To Say.  Add a Rumor.");
    i=open(s1,O_RDWR | O_BINARY);

    if(which) {
        nl();
        lseek(i,0L,SEEK_SET);
        while(read(i,(void *)&arum,sizeof(rumourrec))) {
            if(arum.an) {
                if(!cs())
                    strcpy(s,"Anonymous");
                else
                    sprintf(s,"1%s",arum.by);
            } 
            else strcpy(s,arum.by);
            npr("0%-50.50s 5[3%s5]",arum.rumour,s);
            nl();
        }
        close(i);
        return 0;
    }

    else {
        lseek(i,0L,SEEK_SET);
        l=filelength(i);
        tot=l/sizeof(rumourrec);
        which=random(tot);
        lseek(i,((long)(which))*(sizeof(arum)),SEEK_SET);
        read(i,(void *)&arum,sizeof(arum));
        strcpy(s,arum.rumour);
        close(i);
        return(s);
    }
}

void addsay(void)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    char s[71],s1[MAX_PATH_LEN];
    int i;
    rumourrec astring;

    s[0]=0;
    if(sess.user.restrict & restrict_rumours) {
        pl("Sorry, You are Restricted from Posting Rumours");
        return;
    }

    inputdat("Enter your Rumour",s,70,1);
    if(!s[0]) return;
    logpr("5�> Added Rumor 0: %s",s);
    strcpy(astring.rumour,s);
    strcpy(astring.by,nam(&sess.user,sess.usernum));
    nl();
    npr("5Anonymous? ");
    astring.an=yn();
    sprintf(s1,"%srumours.dat",sys.cfg.datadir);
    i=open(s1,O_RDWR|O_CREAT|O_BINARY, S_IREAD|S_IWRITE);
    lseek(i,0L,SEEK_END);
    write(i,(void *)&astring,sizeof(rumourrec));
    close(i);
}

void readstring(int which)
{
    auto& sys = System::instance();
    FILE *ff;
    char s[161],s1[171];
    int f;

    switch(which) {
    case 0: 
        sprintf(s,"%sstrings.dat",sys.cfg.datadir); 
        break;
    case 1: 
        sprintf(s,"%ssysstr.dat",sys.cfg.datadir); 
        break;
    case 2: 
        sprintf(s,"%ssdesc.dat",sys.cfg.datadir); 
        break;
    }

    f=open(s,O_RDWR|O_BINARY|O_CREAT|O_TRUNC);

    switch(which) {
    case 0: 
        sprintf(s,"strings.txt"); 
        break;
    case 1: 
        strcpy(s,"sysstr.txt"); 
        break;
    case 2: 
        strcpy(s,"sdesc.txt"); 
        break;
    }

    if(!exist(s)) return;
    ff=fopen(s,"rt");

    while(fgets(s1,171,ff)!=NULL) {
        filter(s1,'\n');
        write(f,&s1,sizeof(s));
    }

    close(f);
    fclose(ff);
}


void extractstring(int which)
{
    auto& sys = System::instance();
    int i,done=0;
    FILE *f;
    char s[161];
    long l;

    switch(which) {
    case 0: 
        sprintf(s,"strings.txt"); 
        break;
    case 1: 
        strcpy(s,"sysstr.txt"); 
        break;
    case 2: 
        strcpy(s,"sdesc.txt"); 
        break;
    }

    f=fopen(s,"wt");
    sprintf(s,which?"%ssysstr.dat":"%sstrings.dat",sys.cfg.datadir);
    i=open(s,O_RDWR|O_BINARY);

    while(!done) {
        l=read(i,&s,161);
        if(l<sizeof(stringrec)) done=1;
        strcat(s,"\n");
        fputs(s,f);
    }

    fclose(f);
    close(i);
}


void liststring(int type,int where)
{
    auto& sys = System::instance();
    char s[161],s1[MAX_PATH_LEN];
    int i,num=0,i1;
    stringrec astring;
    rumourrec r;
    userrec u;

    io.mciok=0;
    if(type==1)
        sprintf(s1,"%ssysstr.dat",sys.cfg.datadir);
    else if(type==2)
        sprintf(s1,"%ssdesc.dat",sys.cfg.datadir);
    else sprintf(s1,"%sstrings.dat",sys.cfg.datadir);
    i=open(s1,O_RDWR | O_BINARY);

    lseek(i,((long)(where*9))*(sizeof(astring)),SEEK_SET);
    while(read(i,type==3?(void *)&r:(void *)&astring,type==3?sizeof(rumourrec):sizeof(astring))&&num<9)
        if(!type)
            npr("0%2d. 5[3%-17.17s5] 0%s\r\n",(num++)+1,getdesc(where*9+num+1),&astring.thestring);
    else
        npr("%2d. %s\r\n",(num++)+1,&astring.thestring);
    close(i);
    io.mciok=1;
}

void edstring(int type)
{
    auto& sys = System::instance();
    char s[161],s1[161],c,done=0;
    int i,ednum,set=0;
    stringrec astring;
    rumourrec r;

    do {
        outchr(12);

        liststring(type,set);
        nl();
        outstr("5String Editor (?=Help,[,]) 0");
        c=onek("RETLQ1234567890[]?\r");
        switch(c) {
        case '?': 
            printmenu(9); 
            pausescr(); 
            break;
        case ']': 
            set++; 
            break;
        case '[': 
            if(set>0) set--; 
            break;
        case 'Q': 
            done=1; 
            break;
        case 'T': 
            type++; 
            if(type==3) type=0; 
            break;
        case 'E': 
            extractstring(type); 
            break;
        case 'R': 
            readstring(type); 
            break;
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            if(type==1)
                sprintf(s1,"%ssysstr.dat",sys.cfg.datadir);
            else if(type==2)
                sprintf(s1,"%ssdesc.dat",sys.cfg.datadir);
            else
                sprintf(s1,"%sstrings.dat",sys.cfg.datadir);
            ednum=(set*9)+c-'0'-1;
            i=open(s1,O_RDWR | O_CREAT |O_BINARY | S_IREAD | S_IWRITE );
            if(i<0) pl("Unable to open`P");
            lseek(i,((long)(ednum))*(sizeof(astring)),SEEK_SET);
            read(i,(void *)&astring,sizeof(astring));
            lseek(i,((long)(ednum))*(sizeof(astring)),SEEK_SET);
            io.mciok=0;
            pl("7Current Value:");
            pl(astring.thestring);
            nl();
            binli(s,astring.thestring,161,1,0,0,0);
            if(!s[0]) close(i);
            else {
                if(s[0]==32) {
                    nl();
                    outstr("5Set to NULL String? ");
                    if(yn()) strcpy(s,"");
                }
                memset(&astring,0,sizeof(astring));
                strcpy(astring.thestring,s);
                write(i,(void *)&astring,161L);
                close(i);
            }
            io.mciok=1;
            break;
        }
    } 
    while(!io.hangup&&!done);
}

void searchrum(void)
{
    auto& sys = System::instance();
    char s[161],scan[31];
    int i,num,i1,i2;
    long l;
    userrec u;
    rumourrec r;

    nl();
    inputdat("Text to Scan For",scan,20,1);
    if(!scan[0]) return;
    sprintf(s,"%srumours.dat",sys.cfg.datadir);
    i=open(s,O_RDWR|O_BINARY);
    l=filelength(i);
    num=l/sizeof(rumourrec);
    nl();
    for(i1=0;i1<num;i1++) {
        read(i,&r,sizeof(rumourrec));
        if(strstr(strupr(r.rumour),strupr(scan))) {
            if(r.an&&!cs()) strcpy(s,"Anonymous");
            else {
                i2=finduser(r.by);
                if(i2) {
                    userdb_load(i2,&u);
                    strcpy(s,nam(&u,i2));
                }
            }
            npr("Text Found in rumour number 7%d0, by 2%s\r\n",i1,s);
            pl(r.rumour);
        }
    }
}
