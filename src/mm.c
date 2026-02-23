#include "vars.h"
#pragma hdrstop

#include "menudb.h"

extern int SYSTEMDEBUG;
menurec tg[50];
mmrec pp;

extern struct {
    char         fmt[41],
    fill,
    promptfn[8],
    ansifn[8],
    ansiftfn[8],
    center;
} 
mmfmt;


void handleinput(char *s,int begx);

#define sysstatus_pause_on_message 0x0400


#ifdef PD
extern int usepldns,usepop;
extern char *retfrompldn;
#endif

extern int fastlogon;
extern char registered_name[201];


void packm(void);

char maxcmd,menuat[15],mstack[10][15],mdepth=0;

void conv(void);

int slok(char val[31],char menu)
{
    char ok=1,neg=0,*p,s1[MAX_PATH_LEN],s[MAX_PATH_LEN],curok=1;
    int i;

    if(backdoor) return 1;

    strcpy(s,val);

    p=strtok(s,"&");
    if(p) memmove(s,p,strlen(p)+1);
    else s[0]=0;

    ok=1;
    if(menu==3) {
        menu=0;
    }
    while(s[0]) {
        curok=1;
        neg=0;
        if(s[0]=='!') {
            memmove(s,s+1,strlen(s));
            neg=1;
        }
        switch(toupper(s[0])) {
        case 'A': 
            if(!(thisuser.ar & (1 << toupper(s[1])-'A'))) curok=0; 
            break;
        case 'B': 
            if((modem_speed/100)<atoi(s+1)) curok=0; 
            break;
        case 'C': 
            if(!menu) { 
                if(!postr_ok()); 
                curok=0;
                pl(get_string(40));
            } 
            break;
        case 'D': 
            if(thisuser.dsl<atoi(s+1)) curok=0; 
            break;
        case 'G': 
            if(thisuser.age<atoi(s+1)) curok=0; 
            break;
        case 'I': 
            if(!(thisuser.dar & (1 << toupper(s[1])-'A'))) curok=0;
            break;
        case 'S': 
            if(actsl<atoi(s+1)) curok=0; 
            break;
        case 'U': 
            if(atoi(s+1)!=usernum) curok=0; 
            break;
        case 'V': 
            if(!running_dv) curok=0; 
            break;
        case '@': 
            if(!strchr(conf[curconf].flagstr,s[1])) curok=0; 
            break;
        case '#': 
            if(!sysop2()) curok=0; 
            break;
        case 'F': 
            if(!fastlogon) curok=0; 
            break;
        }
        if(neg) curok=opp(curok);
        p=strtok(NULL,"&");
        if(p) memmove(s,p,strlen(p)+1);
        else s[0]=0;
        if(!curok) ok=0;
    }

    return ok;
}


void msgcommand(char type,char ms[40])
{
    int c,c1,ok=1,i;
    char s[MAX_PATH_LEN],*p;
    unsigned long l;

    switch(type) {
    case 'Y': 
        yourinfomsg();
#ifdef PD
        if(usepldns) pausescr();
#endif
        break;
    case 'P': 
        post(cursub); 
        break;
    case 'N':
        if(ms[1]=='?') {
            p=strtok(ms,";");
            p=strtok(NULL,"");
            if(p!=NULL) outstr(p);
            else
                outstr(get_string2(7));
            ok=yn();
        }
        if(!ok) break;
        if(ms[0]=='G') {
            logtypes(1,"Newscaned All Message Areas");
            gnscan();
        } 
        else if (ms[0]=='C') {
            nscan(usub[cursub].subnum,&i);
            logtypes(1,"NewScaned Message Area 4%s",subboards[usub[cursub].subnum].name);
        } 
        else if(ms[0]=='A') {
            strcpy(s,conf[curconf].flagstr);
            strcpy(conf[curconf].flagstr,"@");
            changedsl();
            logtypes(1,"NewScaned All Conferences");
            gnscan();
            strcpy(conf[curconf].flagstr,s);
            changedsl();
        }
        else if(!ms[0]) {
            nl();
            outstr(get_string2(8));
            c=onek("\rYNA");
            switch(c) {
            case 'A': 
                strcpy(s,conf[curconf].flagstr);
                strcpy(conf[curconf].flagstr,"@");
                changedsl();
                logtypes(1,"NewScaned Messages All Conferences");
                gnscan();
                strcpy(conf[curconf].flagstr,s);
                changedsl();
                break;
            case 'Y':
            case '\r': 
                logtypes(1,"Global Newscaned Message Areas");
                gnscan(); 
                break;
            case 'N':  
                logtypes(1,"NewScaned Message Area 4%s",subboards[usub[cursub].subnum].name);
                nscan(usub[cursub].subnum,&i); 
                break;
            }
        }
        break;
    case 'S': 
        rscanj();  
        break;
    case 'M': 
        readmailj(atoi(ms),0); 
        break;
    case 'E': 
        cursub=0; 
        smail(ms);
        break;
    case 'U': 
        upload_post(); 
        break;
#ifdef QWK
    case 'W': 
        makeqwk(ms[0]); 
        break;
    case 'Z': 
        qwkreply(); 
        break;
#endif
    default: 
        badcommand('M',type);
    }
}

void othercmd(char type,char ms[40])
{
    int i;
    char c,s[MAX_PATH_LEN];

    switch(type)
    {
    case 'X':
        if(!ms[0])
            break;
        set_autoval(atoi(ms)-1);
        break;
    case 'D': 
        selfval(ms); 
        break;
    case 'W': 
        lastfewcall(); 
        break;
    case ';': 
        for(c=0;c<strlen(ms);c++) {
            if(ms[c]==';') ms[c]=13;
        }
        ms[strlen(ms)+1]=13;
        strcpy(charbuffer,&ms[0]);
        charbufferpointer = 1;
        break;
    case '1': 
        oneliner(); 
        break;
    case 'E':
    case 'G': 
        infoform(ms,type=='E');
        logpr(0,"Took InfoForm 4%s",ms);
        break;
    case 'R': 
        logtypes(2,"Read Infoforms");
        pl("Which User's responses do you want to see?");
        outstr(": ");
        input(s,31);
        if(s[0]) readform(ms,s); 
        break;
    case 'C': 
        if(!(nifty.nifstatus & nif_chattype)) reqchat(ms);
        else reqchat1(ms); 
        break;
    case 'U': 
        logtypes(2,"Listed Users");
        list_users();
#ifdef PD
        if(usepldns) pausescr();
#endif
        break;
    case 'L': 
        pl(ms); 
        break;
    case 'F': 
        printfile(ms); 
        break;
    case 'I': 
        nl();
        npr("%s, Compiled %s\r\n",wwiv_version,wwiv_date);
#ifndef BETA
        npr("Registered to: %s\r\n",registered_name);
#endif
        nl();
        pausescr();
        printfile("system");
        pausescr();
        break;
    case 'O':  
        if (sysop2()) pl(get_string(4));
        else pl(get_string(5));
        break;
#ifdef PD
    case 'Y':  
        yourinfo(); 
        if(usepldns) pausescr(); 
        break;
#endif
    case '|':  
        addsay(); 
        break;
    case 'S':  
        get_say(1); 
        break;
    case '\\': 
        searchrum(); 
        break;
    case 'T':  
        bank2(atoi(ms)); 
        break;
    case 'A': 
        today_history(); 
        break;
#ifdef NUV
    case 'N': 
        nuv(); 
        break;
#endif
    case 'P':
        switch(ms[0]) {
        case '1': 
            input_screensize(); 
            break;
        case '2': 
            input_ansistat(); 
            break;
        case '3': 
            if (thisuser.sysstatus & sysstatus_pause_on_page)
                thisuser.sysstatus ^= sysstatus_pause_on_page;
            prt(5,"Pause each screenfull? ");
            if (yn()) thisuser.sysstatus |= sysstatus_pause_on_page;
            nl();
            if (thisuser.sysstatus & sysstatus_pause_on_message)
                thisuser.sysstatus ^= sysstatus_pause_on_message;
            prt(5,"Pause each screenfull while reading messages? ");
            if (yn()) thisuser.sysstatus |= sysstatus_pause_on_message;
            break;
        case '4': 
            modify_mailbox(); 
            break;
        case '5': 
            config_qscan(0); 
            break;
        case '6': 
            input_pw1(); 
            break;
        case '7': 
            make_macros(); 
            break;
        case '8': 
            configpldn(1); 
            break;
        case '9': 
            outstr("5Use the FullScreen Editor? ");
            thisuser.defed=yn();
            break;
        case '0': 
            getmsgformat(); 
            break;
        case 'A': 
            pl("Enter your Default Protocol, 0 for none.");
            i=get_protocol(1); 
            if(i>=0||i==-2) thisuser.defprot=i;
            break;
        case 'B': 
            configpldn(0); 
            break;
        case 'C': 
            selecthelplevel(); 
            break;
        case 'D': 
            getfileformat(); 
            break;
        case 'E': 
            config_qscan(1); 
            break;
        case 'F': 
            print_cur_stat();
#ifdef PD
            if(usepldns) pausescr();
#endif
            break;
        case 'G': 
            change_colors(&thisuser);  
            break;
        case 'H': 
            inputdat("Enter your Comment",s,sizeof(thisuser.comment),1);
            if(s[0]) strcpy(thisuser.comment,s);
            break;
        case 'J': 
            if(thisuser.sysstatus & sysstatus_fullline)
                thisuser.sysstatus ^= sysstatus_fullline;
            npr("5Would you like Hotkey Input? ");
            if(!ny())
                thisuser.sysstatus |= sysstatus_fullline;
            nl();
            break;
        }
    }
}

int ex(char type[2],char ms[MAX_PATH_LEN])
{
    int abort,i,i1;
    long l;
    char s[255],s1[MAX_PATH_LEN],*s2,c,ok=1,*p;


    switch(type[0]) {
    case '?': 
        showmenu(); 
        break;
        /*    case '!':
                                mailsys(ms[0]); 
                                break;*/
    case 'J': 
        amsgcommand(type[1]); 
        break;
    case 'D': 
        rundoor(type[1],ms); 
        break;
    case 'M': 
        msgcommand(type[1],ms);  
        break;
    case 'F':
        switch(type[1]) {
        case 'Z': 
            inputdat("Enter Path to Use (Include Trailing Backslash)",s,60,1);
            insert_dir(umaxdirs,s,99,0);
            udir[umaxdirs].subnum=umaxdirs;
            curdir=umaxdirs;
            logtypes(3,"Created temporary path in 3%s",s);
            break;
        case 'M': 
            mark(curdir); 
            break;
        case 'L': 
            listfiles(ms); 
            break;
        case 'B': 
            batchdled(0);
#ifdef PD
            if(usepldns) pausescr();
#endif
            break;
        case 'G': 
            listgen(); 
            break;
        case 'Y': 
            yourinfodl();
#ifdef PD
            if(usepldns) pausescr();
#endif
            break;
        case 'D': 
            if(ms[0]) newdl(atoi(ms)); 
            else newdl(curdir); 
            break;
        case 'V': 
            arc_cl(1); 
            break;
        case 'R': 
            removefile();
            break;
        case 'P': 
            setldate(); 
            break;
        case 'F': 
            finddescription(ms); 
            break;
        case '1': 
            localupload(); 
            break;
        case '2': 
            editfile(); 
            break;
        case '3': 
            nl();
            outstr("5Validate Globally? ");
            if(yn()) {
                i=curdir;
                for(curdir=0;curdir<num_dirs&&udir[curdir].subnum>=0;curdir++)
                    valfiles();
                curdir=i;
            } 
            else valfiles();
            break;
        case '5': 
            move_file();
            break;
        case '6': 
            sort_all(ms); 
            logtypes(3,"Sorted Files"); 
            break;
        case '8': 
            unlisteddl(ms); 
            break;
        case '9': 
            arc_cl(0); 
            logtypes(3,"Added Archive Comments"); 
            break;
        case '0': 
            arc_cl(2); 
            logtypes(3,"Added Gif Comment to Files"); 
            break;
        case '!': 
            arc_cl(3); 
            logtypes(3,"Added Automatic File Descriptions"); 
            break;
        case 'C': 
            create_file(); 
            break;
        case 'U': 
            upload(ms); 
            break;
        case 'N': 
            lines_listed=0;
            nl();
            if(ms[1]=='?') {
                p=strtok(ms,";");
                p=strtok(NULL,"");
                if(p!=NULL) outstr(p);
                else
                    outstr(get_string2(7));
                ok=yn();
            }
            if(!ok) break;
            abort=0;
            if(ms[0]=='C') {
                setformat();
                nscandir(curdir,&abort,0,&i);
                logtypes(1,"NewScaned File Area %s",directories[udir[curdir].subnum].name);
                break;
            }
            if(ms[0]=='G') {
                nscanall();
                logtypes(1,"NewScaned All File Areas");
                break;
            } 
            else if(ms[0]=='A') {
                strcpy(s,conf[curconf].flagstr);
                strcpy(conf[curconf].flagstr,"@");
                changedsl();
                logtypes(1,"NewScaned File Areas on All Conferences");
                nscanall();
                strcpy(conf[curconf].flagstr,s);
                changedsl();
            }
            else if(!ms[0]) {
                nl();
                outstr(get_string2(8));
                c=onek("\rYNA");
                switch(c) {
                case 'A': 
                    strcpy(s,conf[curconf].flagstr);
                    strcpy(conf[curconf].flagstr,"@");
                    changedsl();
                    logtypes(1,"NewScaned File Areas on All Conferences");
                    nscanall();
                    strcpy(conf[curconf].flagstr,s);
                    changedsl();
                    break;
                case 'Y':
                case '\r': 
                    logtypes(1,"Global Newscaned File Areas");
                    nscanall(); 
                    break;
                case 'N':  
                    nscandir(curdir,&abort,0,&i);
                    logtypes(1,"NewScaned File Area %s",directories[udir[curdir].subnum].name);
                }
            }
            break;
        default: 
            badcommand('F',type[1]);
        } 
        break;
    case 'S': 
        sysopcmd(type[1],ms); 
        break;
    case 'O': 
        othercmd(type[1],ms); 
        break;
    case 'I': 
        hangupcmd(type[1],ms); 
        break;
    case 'W': 
        matrixcmd(type[1]); 
        break;
    case 'Q': 
        switch(type[1])
        {
        case 'A': 
            addbbs(ms[0]?ms:"bbslist.msg"); 
            break;
        case 'R': 
            printfile(ms[0]?ms:"bbslist.msg");
            pausescr();
            break;
        case 'S': 
            searchbbs(ms[0]?ms:"bbslist.msg");
            break;
        default: 
            badcommand('Q',type[1]);
        } 
        break;

    case '=':
        switch(type[1])
        {
        case 'F': 
            if(ms[0]) strcpy(pp.format,ms);
            readmnufmt(pp);
            break;
        case 'G': 
            p=strtok(ms,",;");
            i=atoi(p);
            p=strtok(NULL,";,");
            i1=atoi(p);
            go(i,i1);
            break;
        case 'B': 
            menubatch(ms); 
            return 0;
        case '#': 
            break;
        case 'J': 
            jumpconf(ms); 
            break;
        case ']': 
            curconf++; 
            if(curconf>=num_conf) curconf=0;
            changedsl();
            break;
        case '[': 
            curconf--; 
            if(curconf<0) curconf=num_conf-1;
            changedsl();
            break;
        case '/': 
            nl();
            if(mdepth+1>9) {
                mdepth=2;
                strcpy(mstack[0],nifty.firstmenu);
                strcpy(mstack[1],menuat);
            } 
            else
                strcpy(mstack[mdepth++],menuat);
            if(!readmenu(ms))
                readmenu(mstack[--mdepth]);
            return 0;
        case '\\': 
            nl();
            if(mdepth) {
                if(!read_menu(mstack[--mdepth],0))
                    readmenu(nifty.firstmenu);
            } 
            else {
                mdepth=0;
                strcpy(mstack[0],nifty.firstmenu);
                readmenu(nifty.firstmenu);
            }
            return 0;
        case '^': 
            nl();
            mdepth=0;
            if(!readmenu(ms))
                readmenu(nifty.firstmenu);
            return 0;
        case '*': 
            if(ms[0]=='M') cursub=sublist(ms[1]);
            else if(ms[0]=='F') curdir=dirlist(ms[1]);
#ifdef PD
            if(usepldns) pausescr();
#endif
            break;
        case '+': 
            if(ms[0]=='M') {
                if(cursub<64 && usub[cursub+1].subnum>=0) {
                    ++cursub;
                    msgr++;
                }
                else cursub=0;
                msgr=1;
            }
            else if(ms[0]=='F') {
                if(directories[udir[curdir].subnum].type)
                    changedsl();
                if(curdir<64 && udir[curdir+1].subnum>=0) ++curdir;
                else curdir=0;
            }
            break;
        case '-': 
            if(ms[0]=='M') {
                if (cursub>0)
                    --cursub;
                else
                    while ((usub[cursub+1].subnum>=0) && (cursub<64))
                    ++cursub;
                msgr=1;
            } 
            else if(ms[0]=='F') {
                if(directories[udir[curdir].subnum].type)
                    changedsl();
                if (curdir>0) curdir--;
                else
                    while ((udir[curdir+1].subnum>=0) && (curdir<63)) ++curdir;
            }
            break;
        default: 
            badcommand('=',type[1]);
        } 
        break;
    default: 
        badcommand(type[0],type[1]);
    }
    return 1;
}




void menuman(void)
{
    char cmd,c,begx,s[161],test[MAX_PATH_LEN];
    char *ss=s;
    int i,helpl,i1,x;
    menurec mm;
    char avail[61];
    FILE *ff;

    if(!usepldns&&!(pp.attr & menu_popup)&&!charbufferpointer) {
        nl();
        nl();
    }

    tleft(1);

    if(usub[cursub].subnum==-1) cursub=0;
    helpl=thisuser.helplevel;
    if(pp.helplevel) helpl=pp.helplevel-1;
#ifdef PD
    if(!usepldns&&!(pp.attr & menu_popup)&&!charbufferpointer)
#endif PD
        switch(helpl) {
        case 0: 
            break;
        case 2: 
            showmenu();
        case 1: 
            ansic(13);
            npr("[0");
            for(i=0;i<maxcmd;i++)
                if(!(tg[i].attr==command_pulldown)&&
                    !(tg[i].attr==command_title)&&
                    !(tg[i].attr==command_hidden))
                    npr("%s,",tg[i].key);
            backspace();
            ansic(13);
            npr("]\r\n");
            break;
        }

    if(chatcall&&chatsoundon) chatsound();
    memset(s,0,40);
#ifdef PD
    if((pp.attr & menu_popup)&&okansi()) {
        popup("popup");
        strcpy(s,retfrompldn);
    } 
    else if(usepldns&&okansi()) {
        pldn();
        strcpy(s,retfrompldn);
    } 
    else {
#endif
        sprintf(s,"%s%s.prm",syscfg.menudir,mmfmt.promptfn);
        if(exist(s)&&(pp.attr & menu_extprompt)) {
            if(pp.attr & menu_promptappend)
                pl(pp.prompt);
            ff=fopen(s,"rt");
            while(fgets(s,161,ff)!=NULL) {
                filter(s,'\n');
                outstr(s);
            }
            fclose(ff);
        } 
        else
            outstr(pp.prompt);
        begx=wherex();
        i1=c=0;
        for(i=0;i<maxcmd;i++) {
            if(tg[i].key[0]=='#') i1=1;
            else {
                if(strlen(tg[i].key)==1)
                    avail[c++]=tg[i].key[0];
                else if(strlen(tg[i].key)==2&&tg[i].key[0]=='/')
                    avail[c++]=tg[i].key[1];
            }
        }
        avail[c++]='?';
        avail[c]=0;

        if(pp.boarder==0) {
            if(thisuser.sysstatus & sysstatus_fullline)
                ss=smkey(avail,i1,1,1,1);
            else
                ss=smkey(avail,i1,1,0,0);
        } 
        else if(pp.boarder==1)
            ss=smkey(avail,i1,1,0,0);
        else
            input(ss,51);

        strcpy(s,ss);
        if(SYSTEMDEBUG)
            sysoplog(s);
#ifdef PD
    }
#endif

    if(!s[0]) {
        for(cmd=0;cmd<maxcmd;cmd++) {
            if(tg[cmd].attr==command_default)
                if(!ex(tg[cmd].type,tg[cmd].ms)) cmd=maxcmd;
        }
    } 
    else if((!strcmp(s,"AVAIL"))) {
        for(i=0;i<maxcmd;i++) if(slok(tg[i].sl,1)) npr("2%s0,",tg[i].key);
        backspace();
        nl();
    } 
    else
        if((!strcmp(s,"HELP"))) {
            printmenu(7);
#ifdef PD
            if(usepldns||(pp.attr & menu_popup)) pausescr();
#endif
        } 
    else
#ifdef PD
        if((!strcmp(s,"PULL"))) { 
        usepop=0; 
        usepldns=opp(usepldns); 
    } 
    else
#endif
        if((!strcmp(s,"SPEED"))) npr("ComSpeed=%d, ModemSpeed=%d\r\n",com_speed,modem_speed); 
    else
        if((!strcmp(s,"RELOAD"))) {
            if(actsl<=syscfg.newusersl)
                readmenu(nifty.newusermenu);
            else
                readmenu(nifty.firstmenu);
        }  
    else if((!strcmp(s,"VER"))) {
        nl();
        npr("%s, Compiled %s\r\n",wwiv_version,wwiv_date);
#ifndef BETA
        npr("Registered to: %s\r\n",registered_name);
#endif
        if(running_dv)
            pl(get_string(49));
        nl();
        i1=0;
    } 
        else if((!strcmp(s,"GRAPHICS"))) ex("OP","2");
        else if((!strcmp(s,"CLS"))) {
            if(okansi()) {
                makeansi(15, test, curatr);
                outstr(test);
            }
            outchr(12);
        }
    else if(strstr(s,",")||strstr(s," ")) {
        for(c=0;c<strlen(s);c++) {
            if(s[c]==','||s[c]==32)
                s[c]=13;
        }
        c=strlen(s);
        s[c]=13;
        s[c+1]=0;
        strcpy(&charbuffer[1],&s[0]);
        charbuffer[0]=';';
        charbufferpointer = 1;
    }

    else
        if((!strcmp(s,get_string(37))) && checkacs(6)) {
        getcmdtype();
    } 
    else
        if((!strcmp(s,"/OFF"))) hangup=1; 
    else
        if(s[0]=='?') { 
        if(helpl!=2) { 
            nl(); 
            showmenu(); 
        } 
    }
    else
        handleinput(s,begx);

    if(!charbufferpointer)
        nl();
}

void handleinput(char *s,int begx)
{
    int i,i1,c;

    for(i=0;i<maxcmd&&!hangup;i++) {
        c=slok(tg[i].sl,0);
        if(!c)
            continue;

        if(!strcmp(tg[i].key,s)||!strcmp(tg[i].key,"@")) {
            if(!usepldns&&!(pp.attr & menu_popup)) {
                while(wherex()>begx)
                    backspace();
                pl(tg[i].line);
            }

            if(!ex(tg[i].type,tg[i].ms))
                return;

        } 
        else if(!strcmp(tg[i].key,"#")) {

            if(tg[i].ms[0]=='M') {
                for(i1=0; i1<MAX_SUBS; i1++)
                    if(!strcmp(usub[i1].keys,s))
                        cursub=i1;
            } 
            else if(tg[i].ms[0]=='F') {
                for(i1=0; i1<MAX_DIRS; i1++)
                    if(!strcmp(udir[i1].keys,s))
                        curdir=i1;
            }
        }
    }
}
