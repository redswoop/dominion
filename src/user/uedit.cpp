#include "user/uedit.h"
#include "platform.h"
#include "bbs_output.h"
#include "bbs_input.h"
#include "bbs_ui.h"
#include "conio.h"
#include "files/file1.h"
#include "timest.h"
#include "utility.h"
#include "config.h"
#include "user/newuser.h"
#include "shortmsg.h"
#include "stringed.h"
#include "session.h"
#include "user/userdb.h"
#include "system.h"
#include "user/nuv.h"
#include "misccmd.h"
#include "sysopf.h"
#include "lilo.h"
#include "bbs_path.h"
#pragma hdrstop

#include <stdarg.h>


void deluser(int un)
{
    auto& sys = System::instance();
    User u;
    int i,i1,f,n;
    mailrec m;
    char fn[MAX_PATH_LEN];
    votingrec v;
    voting_response vr;

    { auto p = UserDB::instance().get(un); if (p) u = *p; }
    if ((u.inact() & inact_deleted)==0) {
        rsm(un,u);
        UserDB::instance().index_remove(u.name());
        sys.status.users = UserDB::instance().user_count();
        save_status();
        u.mark_deleted();
        u.set_waiting(0);
        UserDB::instance().store(un,u);
        strcpy(fn, BbsPath::join(sys.cfg.datadir, "voting.dat").c_str());
        f=open(fn,O_RDWR | O_BINARY, S_IREAD | S_IWRITE);
        n=(int) (filelength(f) / sizeof(votingrec)) -1;
        for (i=0; i<20; i++)
            if (u.votes()[i]) {
                if (i<=n) {
                    lseek(f,((long) i) * sizeof(votingrec), SEEK_SET);
                    read(f,(void *)&v,sizeof(votingrec));
                    vr=v.responses[u.votes()[i]-1];
                    vr.numresponses--;
                    v.responses[u.votes()[i]-1]=vr;
                    lseek(f,((long) i) * sizeof(votingrec), SEEK_SET);
                    write(f,(void *)&v,sizeof(votingrec));
                }
                u.votes_mut()[i]=0;
            }
        UserDB::instance().store(un,u);
        close(f);
    }
}

void addtrash(const User& u)
{
    auto& sys = System::instance();
    char s[MAX_PATH_LEN];
    int i;

    npr("5Add 0%s5 to Trashcan? ",u.name());
    if(!yn()) return;
    strcpy(s, BbsPath::join(sys.cfg.gfilesdir, "trashcan.lst").c_str());
    i=open(s,O_RDWR|O_BINARY|O_CREAT,S_IREAD|S_IWRITE);
    lseek(i,0L,SEEK_END);
    sprintf(s,"%s\n",u.name());
    write(i,&s,strlen(s));
    npr("5Add realname (0%s5) also? ",u.realname());
    if(yn()) {
        sprintf(s,"%s\n",u.realname());
        write(i,&s,strlen(s));
    }
    close(i);
    logtypes(3,"4%s 0added to Trashcan",u.name());
}

void print_data(int un, const User& u,int lng)
{
    char s[255],s1[MAX_PATH_LEN],s2[MAX_PATH_LEN],s3[MAX_PATH_LEN],s4[MAX_PATH_LEN];
    int i;

    outchr(12);

    strcpy(s,"");
    strcpy(s1,"");
    if ((u.inact()) & inact_deleted) strcpy(s1,"0(6Deleted0)");
    if ((u.inact()) & inact_lockedout) strcpy(s,"0(8Locked Out0)");
    npr("4[User #%d] %s %s",un,s1,s);
    nl();
    npr("31. Name    :0 %-27.27s",u.display_name(un).c_str());
    npr("32. Realname:0 %s\r\n",u.realname());
    if(lng) {
        npr("33. Password:0");
        io.echo=0;
        npr(" %-27.27s",u.password());
        io.echo=1;
        npr("34. Computer:0 %s\r\n",getComputerType(u.comp_type()));
        npr("35. Address :0 %-27.27s",(u.street()));
        npr("36. City/St :0 %s\r\n",(u.city()));
        npr("37. Fpts    :0 %-27d",u.fpts());
        npr("38. Phone   :0 %s\r\n",u.phone());
    }
    npr("39. SL      :0 %-27d",u.sl());
    npr("30. DSL     :0 %d\r\n",u.dsl());

    if(lng) {
        if(u.exempt() & exempt_ratio) s[0]='R'; 
        else s[0]=32;
        if(u.exempt() & exempt_time) s[1]='T'; 
        else s[1]=32;
        if(u.exempt() & exempt_userlist) s[2]='U'; 
        else s[2]=32;
        if(u.exempt() & exempt_post) s[3]='P'; 
        else s[3]=32;
/*        if(u.exempt() & exempt_ulvalfile) s[4]='V';
        else s[4]=32;
        if(u.exempt() & exempt_dlvalfile) s[5]='D';
        else s[5]=32;
*/
        s[4]=0;
        npr("3A. Exempt  : 0%-27.27s\r\n",s);
    }

    npr("3C. Note    :0 %-27.27s",u.note());
    npr("3D. Comment :0 %.26s\r\n",u.comment());

    strcpy(s3,restrict_string);
    for (i=0; i<=15; i++) {
        if (u.ar() & (1 << i)) s[i]='A'+i;
        else s[i]='a'+i;
        if (u.dar() & (1 << i)) s1[i]='A'+i;
        else s1[i]='a'+i;
        if (u.restrict_flags() & (1 << i)) s2[i]=s3[i];
        else s2[i]='-';
    }
    s[16]=0;
    s1[16]=0;
    s2[16]=0;
    npr("3E. AR      :0 %-27.27s",s);
    npr("3F. DARs    :0 %s\r\n",s1);
    npr("3G. Restrict:0 %s\r\n",s2);

    if(lng) {
        npr("3H. Age     :0 %-27d",u.age());
        npr("3I. Sex     :0 %c\r\n",u.sex());
        npr("3J. Ratio   :0 %-5.3f\r\n",u.ul_dl_ratio());
        npr("3K. PCR     :0 %-5.3f\r\n",u.pcr());
        npr("3Logon Rate :0 %ld",u.lastrate());
        nl();
    }

    if(lng) {
        nl();
        npr("3Message Stats :0 Posts:%-3d   Emails:%-3d  FeedBack:%-3d\r\n",u.msgpost(),u.emailsent(),u.feedbacksent());
        npr("3File Stats    :0  Uled:%-3d   Dled  :%-3d       Upk:%-4ldK         Down:%ldK\r\n",u.uploaded(),u.downloaded(),u.uk(),u.dk());
        npr("3Login Stats   :0 Calls:%-3d   Today :%-3d   Last On:%-8s  First On:%s\r\n",u.logons(),u.ontoday(),u.laston(),u.firston());
        nl();
    }
}


/****************************************************************************/

int usearch(int un,char val[41])
{
    auto& sess = Session::instance();
    char ok=1,neg=0,*p,s1[MAX_PATH_LEN],s[MAX_PATH_LEN],curok=1;
    int i;
    User u;

    { auto p = UserDB::instance().get(un); if (p) u = *p; }
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
            if(!(u.ar() & (1 << toupper(s[1])-'A'))) curok=0;
            break;
        case 'B':
            if((int)u.lastrate()!=atoi(s+1)) curok=0;
            break;
        case 'D':
            if(u.dsl()<atoi(s+1)) curok=0;
            break;
        case 'G':
            if(u.age()<atoi(s+1)) curok=0;
            break;
        case 'I':
            if(!(u.dar() & (1 << toupper(s[1])-'A'))) curok=0;
            break;
        case 'S':
            if(u.sl()<atoi(s+1)) curok=0;
            break;
        case 'U':
            if(atoi(s+1)!=sess.usernum) curok=0;
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


#pragma warn -par

void uedit(int usern)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    char s[MAX_PATH_LEN],s1[MAX_PATH_LEN],s2[MAX_PATH_LEN],ch,ch1,search[41];
    int i,i1,i2,i3,un,done,nu,done1,full,temp_full,tempu,top;
    User u,u1;

    if(!checkpw()) return;

    top=io.topline;
    io.topline=0;
    full=opp(outcom);
    un=usern;
    done=0;
    { auto p = UserDB::instance().get(un); if (p) u = *p; }
    nu=UserDB::instance().max_id();
    do {
        { auto p = UserDB::instance().get(un); if (p) u = *p; }
        done1=0;
        temp_full=0;
        do {
            if ((full) || (temp_full))
                print_data(un,u,1);
            else
                print_data(un,u,0);
            nl();
            prt(5,"User Editor (?=Help):0 ");
            strcpy(s,"@#1234567890ABCDEFGHIJKUQS[],.?|_-/O{}$%");
            if ((sess.actsl==255) || (sys.wfc))
                strcat(s,":;!^");
            ch=onek(s);
            switch(ch) {
#ifdef NUV
            case '@':
                nl();
                pl("5NUV 0commands");
                npr("A:dd, R:emove, P:ullout > ");
                ch=onek("ARP\r");
                switch(ch) {
                case 'P':
                case 'R':
                    if(u.nuv_status()!=(unsigned long)-1) {
                        if(ch=='R')
                            del_nuv(u.nuv_status());
                        u.set_nuv_status((unsigned long)-1);
                        UserDB::instance().store(un,u);
                    } 
                    else
                        pl("no");
                    break;
                case 'A':
                    if(u.nuv_status()==(unsigned long)-1) {
                        nl();
                        npr("5Ask infoform? ");
                        u.set_nuv_status(enter_nuv(u,un,yn()));
                        UserDB::instance().store(un,u);
                    } 
                    else
                        pl("no");
                    break;
                }
                break;
#endif
            case 'S':
                nl();
                inputdat("Swap with which user",s,41,0);
                i=finduser1(s);
                if (i>0) {
                    nl();
                    npr("5Are you SURE you want to swap %s and %s? ",u.display_name(un).c_str(),u1.display_name(i).c_str());
                    if(yn()) {
                        { auto p = UserDB::instance().get(i); if (p) u1 = *p; }
                        UserDB::instance().store(un,u1);
                        UserDB::instance().store(i,u);
                    }
                }
                break;
            case 'O': 
                nl(); 
                inputdat("ACS Search String",search,41,0);
                break;
            case '$':
                u.set_dk(0); u.set_uk(0); u.set_fpts(0); u.set_uploaded(0); u.set_downloaded(0);
                UserDB::instance().store(un,u);
                break;
            case '/':
                u.set_laston(date());
                UserDB::instance().store(un,u);
                break;
            case ':':
                u.set_street("");
                u.set_city("");
                u.set_birth_year(0);
                UserDB::instance().store(un,u);
                break;
            case '5':
                nl();
                inputdat("Street Address",s,31,1);
                if(s[0]) {
                    u.set_street(s);
                    UserDB::instance().store(un,u);
                }
                break;
            case '_':
                { char _nm[31]; strcpy(_nm, u.name()); readform(sys.nifty.nuinf,_nm); }
                break;
            case '-':
                nl();
                inputdat("Which InfoForm",s,8,0);
                if(s[0]) {
                    char _nm[31]; strcpy(_nm, u.name()); readform(s,_nm);
                }
                break;
            case '|':
                nl();
                inputdat("Set to which Security Profile",s,2,0);
                i=atoi(s);
                if(s[0]) {
                    i--;
                    u.set_sl(sys.cfg.autoval[i].sl);
                    u.set_dsl(sys.cfg.autoval[i].dsl);
                    u.set_ar(sys.cfg.autoval[i].ar);
                    u.set_dar(sys.cfg.autoval[i].dar);
                    u.set_restrict(sys.cfg.autoval[i].restrict);
                    UserDB::instance().store(un,u);
                }
                break;

            case 'Q':
                done=1;
                done1=1;
                io.topline=top;
                break;
            case '3':
                nl();
                inputdat("New Password",s,20,0);
                if(s[0]) {
                    prt(5,"Are you sure? ");
                    if(yn()) u.set_password(s);
                }
                UserDB::instance().store(un,u);
                break;
            case ']':
                ++un;
                if (un>nu)
                    un=1;
                done1=1;
                break;
            case '[':
                --un;
                if (un==0)
                    un=nu;
                done1=1;
                break;
            case '}':
                i=0;
                do {
                    un++;
                    if(un>nu) {
                        un=1;
                        pl("End of User List");
                        break;
                    }
                    if(usearch(un,search))
                        i=1;
                } 
                while(!i);
                done1=1;
                break;
            case '{':
                i=0;
                do {
                    un--;
                    if(un==0) {
                        un=1;
                        pl("End of User List");
                        break;
                    }
                    if(usearch(un,search))
                        i=1;
                } 
                while(!i);
                done1=1;
                break;
            case '.':
                full=(!full);
                temp_full=full;
                break;
            case ',':
                temp_full=(!temp_full);
                break;

            case '?':
                printmenu(6);
                getkey();
                break;

            case '^':
                if ((u.inact() & inact_deleted)==0) {
                    prt(5,"Delete? ");
                    if (yn()) {
                        deluser(un);
                        { auto p = UserDB::instance().get(un); if (p) u = *p; }
                    }
                }
                else
                    if (u.inact() & inact_deleted) {
                        u.set_inact(u.inact() ^ inact_deleted);
                        UserDB::instance().index_add(un,u.name());
                        sys.status.users = UserDB::instance().user_count();
                        save_status();
                        UserDB::instance().store(un,u);
                    }
                addtrash(u);
                break;

            case '!':
                if ((u.inact() & inact_lockedout)==0) {
                    prt(5,"Lock Out? ");
                    if (yn()) {
                        u.set_inact(u.inact() ^ inact_lockedout);
                        UserDB::instance().store(un,u);
                    }
                }
                else
                    if (u.inact() & inact_lockedout) {
                    u.set_inact(u.inact() ^ inact_lockedout);
                    UserDB::instance().index_add(un,u.name());
                    sys.status.users = UserDB::instance().user_count();
                    save_status();
                    UserDB::instance().store(un,u);
                }
                break;

            case 'U':
                nl();
                inputdat("Name/Number to Jump",s,31,0);
                i=finduser1(s);
                if (i>0) {
                    un=i;
                    done1=1;
                }
                break;
            case '1':
                nl();
                inputdat("Handle",s,31,0);
                if (s[0]) {
                    UserDB::instance().index_remove(u.name());
                    u.set_name(s);
                    UserDB::instance().index_add(un,u.name());
                    sys.status.users = UserDB::instance().user_count();
                    save_status();
                    UserDB::instance().store(un,u);
                }
                break;

            case '2':
                nl();
                inputdat("Real Name",s,31,1);
                if (s[0]) {
                    u.set_realname(s);
                    UserDB::instance().store(un,u);
                }
                break;
            case '6':
                nl();
                inputdat("City/State",s,31,1);
                if (s[0]) {
                    u.set_city(s);
                    UserDB::instance().store(un,u);
                }
                break;
            case '8':
                nl();
                inputdat("Phone number",s,12,0);
                if (s[0]) {
                    u.set_phone(s);
                    UserDB::instance().store(un,u);
                }
                break;
            case 'C':
                nl();
                inputdat("SysOp Note",s,39,1);
                u.set_note(s);
                UserDB::instance().store(un,u);
                break;
            case 'D':
                nl();
                inputdat("User Note",s,39,1);
                u.set_comment(s);
                UserDB::instance().store(un,u);
                break;
            case 'I':
                input_sex(u);
                UserDB::instance().store(un,u);
                break;
            case 'H':
                nl();
                npr("Birthdate is %02d/%02d/%02d\r\n",(int) u.birth_month(),
                (int) u.birth_day(),
                (int) u.birth_year());
                npr("\r\n3Birthdate\r\n5: 0");
                mpl(8);
                inputdate(s,0);
                nl();
                u.set_birth_month(atoi(s));
                u.set_birth_day(atoi(&(s[3])));
                u.set_birth_year(atoi(&(s[6])));
                u.set_age(years_old(u.birth_month(),u.birth_day(),u.birth_year()));
                UserDB::instance().store(un,u);
                break;
            case '4':
                nl();
                pl("Known computer types:");
                nl();
                for (i=0; i<numComputerTypes(); i++)
                    npr("%d. %s\r\n",i+1,getComputerType(i));
                nl();
                inputdat("Computer Type",s,2,0);
                i2=atoi(s);
                i1=1;
                if ((i2<1) || (i2>i)) i1=0;
                if(i1) u.set_comp_type(i2-1);
                UserDB::instance().store(un,u);
                break;
            case '7':
                nl();
                inputdat("File Points",s,4,0);
                u.set_fpts(atoi(s));
                UserDB::instance().store(un,u);
                break;

            case '9':
                nl();
                inputdat("Security Level",s,3,0);
                u.set_sl(atoi(s));
                UserDB::instance().store(un,u);
                break;

            case '0':
                inputdat("Transfer Level",s,3,0);
                i=atoi(s);
                u.set_dsl(i);
                UserDB::instance().store(un,u);
                break;
            case 'J':
                inputdat("New Specific Ratio (0.000 form)",s,5,0);
                if(s[0]) { float _tmp; sscanf(s,"%f",&_tmp); u.set_ul_dl_ratio(_tmp); }
                UserDB::instance().store(un,u);
                break;
            case 'K':
                inputdat("New Specific PCR (0.000 form)",s,5,0);
                if(s[0]) { float _tmp; sscanf(s,"%f",&_tmp); u.set_pcr(_tmp); }
                UserDB::instance().store(un,u);
                break;
            case 'A':
                i=0;
                do {
                    nl();
                    prt(3,"Toggle Which Exemption (R,T,U,P,D,V)? ");
                    ch1=onek("RTUPQ?DV");
                    switch(ch1) {
                    case 'U':
                        u.set_exempt(u.exempt() ^ exempt_userlist);
                        break;
                    case 'R':
                        u.set_exempt(u.exempt() ^ exempt_ratio);
                        break;
                    case 'T':
                        u.set_exempt(u.exempt() ^ exempt_time);
                        break;
                    case 'P':
                        u.set_exempt(u.exempt() ^ exempt_post);
                        break;
                        /*            case 'D': if(u.exempt & exempt_dlvalfile) u.exempt ^= exempt_dlvalfile;
                                                                      else u.exempt |= exempt_dlvalfile; break;
                                                            case 'V': if(u.exempt & exempt_ulvalfile) u.exempt ^= exempt_ulvalfile;
                                                                      else u.exempt |= exempt_ulvalfile; break;*/
                        case 'Q': 
                            i=1; 
                        break;
                    case '?': 
                        printmenu(18); 
                        break;
                    }
                } 
                while(!i);
                UserDB::instance().store(un,u);
                break;
            case 'G':
                nl();
                pl(restrict_string);
                do {
                    prt(3,"Toggle? ");
                    s[0]=13;
                    s[1]='?';
                    strcpy(&(s[2]),restrict_string);
                    ch1=onek(s);
                    if (ch1==32)
                        ch1=13;
                    if (ch1=='?')
                        printmenu(10);
                    if ((ch1!=13) && (ch1!='?')) {
                        i=-1;
                        for (i1=0; i1<16; i1++)
                            if (ch1==s[i1+2])
                                i=i1;
                        if (i>-1) {
                            u.set_restrict(u.restrict_flags() ^ (1 << i));
                            UserDB::instance().store(un,u);
                        }
                    }
                } 
                while ((!io.hangup) && (ch1=='?'));
                break;
            case 'E':
                nl(); 
                do {
                    prt(3,"Toggle which AR? ");
                    ch1=onek("\rABCDEFGHIJKLMNOPQ");
                    if (ch1!=13&&ch1!='Q') {
                        ch1-='A';
                        u.set_ar(u.ar() ^ (1 << ch1));
                        UserDB::instance().store(un,u);
                    } 
                } 
                while(ch1!='Q'&&!io.hangup);
                break;
            case 'F':
                nl(); 
                do {
                    prt(3,"Toggle which DAR? ");
                    ch1=onek("\rABCDEFGHIJKLMNOPQ");
                    if (ch1!=13&&ch1!='Q') {
                        ch1-='A';
                        u.set_dar(u.dar() ^ (1 << ch1));
                        UserDB::instance().store(un,u);
                    } 
                } 
                while (ch1!='Q'&&!io.hangup);
                break;
            }
        } 
        while ((!done1) && (!io.hangup));
    } 
    while ((!done) && (!io.hangup));
    if (!sys.wfc)
        topscreen();
}

extern int rc;

void val_cur_user(int wait)
{
    auto& sess = Session::instance();
    char sl[4],dsl[4],ar[17],dar[17],restrict[17],ex[17],delff[3];
    char dk[34],uk[34],ndl[33],nul[33],fpts[34],psts[5],lgns[5],timebank[5];
    int cp,i,done;
    char nsave[41];


    strcpy(nsave,sess.user.name());

    if(wait) pr_wait(1);
    sess.user.set_sl(sess.actsl);
    savescreen(&sess.screensave);
    io.curatr=11;
    ansic(3);
    fastscreen("top.bin");
    itoa((int)sess.user.sl(),sl,10);
    itoa((int)sess.user.dsl(),dsl,10);
    itoa((int)sess.user.dk(),dk,10);
    itoa((int)sess.user.uk(),uk,10);
    itoa((int)sess.user.downloaded(),ndl,10);
    itoa((int)sess.user.uploaded(),nul,10);
    itoa((int)sess.user.fpts(),fpts,10);
    itoa((int)sess.user.msgpost(),psts,10);
    itoa((int)sess.user.logons(),lgns,10);
    itoa((int)sess.user.timebank(),timebank,10);
    strcpy(ex,"");

    for (i=0; i<=15; i++) {
        if (sess.user.ar() & (1 << i))
            ar[i]='A'+i;
        else
            ar[i]=32;
        if (sess.user.dar() & (1 << i))
            dar[i]='A'+i;
        else
            dar[i]=32;
        if (sess.user.restrict_flags() & (1 << i))
            restrict[i]=restrict_string[i];
        else
            restrict[i]=32;
    }
    dar[16]=0;
    ar[16]=0;
    restrict[16]=0;

    cp=1;
    done=0;

    io.curatr=15;

    outsat(sess.user.name(),16,2);
    outsat(sess.user.realname(),53,2);
    outsat(sl,16,3);
    outsat(dsl,53,3);
    outsat(ar,16,4);
    outsat(dar,53,4);
    outsat(sess.user.phone(),16,5);
    outsat(restrict,53,5);
    outsat(nul,16,6);
    outsat(ndl,53,6);
    outsat(uk,16,7);
    outsat(dk,53,7);
    outsat(lgns,16,8);
    outsat(psts,53,8);
    outsat(fpts,16,9);
    outsat(timebank,53,9);

    outsat(sess.user.city(),16,15);
    outsat(sess.user.street(),16,16);
    outsat(sess.user.comment(),16,17);
    outsat(sess.user.note(),16,18);

    clickat(sess.user.inact(),inact_deleted,25,11);
    clickat(sess.user.inact(),inact_deleted,41,11);

    clickat(sess.user.sysstatus(),sysstatus_ansi,22,12);
    clickat(sess.user.sysstatus(),sysstatus_color,45,12);

    clickat(sess.user.exempt(),exempt_time,22,13);
    clickat(sess.user.exempt(),exempt_ratio,32,13);
    clickat(sess.user.exempt(),exempt_post,41,13);
    clickat(sess.user.exempt(),exempt_userlist,54,13);

    while (done==0) {
        switch(cp) {
        case 1:
            { char _buf[31]; strcpy(_buf, sess.user.name()); editdata(_buf,25,16,2); sess.user.set_name(_buf); }
            { char _buf[31]; strcpy(_buf, sess.user.name()); strupr(_buf); sess.user.set_name(_buf); }
            break;
        case 2:
            { char _buf[21]; strcpy(_buf, sess.user.realname()); editdata(_buf,25,53,2); sess.user.set_realname(_buf); }
            break;
        case 3:
            sess.user.set_sl(editdig(sl,3,16,3));
            sess.actsl=sess.user.sl();
            break;
        case 4:
            sess.user.set_dsl(editdig(dsl,3,53,3));
            break;
        case 5:
            movecsr(16-1,4-1);
            editline(ar,16,SET,&rc,"ABCDEFGHIJKLMNOP");
            sess.user.set_ar(0);
            for (i=0; i<=15; i++)
                if (ar[i]!=' ') sess.user.set_ar(sess.user.ar() | (1 << i));
            break;
        case 6:
            movecsr(53-1,4-1);
            editline(dar,16,SET,&rc,"ABCDEFGHIJKLMNOP");
            sess.user.set_dar(0);
            for (i=0; i<=15; i++)
                if (dar[i]!=' ') sess.user.set_dar(sess.user.dar() | (1 << i));
            break;
        case 7:
            { char _buf[13]; strcpy(_buf, sess.user.phone()); editdata(_buf,12,16,5); sess.user.set_phone(_buf); }
            break;
        case 8:
            movecsr(53-1,5-1);
            editline(restrict,16,SET,&rc,restrict_string);
            sess.user.set_restrict(0);
            for (i=0; i<=15; i++)
                if (restrict[i]!=' ') sess.user.set_restrict(sess.user.restrict_flags() | (1 << i));
            break;
        case 9:
            sess.user.set_uploaded(editdig(nul,5,16,6));
            break;
        case 10:
            sess.user.set_downloaded(editdig(ndl,5,53,6));
            break;
        case 11:
            sess.user.set_uk(editdig(uk,5,16,7));
            break;
        case 12:
            sess.user.set_dk(editdig(dk,5,53,7));
            break;
        case 13:
            sess.user.set_logons(editdig(lgns,5,16,8));
            break;
        case 14:
            sess.user.set_msgpost(editdig(psts,5,53,8));
            break;
        case 15:
            sess.user.set_fpts(editdig(fpts,5,16,9));
            break;
        case 16:
            sess.user.set_timebank(editdig(timebank,3,53,9));
            break;

        case 17:
            { long _tmp = sess.user.inact(); rc=click(&_tmp,inact_deleted,25,11); sess.user.set_inact(_tmp); }
            break;
        case 18:
            { long _tmp = sess.user.inact(); rc=click(&_tmp,inact_deleted,41,11); sess.user.set_inact(_tmp); }
            break;

        case 19:
            { long _tmp = sess.user.sysstatus(); rc=click(&_tmp,sysstatus_ansi,22,12); sess.user.set_sysstatus(_tmp); }
            break;
        case 20:
            { long _tmp = sess.user.sysstatus(); rc=click(&_tmp,sysstatus_color,45,12); sess.user.set_sysstatus(_tmp); }
            break;


        case 23:
            { long _tmp = sess.user.exempt(); rc=click(&_tmp,exempt_time,22,13); sess.user.set_exempt(_tmp); }
            break;
        case 24:
            { long _tmp = sess.user.exempt(); rc=click(&_tmp,exempt_ratio,32,13); sess.user.set_exempt(_tmp); }
            break;
        case 25:
            { long _tmp = sess.user.exempt(); rc=click(&_tmp,exempt_post,41,13); sess.user.set_exempt(_tmp); }
            break;
        case 26:
            { long _tmp = sess.user.exempt(); rc=click(&_tmp,exempt_userlist,54,13); sess.user.set_exempt(_tmp); }
            break;

        case 27:
            { char _buf[31]; strcpy(_buf, sess.user.city()); editdata(_buf,53,16,15); sess.user.set_city(_buf); }
            break;
        case 28:
            { char _buf[31]; strcpy(_buf, sess.user.street()); editdata(_buf,53,16,16); sess.user.set_street(_buf); }
            break;
        case 29:
            { char _buf[61]; strcpy(_buf, sess.user.comment()); editdata(_buf,53,16,17); sess.user.set_comment(_buf); }
            break;
        case 30:
            { char _buf[61]; strcpy(_buf, sess.user.note()); editdata(_buf,53,16,18); sess.user.set_note(_buf); }
            break;
        }
        switch(rc) {
        case DONE:
            done=1;
            break;
        case NEXT:
            if (cp<30) cp++;
            else cp=1;
            break;
        case PREV:
            if (cp>1) cp--;
            else cp=30;
            break;

        }
    }

    restorescreen(&sess.screensave);
    changedsl();

    if(strcmp(nsave,sess.user.name()))
        reset_files(0);

    if(wait) pr_wait(0);
}

#pragma warn +par

