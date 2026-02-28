#include "subedit.h"
#include "platform.h"
#include "bbs_output.h"
#include "bbs_input.h"
#include "bbs_ui.h"
#include "bbsutl.h"
#include "disk.h"
#include "utility.h"
#include "user/userdb.h"
#include "config.h"
#include "stringed.h"
#include "session.h"
#include "system.h"
#include "lilo.h"
#pragma hdrstop


void noc(char *s, char *s1)
{
    int r=0,w=0;

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
}

void boarddata(int n, char *s)
{
    auto& sys = System::instance();
    char x,y,k,i,s1[MAX_PATH_LEN];
    subboardrec r;

    r=sys.subboards[n];
    if (r.ar==0)
        x=32;
    else {
        for (i=0; i<16; i++)
            if ((1 << i) & r.ar)
                x='A'+i;
    }
    noc(s1,r.name);
    sprintf(s,"3%2d 3%c  3%-40s  3%-8s 3%-4s 3%-4s 3%-3d 3%c",
    n,x,s1,r.filename,r.readacs,r.postacs,r.maxmsgs,r.conf);
}

void showsubs()
{
    auto& sys = System::instance();
    int abort,i;
    char s[180];

    outchr(12);
    abort=0;
    pla("0NN2�0AR2�0Name2��������������������������������������0FileName2�0RAcs2�0PAcs2�0Max2�0Conf2",
    &abort);
    for (i=0; (i<sys.num_subs) && (!abort); i++) {
        boarddata(i,s);
        pla(s,&abort);
    }
}


void modify_sub(int n)
{
    auto& sys = System::instance();
    subboardrec r;
    char s[MAX_PATH_LEN],s1[MAX_PATH_LEN],ch,ch2,*p;
    int i,i1,done;
    unsigned int ui;

    done=0;
    r=sys.subboards[n];

    do {
        outchr(12);
        npr("31. Name         : 0%s\r\n",r.name);
        npr("32. Filename     : 0%s\r\n",r.filename);
        npr("33. Conference   : 0%c\r\n",r.conf);
        npr("34. Read SL      : 0%s\r\n",r.readacs);
        npr("35. Post SL      : 0%s\r\n",r.postacs);
        npr("36. Min. Age     : 0%d\r\n",r.age);
        npr("37. Max Msgs     : 0%d\r\n",r.maxmsgs);
        strcpy(s,"None.");
        if (r.ar!=0) {
            for (i=0; i<16; i++)
                if ((1 << i) & r.ar)
                    s[0]='A'+i;
            s[1]=0;
        }
        npr("38. AR           : 0%s\r\n",s);
        npr("39. Address      : 0%d:%d/%d\r\n",r.add.zone,r.add.net,r.add.node);
        npr("30. Origin Type  : 0%d\r\n",r.origin);
        npr("3E. Netmail Path : 0%s\r\n",r.nmpath);
        s[0]=(char)196;
        if(r.attr & mattr_ansi_only) s[1]='G'; 
        else s[1]=(char)196;
        if(r.attr & mattr_autoscan) s[2]='A'; 
        else s[2]=(char)196;
        if(r.attr & mattr_fidonet) s[4]='F'; 
        else s[4]=(char)196;
        if(r.attr & mattr_nomci) s[6]='M'; 
        else s[6]=(char)196;
        if(r.attr & mattr_netmail) s[7]='N'; 
        else s[7]=(char)196;
        if(r.attr & mattr_private) s[8]='P';
        else s[8]=(char)196;

        if(r.anony & anony_real_name) s[5]='R'; 
        else s[5]=(char)196;
        if(r.anony & anony_enable_anony) s[3]='U'; 
        else s[3]=(char)196;


        s[9]=0;

        npr("3Flags           :0 %s\r\n",s);
        nl();
        nl();
        prt(5,"Message Base Edit (?=Help) ? ");
        ch=onek("!Q1234567890AVFUGRNEMJOP[]?");
        switch(ch) {
        case 'O':
            break;
        case '?': 
            printmenu(28); 
            pausescr(); 
            break;
        case ']': 
            if((n>=0) &&(n<sys.num_subs-1)) {
                sys.subboards[n++]=r;
                r=sys.subboards[n];
            } 
            break;
        case '[': 
            if(n>0) { 
                sys.subboards[n--]=r;
                r=sys.subboards[n];
            } 
            break;
        case 'J': 
            nl(); 
            outstr("To Which Sub? ");
            input(s,3);
            if(s[0]) {
                i=atoi(s);
                sys.subboards[n]=r;
                r=sys.subboards[i];
            } 
            break;
        case 'Q':
            done=1; 
            break;
        case '1':
            nl();
            strcpy(s1,"");
            prt(2,"New Name? ");
            inli(s,s1,40,1);
            if (s[0])
                strcpy(r.name,s);
            break;
        case '2':
            nl();
            inputdat("Filename/Net Directory",s,8,0);
            if ((s[0]!=0) && (strchr(s,'.')==0))
                strcpy(r.filename,s);
            break;
        case '3':
            nl();
            prt(2,"New Conference (@=All): ");
            r.conf=onek("ABCDEFGHIJKLMNOPQRSTUVWXYZ@!1234567890");
            break;
        case '4':
            nl();
            inputdat("New Read ACS",s,21,0);
            if(s[0]) strcpy((char *)r.readacs,s);
            break;
        case '5':
            nl();
            inputdat("New Post ACS",s,21,0);
            if(s[0]) strcpy((char *)r.postacs,s);
            break;
        case '6':
            nl();
            inputdat("Minimum Age",s,3,0);
            i=atoi(s);
            if ((i>=0) && (i<128) && (s[0]))
                r.age=i;
            break;
        case '7':
            nl();
            inputdat("Maximum Messages (1-65534)",s,5,0);
            ui=atoi(s);
            if ((ui>0) && (ui<65534) && (s[0]))
                r.maxmsgs=ui;
            break;
        case '8':
            nl();
            prt(5,"New AR (<SPC>=None) ? ");
            ch2=onek("ABCDEFGHIJKLMNOP ");
            if (ch2==32)
                r.ar=0;
            else
                r.ar=1 << (ch2-'A');
            break;
        case '!':
            r.storage_type=2; 
            break;
        case '9':
            nl();
            inputdat("Enter Specific Origin, or Blank to use Default",s,20,0);
            if(!s[0]) { 
                r.add.zone=0; 
                r.add.net=0; 
                r.add.node=0; 
            }
            else {
                strcpy(s1,s);
                p=strtok(s1,":");
                r.add.zone=atoi(p);
                p=strtok(NULL,"/");
                r.add.net=atoi(p);
                p=strtok(NULL,"");
                r.add.node=atoi(p);
            }
            break;
        case '0':
            nl();
            inputdat("Use which Origin/Address? (0=Main)",s,3,0);
            if(s[0]) r.origin=atoi(s);
            break;
        case 'E':
            nl();
            inputdat("New Netmail Path",s,51,0);
            if(s[0]) strcpy(r.nmpath,s);
            break;
        case 'G': 
            togglebit((long *)&r.attr,mattr_ansi_only);
            break;
        case 'A': 
            togglebit((long *)&r.attr,mattr_autoscan);
            break;
        case 'F': 
            togglebit((long *)&r.attr,mattr_fidonet);
            break;
        case 'M': 
            togglebit((long *)&r.attr,mattr_nomci);
            break;
        case 'P':
            togglebit((long *)&r.attr,mattr_private);
            break;
        case 'N': 
            togglebit((long *)&r.attr,mattr_netmail);
            break;
        case 'U': 
            togglebit((long *)&r.anony,anony_enable_anony);
            break;
        case 'R': 
            togglebit((long *)&r.anony,anony_real_name);
            break;
        }
    } 
    while ((!done) && (!io.hangup));
    sys.subboards[n]=r;
    if (!sys.wfc)
        changedsl();
}


void swap_subs(int sub1, int sub2)
{
    auto& sys = System::instance();
    int i,i1,i2,nu;
    unsigned long tl,tl1;
    subboardrec sbt;
    User u;


    sbt=sys.subboards[sub1];
    sys.subboards[sub1]=sys.subboards[sub2];
    sys.subboards[sub2]=sbt;

}


void insert_sub(int n,int config)
{
    auto& sys = System::instance();
    subboardrec r;
    int i,i1,nu;
    User u;
    long l1,l2,l3;

    for (i=sys.num_subs-1; i>=n; i--)
        sys.subboards[i+1]=sys.subboards[i];

    inputdat("Name for New Sub",r.name,41,0);
    if(!r.name[0])
        strcpy(r.name,"New Sub!");

    inputdat("JAM Filename for New Area",r.filename,8,0);
    if(!r.filename[0])
        strcpy(r.filename,"NONAME");

    r.nmpath[0]=0;
    r.conf='@';
    strcpy((char *)r.readacs,"S20");
    strcpy((char *)r.postacs,"S20");
    r.anony=0;
    r.attr=0;
    togglebit((long *)&r.anony,anony_enable_anony);
    r.age=0;
    r.maxmsgs=50;
    r.ar=0;
    r.storage_type=2;
    r.origin=0;
    memset(&r.add.zone,0,sizeof(r.add));

    sys.subboards[n]=r;
    ++sys.num_subs;

    if(config)
        modify_sub(n);
}


void delete_sub(int n)
{
    auto& sys = System::instance();
    int i,i1,i2,nu;
    User u;
    long l1,l2;
    char s[MAX_PATH_LEN];

    for (i=n; i<sys.num_subs; i++)
        sys.subboards[i]=sys.subboards[i+1];
    --sys.num_subs;
}


void boardedit()
{
    auto& sys = System::instance();
    int i,i1,i2,done,f;
    char s[MAX_PATH_LEN],s1[MAX_PATH_LEN],s2[MAX_PATH_LEN],ch;

    if (!checkpw())
        return;
    showsubs();
    done=0;
    do {
        nl();
        outstr(get_string2(1));
        ch=onek("QDIM?P!");
        switch(ch) {
        case '!':
            input(s,4);
            sys.num_subs=atoi(s);
            break;
        case '?':
            showsubs();
            break;
        case 'Q':
            done=1;
            break;
        case 'M':
            nl();
            prt(2,"Sub number? ");
            input(s,2);
            i=atoi(s);
            if ((s[0]!=0) && (i>=0) && (i<sys.num_subs))
                modify_sub(i);
            break;
        case 'I':
            if (sys.num_subs<199) {
                nl();
                prt(2,"Insert before which sub? ");
                input(s,2);
                i=atoi(s);
                if(i==0)
                    pl("Nothing can be inserted before area 0!");
                else if ((s[0]!=0) && (i>=0) && (i<=sys.num_subs))
                    insert_sub(i,1);
            }
            break;
        case 'D':
            nl();
            prt(2,"Delete which sub? ");
            input(s,2);
            i=atoi(s);
            if(i==0)
                pl("Area #0 Cannot be deleted!");
            else if ((s[0]!=0) && (i>=0) && (i<sys.num_subs)) {
                nl();
                sprintf(s1,"Delete %s? ",sys.subboards[i].name);
                prt(5,s1);
                if (yn())
                    delete_sub(i);
            }
            break;
        }
    } 
    while ((!done) && (!io.hangup));
    sprintf(s,"%ssubs.dat",sys.cfg.datadir);
    f=open(s,O_RDWR | O_BINARY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);
    write(f,(void *)&sys.subboards[0], sys.num_subs * sizeof(subboardrec));
    close(f);
}

void confdata(int n, char *s)
{
    auto& sys = System::instance();
    char x,y,k,i,s1[MAX_PATH_LEN];
    confrec r;

    r=sys.conf[n];
    noc(s1,r.name);
    sprintf(s,"3%2d  3%-40s 3%-8s 3%s",n,s1,r.sl,r.flagstr);
}

void showconf()
{
    auto& sys = System::instance();
    int abort,i;
    char s[180];

    outchr(12);
    abort=0;
    pla("0NN2��0Name2�������������������������������������0SL2�������0Flags2������������������",&abort);
    for (i=0; (i<sys.num_conf) && (!abort); i++) {
        confdata(i,s);
        pla(s,&abort);
    }
}


void modify_conf(int n)
{
    auto& sys = System::instance();
    confrec r;
    char s[MAX_PATH_LEN],s1[MAX_PATH_LEN],ch,ch2;
    int i,i1,done;

    done=0;
    r=sys.conf[n];

    do {
        outchr(12);
        npr("31. Name       : 0%s\r\n",r.name);
        npr("32. ACS        : 0%s\r\n",r.sl);
        npr("33. Flags      : 0%s\r\n",r.flagstr);
        strcpy(s,"All");
        if(r.type=='M') strcpy(s,"Message");
        if(r.type=='F') strcpy(s,"File");
        npr("34. Type       : 0%s\r\n",s);
        nl();
        outstr("5Conefence Editor (?=Help) 0");
        ch=onek("Q1234J[]T");
        switch(ch) {
        case ']': 
            if((n>=0) && (n<sys.num_conf-1))
            { 
                sys.conf[n++]=r;
                r=sys.conf[n];
            } 
            break;

        case '[': 
            if(n>0) { 
                sys.conf[n--]=r;
                r=sys.conf[n];
            } 
            break;
        case 'J': 
            nl(); 
            outstr("To Which Conference? ");
            input(s,3);
            if(s[0]) {
                i=atoi(s);
                sys.conf[n]=r;
                r=sys.conf[i];
            } 
            break;
        case 'Q':
            done=1; 
            break;
        case '1':
            nl();
            strcpy(s1,"");
            npr("3Name for Confernece\r\n5: 0");
            inli(s,s1,40,1);
            if (s[0]) strcpy(r.name,s);
            break;
        case '2':
            nl();
            inputdat("New SL (Can Include Menu Type Codes)",s,10,0);
            if(s[0])  strcpy(r.sl,s);
            break;
        case '3':
            nl();
            inputdat("Flags for this Conference",s,10,0);
            if(s[0]) strcpy(r.flagstr,s);
            break;
        case '4':
            nl();
            outstr("5Enter Conference Type (3M,F,A5) ");
            ch=onek("MFA\r");
            if(ch=='A'||ch=='\r') r.type=0;
            else r.type=ch;
            break;
        }                                 

    } 
    while ((!done) && (!io.hangup));
    sys.conf[n]=r;
    if (!sys.wfc)
        changedsl();
}


void insert_conf(int n)
{
    auto& sys = System::instance();
    confrec r;
    int i,i1,nu;
    User u;
    long l1,l2,l3;

    for (i=sys.num_conf-1; i>=n; i--)
        sys.conf[i+1]=sys.conf[i];

    strcpy(r.name,">New Conference<");
    strcpy(r.sl,"s20");
    strcpy(r.flagstr,"");
    sys.conf[n]=r;
    ++sys.num_conf;
    modify_conf(n);
}


void delete_conf(int n)
{
    auto& sys = System::instance();
    int i,i1,nu;
    User u;
    long l1,l2;

    for (i=n; i<sys.num_conf; i++)
        sys.conf[i]=sys.conf[i+1];
    --sys.num_conf;
    if (!sys.wfc)
        changedsl();
}


void confedit()
{
    auto& sys = System::instance();
    int i,i1,i2,done,f;
    char s[MAX_PATH_LEN],s1[MAX_PATH_LEN],s2[MAX_PATH_LEN],ch;

    if (!checkpw())
        return;
    showconf();
    done=0;
    do {
        nl();
        outstr(get_string2(1));
        ch=onek("QDIM?");
        switch(ch) {
        case '?':
            showconf();
            break;
        case 'Q':
            done=1;
            break;
        case 'M':
            nl();
            prt(2,"Conference number? ");
            input(s,2);
            i=atoi(s);
            if ((s[0]!=0) && (i>=0) && (i<sys.num_conf))
                modify_conf(i);
            break;
        case 'I':
            if (sys.num_conf<10) {
                nl();
                prt(2,"Insert before which conf? ");
                input(s,2);
                i=atoi(s);
                if ((s[0]!=0) && (i>=0) && (i<=sys.num_conf))
                    insert_conf(i);
            }
            break;
        case 'D':
            nl();
            prt(2,"Delete which conf? ");
            input(s,2);
            i=atoi(s);
            if ((s[0]!=0) && (i>=0) && (i<sys.num_conf)) {
                nl();
                sprintf(s1,"Delete %s? ",sys.conf[i].name);
                prt(5,s1);
                if (yn())
                    delete_conf(i);
            }
            break;
        }
    } 
    while ((!done) && (!io.hangup));
    sprintf(s,"%sconf.dat",sys.cfg.datadir);
    f=open(s,O_RDWR | O_BINARY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);
    write(f,(void *)&sys.conf[0], sys.num_conf * sizeof(confrec));
    close(f);
}
