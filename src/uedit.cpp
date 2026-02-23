#include "uedit.h"
#include "platform.h"
#include "fcns.h"
#include "conio.h"
#include "file1.h"
#include "timest.h"
#include "disk.h"
#include "utility.h"
#include "config.h"
#include "newuser.h"
#include "utility1.h"
#include "stringed.h"
#include "session.h"
#include "system.h"
#include "nuv.h"
#include "misccmd.h"
#include "sysopf.h"
#include "lilo.h"
#pragma hdrstop

#include <stdarg.h>


void deluser(int un)
{
    auto& sys = System::instance();
    userrec u;
    int i,i1,f,n;
    mailrec m;
    char fn[MAX_PATH_LEN];
    votingrec v;
    voting_response vr;

    userdb_load(un,&u);
    if ((u.inact & inact_deleted)==0) {
        rsm(un,&u);
        userdb_index_remove(u.name);
        sys.status.users = userdb_user_count();
        save_status();
        u.inact |= inact_deleted;
        u.waiting=0;
        userdb_save(un,&u);
        sprintf(fn,"%svoting.dat",sys.cfg.datadir);
        f=open(fn,O_RDWR | O_BINARY, S_IREAD | S_IWRITE);
        n=(int) (filelength(f) / sizeof(votingrec)) -1;
        for (i=0; i<20; i++)
            if (u.votes[i]) {
                if (i<=n) {
                    lseek(f,((long) i) * sizeof(votingrec), SEEK_SET);
                    read(f,(void *)&v,sizeof(votingrec));
                    vr=v.responses[u.votes[i]-1];
                    vr.numresponses--;
                    v.responses[u.votes[i]-1]=vr;
                    lseek(f,((long) i) * sizeof(votingrec), SEEK_SET);
                    write(f,(void *)&v,sizeof(votingrec));
                }
                u.votes[i]=0;
            }
        userdb_save(un,&u);
        close(f);
    }
}

void addtrash(userrec u)
{
    auto& sys = System::instance();
    char s[MAX_PATH_LEN];
    int i;

    npr("5Add 0%s5 to Trashcan? ",u.name);
    if(!yn()) return;
    sprintf(s,"%strashcan.lst",sys.cfg.gfilesdir);
    i=open(s,O_RDWR|O_BINARY|O_CREAT,S_IREAD|S_IWRITE);
    lseek(i,0L,SEEK_END);
    sprintf(s,"%s\n",u.name);
    write(i,&s,strlen(s));
    npr("5Add realname (0%s5) also? ",u.realname);
    if(yn()) {
        sprintf(s,"%s\n",u.realname);
        write(i,&s,strlen(s));
    }
    close(i);
    logtypes(3,"4%s 0added to Trashcan",u.name);
}

void print_data(int un, userrec *u,int lng)
{
    char s[255],s1[MAX_PATH_LEN],s2[MAX_PATH_LEN],s3[MAX_PATH_LEN],s4[MAX_PATH_LEN];
    int i;

    outchr(12);

    strcpy(s,"");
    strcpy(s1,"");
    if ((u->inact) & inact_deleted) strcpy(s1,"0(6Deleted0)");
    if ((u->inact) & inact_lockedout) strcpy(s,"0(8Locked Out0)");
    npr("4[User #%d] %s %s",un,s1,s);
    nl();
    npr("31. Name    :0 %-27.27s",nam(u,un));
    npr("32. Realname:0 %s\r\n",u->realname);
    if(lng) {
        npr("33. Password:0");
        io.echo=0;
        npr(" %-27.27s",u->pw);
        io.echo=1;
        npr("34. Computer:0 %s\r\n",getComputerType(u->comp_type));
        npr("35. Address :0 %-27.27s",(u->street));
        npr("36. City/St :0 %s\r\n",(u->city));
        npr("37. Fpts    :0 %-27d",u->fpts);
        npr("38. Phone   :0 %s\r\n",u->phone);
    }
    npr("39. SL      :0 %-27d",u->sl);
    npr("30. DSL     :0 %d\r\n",u->dsl);

    if(lng) {
        if(u->exempt & exempt_ratio) s[0]='R'; 
        else s[0]=32;
        if(u->exempt & exempt_time) s[1]='T'; 
        else s[1]=32;
        if(u->exempt & exempt_userlist) s[2]='U'; 
        else s[2]=32;
        if(u->exempt & exempt_post) s[3]='P'; 
        else s[3]=32;
/*        if(u->exempt & exempt_ulvalfile) s[4]='V';
        else s[4]=32;
        if(u->exempt & exempt_dlvalfile) s[5]='D';
        else s[5]=32;
*/
        s[4]=0;
        npr("3A. Exempt  : 0%-27.27s\r\n",s);
    }

    npr("3C. Note    :0 %-27.27s",u->note);
    npr("3D. Comment :0 %.26s\r\n",u->comment);

    strcpy(s3,restrict_string);
    for (i=0; i<=15; i++) {
        if (u->ar & (1 << i)) s[i]='A'+i;
        else s[i]='a'+i;
        if (u->dar & (1 << i)) s1[i]='A'+i;
        else s1[i]='a'+i;
        if (u->restrict & (1 << i)) s2[i]=s3[i];
        else s2[i]='-';
    }
    s[16]=0;
    s1[16]=0;
    s2[16]=0;
    npr("3E. AR      :0 %-27.27s",s);
    npr("3F. DARs    :0 %s\r\n",s1);
    npr("3G. Restrict:0 %s\r\n",s2);

    if(lng) {
        npr("3H. Age     :0 %-27d",u->age);
        npr("3I. Sex     :0 %c\r\n",u->sex);
        npr("3J. Ratio   :0 %-5.3f\r\n",u->ratio);
        npr("3K. PCR     :0 %-5.3f\r\n",u->pcr);
        npr("3Logon Rate :0 %ld",u->lastrate);
        nl();
    }

    if(lng) {
        nl();
        npr("3Message Stats :0 Posts:%-3d   Emails:%-3d  FeedBack:%-3d\r\n",u->msgpost,u->emailsent,u->feedbacksent);
        npr("3File Stats    :0  Uled:%-3d   Dled  :%-3d       Upk:%-4ldK         Down:%ldK\r\n",u->uploaded,u->downloaded,u->uk,u->dk);
        npr("3Login Stats   :0 Calls:%-3d   Today :%-3d   Last On:%-8s  First On:%s\r\n",u->logons,u->ontoday,u->laston,u->firston);
        nl();
    }
}


/****************************************************************************/

int usearch(int un,char val[41])
{
    auto& sess = Session::instance();
    char ok=1,neg=0,*p,s1[MAX_PATH_LEN],s[MAX_PATH_LEN],curok=1;
    int i;
    userrec u;

    userdb_load(un,&u);
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
            if(!(u.ar & (1 << toupper(s[1])-'A'))) curok=0; 
            break;
        case 'B': 
            if(u.lastrate!=atoi(s+1)) curok=0; 
            break;
        case 'D': 
            if(u.dsl<atoi(s+1)) curok=0; 
            break;
        case 'G': 
            if(u.age<atoi(s+1)) curok=0; 
            break;
        case 'I': 
            if(!(u.dar & (1 << toupper(s[1])-'A'))) curok=0;
            break;
        case 'S': 
            if(u.sl<atoi(s+1)) curok=0; 
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
    userrec u,u1;

    if(!checkpw()) return;

    top=io.topline;
    io.topline=0;
    full=opp(outcom);
    un=usern;
    done=0;
    userdb_load(un,&u);
    nu=userdb_max_num();
    do {
        userdb_load(un,&u);
        done1=0;
        temp_full=0;
        do {
            if ((full) || (temp_full))
                print_data(un,&u,1);
            else
                print_data(un,&u,0);
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
                    if(u.nuv!=-1) {
                        if(ch=='R')
                            del_nuv(u.nuv);
                        u.nuv=-1;
                        userdb_save(un,&u);
                    } 
                    else
                        pl("no");
                    break;
                case 'A':
                    if(u.nuv==-1) {
                        nl();
                        npr("5Ask infoform? ");
                        u.nuv=enter_nuv(u,un,yn());
                        userdb_save(un,&u);
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
                    npr("5Are you SURE you want to swap %s and %s? ",nam(&u,un),nam(&u1,i));
                    if(yn()) {
                        userdb_load(i,&u1);
                        userdb_save(un,&u1);
                        userdb_save(i,&u);
                    }
                }
                break;
            case 'O': 
                nl(); 
                inputdat("ACS Search String",search,41,0);
                break;
            case '$': 
                u.dk=u.uk=u.fpts=u.uploaded=u.downloaded=0;
                userdb_save(un,&u);
                break;
            case '/': 
                strcpy(u.laston,date()); 
                userdb_save(un,&u); 
                break;
            case ':': 
                u.street[0]=0; 
                u.city[0]=0; 
                u.year=0; 
                userdb_save(un,&u); 
                break;
            case '5': 
                nl();
                inputdat("Street Address",s,31,1);
                if(s[0]) { 
                    strcpy(u.street,s);
                    userdb_save(un,&u); 
                }
                break;
            case '_': 
                readform(sys.nifty.nuinf,u.name); 
                break;
            case '-': 
                nl();
                inputdat("Which InfoForm",s,8,0);
                if(s[0])
                    readform(s,u.name);
                break;
            case '|':
                nl();
                inputdat("Set to which Security Profile",s,2,0);
                i=atoi(s);
                if(s[0]) {
                    i--;
                    u.sl=sys.cfg.autoval[i].sl;
                    u.dsl=sys.cfg.autoval[i].dsl;
                    u.ar=sys.cfg.autoval[i].ar;
                    u.dar=sys.cfg.autoval[i].dar;
                    u.restrict=sys.cfg.autoval[i].restrict;
                    userdb_save(un,&u);
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
                    if(yn()) strcpy(u.pw,s); 
                }
                userdb_save(un,&u);
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
                if ((u.inact & inact_deleted)==0) {
                    prt(5,"Delete? ");
                    if (yn()) {
                        deluser(un);
                        userdb_load(un,&u);
                    }
                } 
                else
                    if (u.inact & inact_deleted) {
                        u.inact ^= inact_deleted;
                        userdb_index_add(un,u.name);
                        sys.status.users = userdb_user_count();
                        save_status();
                        userdb_save(un,&u);
                    }
                addtrash(u);
                break;

            case '!':
                if ((u.inact & inact_lockedout)==0) {
                    prt(5,"Lock Out? ");
                    if (yn()) {
                        u.inact ^= inact_lockedout;
                        userdb_save(un,&u);
                    }
                } 
                else
                    if (u.inact & inact_lockedout) {
                    u.inact ^= inact_lockedout;
                    userdb_index_add(un,u.name);
                    sys.status.users = userdb_user_count();
                    save_status();
                    userdb_save(un,&u);
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
                    userdb_index_remove(u.name);
                    strcpy(u.name,s);
                    userdb_index_add(un,u.name);
                    sys.status.users = userdb_user_count();
                    save_status();
                    userdb_save(un,&u);
                }
                break;

            case '2':
                nl();
                inputdat("Real Name",s,31,1);
                if (s[0]) {
                    strcpy(u.realname,s);
                    userdb_save(un,&u);
                }
                break;
            case '6':
                nl();
                inputdat("City/State",s,31,1);
                if (s[0]) {
                    strcpy(u.city,s);
                    userdb_save(un,&u);
                }
                break;
            case '8':
                nl();
                inputdat("Phone number",s,12,0);
                if (s[0]) {
                    strcpy(u.phone,s);
                    userdb_save(un,&u);
                }
                break;
            case 'C':
                nl();
                inputdat("SysOp Note",s,39,1);
                strcpy(u.note,s);
                userdb_save(un,&u);
                break;
            case 'D':
                nl();
                inputdat("User Note",s,39,1);
                strcpy(u.comment,s);
                userdb_save(un,&u);
                break;
            case 'I': 
                input_sex(&u); 
                userdb_save(un,&u); 
                break;
            case 'H':
                nl();
                npr("Birthdate is %02d/%02d/%02d\r\n",(int) u.month,
                (int) u.day,
                (int) u.year);
                npr("\r\n3Birthdate\r\n5: 0");
                mpl(8);
                inputdate(s,0);
                nl();
                u.month=atoi(s);
                u.day=atoi(&(s[3]));
                u.year=atoi(&(s[6]));
                u.age=years_old(u.month,u.day,u.year);
                userdb_save(un,&u);
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
                if(i1) u.comp_type=i2-1;
                userdb_save(un,&u);
                break;
            case '7':
                nl();
                inputdat("File Points",s,4,0);
                u.fpts=atoi(s);
                userdb_save(un,&u);
                break;

            case '9':
                nl();
                inputdat("Security Level",s,3,0);
                u.sl=atoi(s);
                userdb_save(un,&u);
                break;

            case '0':
                inputdat("Transfer Level",s,3,0);
                i=atoi(s);
                u.dsl=i;
                userdb_save(un,&u);
                break;
            case 'J': 
                inputdat("New Specific Ratio (0.000 form)",s,5,0);
                if(s[0]) sscanf(s,"%f",&u.ratio);
                userdb_save(un,&u);
                break;
            case 'K': 
                inputdat("New Specific PCR (0.000 form)",s,5,0);
                if(s[0]) sscanf(s,"%f",&u.pcr);
                userdb_save(un,&u);
                break;
            case 'A':
                i=0;
                do {
                    nl();
                    prt(3,"Toggle Which Exemption (R,T,U,P,D,V)? ");
                    ch1=onek("RTUPQ?DV");
                    switch(ch1) {
                    case 'U': 
                        if(u.exempt & exempt_userlist) u.exempt ^= exempt_userlist;
                        else u.exempt |= exempt_userlist; 
                        break;
                    case 'R': 
                        if(u.exempt & exempt_ratio) u.exempt ^= exempt_ratio;
                        else u.exempt |= exempt_ratio; 
                        break;
                    case 'T': 
                        if(u.exempt & exempt_time) u.exempt ^= exempt_time;
                        else u.exempt |= exempt_time; 
                        break;
                    case 'P': 
                        if(u.exempt & exempt_post) u.exempt ^= exempt_post;
                        else u.exempt |= exempt_post; 
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
                userdb_save(un,&u);
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
                            u.restrict ^= (1 << i);
                            userdb_save(un,&u);
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
                        u.ar ^= (1 << ch1);
                        userdb_save(un,&u);
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
                        u.dar ^= (1 << ch1);
                        userdb_save(un,&u);
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

#pragma warn +par

