#include "vars.h"
#pragma hdrstop


int num_nuv(char *fn)
{
    char s[MAX_PATH_LEN];
    int nn,i;

    sprintf(s,"%s%s",syscfg.datadir,fn);
    i=open(s,O_BINARY|O_RDWR);
    nn=filelength(i)/sizeof(nuvdata);
    close(i);

    return nn;
}

void read_nuv(unsigned int user, char *fn, nuvdata *newuser)
{
    char s[MAX_PATH_LEN];
    int i;

    sprintf(s,"%s%s",syscfg.datadir,fn);
    i=open(s,O_BINARY|O_RDWR|O_CREAT,S_IREAD|S_IWRITE);
    lseek(i,sizeof(nuvdata)*user,SEEK_SET);
    read(i,newuser,sizeof(nuvdata));
    close(i);

}

void write_nuv(unsigned int user, char *fn, nuvdata *newuser)
{
    char s[MAX_PATH_LEN];
    int i;
    nuvdata n;

    n= *newuser;

    sprintf(s,"%s%s",syscfg.datadir,fn);
    i=open(s,O_BINARY|O_RDWR|O_CREAT,S_IREAD|S_IWRITE);
    lseek(i,sizeof(nuvdata)*user,0);
    write(i,&n,sizeof(nuvdata));
    close(i);
}


void del_nuv(unsigned int user)
{
    char s[MAX_PATH_LEN],s1[MAX_PATH_LEN];
    nuvdata dn;
    int dnc,nnu,i,o;

    sprintf(s,"%sNUV.DAT",syscfg.datadir);
    i=open(s,O_BINARY|O_RDWR);

    sprintf(s1,"%sNUV.BAK",syscfg.datadir);
    o=open(s1,O_BINARY|O_RDWR|O_CREAT|O_TRUNC,S_IREAD|S_IWRITE);

    nnu=filelength(i)/sizeof(dn);

    for (dnc = 0; dnc < nnu; dnc++) {
        if (dnc != user) {
            read(i,&dn,sizeof(dn));
            write(o,&dn,sizeof(dn));
        }
    }

    close(i);
    close(o);

    if (remove(s) != 0)
        pl("Unable to Delete File NUV.dat");
    rename(s1,s);
}


int enter_nuv(userrec tu,int un,int form)
{
    char s[MAX_PATH_LEN];
    int i,num;
    nuvdata nu;

    num=num_nuv("NUV.dat");

    for(i=0;i<num;i++) {
        read_nuv(i,"nuv.dat",&nu);
        if(nu.num==un) {
            pl("Already in NUV");
            pausescr();
            return 0;
        }
    }

    nu.num = un;
    nu.age = tu.age;
    strcpy(nu.name,tu.name);
    strcpy(nu.firston,tu.firston);

    nu.vote_yes = 0;
    nu.vote_no = 0;
    nu.vcmt_num = 0;
    strcpy(nu.snote,"");

    for (i = 0; i <20; i++) {
        strcpy(nu.vote_comment[i].name,"");
        nu.vote_comment[i].vote = 0;
        nu.vote_comment[i].counts = 0;
        nu.vote_comment[i].sl = 0;
        strcpy(nu.vote_comment[i].say,"");
    }

    num=num_nuv("NUV.DAT");
    write_nuv(num,"NUV.DAT",&nu);
    if(form) {
        infoform(nifty.nuvinf,0);
        printfile("nuvmsg");
    }
    logtypes(2,"%s added to NUV",nam(&tu,un));
    return i;
}


int avoted(unsigned int user)
{
    nuvdata v;
    int i,found=0;
    char s[MAX_PATH_LEN];


    read_nuv(user,"NUV.DAT",&v);

    strcpy(s,nam(&thisuser,usernum));

    for (i=0; i < v.vcmt_num; i++)
        if (!strcmp(v.vote_comment[i].name,s))
            found = 1;
    return(found);
}

void print_nuv(nuvdata v)
{
    char s[151];
    int i;
    userrec u;

    read_user(v.num,&u);
    outchr(12);
    npr("3Voting On5: 3%s\r\n",nam(&u,v.num));
    nl();

    npr("3Yes Votes5: 2%d 3- Required: 3%d \r\n",v.vote_yes,nifty.nuvyes);
    npr("3No Votes 5: 2%d 3- Required: 3%d \r\n",v.vote_no,nifty.nuvbad);
    npr("3First on 5: 2%s\r\n",v.firston);
    nl();
    npr("3Comments On 5: 3%s3...",nam(&u,v.num));
    nl();

    for (i = 0; i < v.vcmt_num; i=i+1) {
        if (v.vote_comment[i].say[0]) {
            npr("3%-25s5: 2%s",v.vote_comment[i].name,v.vote_comment[i].vote?"Yay":"Nay",v.vote_comment[i].say);
            nl();
        }
    }
    nl();
}

int vote_nuv(unsigned int user, nuvdata *resn,int *done1)
{
    nuvdata vn;
    char s[MAX_PATH_LEN],cmt[MAX_PATH_LEN];
    int vv=0,done=0;
    userrec u;

    read_nuv(user,"NUV.DAT",&vn);

    /*    if(so()) {
            nl();
            pl("0SysOp's Note");
            npr("5%s\r\n",vn.snote);
            nl();
            prt(5,"Change SysOp's Note? ");
            if (yn()) {
                inputl(vn.snote,65);
                nl();
            } 
            else nl();
        }*/

    while(!done&&!hangup) {
        print_nuv(vn);

        if(vv!=0) {
            npr("3Your Vote Will Be: ");
            if(vv==1)
                npr("Yay");
            else if(vv==-1)
                npr("Nay");
            nl();
        }
        npr(get_string(88));
        switch(onek("YNISCQ\r")) {
        case 'Q': 
            *done1=1; 
            done=1; 
            break;
        case 'S': 
            vv=0;
        case '\r': 
            done=1; 
            break;
        case 'I':
            read_user(vn.num,&u);
            readform(nifty.nuvinf,u.name);
            break;

        case 'Y':
            vv=1;
            break;

        case 'N':
            vv=-1;
            break;

        case 'C':
            nl();
            if(vv==0) {
                nl();
                pl("You must vote before you can comment.");
                nl();
                pausescr();
            } 
            else {
                npr("5Would You Like To Comment On This New User? ");
                if (yn()) {
                    inputdat("Comment to be seen by other voters",s,41,1);
                    if(s[0])
                        strcpy(cmt,s);
                }
            }
            break;
        }
    }


    if(vv!=0) {
        if(vv==1)
            vn.vote_yes++;
        else if(vv==-1)
            vn.vote_no++;

        strcpy(vn.vote_comment[vn.vcmt_num].name,nam(&thisuser,usernum));
        vn.vote_comment[vn.vcmt_num].vote = vv;
        vn.vote_comment[vn.vcmt_num].sl = thisuser.sl;

        vn.vote_comment[vn.vcmt_num].say[0]=0;

        if(cmt[0])
            strcpy(vn.vote_comment[vn.vcmt_num].say,s);
        vn.vcmt_num++;
    }

    write_nuv(user,"NUV.DAT",&vn);
    *resn = vn;

    if ((vn.vote_yes>=nifty.nuvyes)||(vn.vote_no>=nifty.nuvbad)) {
        val_nuv(user);
        return 1;
    }

    return 0;
}


void val_nuv(unsigned int user)
{
    int i,i1;
    nuvdata valn;
    userrec u;
    int work;


    read_nuv(user,"NUV.DAT",&valn);
    del_nuv(user);
    i1 = valn.num;
    read_user(i1,&u);
    u.nuv=-1;

    if (valn.vote_no >= nifty.nuvbad) {
        switch(nifty.nuvaction) {
        case 0:
            pl("7Deleting User");
            deluser(i1);
            logtypes(3,"NUV Deleted %s",nam(&u,i1));
            nl();
            break;
        case 1:
            pl("7Locking Out User");
            u.inact |= inact_lockedout;
            logtypes(3,"NUV Locking Out %s",nam(&u,i1));
            write_user(i1,&u);
            u.nuv=0;
            nl();
            break;
        case 2:
            pl("7Bad Validating User");
            u.sl=syscfg.autoval[nifty.nuvbadlevel-1].sl;
            u.dsl=syscfg.autoval[nifty.nuvbadlevel-1].dsl;
            u.ar=syscfg.autoval[nifty.nuvbadlevel-1].ar;
            u.dar=syscfg.autoval[nifty.nuvbadlevel-1].dar;
            u.restrict=syscfg.autoval[nifty.nuvbadlevel-1].restrict;
            logtypes(3,"NUV Bad Validated %s",nam(&u,i1));
            u.nuv=0;
            write_user(i1,&u);
            break;
        }
    }
    else {
        pl("7Validating User");
        u.sl=syscfg.autoval[nifty.nuvlevel-1].sl;
        u.dsl=syscfg.autoval[nifty.nuvlevel-1].dsl;
        u.ar=syscfg.autoval[nifty.nuvlevel-1].ar;
        u.dar=syscfg.autoval[nifty.nuvlevel-1].dar;
        u.restrict=syscfg.autoval[nifty.nuvlevel-1].restrict;
        logtypes(3,"NUV Validated %s",nam(&u,i1));
        u.nuv=0;
        write_user(i1,&u);
    }
}

void nuv(void)
{
    nuvdata newuser;
    userrec u;
    char s[MAX_PATH_LEN];
    int i,cnt,done=0,sh=0,done1;

    strcpy(s,nifty.nuvsl);
    if(!slok(s,0)) {
        nl();
        pl(get_string(59));
        nl();
        return;
    }

    i = 1; 
    cnt = num_nuv("NUV.DAT");

    if(!cnt) {
        nl();
        pl(get_string(58));
        nl();
        return;
    }


    while(!done&&!hangup) {
        outchr(12);
        npr("5Number of New Users: 2%d",cnt);
        nl();
        nl();
        npr(get_string(57));
        s[0]=onek("?LVQ!\r");
        switch(s[0]) {
        case '!': 
            if(!so()) break;
            nl();
            inputdat("Remove Which? ",s,3,0);
            if(!s[0]) break;
            del_nuv(atoi(s));
            cnt = num_nuv("NUV.DAT");
            break;
        case '?': 
            printmenu(31);
            pausescr();
            break;
        case 'L':
            nl();
            for(i=0;i<cnt;i++) {
                read_nuv(i,"NUV.DAT",&newuser);
                read_user(newuser.num,&u);
                npr("1<1%d1> 0%-35s 3[3%.3s3] 5[5%s5]\r\n",i,nam(&u,newuser.num),u.phone,u.comment);
            }
            nl();
            pausescr();
            break;
        case '\r':
        case 'Q':
            done=1;
            break;
        case 'V':
            i=0;
            sh=0;
            done1=0;
            while (i < cnt &&!hangup&&!done1) {
                read_nuv(i,"NUV.DAT",&newuser);
                nl();
                if (!avoted(i)) {
                    sh=1;
                    if(vote_nuv(i,&newuser,&done1)) {
                        i--;
                        cnt--;
                    }
                }
                i++;
            }
            if(!sh) {
                nl();
                pl("Sorry, but you have already voted on all the users");
                pausescr();
            }
        }
    } 
}
