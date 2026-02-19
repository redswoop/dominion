#include "vars.h"

#pragma hdrstop
extern int colblock;

void change_colors(userrec *u1)
{
    int i,done,i1,i2;
    char s[81],ch,nc,*ss,s1[81];
    userrec u;

    u=*u1;
    done=0;
    do {
        outchr(12);
        printfile("Collist");
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
                if(colblock)
                    nifty.defaultcol[i]=nc;
                else {
                    u.colors[i]=nc;
                }
            } 
            else {
                pl("Aborted.");
            }
        }
    } 
    while ((!done) && (!hangup));

    *u1=u;
}



void print_cur_stat()
{
    char s[81],s1[81],s2[81];
    userrec ur;

    outchr(12);
    dtitle("Preferences");

    npr("0Screen size       : %d X %d\r\n",
    thisuser.screenchars,
    thisuser.screenlines);

    if(thisuser.sysstatus & sysstatus_avatar) strcpy(s,"Avatar ");
    else
        if(thisuser.sysstatus & sysstatus_ansi) strcpy(s,"Ansi ");
    else strcpy(s,"Ascii");
    if(thisuser.sysstatus & sysstatus_color) strcat(s,"Colour");
    else strcat(s,"Mono");

    npr("Graphics          : %s\r\n",s);
    npr("Help Level        : ");
    switch(thisuser.helplevel) {
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
    npr("Comment           : %s\r\n",thisuser.comment);
    npr("Pause on screen   : %s\r\n",
    (thisuser.sysstatus & sysstatus_pause_on_page)?"On":"Off");
    outstr("Mailbox           : ");
    if ((thisuser.forwardusr==0))
        pl("Normal");
    else {
        {
            if (thisuser.forwardusr==255) {
                pl("Closed");
            } 
            else {
                read_user(thisuser.forwardusr,&ur);
                if (ur.inact & inact_deleted) {
                    thisuser.forwardusr=0;
                    pl("Normal");
                } 
                else {
                    sprintf(s,"Forward to %s",nam(&ur,thisuser.forwardusr));
                    pl(s);
                }
            }
        }
    }
    npr("Default Protocol  : %s\r\n",proto[thisuser.defprot].description);
    npr("File List Format  : Type %d\r\n",thisuser.flisttype);
    npr("Hotkeys           : %s\r\n",(thisuser.sysstatus & sysstatus_fullline)?"No":"Yes");

    nl();
    if(thisuser.helplevel==2) pausescr();
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
    static char s[81];
    char s1[81];


    if (thisuser.sysstatus & sysstatus_color) {
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
            for (i=0; dl?udir[i].subnum!=-1:usub[i].subnum!=-1; i++)
                if(!dl)
                    togglenws(i,&thisuser,on);
                else
                    thisuser.nscn[i]=on;
        } else if (atoi(s))
            for (i=0; dl?i<MAX_DIRS:i<MAX_SUBS; i++) {
                if(!dl) {
                    if (strcmp(usub[i].keys,s)==0) {
                        on=inscan(i,&thisuser);
                        togglenws(i,&thisuser,!on);
                    }
                } else {
                    if (strcmp(udir[i].keys,s)==0) {
                        if(thisuser.nscn[udir[i].subnum]<0)
                            thisuser.nscn[udir[i].subnum]=0;
                        else
                            thisuser.nscn[udir[i].subnum]=-1;
                    }
                }
            }
    } 
    while ((!done) && (!hangup));
}


void list_macro(unsigned char *s)
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
    unsigned char tempmac[81],s[81];
    unsigned char ch,ch1;
    int i,i1,done,done1;

    done=0;
    do {
        outchr(12);
        pl("Ctrl-A macro: ");
        list_macro(&(thisuser.macros[2][0]));
        nl();
        pl("Ctrl-D macro: ");
        list_macro(&(thisuser.macros[0][0]));
        nl();
        pl("Ctrl-F macro: ");
        list_macro(&(thisuser.macros[1][0]));
        nl();
        pl("Ctrl-Y macro: ");
        list_macro(&(thisuser.macros[3][0]));
        nl();
        outstr("5Macro Editor 0(3A,D,F,Y,Q=Quit0)5 : ");
        ch=onek("QADFY");
        if(ch=='Q') done=1;
        else {
            if(ch=='Y') i1=3;
            else if(ch=='A') i1=2;
            else if(ch=='D') i1=0;
            else if(ch=='F') i1=1;
            strcpy(s,&(thisuser.macros[i1][0]));
            thisuser.macros[i1][0]=0;
            done1=0;
            i=0;
            nl();
            pl("Enter your macro now, hit ctrl-Z when done.");
            nl();
            okskey=0;
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
            while ((!done1) && (!hangup));
            okskey=1;
            tempmac[i]=0;
            nl();
            pl("You entered:");
            nl();
            nl();
            list_macro(tempmac);
            nl();
            prt(5,"Is this OK? ");
            if (yn()) {
                strcpy(&(thisuser.macros[i1][0]),tempmac);
                nl();
                pl("Macro saved.");
            } 
            else {
                nl();
                strcpy(&(thisuser.macros[i1][0]),s);
                pl("Nothing saved.");
            }
        }
    } 
    while ((!done) && (!hangup));
}


void input_pw1()
{
    char s[81],s1[81];
    int ok;

    nl();
    prt(5,"Change password? ");
    if (yn()) {
        nl();
        pl("You must now enter your current password.");
        outstr(": ");
        echo=0;
        input(s,19);
        if (strcmp(s,thisuser.pw)) {
            nl();
            pl("Incorrect.");
            nl();
            return;
        }
        nl();
        nl();
        pl("Enter your new password, 3 to 20 characters long.");
        outstr(": ");
        echo=0;
        input(s,19);
        nl();
        nl();
        pl("Repeat password for verification.");
        outstr(": ");
        echo=0;
        input(s1,19);
        if (strcmp(s,s1)==0) {
            if (strlen(s1)<3) {
                nl();
                pl("Password must be 3-20 characters long.");
                pl("Password was not changed.");
                nl();
            } 
            else {
                strcpy(thisuser.pw,s);
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
    char s[81];

    nl();

    prt(5,"Do you want to close your mailbox? ");
    if (yn()) {
        prt(5,"Are you sure? ");
        if (yn()) {
            thisuser.forwardusr=-1;
            return;
        }
    }
    prt(5,"Do you want to forward your mail? ");
    if (!yn()) {
        thisuser.forwardusr=0;
        return;
    }
    nl();

    nl();
    prt(2,"Forward to which user? ");
    input(s,40);
    i=finduser1(s);
    if (i==usernum) {
        thisuser.forwardusr=0;
        nl();
        pl("Forwarding reset.");
        nl();
        return;
    }
    if (i>0) {
        thisuser.forwardusr=i;
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
        sprintf(s,"%sFile%d.fmt",syscfg.gfilesdir,c);
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
    while(!done&&!hangup);

    nl();
    outstr(get_string(79));
    input(s,3);
    if(s[0]&&atoi(s)<c&&!strchr(s,'Q')&&!strchr(s,'A')) thisuser.flisttype=atoi(s);
    else if(!s[0]) { 
        thisuser.flisttype=1; 
        return; 
    }
    else if(strchr(s,'A')) {
        outchr(12);
        printmenu(32);
        strcpy(s1,"");
        npr("3Enter Your File Format\r\n5:0 ");
        inli(s,s1,81,1);
        if(s[0]) {
            stuff_in1(s1,s,"Dom30a  .Zip","Dominion BBS 3.0"," 300","   0","  1",nam(&thisuser,usernum),date(),"100","00:14:26","");
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
                sprintf(s1,"%sfile%d.fmt",syscfg.gfilesdir,c);
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
                sprintf(s,"Added by: %s on %s\n",nam(&thisuser,usernum),date());
                fputs(s,f);
                fclose(f);
                thisuser.flisttype=c;
                logtypes(2,"Added File Format: 4%d",c);
            }
        }
    }
    if(!thisuser.flisttype) thisuser.flisttype=1;
}

void setcolors(userrec *uu)
{
    userrec u;
    int i;

    u=*uu;
    if(u.sysstatus &  sysstatus_color) {
        for(i=0;i<20;i++)
            u.colors[i]=nifty.defaultcol[i];
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

    if(thisuser.sysstatus & sysstatus_ansi)
        togglebit((long *)&thisuser.sysstatus,sysstatus_ansi);

    if(thisuser.sysstatus & sysstatus_color)
        togglebit((long *)&thisuser.sysstatus,sysstatus_color);

    if(thisuser.sysstatus & sysstatus_avatar)
        togglebit((long *)&thisuser.sysstatus,sysstatus_avatar);

    if(thisuser.sysstatus & sysstatus_rip)
        togglebit((long *)&thisuser.sysstatus,sysstatus_rip);

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
        thisuser.sysstatus |= sysstatus_ansi;
        outstr("Do you want color? ");
        if (ny())
            togglebit((long *)&thisuser.sysstatus,sysstatus_color);
        ansic(0);
        nl();
        outstr("Do you want Avatar? (No if unsure): ");
        if(yn())
            togglebit((long *)&thisuser.sysstatus,sysstatus_avatar);
        nl();
        outstr("Do you want RIPscript support? ");
        if(yn())
            togglebit((long *)&thisuser.sysstatus,sysstatus_rip);
    }
    setcolors(&thisuser);
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
        thisuser.helplevel=1; 
        break;
    case 'E': 
        thisuser.helplevel=0; 
        break;
    case 'I': 
        thisuser.helplevel=2; 
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
        sprintf(s,"%sMsg%d.fmt",syscfg.gfilesdir,c);
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
    while(!done&&!hangup);

    nl();
    outstr(get_string(79));
    input(s,3);
    if(s[0]&&atoi(s)<c&&!strchr(s,'Q')) thisuser.mlisttype=atoi(s);
    else if(!s[0]) { 
        thisuser.mlisttype=1;
        return; 
    }

    if(!thisuser.mlisttype) thisuser.mlisttype=1;
}
