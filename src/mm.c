#include "platform.h"
#include "fcns.h"
#include "session.h"
#include "system.h"
#include "version.h"
#pragma hdrstop

#include "menudb.h"

extern int SYSTEMDEBUG;


static auto& sys = System::instance();
static auto& sess = Session::instance();

struct mmfmt_t {
    char         fmt[41],
    fill,
    promptfn[8],
    ansifn[8],
    ansiftfn[8],
    center;
};
extern struct mmfmt_t mmfmt;


void handleinput(char *s,int begx);

#define sysstatus_pause_on_message 0x0400


#ifdef PD
extern int usepldns,usepop;
extern char *retfrompldn;
#endif

extern int fastlogon;


void packm(void);

/* sess.maxcmd, sess.menuat, sess.mstack, sess.mdepth moved to vars.h (Phase B0) */

void conv(void);

int slok(char val[31],char menu)
{
    char ok=1,neg=0,*p,s1[MAX_PATH_LEN],s[MAX_PATH_LEN],curok=1;
    int i;

    if(sess.backdoor) return 1;

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
            if(!(sess.user.ar & (1 << toupper(s[1])-'A'))) curok=0; 
            break;
        case 'B': 
            if((sess.modem_speed/100)<atoi(s+1)) curok=0; 
            break;
        case 'C': 
            if(!menu) { 
                if(!postr_ok()); 
                curok=0;
                pl(get_string(40));
            } 
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
        post(sess.cursub); 
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
            nscan(sess.usub[sess.cursub].subnum,&i);
            logtypes(1,"NewScaned Message Area 4%s",sys.subboards[sess.usub[sess.cursub].subnum].name);
        } 
        else if(ms[0]=='A') {
            strcpy(s,sys.conf[sess.curconf].flagstr);
            strcpy(sys.conf[sess.curconf].flagstr,"@");
            changedsl();
            logtypes(1,"NewScaned All Conferences");
            gnscan();
            strcpy(sys.conf[sess.curconf].flagstr,s);
            changedsl();
        }
        else if(!ms[0]) {
            nl();
            outstr(get_string2(8));
            c=onek("\rYNA");
            switch(c) {
            case 'A': 
                strcpy(s,sys.conf[sess.curconf].flagstr);
                strcpy(sys.conf[sess.curconf].flagstr,"@");
                changedsl();
                logtypes(1,"NewScaned Messages All Conferences");
                gnscan();
                strcpy(sys.conf[sess.curconf].flagstr,s);
                changedsl();
                break;
            case 'Y':
            case '\r': 
                logtypes(1,"Global Newscaned Message Areas");
                gnscan(); 
                break;
            case 'N':  
                logtypes(1,"NewScaned Message Area 4%s",sys.subboards[sess.usub[sess.cursub].subnum].name);
                nscan(sess.usub[sess.cursub].subnum,&i); 
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
        sess.cursub=0; 
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
        strcpy(io.charbuffer,&ms[0]);
        io.charbufferpointer = 1;
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
        if(!(sys.nifty.nifstatus & nif_chattype)) reqchat(ms);
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
            if (sess.user.sysstatus & sysstatus_pause_on_page)
                sess.user.sysstatus ^= sysstatus_pause_on_page;
            prt(5,"Pause each screenfull? ");
            if (yn()) sess.user.sysstatus |= sysstatus_pause_on_page;
            nl();
            if (sess.user.sysstatus & sysstatus_pause_on_message)
                sess.user.sysstatus ^= sysstatus_pause_on_message;
            prt(5,"Pause each screenfull while reading messages? ");
            if (yn()) sess.user.sysstatus |= sysstatus_pause_on_message;
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
            sess.user.defed=yn();
            break;
        case '0': 
            getmsgformat(); 
            break;
        case 'A': 
            pl("Enter your Default Protocol, 0 for none.");
            i=get_protocol(1); 
            if(i>=0||i==-2) sess.user.defprot=i;
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
            change_colors(&sess.user);  
            break;
        case 'H': 
            inputdat("Enter your Comment",s,sizeof(sess.user.comment),1);
            if(s[0]) strcpy(sess.user.comment,s);
            break;
        case 'J': 
            if(sess.user.sysstatus & sysstatus_fullline)
                sess.user.sysstatus ^= sysstatus_fullline;
            npr("5Would you like Hotkey Input? ");
            if(!ny())
                sess.user.sysstatus |= sysstatus_fullline;
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
            insert_dir(sess.umaxdirs,s,99,0);
            sess.udir[sess.umaxdirs].subnum=sess.umaxdirs;
            sess.curdir=sess.umaxdirs;
            logtypes(3,"Created temporary path in 3%s",s);
            break;
        case 'M': 
            mark(sess.curdir); 
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
            else newdl(sess.curdir); 
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
                i=sess.curdir;
                for(sess.curdir=0;sess.curdir<sys.num_dirs&&sess.udir[sess.curdir].subnum>=0;sess.curdir++)
                    valfiles();
                sess.curdir=i;
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
            io.lines_listed=0;
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
                nscandir(sess.curdir,&abort,0,&i);
                logtypes(1,"NewScaned File Area %s",sys.directories[sess.udir[sess.curdir].subnum].name);
                break;
            }
            if(ms[0]=='G') {
                nscanall();
                logtypes(1,"NewScaned All File Areas");
                break;
            } 
            else if(ms[0]=='A') {
                strcpy(s,sys.conf[sess.curconf].flagstr);
                strcpy(sys.conf[sess.curconf].flagstr,"@");
                changedsl();
                logtypes(1,"NewScaned File Areas on All Conferences");
                nscanall();
                strcpy(sys.conf[sess.curconf].flagstr,s);
                changedsl();
            }
            else if(!ms[0]) {
                nl();
                outstr(get_string2(8));
                c=onek("\rYNA");
                switch(c) {
                case 'A': 
                    strcpy(s,sys.conf[sess.curconf].flagstr);
                    strcpy(sys.conf[sess.curconf].flagstr,"@");
                    changedsl();
                    logtypes(1,"NewScaned File Areas on All Conferences");
                    nscanall();
                    strcpy(sys.conf[sess.curconf].flagstr,s);
                    changedsl();
                    break;
                case 'Y':
                case '\r': 
                    logtypes(1,"Global Newscaned File Areas");
                    nscanall(); 
                    break;
                case 'N':  
                    nscandir(sess.curdir,&abort,0,&i);
                    logtypes(1,"NewScaned File Area %s",sys.directories[sess.udir[sess.curdir].subnum].name);
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
            addbbs(ms[0]?ms:(char *)"bbslist.msg");
            break;
        case 'R':
            printfile(ms[0]?ms:(char *)"bbslist.msg");
            pausescr();
            break;
        case 'S':
            searchbbs(ms[0]?ms:(char *)"bbslist.msg");
            break;
        default: 
            badcommand('Q',type[1]);
        } 
        break;

    case '=':
        switch(type[1])
        {
        case 'F': 
            if(ms[0]) strcpy(sess.pp.format,ms);
            readmnufmt(sess.pp);
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
            sess.curconf++; 
            if(sess.curconf>=sys.num_conf) sess.curconf=0;
            changedsl();
            break;
        case '[': 
            sess.curconf--; 
            if(sess.curconf<0) sess.curconf=sys.num_conf-1;
            changedsl();
            break;
        case '/': 
            nl();
            if(sess.mdepth+1>9) {
                sess.mdepth=2;
                strcpy(sess.mstack[0],sys.nifty.firstmenu);
                strcpy(sess.mstack[1],sess.menuat);
            } 
            else
                strcpy(sess.mstack[sess.mdepth++],sess.menuat);
            if(!readmenu(ms))
                readmenu(sess.mstack[--sess.mdepth]);
            return 0;
        case '\\': 
            nl();
            if(sess.mdepth) {
                if(!read_menu(sess.mstack[--sess.mdepth],0))
                    readmenu(sys.nifty.firstmenu);
            } 
            else {
                sess.mdepth=0;
                strcpy(sess.mstack[0],sys.nifty.firstmenu);
                readmenu(sys.nifty.firstmenu);
            }
            return 0;
        case '^': 
            nl();
            sess.mdepth=0;
            if(!readmenu(ms))
                readmenu(sys.nifty.firstmenu);
            return 0;
        case '*': 
            if(ms[0]=='M') sess.cursub=sublist(ms[1]);
            else if(ms[0]=='F') sess.curdir=dirlist(ms[1]);
#ifdef PD
            if(usepldns) pausescr();
#endif
            break;
        case '+': 
            if(ms[0]=='M') {
                if(sess.cursub<64 && sess.usub[sess.cursub+1].subnum>=0) {
                    ++sess.cursub;
                    sess.msgr++;
                }
                else sess.cursub=0;
                sess.msgr=1;
            }
            else if(ms[0]=='F') {
                if(sys.directories[sess.udir[sess.curdir].subnum].type)
                    changedsl();
                if(sess.curdir<64 && sess.udir[sess.curdir+1].subnum>=0) ++sess.curdir;
                else sess.curdir=0;
            }
            break;
        case '-': 
            if(ms[0]=='M') {
                if (sess.cursub>0)
                    --sess.cursub;
                else
                    while ((sess.usub[sess.cursub+1].subnum>=0) && (sess.cursub<64))
                    ++sess.cursub;
                sess.msgr=1;
            } 
            else if(ms[0]=='F') {
                if(sys.directories[sess.udir[sess.curdir].subnum].type)
                    changedsl();
                if (sess.curdir>0) sess.curdir--;
                else
                    while ((sess.udir[sess.curdir+1].subnum>=0) && (sess.curdir<63)) ++sess.curdir;
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

    if(!usepldns&&!(sess.pp.attr & menu_popup)&&!io.charbufferpointer) {
        nl();
        nl();
    }

    tleft(1);

    if(sess.usub[sess.cursub].subnum==-1) sess.cursub=0;
    helpl=sess.user.helplevel;
    if(sess.pp.helplevel) helpl=sess.pp.helplevel-1;
#ifdef PD
    if(!usepldns&&!(sess.pp.attr & menu_popup)&&!io.charbufferpointer)
#endif PD
        switch(helpl) {
        case 0: 
            break;
        case 2: 
            showmenu();
        case 1: 
            ansic(13);
            npr("[0");
            for(i=0;i<sess.maxcmd;i++)
                if(!(sess.tg[i].attr==command_pulldown)&&
                    !(sess.tg[i].attr==command_title)&&
                    !(sess.tg[i].attr==command_hidden))
                    npr("%s,",sess.tg[i].key);
            backspace();
            ansic(13);
            npr("]\r\n");
            break;
        }

    if(io.chatcall&&sess.chatsoundon) chatsound();
    memset(s,0,40);
#ifdef PD
    if((sess.pp.attr & menu_popup)&&okansi()) {
        popup("popup");
        strcpy(s,retfrompldn);
    } 
    else if(usepldns&&okansi()) {
        pldn();
        strcpy(s,retfrompldn);
    } 
    else {
#endif
        sprintf(s,"%s%s.prm",sys.cfg.menudir,mmfmt.promptfn);
        if(exist(s)&&(sess.pp.attr & menu_extprompt)) {
            if(sess.pp.attr & menu_promptappend)
                pl(sess.pp.prompt);
            ff=fopen(s,"rt");
            while(fgets(s,161,ff)!=NULL) {
                filter(s,'\n');
                outstr(s);
            }
            fclose(ff);
        } 
        else
            outstr(sess.pp.prompt);
        begx=wherex();
        i1=c=0;
        for(i=0;i<sess.maxcmd;i++) {
            if(sess.tg[i].key[0]=='#') {
                if(sess.tg[i].ms[0]=='M' || sess.tg[i].ms[0]=='F')
                    i1=1;
                else if(strlen(sess.tg[i].key)==1)
                    avail[c++]='#';
            }
            else {
                if(strlen(sess.tg[i].key)==1)
                    avail[c++]=sess.tg[i].key[0];
                else if(strlen(sess.tg[i].key)==2&&sess.tg[i].key[0]=='/')
                    avail[c++]=sess.tg[i].key[1];
            }
        }
        avail[c++]='?';
        avail[c]=0;

        if(sess.pp.boarder==0) {
            if(sess.user.sysstatus & sysstatus_fullline)
                ss=smkey(avail,i1,1,1,1);
            else
                ss=smkey(avail,i1,1,0,0);
        } 
        else if(sess.pp.boarder==1)
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
        for(cmd=0;cmd<sess.maxcmd;cmd++) {
            if(sess.tg[cmd].attr==command_default)
                if(!ex(sess.tg[cmd].type,sess.tg[cmd].ms)) cmd=sess.maxcmd;
        }
    } 
    else if((!strcmp(s,"AVAIL"))) {
        for(i=0;i<sess.maxcmd;i++) if(slok(sess.tg[i].sl,1)) npr("2%s0,",sess.tg[i].key);
        backspace();
        nl();
    } 
    else
        if((!strcmp(s,"HELP"))) {
            printmenu(7);
#ifdef PD
            if(usepldns||(sess.pp.attr & menu_popup)) pausescr();
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
        if((!strcmp(s,"SPEED"))) npr("ComSpeed=%d, ModemSpeed=%d\r\n",sess.com_speed,sess.modem_speed); 
    else
        if((!strcmp(s,"RELOAD"))) {
            if(sess.actsl<=sys.cfg.newusersl)
                readmenu(sys.nifty.newusermenu);
            else
                readmenu(sys.nifty.firstmenu);
        }  
    else if((!strcmp(s,"VER"))) {
        nl();
        npr("%s, Compiled %s\r\n",wwiv_version,wwiv_date);
        nl();
        i1=0;
    } 
        else if((!strcmp(s,"GRAPHICS"))) ex("OP","2");
        else if((!strcmp(s,"CLS"))) {
            if(okansi()) {
                makeansi(15, test, io.curatr);
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
        strcpy(&io.charbuffer[1],&s[0]);
        io.charbuffer[0]=';';
        io.charbufferpointer = 1;
    }

    else
        if((!strcmp(s,get_string(37))) && checkacs(6)) {
        getcmdtype();
    } 
    else
        if((!strcmp(s,"/OFF"))) io.hangup=1; 
    else
        if(s[0]=='?') { 
        if(helpl!=2) { 
            nl(); 
            showmenu(); 
        } 
    }
    else
        handleinput(s,begx);

    if(!io.charbufferpointer)
        nl();
}

void handleinput(char *s,int begx)
{
    int i,i1,c;

    for(i=0;i<sess.maxcmd&&!io.hangup;i++) {
        c=slok(sess.tg[i].sl,0);
        if(!c)
            continue;

        if(!strcmp(sess.tg[i].key,s)||!strcmp(sess.tg[i].key,"@")) {
            if(!usepldns&&!(sess.pp.attr & menu_popup)) {
                while(wherex()>begx)
                    backspace();
                pl(sess.tg[i].line);
            }

            if(!ex(sess.tg[i].type,sess.tg[i].ms))
                return;

        } 
        else if(!strcmp(sess.tg[i].key,"#")) {

            if(sess.tg[i].ms[0]=='M') {
                for(i1=0; i1<MAX_SUBS; i1++)
                    if(!strcmp(sess.usub[i1].keys,s))
                        sess.cursub=i1;
            } 
            else if(sess.tg[i].ms[0]=='F') {
                for(i1=0; i1<MAX_DIRS; i1++)
                    if(!strcmp(sess.udir[i1].keys,s))
                        sess.curdir=i1;
            }
        }
    }
}
