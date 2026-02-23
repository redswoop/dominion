#include "platform.h"
#include "fcns.h"
#include "session.h"
#include "system.h"
#pragma hdrstop

#include "menudb.h"

/* sess.pp, sess.tg, sess.maxcmd, sess.menuat now in vars.h (Phase B0) */
extern int fastlogon;


static auto& sys = System::instance();
static auto& sess = Session::instance();

struct mmfmt_t {
    char         fmt[41],
    fill,
    promptfn[8],
    ansifn[8],
    ansiftfn[8],
    center;
}
mmfmt;


char *getfmt(char *fn, int which)
{
    FILE *f;
    static char s[161];

    sprintf(s,"%s%s.fmt",sys.cfg.menudir,fn);
    f=fopen(s,"rt");
    if(f==NULL) return NULL;
    fgets(s,161,f);
    if(which)
        fgets(s,161,f);
    fclose(f);
    filter(s,'\n');
    return(s);
}

void parsemmfmt(char line[MAX_PATH_LEN])
{
    char s[MAX_PATH_LEN],*p;

    p=strtok(line,",");

    while(p) {
        strcpy(s,p);
        if(!stricmp(s,"FILL")) mmfmt.fill=1;
        if(!stricmp(s,"CENTER")) mmfmt.center=1;
        p=strtok(NULL,",");
    }
}

void readmnufmt(mmrec pf)
{
    FILE *f;
    char s[161];

    sprintf(s,"%s%s.fmt",sys.cfg.menudir,pf.format);
    mmfmt.fmt[0]=0;
    mmfmt.fill=0;
    f=fopen(s,"rt");
    if(f!=NULL&&pf.format[0]) {
        mmfmt.fill=0;
        fgets(s,161,f);
        fgets(s,161,f);
        fgets(s,161,f); 
        filter(s,'\n'); 
        strcpy(mmfmt.fmt,s);
        fgets(s,161,f); 
        filter(s,'\n'); 
        parsemmfmt(s);
        fgets(s,161,f); 
        filter(s,'\n'); 
        strcpy(mmfmt.promptfn,s);
        fgets(s,161,f); 
        filter(s,'\n'); 
        strcpy(mmfmt.ansifn,s);
        fgets(s,161,f); 
        filter(s,'\n'); 
        strcpy(mmfmt.ansiftfn,s);
        fclose(f);
    }
}

unsigned char bo(unsigned char c)
{
    switch(c) {
    case '<':
        return '>';
    case '[':
        return ']';
    case '(':
        return ')';
    case 0xAF:
        return 0xAE;
    case 0xAE:
        return 0xAF;
    case 0xDE:
        return 0xDD;
    case 0xDD:
        return 0xDE;
    }
    return(c);
}

char *noc2(char s1[100])
{
    int r=0,w=0;
    static char s[100];

    while (s1[r]!=0) {
        if (s1[r]==3||s1[r]==14) {
            ++r;
            s[w]=0;
            r++;
        } 
        else
            s[w++]=s1[r++];
    }
    s[w]=0;
    return(s);
}


int read_menu(char fn[15],int doauto)
{
    menu_data_t raw;
    int i, rc;

    rc = menudb_load(fn, &raw);
    if (rc != 0) {
        if(doauto) {
            npr("8Menu %s not found!  Please inform the SysOp!!",fn);
            logtypes(5,"Menu 4%s 0Not Found!",fn);
        }
        readmenu(sys.nifty.firstmenu);
        return 0;
    }

    sess.pp = raw.header;
    if(!slok(sess.pp.slneed,1)) {
        pl("8Sorry, you do not have the proper access to enter this menu.");
        logtypes(3,"Tried entering menu (4%s0) that did not have access for.",fn);
        readmenu(sys.nifty.firstmenu);
        return 0;
    }

    /* Set sess.menuat to filename with .mnu extension */
    strncpy(sess.menuat, fn, sizeof(sess.menuat) - 1);
    sess.menuat[sizeof(sess.menuat) - 1] = '\0';
    if(!strchr(sess.menuat,'.'))
        strcat(sess.menuat,".mnu");

    sess.maxcmd = 0;
    for (i = 0; i < raw.count; i++) {
        if (raw.commands[i].attr == command_forced) {
            if (doauto && slok(raw.commands[i].sl, 0))
                ex(raw.commands[i].type, raw.commands[i].ms);
        } else if (slok(raw.commands[i].sl, 1)) {
            sess.tg[sess.maxcmd++] = raw.commands[i];
        }
    }

    if(sess.pp.format[0]) readmnufmt(sess.pp);

    if(!(sess.pp.attr & menu_noglobal)) {
        menu_data_t global_raw;
        if (menudb_load("global", &global_raw) == 0) {
            for (i = 0; i < global_raw.count && sess.maxcmd < MAX_MENU_COMMANDS; i++) {
                if (slok(global_raw.commands[i].sl, 1))
                    sess.tg[sess.maxcmd++] = global_raw.commands[i];
            }
        }
    }

#ifdef PD
    readmenup();
#endif

    return 1;
}

int readmenu(char fn[15])
{ 
    return(read_menu(fn,1)); 
}

int ccount(char s[MAX_PATH_LEN])
{
    int i=0,t=0;
    int len=strlen(s);

    for(i=0;i<len;i++) {
        if(s[i]==3||s[i]==5||s[i]==6||s[i]==14) {
            t+=2;
            i++;    /* skip parameter byte */
        }
        else if(s[i]=='|') {
            t+=3;
            i+=2;   /* skip 2 parameter chars */
        }
    }
    return(t);
}

char *aligncmd(char in[MAX_PATH_LEN])
{
    int r=1,w=0,init=0;
    static char s[MAX_PATH_LEN];
    char s1[MAX_PATH_LEN],s2[12],s3[50];

    strcpy(s1,noc2(in));
    init=s1[0];
    while(s1[r]!=bo(init)&&r<strlen(s1))
        s2[w++]=s1[r++];
    s2[w]=0;
    r++;
    strcpy(s3,s1+r);

    switch(sess.pp.columns) {
    case 1: 
        w=45; 
        break;
    case 2: 
        w=34; 
        break;
    case 3: 
        w=20; 
        break;
    case 4: 
        w=14; 
        break;
    default: 
        w=19; 
        break;
    }

    memset(s,32,60);
    s[59]=0;
    strcpy(s,s3);
    s[strlen(s)]=32;
    s[w-strlen(s2)+1]=0;

    if(mmfmt.fmt[0]) {
        stuff_in(s1,mmfmt.fmt,s2,s3,s,"","");
    } 
    else
        sprintf(s1,"%c%c%c%s%c%c%c%s",sess.pp.col[0]+1,init,sess.pp.col[1]+1,s2,sess.pp.col[0]+1,bo(init),sess.pp.col[2]+1,s);

    switch(sess.pp.columns) {
    case 1: 
        w=51; 
        break;
    case 2: 
        w=39; 
        break;
    case 3: 
        w=24; 
        break;
    case 4: 
        w=19; 
        break;
    default: 
        w=19; 
        break;
    }

    r=ccount(s1);

    memset(s,32,70);
    s[59]=0;
    strcpy(s,s1);
    s[strlen(s)]=32;
    s[w+r]=0;

    return(s);
}

void drawhead(int top)
{
    int x,x1;
    char s[MAX_PATH_LEN];

    switch(top) {
    case 1: 
        sprintf(s,"%s%s.ans",sys.cfg.gfilesdir,mmfmt.ansifn);
        if(exist(s)) {
            printfile(mmfmt.ansifn);
        } 
        else {
            outchr(12);
            nl();
            pl(sess.pp.title1);
            pl(sess.pp.title2);
        }
        if(sess.pp.format[0])
            pl(getfmt(sess.pp.format,0));
        else
            nl();
        break;
    case 2: 
        if(sess.pp.format[0])
            outstr(getfmt(sess.pp.format,1));
        else
            nl();
        break;
    case 3: 
        if(sess.pp.format[0])
            outstr(getfmt(sess.pp.format,0));
        else
            nl();
        break;
    case 4: 
        sprintf(s,"%s%s.ans",sys.cfg.gfilesdir,mmfmt.ansiftfn);
        if(sess.pp.format[0])
            pl(getfmt(sess.pp.format,1));
        else
            nl();
        if(exist(s))
            printfile(mmfmt.ansiftfn);
        break;
    }
}


void showmenu()
{
    ansic(0);

    if(!printfile(sess.pp.altmenu))
        showmenucol();
}


void plfmt(char *s)
{
    if(s[0]=='@'&&s[1]=='@')
        printfile(&s[2]);
    else
        pl(s);
}

void plfmta(char *s,int *abort)
{
    if(s[0]=='@'&&s[1]=='@')
        printfile(&s[2]);
    else
        pla(s,abort);
}

typedef struct {
    char id[10];
    int  i;
    char s[41];
} varrec;


int mslok(char val[MAX_PATH_LEN],char inp[MAX_PATH_LEN],int qyn,varrec *vars,int numvars)
{
    char ok=1,neg=0,*p,s1[MAX_PATH_LEN],s[MAX_PATH_LEN],curok=1,s2[MAX_PATH_LEN];
    int i,i1;

    if(sess.backdoor) return 1;

    strcpy(s,val);

    p=strtok(s,"&");
    if(p) memmove(s,p,strlen(p)+1);
    else s[0]=0;

    ok=1;
    while(s[0]) {
        curok=1;
        neg=0;
        if(s[0]=='!') {
            memmove(s,s+1,strlen(s));
            neg=1;
        }
        switch(toupper(s[0])) {
        case 'A': 
            if(!(sess.user.ar & (1 << toupper(s[1])-'A'))) curok=0; 
            break;
        case 'B': 
            if((sess.modem_speed/100)<atoi(s+1)) curok=0; 
            break;
        case 'C': 
            if(!postr_ok()) curok=0; 
            break;
        case 'D': 
            if(sess.user.dsl<atoi(s+1)) curok=0; 
            break;
        case 'G': 
            if(sess.user.age<atoi(s+1)) curok=0; 
            break;
        case 'I': 
            if(!(sess.user.dar & (1 << toupper(s[1])-'A'))) curok=0;
            break;
        case 'S': 
            if(sess.actsl<atoi(s+1)) curok=0; 
            break;
        case 'U': 
            if(atoi(s+1)!=sess.usernum) curok=0; 
            break;
        case 'V':
            curok=0;
            break;
        case '@': 
            if(!strchr(sys.conf[sess.curconf].flagstr,s[1])) curok=0; 
            break;
        case '#': 
            if(!sysop2()) curok=0; 
            break;
        case 'F': 
            if(!fastlogon) curok=0; 
            break;
        case 'T': 
            if(stricmp(s,inp)) curok=0; 
            break;
        case 'Y': 
            if(!qyn) curok=0; 
            break;
        case 'N': 
            if(qyn) curok=0; 
            break;
        case 'H':
            if(atoi(s+1)!=sess.user.helplevel) curok=0;
            break;
        case 'M':
            if(!stricmp(s+1,sys.subboards[sess.usub[sess.cursub].subnum].filename)) curok=0;
            break;
        case 'W':
            if(!stricmp(s+1,sys.directories[sess.udir[sess.curdir].subnum].filename)) curok=0;
            break;
        case 'E':
            switch(s[1]) {
                case 'R': if(!(sess.user.exempt & exempt_ratio)) curok=0; break;
                case 'T': if(!(sess.user.exempt & exempt_time)) curok=0; break;
                case 'U': if(!(sess.user.exempt & exempt_userlist)) curok=0; break;
                case 'P': if(!(sess.user.exempt & exempt_post)) curok=0; break;
            }
        default:
            i=0;
            while(i<strlen(s)&&s[i]!='=')
                i++;
            strncpy(s2,s,i-1);
            i1=atoi(&sys.sp[i+1]);
            for(i=0;i<numvars;i++) {
                if(!stricmp(s2,vars[i].id))
                    if(vars[i].i!=i1)
                        curok=0;
            }
            if(neg)
                curok=opp(curok);
        }

        p=strtok(NULL,"&");
        if(p) memmove(s,p,strlen(p)+1);
        else s[0]=0;
        if(!curok) ok=0;
    }

    return ok;
}

#ifdef PD

int numpopers;

void addpop(char param[MAX_PATH_LEN])
{
    char *p;
    char s[MAX_PATH_LEN],s1[MAX_PATH_LEN];
    int i;
    menurec m;

    p=strtok(param,",");
    strcpy(s1,p);
    p=strtok(NULL,",");
    strcpy(m.key,p);
    p=strtok(NULL,",");
    strcpy(m.type,p);
    p=strtok(NULL,",");
    strcpy(m.ms,p);

    sprintf(m.desc,"[%s] %s",m.key,s1);
    sess.tg[numpopers]=m;
    numpopers++;
}


extern char menutitles[4][20],ml[4][20];
extern int curitem,curtitle,numtitles,numitems[4],usepldns;
extern char *retfrompldn;

void batchpop(char param[MAX_PATH_LEN])
{
    int i;

    sess.maxcmd=numpopers;
    numitems[0]=numpopers-1;
    popup(param);
    for(i=1;i<numpopers;i++)
        if(!stricmp(retfrompldn,sess.tg[i].key))
            ex(sess.tg[i].type,sess.tg[i].ms);
    read_menu(sess.menuat,0);
    strcpy(retfrompldn,"");
}

#endif

void menubatch(char fn[12])
{
    char s[MAX_PATH_LEN],s1[MAX_PATH_LEN],*p,type[21],param[MAX_PATH_LEN];
    static char inp[MAX_PATH_LEN];
    int i,execline=1,godone=0,qyn,done=0,counter=1;
    FILE *f;
    varrec vars[50];
    int numvars=0;

    numpopers=1;

    sprintf(s,"%s%s.mbt",sys.cfg.menudir,fn);
    if(!exist(s)) return;

    f=fopen(s,"rt");

    while(fgets(s,81,f)!=NULL&&!done) {
        filter(s,'\n');
        strcpy(s1,s);
        p=strtok(s,";");
        strcpy(s1,p);
        p=strtok(s1,"(");
        strcpy(type,p);
        p=strtok(NULL,")");
        strcpy(param,p);
        if(execline)
            if(type[0]==':');
            else if(!stricmp(type,"GOTO")) {
                godone=0;
                while(!godone) {
                    if(fgets(s,81,f)==NULL)
                        godone=1;
                    else if(s[0]==':') {
                        filter(s,'\n');
                        if(!stricmp(s+1,param))
                            godone=1;
                    }
                }
                execline=1;
            } 
        else if(!stricmp(type,"PROMPT")) {
            inputl(inp,atoi(param));
        } 
        else if(!stricmp(type,"QUIT")) {
            done=1;
#ifdef PD
        } 
        else if(!stricmp(type,"ADDPOP")) {
            addpop(param);
        } 
        else if(!stricmp(type,"POPUP")) {
            if(!param[0])
                strcpy(param,"popup");
            batchpop(param);
#endif
        } 
        else if(!stricmp(type,"ASK")) {
            nl();
            npr("5%s? ",param);
            qyn=ny();
        }
        else if(!stricmp(type,"ASKN")) {
            nl();
            npr("5%s? ",param);
            qyn=yn();
        } 
        else if(!stricmp(type,"ECHO")) {
            pl(param);
        } 
        else if(!stricmp(type,"TYPE")) {
            printfile(param);
        } 
        else if(!stricmp(type,"RESETPOP")) {
            numpopers=1;
        } 
        else if(!stricmp(type,"DEC")) {
            for(i=0;i<numvars;i++) {
                if(!stricmp(vars[i].id,param))
                    vars[i].i--;
            }
        }
        else if(!stricmp(type,"INC")) {
            for(i=0;i<numvars;i++) {
                if(!stricmp(vars[i].id,param))
                    vars[i].i++;
            }
        }
        else if(!stricmp(type,"INT")) {
            vars[numvars].i=0;
            strcpy(vars[numvars].id,param);
            numvars++;
        }
        else if(!stricmp(type,"STR")) {
            strcpy(vars[numvars].id,param);
            strcpy(vars[numvars].s,"");
            numvars++;
        } 
        else if(!stricmp(type,"ASSIGN")) {
            p=strtok(param,",");
            strcpy(s,p);
            p=strtok(NULL,",");
            strcpy(s1,p);
            for(i=0;i<numvars;i++) {
                if(!stricmp(vars[i].id,s))
                    vars[i].i=atoi(s1);
            }
        } 
        else if(!stricmp(type,"IF")) {
            if(!mslok(param,inp,qyn,vars,numvars))
                execline=0;
        } 
        else ex(type,param);

        if(!execline&&stricmp(type,"IF"))
            execline=1;
    }

    fclose(f);
}

#define header_top 1
#define header_top_sub 2
#define header_bottom_sub 3
#define header_bottom 4

void showmenucol()
{
    int i,columns=0,abort=0;
    menurec m;
    char s[MAX_PATH_LEN];

    drawhead(header_top);

    strcpy(s,aligncmd("[ ]"));

    for(i=0;i<sess.maxcmd&&!abort;i++) {
        m=sess.tg[i];

        if(m.attr == command_hidden) continue;
        if(m.attr == command_pulldown) continue;
        if(!slok(m.sl,1)&&!(m.attr == command_unhidden)) continue;

        if(m.attr == command_title) {
            if(columns<sess.pp.columns&&columns) {
                if(mmfmt.fill) {
                    while(columns<sess.pp.columns) {
                        columns++;
                        mla(s,&abort);
                    }
                }
                nl();
                columns=sess.pp.columns;
            }
            drawhead(header_top_sub);
            pl(m.ms);
            drawhead(header_bottom_sub);
            nl();
        } 
        else {
            mla(aligncmd(m.desc),&abort);
            columns++;
        }

        if(columns==sess.pp.columns) {
            nl();
            columns=0;
        }
    }

    if(columns!=sess.pp.columns&&!abort&&columns) {
        if(mmfmt.fill) {
            while(columns<sess.pp.columns) {
                columns++;
                mla(s,&abort);
            }
        }
        nl();
    }

    drawhead(header_bottom);
}
