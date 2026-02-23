#include "platform.h"
#include "fcns.h"
#include "session.h"
#include "system.h"
#pragma hdrstop

#include <time.h>

#define SETREC(i)  lseek(sess.dlf,((long) (i))*((long)sizeof(uploadsrec)),SEEK_SET);


int getrec(char *spec,int *type)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    char s[MAX_PATH_LEN],ch;
    uploadsrec u;
    int i=0,i1=0;

    if(atoi(spec)>0)
        return atoi(spec);

    npr("0A5ll files, 0O5ffline, or 0U5nvaldidated Only? ");
    ch=onek("\rAOU");
    if(ch=='A'||ch=='\r')
        *type=0;
    else if(ch=='O')
        *type=1;
    else if(ch=='U')
        *type=2;

    while(!i1) {
        i=nrecno(spec,i);
        if(i==-1)
            break;
        SETREC(i);
        if(*type==1) {
            read(sess.dlf,&u,sizeof(uploadsrec));
            strcpy(s,sys.directories[sess.udir[sess.curdir].subnum].dpath);
            strcat(s,u.filename);
            if(exist(s))
                i1=0;
            else
                break;
        } else if(*type==2) {
            if(!u.ats[1])
                i1=0;
            else
                break;
        } else
             break;
    }

    return i;
}

void getnextrec(char *spec,int *cp,int type)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    int i= *cp,i1;
    uploadsrec u;
    char s[MAX_PATH_LEN];

    if(atoi(spec)>0) {
        *cp = -1;
        return;
    }

    i1=0;
    while(!i1) {
        *cp=nrecno(spec,i);
        i=*cp;
        if(*cp==-1)
            break;
        SETREC(*cp);
        if(type==1) {
            read(sess.dlf,&u,sizeof(uploadsrec));
            strcpy(s,sys.directories[sess.udir[sess.curdir].subnum].dpath);
            strcat(s,u.filename);
            if(exist(s))
                i1=0;
            else
                break;
        } else if(type==2) {
            if(!u.ats[1])
                i1=0;
            else
                break;
        } else
            break;
    }

}


int comparedl(uploadsrec *x, uploadsrec *y, int type)
{
    switch(type) {
    case 0:
        return(strcmp(x->filename,y->filename));
    case 1:
        if (x->daten < y->daten)
            return(-1);
        else
            if (x->daten > y->daten)
            return(1);
        else
            return(0);
    case 2:
        if (x->daten < y->daten)
            return(1);
        else
            if (x->daten > y->daten)
            return(-1);
        else
            return(0);
    }
    return(0);
}


void quicksort(int l,int r,int type)
{
    auto& sess = Session::instance();
    register int i,j;
    uploadsrec a,a2,x;

    i=l; 
    j=r;
    SETREC(((l+r)/2));
    read(sess.dlf, (void *)&x,sizeof(uploadsrec));
    do {
        SETREC(i);
        read(sess.dlf, (void *)&a,sizeof(uploadsrec));
        while (comparedl(&a,&x,type)<0) {
            SETREC(++i);
            read(sess.dlf, (void *)&a,sizeof(uploadsrec));
        }
        SETREC(j);
        read(sess.dlf, (void *)&a2,sizeof(uploadsrec));
        while (comparedl(&a2,&x,type)>0) {
            SETREC(--j);
            read(sess.dlf, (void *)&a2,sizeof(uploadsrec));
        }
        if (i<=j) {
            if (i!=j) {
                SETREC(i);
                write(sess.dlf,(void *)&a2,sizeof(uploadsrec));
                SETREC(j);
                write(sess.dlf,(void *)&a,sizeof(uploadsrec));
            }
            i++;
            j--;
        }
    } 
    while (i<j);
    if (l<j)
        quicksort(l,j,type);
    if (i<r)
        quicksort(i,r,type);
}


void sortdir(int dn, int type)
{
    auto& sess = Session::instance();
    dliscan1(dn);
    if (sess.numf>1)
        quicksort(1,sess.numf,type);
    closedl();
}


void sort_all(char ms[MAX_PATH_LEN])
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    int type,i,all,abort=0,next;

    if(!ms[0]) {
        nl();
        npr("5Sort all dirs? ");
        all=yn(); 
        nl();
        npr("5Sort by date? ");
        if(yn()) type=2; 
        else type=0;
    } 
    else {
        if(ms[0]=='G') all=1; 
        else all=0;
        if(ms[1]=='D') type=2; 
        else type=0;
    }

    if(!all)
        sortdir(sess.udir[sess.curdir].subnum,type);
    else
        for (i=0; (i<64) && (sess.udir[i].subnum!=-1) && (!abort); i++) {
        dliscan1(i);
        if(!ms[0]) {
            npr("5Sorting 0%-40s 3(3%d Files3)\r\n",sys.directories[sess.udir[i].subnum].name,sess.numf);
            checka(&abort,&next,0);
        }
        sortdir(i,type);
    }
}


void valfiles()
{
    auto& sess = Session::instance();
    char s[MAX_PATH_LEN],s1[MAX_PATH_LEN],s2[MAX_PATH_LEN],*ss,s3[MAX_PATH_LEN],ch;
    int i,cp,done,valall=0;
    uploadsrec u;
    userrec uu;

    strcpy(s,"*.*");
    align(s);
    dliscan();
    done=0;
    strcpy(s3,s);
    i=recno(s);
    while (i>0&&!done) {
        cp=i;
        SETREC(i);
        read(sess.dlf,(void *)&u,sizeof(uploadsrec));
        if(!u.ats[0]) {
            nl();
            nl();
            if(valall) {
                u.ats[0]=1;
                SETREC(i);
                write(sess.dlf,(void *)&u,sizeof(uploadsrec));
                sprintf(s2,"%s was Validated on %s",u.filename,date());
                ssm(u.ownerusr,0,s2);
                userdb_load(u.ownerusr,&uu);
                u.points=((u.numbytes+1023)/10240);
                uu.fpts+=u.points;
                userdb_save(u.ownerusr,&uu);
                logtypes(3,"Validated file 4%s0 to 4%d0 points",u.filename,u.points);
            } 
            else {
                printfileinfo(&u,sess.udir[sess.curdir].subnum);
                outstr("7Validate 0(2Y/N/P/Q/All0)7? ");
                ch=onek("YNQPA\r");
                switch(ch) {
                case 'P':
                case 'Y':
                case 'A':
                case '\r': 
                    u.ats[0]=1;
                    if(ch=='A') valall=1;
                    if(ch=='P') {
                        nl();
                        outstr("Enter Points: ");
                        input(s2,5);
                        if(s2[0]) u.points=atoi(s2);
                        else u.points=((u.numbytes+1023)/10240);
                    } 
                    else
                        u.points=((u.numbytes+1023)/10240);
                    SETREC(i);
                    write(sess.dlf,(void *)&u,sizeof(uploadsrec));
                    sprintf(s2,"%s was Validated on %s",u.filename,date());
                    ssm(u.ownerusr,0,s2);
                    userdb_load(u.ownerusr,&uu);
                    uu.fpts+=u.points;
                    userdb_save(u.ownerusr,&uu);
                    logtypes(3,"Validated file 4%s0 to 4%d0 points",u.filename,u.points);
                    break;
                case 'N': 
                    break;
                case 'Q': 
                    done=1; 
                    break;
                }
            }
        }
        i=nrecno(s3,cp);
    }
    closedl();
}


int upload_file(char *fn, int dn,int *ato)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    directoryrec d;
    uploadsrec u,u1;
    int i,i1,i2,ok=1,f;
    char s[MAX_PATH_LEN],s1[MAX_PATH_LEN],ff[MAX_PATH_LEN],*ss;
    long l,len;
    double ti;

    d=sys.directories[dn];
    strcpy(s,fn);
    align(s);
    strcpy(u.filename,s);
    u.ownerusr=sess.usernum;
    u.ownersys=0;
    u.numdloads=0;
    u.filetype=0;
    u.mask=0;
    u.points=0;
    u.ats[0]=1;
    strcpy(ff,d.dpath);
    strcat(ff,s);
    f=open(ff,O_RDONLY | O_BINARY);
    l=filelength(f);
    u.numbytes=l;
    close(f);
    strcpy(u.upby,nam(&sess.user,sess.usernum));
    strcpy(u.date,date());
    npr("0%s5:3 %4ldk 5:2 ",u.filename,(u.numbytes+1023)/1024);
    if(!*ato) {
        mpl(39);
        inputl(u.description,39);
        if (u.description[0]=='.')
            switch(toupper(u.description[1])) {
            case 'N': 
                return -1;
            case 'S': 
                ok=0; 
                break;
            case 'D': 
                ok=0; 
                unlink(ff); 
                break;
            case 'A':
                strcpy(u.description,get_string(85));
                *ato=1;
                break;
            case 'Q': 
                return -2;
            }
    }
    if(u.description[0]=='/') {
        modify_extended_description(&ss);
        add_extended_description(u.filename,ss);
        free(ss);
        u.mask |= mask_extended;
        strcpy(s1,u.description+1);
        strcpy(u.description,s1);
    }
    if (u.description[0]==0)
        strcpy(u.description,get_string(85));
    if(ok) {
        sess.user.fpts+=(u.numbytes+1023)/10240;
        ++sess.user.uploaded;
        if (strstr(u.filename,".GIF"))
            addgif(&u,d.dpath);
        comment_arc(stripfn(u.filename),d.dpath,d.upath);
        strcpy(ff,d.dpath);
        strcat(ff,stripfn(u.filename));
        adddiz(ff,&u);
        u.points=((l+1023)/10240);
        sess.user.uk += ((l+1023)/1024);
        time(&l);
        u.daten=l;
        for (i=sess.numf; i>=1; i--) {
            SETREC(i);
            read(sess.dlf,(void *)&u1,sizeof(uploadsrec));
            SETREC(i+1);
            write(sess.dlf,(void *)&u1,sizeof(uploadsrec));
        }
        SETREC(1);
        write(sess.dlf,(void *)&u,sizeof(uploadsrec));
        ++sess.numf;
        u1.numbytes=sess.numf;
        SETREC(0);
        write(sess.dlf,(void *)&u1,sizeof(uploadsrec));
        ++sys.status.uptoday;
        save_status();
        logpr("3+ 2Locally uploaded %s on %s",u.filename,d.name);
    }
    return(1);
}


int uploadall(int dn, char s[20])
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    int i,i1,f1,maxf,ok,ocd,title=0,ato=0;
    char s1[MAX_PATH_LEN];
    struct ffblk ff;
    uploadsrec u;

    dliscan1(dn);
    ocd=sess.curdir;
    sess.curdir=dn;
    nl();
    if(!stricmp(s,"????????.???"))
        strcpy(s,"*.*");
    strcpy(s1,(sys.directories[dn].dpath));
    maxf=sys.directories[dn].maxfiles;
    strcat(s1,s);
    f1=findfirst(s1,&ff,0);
    ok=1;
    while ((f1==0) && (!io.hangup) && (sess.numf<maxf) && (ok)) {
        strcpy(s,(ff.ff_name));
        align(s);
        i=recno(s);
        if (i==-1) {
            if(!title) {
                pl("5Enter File Descriptions. A / As the first letter gets Extened Description.");
                pl("2.S to Skip File, .N to Skip Directory, .Q to Quit, .D to Delete");
                pl("Blank Lines will be 'No Description Given at Upload'");
                title=1;
                ato=0;
            }

            i1=upload_file(s,dn,&ato);
            if(i1==-1)
                ok=0;
            if(i1==-2) return(i1);
        } 
        else {
            SETREC(i);
            read(sess.dlf,(void *)&u, sizeof(uploadsrec));
        }
        f1=findnext(&ff);
    }
    sess.curdir=ocd;
    closedl();
    if (!ok)
        pl("Aborted.");
    if (sess.numf>=maxf)
        pl("directory full.");
    return(i1);
}

void removefile(void)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    int i,i1,ok,rm,abort,rdlp,type;
    char ch,s[MAX_PATH_LEN],s1[MAX_PATH_LEN],spec[13];
    uploadsrec u;
    userrec uu;

    nl();
    file_mask(spec);
    dliscan();
    nl();

    i=getrec(spec,&type);

    abort=0;
    while ((!io.hangup) && (i!=-1) && (!abort)) {
        SETREC(i);
        read(sess.dlf,&u,sizeof(uploadsrec));
        if (dcs() || (u.ownerusr==sess.usernum) ) {
            nl();
            printfileinfo(&u,sess.udir[sess.curdir].subnum);
            prt(5,"Remove? (Y/N/Q) ");
            ch=onek("QNY");
            if (ch=='Q')
                abort=1;
            if (ch=='Y') {
                rdlp=1;
                if (dcs()) {
                    prt(5,"Delete file too? ");
                    rm=yn();
                    if (rm) {
                        prt(5,"Remove DL fpts? ");
                        rdlp=yn();
                    }
                } 
                else
                    rm=1;
                if (rm) {
                    strcpy(s1,(sys.directories[sess.udir[sess.curdir].subnum].dpath));
                    strcat(s1,u.filename);
                    unlink(s1);
                    if ((rdlp) && (u.ownersys==0)) {
                        userdb_load(u.ownerusr,&uu);
                        if ((uu.inact & inact_deleted)==0) {
                            --uu.uploaded;
                            uu.uk -= ((u.numbytes+1023)/1024);
                            uu.fpts-=u.points;
                            userdb_save(u.ownerusr,&uu);
                        }
                    }
                }
                if (u.mask & mask_extended)
                    delete_extended_description(u.filename);
                logtypes(3,"4%s 0Removed off of 4%s",u.filename,
                sys.directories[sess.udir[sess.curdir].subnum].name);
                for (i1=i; i1<sess.numf; i1++) {
                    SETREC(i1+1);
                    read(sess.dlf,(void *)&u,sizeof(uploadsrec));
                    SETREC(i1);
                    write(sess.dlf,(void *)&u,sizeof(uploadsrec));
                }
                --i;
                --sess.numf;
                u.numbytes=sess.numf;
                SETREC(0);
                write(sess.dlf,(void *)&u,sizeof(uploadsrec));
            }
        }

        getnextrec(spec,(int *)&i,type);
    }

    closedl();
}


void editfile()
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    char s[MAX_PATH_LEN],s1[MAX_PATH_LEN],s2[MAX_PATH_LEN],*ss,s3[MAX_PATH_LEN],ch,changed;
    int i,cp,done,type,m,dd,y,i1;
    uploadsrec u;
    userrec ur;
    struct date d;
    struct time t;

    nl();
    file_mask(s);
    dliscan();
    nl();
    strcpy(s3,s);

    cp=getrec(s,&type);

    done=0;
    while (done!=2&&cp!=-1) {
        done=0;
        SETREC(cp);
        read(sess.dlf,(void *)&u,sizeof(uploadsrec));
        nl();
        changed=0;
        while (!done) {
            printfileinfo(&u,sess.udir[sess.curdir].subnum);
            nl();
            npr("5File Information Editor (?=Help) ");
            ch=onek("DQNGFVEUPS!?BX\r");
            switch(ch) {
            case 'X':
                togglebit((long *)&u.mask,mask_unavail);
                changed=1;
                break;
            case 'B':
                inputdat("Private User Name/Number",s,31,0);
                if(!s[0]) {
                    npr("5Make public? ");
                    u.ownersys=!yn();
                } 
                else {
                    i=finduser1(s);
                    if(i) {
                        u.ownersys=i;
                        nl();
                        userdb_load(u.ownersys,&ur);
                        npr("3File now private for 0%s\r\n",nam(&ur,u.ownersys));
                        changed=1;
                    }
                }
                break;
            case '?': 
                printmenu(23); 
                break;
            case '!': 
                if(u.ats[1]!=100)
                    u.ats[1]=100;
                else
                    u.ats[1]=0;
                changed=1;
                break;
            case '\r': 
                done=1; 
                break;
            case 'Q': 
                done=2; 
                break;
            case 'S':
                nl();
                npr("3Upload Date\r\n5: 0");
                inputdate(s,0);
                m=atoi(s);
                dd=atoi(&(s[3]));
                y=atoi(&(s[6]))+1900;
                if ((strlen(s)==8) && (m>0) && (m<=12) && (dd>0) && (dd<32) && (y>=1980)) {
                    t.ti_min=0;
                    t.ti_hour=0;
                    t.ti_hund=0;
                    t.ti_sec=0;
                    d.da_year=y;
                    d.da_day=dd;
                    d.da_mon=m;
                    u.daten=dostounix(&d,&t);
                    strcpy(u.date,s);
                    changed=1;
                }
                break;
            case 'U': 
                changed=1; 
                nl();
                inputdat("Uploader",s,45,1);
                if(s[0])
                    strcpy(u.upby,s);
                break;
            case 'F': 
                changed=1; 
                nl();
                inputdat("Filename",s,12,0);
                if (!okfn(s))
                    s[0]=0;
                if (s[0]) {
                    align(s);
                    if (strcmp(s,"        .   ")) {
                        strcpy(s1,sys.directories[sess.udir[sess.curdir].subnum].dpath);
                        strcpy(s2,s1);
                        strcat(s1,s);
                        if (exist(s1))
                            pl("Filename already in use; not changed.");
                        else {
                            strcat(s2,u.filename);
                            rename(s2,s1);
                            if (exist(s1)) {
                                ss=read_extended_description(u.filename);
                                if (ss) {
                                    delete_extended_description(u.filename);
                                    add_extended_description(s,ss);
                                    free(ss);
                                }
                                strcpy(u.filename,s);
                            } 
                            else
                                pl("Bad filename.");
                        }
                    }
                }
                break;
            case 'D': 
                changed=1;
                nl();
                inputdat("Description",s,39,1);
                if (s[0])
                    strcpy(u.description,s);
                break;
            case 'V': 
                changed=1;
                if(u.ats[0]) u.ats[0]=0; 
                else u.ats[0]=1; 
                break;
            case 'P': 
                changed=1;
                nl();
                inputdat("File Points",s,3,0);
                if(s[0]) u.points=atoi(s);
                break;
            case 'E': 
                changed=1;
                ss=read_extended_description(u.filename);
                nl();
                nl();
                prt(5,"Modify extended description? ");
                if (yn()) {
                    nl();
                    if (ss) {
                        prt(5,"Delete it? ");
                        if (yn()) {
                            free(ss);
                            delete_extended_description(u.filename);
                            u.mask &= ~mask_extended;
                        } 
                        else {
                            u.mask |= mask_extended;
                            modify_extended_description(&ss);
                            if (ss) {
                                delete_extended_description(u.filename);
                                add_extended_description(u.filename,ss);
                                free(ss);
                            }
                        }
                    } 
                    else {
                        modify_extended_description(&ss);
                        if (ss) {
                            add_extended_description(u.filename,ss);
                            free(ss);
                            u.mask |= mask_extended;
                        } 
                        else
                            u.mask &= ~mask_extended;
                    }
                } 
                else
                    if (ss) {
                    free(ss);
                    u.mask |= mask_extended;
                } 
                else
                    u.mask &= ~mask_extended;
                break;
            }
        }
        if(changed)
            logtypes(3,"Edited file information for 4%s",u.filename);
        SETREC(cp);
        write(sess.dlf,(void *)&u,sizeof(uploadsrec));
        getnextrec(s3,(int *)&cp,type);
    }
    closedl();
}

void localupload()
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    char s[20],s1[20];
    int i=1,i1=0,done=0;

    file_mask(s);
    stripfn(s);
    outstr("5Upload to All Areas? ");
    if(yn()) {
        while(!done) {
            strcpy(s1,s);
            if(i1>sys.num_dirs) {
                done=1;
                continue;
            }
            if(sess.udir[i1].subnum<0) {
                done=1;
                continue;
            }
            npr("5Local Uploading: 0 %s\r\n",sys.directories[sess.udir[i1].subnum].name);
            i=uploadall(sess.udir[i1].subnum,s1);
            if(i==-2) return;
            i1++;
        }
    } 
    else uploadall(sess.udir[sess.curdir].subnum,s);
}

void create_file()
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    directoryrec d;
    uploadsrec u,u1;
    int i,i1,i2,ok,f;
    char s[MAX_PATH_LEN],s1[MAX_PATH_LEN],ff[MAX_PATH_LEN],*ss;
    long l,len;
    double ti;

    d=sys.directories[sess.udir[sess.curdir].subnum];

    if(file_mask(u.filename)==-1)
        return;

    dliscan();
    u.ownerusr=sess.usernum;
    u.ownersys=0;
    u.numdloads=0;
    u.filetype=0;
    u.mask=0;
    u.points=0;
    u.ats[0]=1;
    strcpy(ff,d.dpath);
    strcat(ff,s);
    f=open(ff,O_RDONLY | O_BINARY);
    if(f<0) {
        inputdat("Approx. Size in K",s,15,0);
        u.numbytes=(long)atoi(s)*1024;
        l=u.numbytes;
    } 
    else {
        l=filelength(f);
        u.numbytes=l;
        close(f);
    }
    strcpy(u.upby,nam(&sess.user,sess.usernum));
    strcpy(u.date,date());
    npr("0%s5:3 %4ldk 5:2 ",u.filename,(u.numbytes+1023)/1024);
    mpl(39);
    inputl(u.description,39);
    if(u.description[0]=='/') {
        modify_extended_description(&ss);
        add_extended_description(u.filename,ss);
        free(ss);
        u.mask |= mask_extended;
        strcpy(s1,u.description+1);
        strcpy(u.description,s1);
    }
    if (u.description[0]==0)
        strcpy(u.description,"No Description Given at Upload");
    nl();
    npr("5Create this file? ");
    ok=yn();
    if(ok) {
        sess.user.fpts+=(u.numbytes+1023)/10240;
        ++sess.user.uploaded;
        if (strstr(u.filename,".GIF"))
            addgif(&u,d.dpath);
        comment_arc(stripfn(u.filename),d.dpath,d.upath);
        strcpy(ff,d.dpath);
        strcat(ff,stripfn(u.filename));
        adddiz(ff,&u);
        u.points=((l+1023)/10240);
        sess.user.uk += ((l+1023)/1024);
        time(&l);
        u.daten=l;
        for (i=sess.numf; i>=1; i--) {
            SETREC(i);
            read(sess.dlf,(void *)&u1,sizeof(uploadsrec));
            SETREC(i+1);
            write(sess.dlf,(void *)&u1,sizeof(uploadsrec));
        }
        SETREC(1);
        write(sess.dlf,(void *)&u,sizeof(uploadsrec));
        ++sess.numf;
        u1.numbytes=sess.numf;
        SETREC(0);
        write(sess.dlf,(void *)&u1,sizeof(uploadsrec));
        ++sys.status.uptoday;
        save_status();
        logtypes(3,"Created File %s on %s",u.filename,d.name);
    }
    closedl();
}

void del_entry(int which)
{
    auto& sess = Session::instance();
    int i;
    uploadsrec u;

    for (i=which; i<sess.numf; i++) {
        SETREC(i+1);
        read(sess.dlf,(void *)&u,sizeof(uploadsrec));
        SETREC(i);
        write(sess.dlf,(void *)&u,sizeof(uploadsrec));
    }
    sess.numf--;
}

void move_file(void)
{
    auto& sys = System::instance();
    auto& sess = Session::instance();
    char sx[MAX_PATH_LEN],s[MAX_PATH_LEN],s1[MAX_PATH_LEN],s2[MAX_PATH_LEN],ch,*ss,type[10];
    int i,i1,ok,d1,d2,done,cp,abort=0,dest,ttype;
    uploadsrec u,u1,u2;
    char *b;

    ok=0;
    nl();
    nl();

    file_mask(sx);

    dliscan();
    i=getrec(sx,&ttype);

    done=0;
    while ((!io.hangup) && (i>0) && (!done)) {
        cp=i;
        dliscan();
        SETREC(i);
        read(sess.dlf,(void *)&u,sizeof(uploadsrec));
        nl();
        printinfo(&u,&abort,0);
        nl();
        ok=0;
        prt(5,"Move this (Y/N/Q)? ");
        ch=onek("QNY");
        if (ch=='Q')
            done=1;
        if (ch=='Y') {
            strcpy(s1,sys.directories[sess.udir[sess.curdir].subnum].dpath);
            strcat(s1,u.filename);
            d1=-1;
            do {
                nl();
                prt(5,"Destination? ");
                input(type,3);
                if (type[0]=='?')
                    dirlist(0);
            }
            while ((!io.hangup) && (type[0]=='?'));
            if (type[0])
                for (i1=0; (i1<64) && (sess.udir[i1].subnum!=-1); i1++)
                    if (strcmp(sess.udir[i1].keys,type)==0)
                        d1=i1;
            if(i1<64&&sess.udir[i1].subnum!=-1)
                d1=i1;
            if (d1!=-1) {
                ok=1;
                d1=sess.udir[d1].subnum;
                dest=d1;
                dliscan1(d1);
                if (recno(u.filename)>0) {
                    ok=0;
                    nl();
                    pl("Filename already in use in that directory.");
                }
                if (sess.numf>=sys.directories[d1].maxfiles) {
                    ok=0;
                    nl();
                    pl("Too many files in that directory.");
                }
                if (freek1(sys.directories[d1].dpath)<((double)(u.numbytes/1024L)+3)) {
                    ok=0;
                    nl();
                    pl("Not enough disk space to move it.");
                }
                closedl();
                dliscan();
            }
            else {
                ok=0;
            }
        }

        if (ok) {
            --cp;
            logtypes(3,"Moved file 4%s0 to 2%s",u.filename,sys.directories[d1].name);
            del_entry(i);
            u1.numbytes=sess.numf;
            SETREC(0);
            write(sess.dlf,(void *)&u1,sizeof(uploadsrec));
            ss=read_extended_description(u.filename);
            if (ss)
                delete_extended_description(u.filename);
            closedl();

            strcpy(s2,sys.directories[d1].dpath);
            strcat(s2,u.filename);
            dliscan1(d1);
            for (i=sess.numf; i>=1; i--) {
                SETREC(i);
                read(sess.dlf,(void *)&u1,sizeof(uploadsrec));
                SETREC(i+1);
                write(sess.dlf,(void *)&u1,sizeof(uploadsrec));
            }
            SETREC(1);
            write(sess.dlf,(void *)&u,sizeof(uploadsrec));
            ++sess.numf;
            u1.numbytes=sess.numf;
            SETREC(0);
            write(sess.dlf,(void *)&u1,sizeof(uploadsrec));
            if (ss) {
                add_extended_description(u.filename,ss);
                free(ss);
            }

            closedl();

            if ((strcmp(s1,s2)!=0) && (exist(s1))) {
                d2=0;
                if ((s1[1]!=':') && (s2[1]!=':'))
                    d2=1;
                if ((s1[1]==':') && (s2[1]==':') && (s1[0]==s2[0]))
                    d2=1;
                if (d2) {
                    rename(s1,s2);
                    unlink(s1);
                }
                else {
                    if ((b=(char *)malloca(16400))==NULL)
                        return;
                    d1=open(s1,O_RDONLY | O_BINARY);
                    d2=open(s2,O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
                    i=read(d1,(void *)b,16384);
                    while (i>0) {
                        write(d2,(void *)b,i);
                        i=read(d1,(void *)b,16384);
                    }
                    close(d1);
                    close(d2);
                    unlink(s1);
                    free(b);
                }
            }
            nl();
            npr("File Moved to %s\r\n",sys.directories[dest].name);
        }

        getnextrec(sx,(int *)&cp,ttype);
    }

    closedl();
}
