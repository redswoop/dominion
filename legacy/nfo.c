#include "platform.h"
#include "nfo.h"

void userabort(void)
{
    nl();
    pl("Exiting");
    exit(0);
}

#pragma warn -sus

void togglebit(int *byte,int bit)
{
    if(*byte & bit) *byte ^=bit;
    else *byte |=bit;
}

nforec nfo;

void createnewnfo(char fn[81])
{
    int f;
    char s[81];

    f=open(fn,O_BINARY|O_RDWR|O_CREAT|O_TRUNC,S_IREAD|S_IWRITE);
    inputdat("File Name",nfo.fn,12);
    inputdat("Description",nfo.desc,71);
    inputdat("Group",nfo.group,71);
    inputdat("Total Files in Group",s,5);
    nfo.total=atoi(s);
    inputdat("Current Member",s,5);
    nfo.current=atoi(s);
    nfo.sound=nfo.video=nfo.input=0;
    write(f,&nfo,sizeof(nfo));
    close(f);
}

void displaysoundopt(void)
{
    char s[81];

    strcpy(s,"");

    if(nfo.sound & sound_std)
        strcat(s,"Std ");

    if(nfo.sound & sound_adlib)
        strcat(s,"Adlib ");
    if(nfo.sound & sound_sb)
        strcat(s,"SB ");
    if(nfo.sound & sound_sbpro)
        strcat(s,"SBPro ");
    if(nfo.sound & sound_gravis)
        strcat(s,"Gravis ");
    if(nfo.sound & sound_LAPC1)
        strcat(s,"LAPC1 ");
    pl(s);

}

void displayvideoopt(void)
{
    char s[81];

    strcpy(s,"");

    if(nfo.video & video_vga800)
        strcat(s,"Vga800 ");

    if(nfo.video & video_vga1024)
        strcat(s,"Vga1024 ");

    if(nfo.video & video_16c)
        strcat(s,"16c ");

    if(nfo.video & video_256c)
        strcat(s,"256c ");

    if(nfo.video & video_extc)
        strcat(s,"Hi-Color ");

    if(nfo.video & video_vga320)
        strcat(s,"Vga320 ");

    if(nfo.video & video_vga640)
        strcat(s,"Vga640 ");

    if(nfo.video & video_ega)
        strcat(s,"Ega ");

    if(nfo.video & video_cga)
        strcat(s,"Cga ");

    pl(s);
}

void displayinputopt(void)
{
    char s[81];

    strcpy(s,"");

    if(nfo.input & input_joystick)
        strcat(s,"Joystick ");

    if(nfo.input & input_thrust)
        strcat(s,"Thrustmaster ");

    if(nfo.input & input_mouse)
        strcat(s,"Mouse ");

    if(nfo.input & input_modem)
        strcat(s,"Modem ");

    pl(s);
}

void displayosopt(void)
{
    char s[81];

    strcpy(s,"");

    if(nfo.os & os_DOS)
        strcat(s,"DOS ");

    if(nfo.os & os_OS2)
        strcat(s,"OS2 ");

    if(nfo.os & os_WIN)
        strcat(s,"Windows ");

    if(nfo.os & os_NT)
        strcat(s,"NT ");

    if(nfo.os & os_NEXT)
        strcat(s,"NeXTSTEP ");

    if(nfo.os & os_GEO)
        strcat(s,"GeoWorks ");

    if(nfo.os & os_NOVELL)
        strcat(s,"Novell ");

    if(nfo.os & os_XWIN)
        strcat(s,"XWindows ");

    if(nfo.os & os_UNIX)
        strcat(s,"UNIX ");

    pl(s);
}



void bitset(char *msg,int byte,int bit)
{
    npr("%-30s: %-3s",msg,byte & bit?"Yes":"No");
}


void editsound(void)
{
    int done=0;

    do {
        clrscr();
        pr("0Current Sound Options: ");
        displaysoundopt();
        nl();
        bitset("1. Internal Speaker",nfo.sound,sound_std);
        bitset("2. Adlib",nfo.sound,sound_adlib);
        bitset("3. SB",nfo.sound,sound_sb);
        bitset("4. SBPro",nfo.sound,sound_sbpro);
        bitset("5. Gravis",nfo.sound,sound_gravis);
        bitset("6. LAPC-1",nfo.sound,sound_LAPC1);

        switch(toupper(getch())) {
            case '1': togglebit(&nfo.sound,sound_std); break;
            case '2': togglebit(&nfo.sound,sound_adlib); break;
            case '3': togglebit(&nfo.sound,sound_sb); break;
            case '4': togglebit(&nfo.sound,sound_sbpro); break;
            case '5': togglebit(&nfo.sound,sound_gravis); break;
            case '6': togglebit(&nfo.sound,sound_LAPC1); break;
            case 'Q': done=1; break;
        }
    } while(!done);
}

void editvideo(void)
{
    int done=0;

    do {
        clrscr();
        pr("0Current Video Options: ");
        displayvideoopt();
        nl();
        bitset("1. Vga800",nfo.video,video_vga800);
        bitset("2. Vga1024",nfo.video,video_vga1024);
        bitset("3. 16c",nfo.video,video_16c);
        bitset("4. 256c",nfo.video,video_256c);
        bitset("5. Hi-Color",nfo.video,video_extc);
        bitset("6. Vga320",nfo.video,video_vga320);
        bitset("7. Vga640",nfo.video,video_vga640);
        bitset("8. Ega",nfo.video,video_ega);
        bitset("9. Cga",nfo.video,video_cga);

        switch(toupper(getch())) {
            case '1': togglebit(&nfo.video,video_vga800); break;
            case '2': togglebit(&nfo.video,video_vga1024); break;
            case '3': togglebit(&nfo.video,video_16c); break;
            case '4': togglebit(&nfo.video,video_256c); break;
            case '5': togglebit(&nfo.video,video_extc); break;
            case '6': togglebit(&nfo.video,video_vga320); break;
            case '7': togglebit(&nfo.video,video_vga640); break;
            case '8': togglebit(&nfo.video,video_ega); break;
            case '9': togglebit(&nfo.video,video_cga); break;
            case 'Q': done=1; break;
        }
    } while(!done);
}


void editinput(void)
{
    int done=0;

    do {
        clrscr();
        pr("0Current Input Options: ");
        displayinputopt();
        nl();
        bitset("1. Joystick",nfo.input,input_joystick);
        bitset("2. Thrust",nfo.input,input_thrust);
        bitset("3. Mouse",nfo.input,input_mouse);
        bitset("4. Modem",nfo.input,input_modem);

        switch(toupper(getch())) {
            case '1': togglebit(&nfo.input,input_joystick); break;
            case '2': togglebit(&nfo.input,input_thrust); break;
            case '3': togglebit(&nfo.input,input_mouse); break;
            case '4': togglebit(&nfo.input,input_modem); break;
            case 'Q': done=1; break;
        }
    } while(!done);
}


void editos(void)
{
    int done=0;

    do {
        clrscr();
        pr("0Current Operating System Options: ");
        displayosopt();
        nl();
        bitset("1. DOS",nfo.os,os_DOS);
        bitset("2. OS2",nfo.os,os_OS2);
        bitset("3. Windows",nfo.os,os_WIN);
        bitset("4. NT",nfo.os,os_NT);
        bitset("5. NeXTSTEP",nfo.os,os_NEXT);
        bitset("6. GeoWorks",nfo.os,os_GEO);
        bitset("7. Novell",nfo.os,os_NOVELL);
        bitset("8. Xwindows",nfo.os,os_XWIN);
        bitset("9. UNIX",nfo.os,os_UNIX);

        switch(toupper(getch())) {
            case '1': togglebit(&nfo.os,os_DOS); break;
            case '2': togglebit(&nfo.os,os_OS2); break;
            case '3': togglebit(&nfo.os,os_WIN); break;
            case '4': togglebit(&nfo.os,os_NT); break;
            case '5': togglebit(&nfo.os,os_NEXT); break;
            case '6': togglebit(&nfo.os,os_GEO); break;
            case '7': togglebit(&nfo.os,os_NOVELL); break;
            case '8': togglebit(&nfo.os,os_XWIN); break;
            case '9': togglebit(&nfo.os,os_UNIX); break;
            case 'Q': done=1; break;
        }
    } while(!done);
}


void setfdate()
{
  char s[81];
  int m,dd,y;

  inputdat("New Release Date",s,9);
  m=atoi(s);
  dd=atoi(&(s[3]));
  y=atoi(&(s[6]))+1900;
  if ((strlen(s)==8) && (m>0) && (m<=12) && (dd>0) && (dd<32) && (y>=1980)) {
    nfo.rdate.da_year=y;
    nfo.rdate.da_day=dd;
    nfo.rdate.da_mon=m;
  }

  m=dd=y=0;

  inputdat("New Release Time",s,9);
  m=atoi(s);
  dd=atoi(&(s[3]));
  y=atoi(&(s[6]));
  if (strlen(s)==8 && m>0 && m<24 && dd>0 && dd<60 && y<60) {
    nfo.rtime.ti_min=dd;
    nfo.rtime.ti_hour=m;
    nfo.rtime.ti_hund=0;
    nfo.rtime.ti_sec=y;
  }
}

void main(int ac, char **ar)
{
    char s[81],fn[81],ch;
    int done=0,f;

    if(ac<2) {
        nl();
        inputdat("File to Edit/Create",fn,71);
        if(!fn[0])
            userabort();
    } else
        strcpy(fn,ar[1]);

    f=open(fn,O_BINARY|O_RDWR);
    if(f<0) {
        createnewnfo(fn);
    } else {
        read(f,&nfo,sizeof(nfo));
        close(f);
    }

    do {
        clrscr();
        npr("1. Release File Name: %s",nfo.fn);
        npr("2. Description      : %s",nfo.desc);
        npr("3. Group            : %s",nfo.group);
        npr("4. Number of Disks  : %d",nfo.total);
        npr("5. This file is     : %d",nfo.current);
        npr("6. File Size        : %ld",nfo.size);
        npr("7. Installed Size   : %ld",nfo.completesize);
        npr("8. Files in Release : %d",nfo.numf);
        npr("9. Release Date/Time: %02d/%02d/%02d (%02d:%02d:%02d)",
                                    nfo.rdate.da_mon,
                                    nfo.rdate.da_day,
                                    nfo.rdate.da_year-1900,
                                    nfo.rtime.ti_hour,
                                    nfo.rtime.ti_min,
                                    nfo.rtime.ti_sec
                                    );
         pr("A. Sound Options    : "); displaysoundopt();
         pr("B. Video Options    : "); displayvideoopt();
         pr("C. Input Options    : "); displayinputopt();
         pr("D. OS Options       : "); displayosopt();
        nl();
        nl();
        outstr("5Select (Q=Quit) ");
        ch=onek("123456789ABCDQ");
        nl();
        switch(ch) {
            case 'Q': done=1;
                     break;
            case '1': inputdat("File Name",nfo.fn,12);
                      break;
            case '2': inputdat("Description",nfo.desc,71);
                      break;
            case '3': inputdat("Group",nfo.group,71);
                      break;
            case '4': inputdat("Total Number of files in Group",s,5);
                      nfo.total=atoi(s);
                      break;
           case '5': inputdat("Current Member of Group",s,5);
                      nfo.current=atoi(s);
                      break;
           case '6': inputdat("File Size",s,20);
                      sscanf(s,"%ld",&nfo.size);
                      break;
           case '7': inputdat("Installed Size",s,20);
                     sscanf(s,"%ld",&nfo.completesize);
                     break;
           case '8': inputdat("Files in Release",s,20);
                     nfo.numf=atoi(s);
                     break;
           case '9': setfdate();
                     break;

           case 'A': editsound();
                     break;
           case 'B': editvideo();
                     break;
           case 'C': editinput();
                     break;
           case 'D': editos();
                     break;
        }
    } while(!done);


    f=open(fn,O_BINARY|O_RDWR);
    write(f,&nfo,sizeof(nfo));
    close(f);
}
