#include "diredit.h"
#include "platform.h"
#include "fcns.h"
#include "config.h"
#include "stringed.h"
#include "session.h"
#include "system.h"
#include "subedit.h"
#include "lilo.h"
#pragma hdrstop


void dirdata(int n, char *s)
{
    auto& sys = System::instance();
    char s1[MAX_PATH_LEN];
    directoryrec r;

    r=sys.directories[n];
    noc(s1,r.name);
    sprintf(s,"3%2d  3%-40s  3%-8s 3%-7s 3%-4d 3%-9.9s",
    n,s1,r.filename,r.acs,r.maxfiles,r.dpath);
}

void showdirs()
{
    auto& sys = System::instance();
    int abort,i;
    char s[180];

    outchr(12);
    abort=0;
    pla("0NN2�0Name2��������������������������������������0Filename2�0Dsl2�0AGE2�0Max2��0Path2���",
    &abort);
    for (i=0; (i<sys.num_dirs) && (!abort); i++) {
        dirdata(i,s);
        pla(s,&abort);
    }
}


void modify_dir(int n)
{
    auto& sys = System::instance();
    directoryrec r;
    char s[MAX_PATH_LEN],s1[MAX_PATH_LEN],ch,ch2,*ss;
    int i,i1,done;

    done=0;
    r=sys.directories[n];
    do {
        outchr(12);
        npr("31. Name       :0 %s\r\n",r.name);
        npr("32. Filename   :0 %s\r\n",r.filename);
        npr("33. Path       :0 '%s'\r\n",r.dpath);
        npr("34. Access ACS :0 %s\r\n",r.acs);
        npr("35. Confernce  :0 %c",r.confnum); 
        nl();
        npr("36. Max Files  :0 %d\r\n",r.maxfiles);
        strcpy(s,"None.");
        if (r.dar!=0) {
            for (i=0; i<16; i++)
                if ((1 << i) & r.dar)
                    s[0]='A'+i;
            s[1]=0;
        }    
        npr("37. DAR        :0 %s\r\n",s);
        npr("38. Cmnt File  :0 %s\r\n",r.upath);
        npr("39. List ACS   :0 %s\r\n",r.vacs);
        if (r.mask & mask_no_uploads) s[0]='U';
        else s[0]='\xFA';
        if (r.mask & mask_archive)  s[1]='A';
        else s[1]='\xFA';
        if (r.mask & mask_autocredit) s[2]='C';
        else s[2]='\xFA';
        if (r.mask & mask_no_ratio) s[3]='R';
        else s[3]='\xFA';
        if (r.mask & mask_FDN) s[4]='N';
        else s[4]='\xFA';
        if (r.mask & mask_PD) s[5]='G';
        else s[5]='\xFA';
        s[6]=0;
        npr("3Flags         :0 %s\r\n",s);
        nl();
        prt(5,"File Base Edit (?=Help)0 ");
        ch=onek("Q123456789UGACRJN[]?");
        switch(ch) {
        case '?': 
            printmenu(26); 
            pausescr(); 
            break;
        case 'U': 
            r.mask ^= mask_no_uploads; 
            break;
        case 'A': 
            r.mask ^= mask_archive; 
            break;
        case 'C': 
            r.mask ^= mask_autocredit; 
            break;
        case 'R': 
            r.mask ^= mask_no_ratio; 
            break;
        case 'N': 
            togglebit((long *)&r.mask,mask_FDN); 
            break;
        case 'G': 
            togglebit((long *)&r.mask,mask_PD); 
            break;
        case 'Q': 
            done=1; 
            break;
        case ']': 
            if((n>=0) && (n<sys.num_dirs-1))  { 
                sys.directories[n++]=r; 
                r=sys.directories[n];
            }
            break;
        case '[': 
            if(n>0 ){ 
                sys.directories[n--]=r; 
                r=sys.directories[n];
            } 
            break;
        case 'J': 
            nl(); 
            prt(4,"Jump to which dir? ");
            input(s,4); 
            if(s[0]) {
                i=atoi(s);
                sys.directories[n]=r;
                r=sys.directories[i];
            }
            break;
        case '1':
            nl();
            prt(2,"New Name? ");
            inli(s,"",41,1);
            if (s[0])
                strcpy(r.name,s);
            break;
        case '8':
            nl();
            inputdat("Comment File Name",s,12,0);
            if(s[0]) strcpy(r.upath,s);
            break;
        case '2':
            nl();
            inputdat("New Filename",s,8,0);
            if ((s[0]!=0) && (strchr(s,'.')==0))
                strcpy(r.filename,s);
            break;
        case '3':
            nl();
            pl("You must enter the FULL PATHNAME to the directory.");
            inputdat("FULL Pathname to Desired Directory",s,75,0);
            if (s[0]) {
                if (chdir(s)) {
                    chdir(sys.cdir);
                    if (mkdir(s)) {
                        pl("Error Creating/Changing to directory");
                        pausescr();
                        s[0]=0;
                    }
                } 
                else {
                    chdir(sys.cdir);
                }
                if (s[0]) {
                    if(s[strlen(s)-1]!='\\')
                        strcat(s,"\\");
                    strcpy(r.dpath,s);
                }
            }
            break;
        case '4':
            nl();
            inputdat("Enter General Access ACS",s,20,0);
            if(s[0]) strcpy(r.acs,s);
            break;
        case '9':
            nl();
            inputdat("Enter List ACS",s,20,0);
            if(s[0]) strcpy(r.vacs,s);
            break;
        case '5':
            nl();
            npr("5New Confernce Flag? ");
            r.confnum=onek("ABCDEFGHIJKLMNOPQRSTUVWXYZ@!123456790");
            break;
        case '6':
            nl();
            inputdat("Maximum Files for this Area",s,5,0);
            i=atoi(s);
            if ((i>0) && (i<32676) && (s[0]))
                r.maxfiles=i;
            break;
        case '7':
            nl();
            npr("5New DAR? ");
            ch2=onek("ABCDEFGHIJKLMNOP\r");
            if (ch2=='\r')
                r.dar=0;
            else
                r.dar=1 << (ch2-'A');
            break;
        }
    } 
    while ((!done) && (!io.hangup));
    sys.directories[n]=r;
    if (!sys.wfc)
        changedsl();
}

void swap_dirs(int dir1, int dir2)
{
    auto& sys = System::instance();
    int i,i1,i2,nu;
    directoryrec drt;

    drt=sys.directories[dir1];
    sys.directories[dir1]=sys.directories[dir2];
    sys.directories[dir2]=drt;
}


void insert_dir(int n,char path[60], int temp,int config)
{
    auto& sys = System::instance();
    directoryrec r;
    int i,i1,nu;
    userrec u;
    long l1,l2,l3;

    if(!temp) {
        for (i=sys.num_dirs-1; i>=n; i--)
            sys.directories[i+1]=sys.directories[i];
    }
    if(temp) strcpy(r.name,"<< Temporary >>");
    else strcpy(r.name,"�> New Directory <�");
    if(temp) strcpy(r.filename,"TEMPDIR");
    else strcpy(r.filename,"NEWDIR");
    strcpy(r.dpath,path);
    strcpy(r.upath,"COMMENT");
    strcpy(r.acs,"D50");
    strcpy(r.vacs,"D50");
    r.maxfiles=50;
    r.dar=0;
    r.type=temp;
    r.mask=0;
    r.confnum='@';
    sys.directories[n]=r;
    if(!temp) {
        ++sys.num_dirs;
        userdb_load(1,&u);
        nu=userdb_max_num() + 1;
        for (i=1; i<nu; i++) {
            userdb_load(i,&u);
            for(i1=n;i1<200;i1++)
                u.nscn[i1]=u.nscn[i1+1];
            userdb_save(i,&u);
        }
    }

    if(config)
        modify_dir(n);
}


void delete_dir(int n)
{
    auto& sys = System::instance();
    int i,i1,nu;
    userrec u;
    long l1,l2;

    for (i=n; i<sys.num_dirs; i++)
        sys.directories[i]=sys.directories[i+1];
    --sys.num_dirs;
    userdb_load(1,&u);
    nu=userdb_max_num() + 1;

    for (i=1; i<nu; i++) {
        userdb_load(i,&u);
        for(i1=200;i1<n;i--)
            u.nscn[i1]=u.nscn[i1-1];
        userdb_save(i,&u);
    }

    if (!sys.wfc)
        changedsl();
}


void diredit()
{
    auto& sys = System::instance();
    int i,i1,i2,done,f;
    char s[MAX_PATH_LEN],s1[MAX_PATH_LEN],s2[MAX_PATH_LEN],ch;

    if (!checkpw())
        return;
    showdirs();
    done=0;
    do {
        nl();
        outstr(get_string2(1));
        ch=onek("QPDIM?");
        switch(ch) {
        case '?':
            showdirs();
            break;
        case 'Q':
            done=1;
            break;

        case 'P':
            if (sys.num_dirs<MAX_DIRS) {
                nl();
                prt(2,"Move which Area? ");
                input(s,3);
                i1=atoi(s);
                if ((!s[0]) || (i1<0) || (i1>sys.num_dirs))
                    break;
                nl();
                prt(2,"Place before which area? ");
                input(s,3);
                i2=atoi(s);
                if ((!s[0]) || (i2<0) || (i2%32==0) || (i2>sys.num_dirs) || (i1==i2))
                    break;
                nl();
                if (i2<i1)
                    i1++;
                insert_dir(i2,sys.cfg.dloadsdir,0,0);
                swap_dirs(i1,i2); 
                delete_dir(i1);   
                showdirs();
            } 
            else {
                nl();
                npr("There must be less than %d areas to move one",MAX_DIRS);
            }
            break;


        case 'M':
            nl();
            prt(2,"Dir number? ");
            input(s,2);
            i=atoi(s);
            if ((s[0]!=0) && (i>=0) && (i<sys.num_dirs))
                modify_dir(i);
            break;
        case 'I':
            if (sys.num_dirs<200) {
                nl();
                prt(2,"Insert before which dir? ");
                input(s,2);
                i=atoi(s);
                if ((s[0]!=0) && (i>=0) && (i<=sys.num_dirs))
                    insert_dir(i,sys.cfg.dloadsdir,0,0);
            }
            break;
        case 'D':
            nl();
            prt(2,"Delete which dir? ");
            input(s,2);
            i=atoi(s);
            if ((s[0]!=0) && (i>=0) && (i<sys.num_dirs)) {
                nl();
                sprintf(s1,"Delete %s? ",sys.directories[i].name);
                prt(5,s1);
                if (yn())
                    delete_dir(i);
            }
            break;
        }
    } 
    while ((!done) && (!io.hangup));
    sprintf(s,"%sdirs.dat",sys.cfg.datadir);
    f=open(s,O_RDWR | O_BINARY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);
    write(f,(void *)&sys.directories[0], sys.num_dirs * sizeof(directoryrec));
    close(f);
}


void protdata(int n, char *s)
{
    auto& sys = System::instance();
    protocolrec r;

    r=sys.proto[n];
    sprintf(s,"3%-2d  3  %c    3%-59s",n,r.singleok?'Y':'N',r.description);
}

void showprots()
{
    auto& sys = System::instance();
    int abort,i;
    char s[180];

    outchr(12);
    abort=0;
    pla("0NN2��0Single2��0Description2��������������������������������������������������������",&abort);
    for (i=0; (i<sys.numextrn) && (!abort); i++) {
        protdata(i,s);
        pla(s,&abort);
    }
}


void modify_prot(int n)
{
    auto& sys = System::instance();
    protocolrec r;
    char s[MAX_PATH_LEN],s1[MAX_PATH_LEN],ch,ch2,*ss;
    int i,i1,done;

    done=0;
    r=sys.proto[n];
    do {
        outchr(12);
        npr("01. Descrition        : 3%s\r\n",r.description);
        npr("02. Key               : 3%c\r\n",r.key);
        npr("03. Batch Receive     : 3%s\r\n",r.receivebatch);
        npr("04. Batch Send        : 3%s\r\n",r.sendbatch);
        npr("05. File Sent         : 3%c",r.ok1); 
        nl();
        npr("06. File Received     : 3%c",r.ok2); 
        nl();
        npr("07. Error in Transfer : 3%c",r.nok1); 
        nl();
        npr("08. Single            : 3%s\r\n",r.singleok? "Ok":"No");
        if(r.singleok) {
            npr("09. Single Receieve   : 3%s\r\n",r.receivefn);
            npr("00. Single Send       : 3%s\r\n",r.sendfn);
        }
        nl();
        pl("%1 = Com Port and Speed");
        pl("%2 = Com Port");
        pl("%3 = File Name");
        pl("%4 = Port Speed");
        pl("%5 = Batch List File (Without @)");
        nl();
        outstr("5Protocol Editor (0-9,J,Q,[,]): ");
        ch=onek("1234567890J[]Q");
        switch(ch) {
        case 'Q': 
            done=1; 
            break;
        case '8': 
            r.singleok=opp(r.singleok); 
            break;
        case '2': 
            outstr("Enter Key for this Protocol: "); 
            r.key=toupper(getkey()); 
            break;
        case ']': 
            if((n>=0) && (n<sys.numextrn-1))  { 
                sys.proto[n++]=r; 
                r=sys.proto[n];
            }
            break;
        case '[': 
            if(n>0){ 
                sys.proto[n--]=r; 
                r=sys.proto[n];
            } 
            break;
        case 'J': 
            nl(); 
            prt(4,"Jump to which Protocol? ");
            input(s,4); 
            if(s[0]) {
                i=atoi(s);
                sys.proto[n]=r;
                r=sys.proto[i];
            }
            break;
        case '1': 
            nl(); /*pl("New Description? "); nl();
                                  mpl(75); inputl(s,79);*/
            inputdat("Description",s,75,1);
            if(s[0]) strcpy(r.description,s); 
            break;
        case '9': 
            nl(); /*pl("New Receive Command Line? ");nl();
                                  mpl(75); inputl(s,79);*/
            inputdat("New Receive Command Line",s,75,1);
            if(s[0]) strcpy(r.receivefn,s); 
            break;
        case '0': 
            nl(); /*pl("New Send Command Line? ");   nl();
                                  mpl(75); inputl(s,79);*/
            inputdat("New Send Command Line",s,75,1);
            if(s[0]) strcpy(r.sendfn,s); 
            break;
        case '5': 
            nl(); 
            npr("3File Sent\r\n5: ");
            input(s,3); 
            if(s[0]) r.ok1=s[0]; 
            break;
        case '6': 
            nl(); 
            npr("3File Received\r\n5: ");
            input(s,3); 
            if(s[0]) r.ok2=s[0]; 
            break;
        case '7': 
            nl(); 
            npr("3Error in Transfer\r\n5: ");
            input(s,3); 
            if(s[0]) r.nok1=s[0]; 
            break;
        case '3': 
            nl(); /*pl("New Receive Batch Command Line? ");nl();
                                  mpl(75); inputl(s,79);*/
            inputdat("New Receive Batch Command Line",s,75,1);
            if(s[0]) strcpy(r.receivebatch,s); 
            break;
        case '4': 
            nl(); /*pl("New Send Batch Command Line? "); nl();
                                  mpl(75); inputl(s,79);*/
            inputdat("New Send Batch Command Line",s,75,1);
            if(s[0]) strcpy(r.sendbatch,s); 
            break;
        }
    } 
    while ((!done) && (!io.hangup));
    sys.proto[n]=r;
}


void insert_prot(int n)
{
    auto& sys = System::instance();
    protocolrec r;
    int i,i1,nu;
    long l1,l2,l3;
    char s[MAX_PATH_LEN];


    for (i=sys.numextrn-1; i>=n; i--)
        sys.proto[i+1]=sys.proto[i];
    sys.numextrn++;

    strcpy(r.description,"** New Protocol **");
    strcpy(r.receivefn,"");
    strcpy(r.sendfn,"");
    r.ok1=0;
    r.nok1=1;
    r.singleok=0;
    strcpy(r.sendbatch,"");
    strcpy(r.receivebatch,"");
    sys.proto[n]=r;
    modify_prot(n);
}


void delete_prot(int n)
{
    auto& sys = System::instance();
    int i;

    for (i=n; i<sys.numextrn; i++)
        sys.proto[i]=sys.proto[i+1];
    --sys.numextrn;
}


void protedit()
{
    auto& sys = System::instance();
    int i,i1,i2,done,f;
    char s[MAX_PATH_LEN],s1[MAX_PATH_LEN],s2[MAX_PATH_LEN],ch;

    if (!checkpw())
        return;
    showprots();
    done=0;
    do {
        nl();
        outstr(get_string2(1));
        ch=onek("QDIM?");
        switch(ch) {
        case '?':
            showprots();
            break;
        case 'Q':
            done=1;
            break;
        case 'M':
            nl();
            prt(2,"Prot number? ");
            input(s,2);
            i=atoi(s);
            if ((s[0]!=0) && (i>=0) && (i<sys.numextrn))
                modify_prot(i);
            break;
        case 'I':
            if (sys.numextrn<200) {
                nl();
                prt(2,"Insert before which prot? ");
                input(s,2);
                i=atoi(s);
                if ((s[0]!=0) && (i>=0) && (i<=sys.numextrn))
                    insert_prot(i);
            }
            break;
        case 'D':
            nl();
            prt(2,"Delete which prot? ");
            input(s,2);
            i=atoi(s);
            if ((s[0]!=0) && (i>=0) && (i<sys.numextrn)) {
                nl();
                sprintf(s1,"Delete %s? ",sys.proto[i].description);
                prt(5,s1);
                if (yn())
                    delete_prot(i);
            }
            break;
        }
    } 
    while ((!done) && (!io.hangup));
    sprintf(s,"%sprotocol.dat",sys.cfg.datadir);
    f=open(s,O_RDWR | O_BINARY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);
    write(f,(void *)&sys.proto[0], sys.numextrn * sizeof(protocolrec));
    close(f);
}

