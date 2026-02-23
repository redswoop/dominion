#include "platform.h"
#include "fcns.h"
#include "session.h"
#include "system.h"

#pragma hdrstop


static auto& sys = System::instance();
static auto& sess = Session::instance();

void change_colors(userrec *u1)
{
    int i,done,i1,i2;
    char s[MAX_PATH_LEN],ch,nc,*ss,s1[MAX_PATH_LEN];
    userrec u;

    u=*u1;
    done=0;
    do {
        outchr(12);
        printfile("collist");
        nl();
        inputdat("Enter # of Color to Edit, Q=Quit",ss,2,0);
        if (ss[0]=='Q') done=1;
        else if(!ss[0]) done=1;
        else {
            i=atoi(ss);
            if (u.sysstatus & sysstatus_color)  {
                color_list();
                ansic(0);
                nl();
                prt(2,"Foreground? ");
                ch=onek("01234567\r");
                if(ch=='\r') ch='0';
                nc=ch-'0';
                prt(2,"Background? ");
                ch=onek("01234567\r");
                if(ch=='\r') ch='0';
                nc=nc | ((ch-'0') << 4);
            } 
            else {
                nl();
                prt(5,"Inversed? ");
                if (yn()) {
                    if ((u.colors[1] & 0x70) == 0)
                        nc=0 | ((u.colors[1] & 0x07) << 4);
                    else
                        nc=(u.colors[1] & 0x70);
                } 
                else {
                    if ((u.colors[1] & 0x70) == 0)
                        nc=0 | (u.colors[1] & 0x07);
                    else
                        nc=((u.colors[1] & 0x70) >> 4);
                }
            }
            prt(5,"Intensified? ");
            if (yn())
                nc |= 0x08;

            prt(5,"Blinking? ");
            if (yn())
                nc |= 0x80;

            setc(nc);
            pl(describe(nc));
            ansic(0);
            prt(5,"Is this what you want? ");
            if (yn()) {
                pl("Color saved.");
                if(io.colblock)
                    sys.nifty.defaultcol[i]=nc;
                else {
                    u.colors[i]=nc;
                }
            } 
            else {
                pl("Aborted.");
            }
        }
    } 
    while ((!done) && (!io.hangup));

    *u1=u;
}



void print_cur_stat()
{
    char s[MAX_PATH_LEN],s1[MAX_PATH_LEN],s2[MAX_PATH_LEN];
    userrec ur;

    outchr(12);
    dtitle("Preferences");

    npr("0Screen size       : %d X %d\r\n",
    sess.user.screenchars,
    sess.user.screenlines);

    if(sess.user.sysstatus & sysstatus_avatar) strcpy(s,"Avatar ");
    else
        if(sess.user.sysstatus & sysstatus_ansi) strcpy(s,"Ansi ");
    else strcpy(s,"Ascii");
    if(sess.user.sysstatus & sysstatus_color) strcat(s,"Colour");
    else strcat(s,"Mono");

    npr("Graphics          : %s\r\n",s);
    npr("Help Level        : ");
    switch(sess.user.helplevel) {
    case 0: 
        pl("Expert"); 
        break;
    case 1: 
        pl("Regular"); 
        break;
    case 2: 
        pl("Idiot"); 
        break;
    }
    npr("Comment           : %s\r\n",sess.user.comment);
    npr("Pause on screen   : %s\r\n",
    (sess.user.sysstatus & sysstatus_pause_on_page)?"On":"Off");
    outstr("Mailbox           : ");
    if ((sess.user.forwardusr==0))
        pl("Normal");
    else {
        {
            if (sess.user.forwardusr==255) {
                pl("Closed");
            } 
            else {
                userdb_load(sess.user.forwardusr,&ur);
                if (ur.inact & inact_deleted) {
                    sess.user.forwardusr=0;
                    pl("Normal");
                } 
                else {
                    sprintf(s,"Forward to %s",nam(&ur,sess.user.forwardusr));
                    pl(s);
                }
            }
        }
    }
    npr("Default Protocol  : %s\r\n",sys.proto[sess.user.defprot].description);
    npr("File List Format  : Type %d\r\n",sess.user.flisttype);
    npr("Hotkeys           : %s\r\n",(sess.user.sysstatus & sysstatus_fullline)?"No":"Yes");

    nl();
    if(sess.user.helplevel==2) pausescr();
}


char *cn(char c)
{

    switch(c) {
    case 0:
        return("Black");
    case 1:
        return("Blue");
    case 2:
        return("Green");
    case 3:
        return("Cyan");
    case 4:
        return("Red");
    case 5:
        return("Magenta");
    case 6:
        return("Yellow");
    case 7:
        return("White");
    }
    return("");
}

char *describe(char col)
{
    static char s[MAX_PATH_LEN];
    char s1[MAX_PATH_LEN];


    if (sess.user.sysstatus & sysstatus_color) {
        strcpy(s1,cn(col&0x07));
        sprintf(s,"%s on %s", s1, cn((col>>4)&0x07));
    } 
    else {
        if ((col & 0x07) == 0)
            strcpy(s,"Inversed");
        else
            strcpy(s,"Normal");
    }
    if (col & 0x08) strcat(s,", Intense");
    if (col & 0x80) strcat(s,", Blinking");
    return(s);
}

void color_list()
{
    int i;

    nl();
    nl();
    for (i=0; i<8; i++) {
        if (i==0)
            setc(0x70);
        else
            setc(i);
        npr("%d. %s",i,cn(i));
        setc(0x07);
        nl();
    }
}





void config_qscan(int dl)
{
    char s[71],c;
    int i,done,on;

    done=0;
    outchr(12);
    if(!dl)
        sublist('L');
    else
        dirlist('L');

    do {
        nl();
        outstr("5NewScan Configuration: 0# to Toggle, [Q]uit, [T]oggle All, [?]=List: ");
        input(s,3);
        nl();
        if(!strcmp(s,"?")) {
            outchr(12);
            if(!dl)
                sublist('L');
            else
                dirlist('L');
        } else if (strcmp(s,"Q")==0)
            done=1;
        else if(!strcmp(s,"T")) {
            npr("Toggle 7O0n or Of7f0? ");
            on=1;
            if(onek("OF\r")=='F')
                on=-1;
            for (i=0; dl?sess.udir[i].subnum!=-1:sess.usub[i].subnum!=-1; i++)
                if(!dl)
                    togglenws(i,&sess.user,on);
                else
                    sess.user.nscn[i]=on;
        } else if (atoi(s))
            for (i=0; dl?i<MAX_DIRS:i<MAX_SUBS; i++) {
                if(!dl) {
                    if (strcmp(sess.usub[i].keys,s)==0) {
                        on=inscan(i,&sess.user);
                        togglenws(i,&sess.user,!on);
                    }
                } else {
                    if (strcmp(sess.udir[i].keys,s)==0) {
                        if(sess.user.nscn[sess.udir[i].subnum]<0)
                            sess.user.nscn[sess.udir[i].subnum]=0;
                        else
                            sess.user.nscn[sess.udir[i].subnum]=-1;
                    }
                }
            }
    } 
    while ((!done) && (!io.hangup));
}


void list_macro(char *s)
{
    int i;

    i=0;
    outchr('\"');
    while ((i<80) && (s[i]!=0)) {
        if (s[i]>=32)
            outchr(s[i]);
        else {
            outchr('^');
            outchr(s[i]+64);
        }
        ++i;
    }
    outchr('"');
    nl();
}


void make_macros()
{
    char tempmac[MAX_PATH_LEN],s[MAX_PATH_LEN];
    unsigned char ch,ch1;
    int i,i1,done,done1;

    done=0;
    do {
        outchr(12);
        pl("Ctrl-A macro: ");
        list_macro(&(sess.user.macros[2][0]));
        nl();
        pl("Ctrl-D macro: ");
        list_macro(&(sess.user.macros[0][0]));
        nl();
        pl("Ctrl-F macro: ");
        list_macro(&(sess.user.macros[1][0]));
        nl();
        pl("Ctrl-Y macro: ");
        list_macro(&(sess.user.macros[3][0]));
        nl();
        outstr("5Macro Editor 0(3A,D,F,Y,Q=Quit0)5 : ");
        ch=onek("QADFY");
        if(ch=='Q') done=1;
        else {
            if(ch=='Y') i1=3;
            else if(ch=='A') i1=2;
            else if(ch=='D') i1=0;
            else if(ch=='F') i1=1;
            strcpy(s,&(sess.user.macros[i1][0]));
            sess.user.macros[i1][0]=0;
            done1=0;
            i=0;
            nl();
            pl("Enter your macro now, hit ctrl-Z when done.");
            nl();
            sess.okskey=0;
            do {
                ch1=getkey();
                if (ch1==26)
                    done1=1;
                else
                    if (ch1==8) {
                        if (i>0) {
                            i--;
                            backspace();
                            if (tempmac[i]<32)
                                backspace();
                        }
                    } 
                else {
                    if (ch1>=32) {
                        tempmac[i++]=ch1;
                        outchr(ch1);
                    } 
                    else {
                        tempmac[i++]=ch1;
                        outchr('^');
                        outchr(ch1+64);
                    }
                }
                if (i>=78)
                    done1=1;
            } 
            while ((!done1) && (!io.hangup));
            sess.okskey=1;
            tempmac[i]=0;
            nl();
            pl("You entered:");
            nl();
            nl();
            list_macro(tempmac);
            nl();
            prt(5,"Is this OK? ");
            if (yn()) {
                strcpy(&(sess.user.macros[i1][0]),tempmac);
                nl();
                pl("Macro saved.");
            } 
            else {
                nl();
                strcpy(&(sess.user.macros[i1][0]),s);
                pl("Nothing saved.");
            }
        }
    } 
    while ((!done) && (!io.hangup));
}


void input_pw1()
{
    char s[MAX_PATH_LEN],s1[MAX_PATH_LEN];
    int ok;

    nl();
    prt(5,"Change password? ");
    if (yn()) {
        nl();
        pl("You must now enter your current password.");
        outstr(": ");
        io.echo=0;
        input(s,19);
        if (strcmp(s,sess.user.pw)) {
            nl();
            pl("Incorrect.");
            nl();
            return;
        }
        nl();
        nl();
        pl("Enter your new password, 3 to 20 characters long.");
        outstr(": ");
        io.echo=0;
        input(s,19);
        nl();
        nl();
        pl("Repeat password for verification.");
        outstr(": ");
        io.echo=0;
        input(s1,19);
        if (strcmp(s,s1)==0) {
            if (strlen(s1)<3) {
                nl();
                pl("Password must be 3-20 characters long.");
                pl("Password was not changed.");
                nl();
            } 
            else {
                strcpy(sess.user.pw,s);
                nl();
                pl("Password changed.");
                nl();
                sysoplog("Changed Password.");
            }
        } 
        else {
            nl();
            pl("VERIFY FAILED.");
            pl("Password not changed.");
            nl();
        }
    }
}

void modify_mailbox()
{
    int i,i1,i2;
    unsigned int u;
    char s[MAX_PATH_LEN];

    nl();

    prt(5,"Do you want to close your mailbox? ");
    if (yn()) {
        prt(5,"Are you sure? ");
        if (yn()) {
            sess.user.forwardusr=-1;
            return;
        }
    }
    prt(5,"Do you want to forward your mail? ");
    if (!yn()) {
        sess.user.forwardusr=0;
        return;
    }
    nl();

    nl();
    prt(2,"Forward to which user? ");
    input(s,40);
    i=finduser1(s);
    if (i==sess.usernum) {
        sess.user.forwardusr=0;
        nl();
        pl("Forwarding reset.");
        nl();
        return;
    }
    if (i>0) {
        sess.user.forwardusr=i;
        nl();
        pl("Saved.");
        nl();
    }
}


void getfileformat()
{
    FILE *f;
    char s[181],s1[181],s2[10],ch;
    char done=0,c=1;
    int i,i1;

    nl();
    dtitle("Dominion User Defined File Formats");
    done=0;

    logtypes(2,"Changed File List Format");
    do {
        sprintf(s,"%sfile%d.fmt",sys.cfg.gfilesdir,c);
        f=fopen(s,"rt");
        if(f!=NULL) {
            fgets(s,81,f);
            filter(s,'\n');
            stuff_in1(s1,s,"Dom30a  .Zip","Dominion BBS 3.0"," 300","   0","  1","Fallen Angel","01/01/92","100","00:14:26","");
            sprintf(s,"<%d> %s",c,s1);
            c++;
            pl(s);
            fclose(f);
        } 
        else done=1;
    } 
    while(!done&&!io.hangup);

    nl();
    outstr(get_string(79));
    input(s,3);
    if(s[0]&&atoi(s)<c&&!strchr(s,'Q')&&!strchr(s,'A')) sess.user.flisttype=atoi(s);
    else if(!s[0]) { 
        sess.user.flisttype=1; 
        return; 
    }
    else if(strchr(s,'A')) {
        outchr(12);
        printmenu(32);
        strcpy(s1,"");
        npr("3Enter Your File Format\r\n5:0 ");
        inli(s,s1,81,1);
        if(s[0]) {
            stuff_in1(s1,s,"Dom30a  .Zip","Dominion BBS 3.0"," 300","   0","  1",nam(&sess.user,sess.usernum),date(),"100","00:14:26","");
            npr("%s\r\n\r\n",s1);
            outstr("5Is this what you want? ");
            if(ny()) {
                printfile("collist");
                outstr("5What color would you like the header to be? ");
                input(s2,2);
                i=atoi(s2);
                outstr("5What color would you like the footer to be? ");
                input(s2,2);
                i1=atoi(s2);
                sprintf(s1,"%sfile%d.fmt",sys.cfg.gfilesdir,c);
                f=fopen(s1,"wt");
                fputs(s,f);
                fputs("\n",f);
                fputs(s,f);
                fputs("\n",f);
                fputs(s,f);
                fputs("\n",f);
                for(ch=0;ch<80;ch++)
                    s1[ch]=196;
                s1[ch]=0;
                fprintf(f,"%c%s\n",i+1,s1);
                fprintf(f,"%c%s\n",i1+1,s1);
                sprintf(s,"Added by: %s on %s\n",nam(&sess.user,sess.usernum),date());
                fputs(s,f);
                fclose(f);
                sess.user.flisttype=c;
                logtypes(2,"Added File Format: 4%d",c);
            }
        }
    }
    if(!sess.user.flisttype) sess.user.flisttype=1;
}

void setcolors(userrec *uu)
{
    userrec u;
    int i;

    u=*uu;
    if(u.sysstatus &  sysstatus_color) {
        for(i=0;i<20;i++)
            u.colors[i]=sys.nifty.defaultcol[i];
    } 
    else {
        u.colors[0]=15;
        u.colors[1]=7;
        u.colors[2]=7;
        u.colors[3]=15;
        u.colors[4]=15;
        u.colors[5]=7;
        u.colors[6]=16*7;
        u.colors[7]=15;
        u.colors[8]=143;
        u.colors[9]=15;
        u.colors[10]=16*7;
        u.colors[11]=7;
        u.colors[12]=15;
        u.colors[13]=7;
        u.colors[14]=15;
        u.colors[15]=7;
        u.colors[16]=143;
        u.colors[17]=16*7;
        u.colors[18]=143;
        u.colors[19]=7;
    }
    *uu=u;
}



void input_ansistat()
{
    int i,c,c2;
    char ch;

    if(sess.user.sysstatus & sysstatus_ansi)
        togglebit((long *)&sess.user.sysstatus,sysstatus_ansi);

    if(sess.user.sysstatus & sysstatus_color)
        togglebit((long *)&sess.user.sysstatus,sysstatus_color);

    if(sess.user.sysstatus & sysstatus_avatar)
        togglebit((long *)&sess.user.sysstatus,sysstatus_avatar);

    if(sess.user.sysstatus & sysstatus_rip)
        togglebit((long *)&sess.user.sysstatus,sysstatus_rip);

    nl();

    if (check_ansi()) {
        pl("ANSI Graphics Detected.");
        outstr("Do you wish to use it? ");
    } 
    else {
        pl("[0;31mThis is in Red!");
        nl();
        outstr("Is the above line in red? ");
    }

    if (yn()) {
        nl();
        sess.user.sysstatus |= sysstatus_ansi;
        outstr("Do you want color? ");
        if (ny())
            togglebit((long *)&sess.user.sysstatus,sysstatus_color);
        ansic(0);
        nl();
        outstr("Do you want Avatar? (No if unsure): ");
        if(yn())
            togglebit((long *)&sess.user.sysstatus,sysstatus_avatar);
        nl();
        outstr("Do you want RIPscript support? ");
        if(yn())
            togglebit((long *)&sess.user.sysstatus,sysstatus_rip);
    }
    setcolors(&sess.user);
}

void selecthelplevel()
{
    char c;
    nl();
    dtitle("Select a Help Level");
    printmenu(25);
    outstr("9Select your Help Level: ");
    c=onek("ERI\r");
    switch(c) {
    case 'R':
    case '\r': 
        sess.user.helplevel=1; 
        break;
    case 'E': 
        sess.user.helplevel=0; 
        break;
    case 'I': 
        sess.user.helplevel=2; 
        break;
    }
    ansic(0);
    nl();
}


void getmsgformat()
{
    FILE *f;
    char s[181],s1[181],s2[10],ch;
    char done=0,c=1;
    int i,i1;

    nl();
    dtitle("Dominion User Defined Msg Formats");
    done=0;

    logtypes(2,"Changed Msg List Format");
    do {
        sprintf(s,"%smsg%d.fmt",sys.cfg.gfilesdir,c);
        f=fopen(s,"rt");
        if(f!=NULL) {
            fgets(s1,81,f);
            filter(s1,'\n');
            sprintf(s,"<%d> %s",c,s1);
            c++;
            pl(s);
            fclose(f);
        } 
        else done=1;
    } 
    while(!done&&!io.hangup);

    nl();
    outstr(get_string(79));
    input(s,3);
    if(s[0]&&atoi(s)<c&&!strchr(s,'Q')) sess.user.mlisttype=atoi(s);
    else if(!s[0]) { 
        sess.user.mlisttype=1;
        return; 
    }

    if(!sess.user.mlisttype) sess.user.mlisttype=1;
}
