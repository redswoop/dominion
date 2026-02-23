#include "vars.h"
#pragma hdrstop

#include "menudb.h"

/* tg, pp, menuat, maxcmd now in vars.h (Phase B0) */


void menuinfoed(char fn[15])
{
    mciok=0;

    outchr(12);
    npr("3Menu File Name: %s\r\n",fn);
    nl();
    npr("31. Prompt       : 0%s\r\n",pp.prompt);
    npr("32. Titles       : 0%s\r\n",pp.title1);
    if(pp.title2[0]) npr("3                :0 %s\r\n",pp.title2);
    npr("33. Ansi Menu    : 0%s\r\n",pp.altmenu[0]?pp.altmenu:"\"\"");
    npr("34. Security     : 0%s\r\n",pp.slneed[0]?pp.slneed:"None");
    npr("35. Change Style : 0%d,%d,%d - %d Columns\r\n",pp.col[0],pp.col[1],pp.col[2],pp.columns);
    npr("36. Help Level   : 0%d\r\n",pp.helplevel-1);
    npr("37. Format File  : 0%s\r\n",pp.format);
    npr("38. MCI Name     : 0%s\r\n",pp.pausefile);
    npr("39. Extended Help: 0%s\r\n",pp.helpfile);
#ifdef PD
    npr("30. Pldn Prompt  : 0%s\r\n",pp.prompt2);
#endif
    npr("3A. HotKeys      : 0");
    if(pp.boarder==1)
        pl("Forced");
    else if(pp.boarder==2)
        pl("Off");
    else if(!pp.boarder)
        pl("Normal");
    npr("3B. Ext. Prompt  : 0%s\r\n",(pp.attr & menu_extprompt)?"Okay":"Internal Only");
    npr("3C. Pulldown Ok  : 0%s\r\n",(pp.attr & menu_pulldown)?"Yes":"No");
    npr("3F. Append Prmpt : 0%s\r\n",(pp.attr & menu_promptappend)?"Yes":"No");
    npr("3G. PopUp        : 0%s\r\n",(pp.attr & menu_popup)?"Yes":"No");
    npr("3H. Use Global   : 0%s\r\n",(pp.attr & menu_noglobal)?"No":"Yes");
    nl();
    mciok=1;
}


void edmenu (int x)
{
    int i,y,z,f;
    char ch,s[MAX_PATH_LEN],done=0,s1[MAX_PATH_LEN],fn[15];

    do {
        outchr(12);
        npr("Editing Command #%d\r\n",x); 
        nl();
        npr("1. Desc     : %s\r\n",tg[x].desc);
        npr("2. Key      : %s\r\n",tg[x].key);
        npr("3. Type     : %.2s\r\n",tg[x].type);
        npr("4. Attirbute: ");
        if(tg[x].attr==command_hidden) npr("Hidden ");
        else
            if(tg[x].attr==command_unhidden) npr("Unhidden ");
            else
                if(tg[x].attr==command_forced) npr("Forced ");
                else
                    if(tg[x].attr==command_title) npr("Sub-Section Title ");
                    else
                        if(tg[x].attr==command_pulldown) npr("Pull Down Seperator ");
                        else
                            if(tg[x].attr==command_default) npr("Default ");
                            else
                                npr("None");
        nl();
        npr("5. Security : %s\r\n",tg[x].sl);
        npr("6. Line     : %s\r\n",tg[x].line);
        npr("7. Parameter: %s\r\n",tg[x].ms);
        nl();
        outstr("5Command Editor (Q=Quit, V=View, J=Jump, [,]) ");
        ch=onek("1234567QV?[]J");
        nl();
        switch(ch)
        {
        case 'J': 
            inputdat("To Which Command",s,2,0);
            if(s[0]&&(atoi(s)<maxcmd)) x=atoi(s);
            break;
        case 'V': 
            showmenucol();
            pausescr(); 
            break;
        case ']': 
            if(x<maxcmd-1) x++; 
            break;
        case '[': 
            if(x>0) x--; 
            break;
        case 'Q': 
            done=1; 
            break;
        case '1': 
            npr("3Description\r\n5: ");
            inli(s,tg[x].desc,50,1);
            if(s[0]) strcpy(tg[x].desc,s); 
            break;
        case '3': 
            inputdat("Type",s,2,0);
            if(s[0]) {
                tg[x].type[0]=s[0];
                tg[x].type[1]=s[1];
            }
            break;
        case '2': 
            inputdat("Key",s,8,0);
            if(s[0]) {
                strcpy(tg[x].key,s);
            }
            break;
        case '5': 
thespot:
            inputdat("Security (?=Help)",s,21,0);
            if(s[0]=='?') {
                printmenu(27);
                goto thespot;
            }
            if(s[0]) strcpy(tg[x].sl,s); 
            break;
        case '6': 
            npr("3Command Line\r\n5: ");
            inli(s,tg[x].line,50,1);
            if(s[0]) strcpy(tg[x].line,s); 
            break;
        case '7': 
            npr("3Command Parameters\r\n5: ");
            inli(s,tg[x].ms,80,1);
            strcpy(tg[x].ms,s); 
            break;
        case '4': 
            npr("%s\r\n: ",get_string2(10));
            tg[x].attr=0;
            s[0]=onek("\rHUDFTP");
            if(s[0]=='\r'); 
            else switch(s[0]) {
            case 'H': 
                tg[x].attr=command_hidden; 
                break;
            case 'U': 
                tg[x].attr=command_unhidden; 
                break;
            case 'T': 
                tg[x].attr=command_title; 
                break;
            case 'P': 
                tg[x].attr=command_pulldown; 
                break;
            case 'F': 
                tg[x].attr=command_forced; 
                break;
            case 'D': 
                tg[x].attr=command_default; 
                break;
            }
            break;
        }
    } 
    while(!done&&!hangup);
}

void top(char fn[15])
{
    int x,n=0;
    char s[MAX_PATH_LEN],s1[161];

    outchr(12);
    npr("5Current Menu: 3%s\r\n",fn);
    nl();
    pl("2##. Type5�2Keys    5�2SL      ##. Type5�2Keys    5�2SL     �##. Type5�2Keys    5�2SL");
    pl("5������������������������������������������������������������������������������");

    for(x=0;x<maxcmd;x++) {
        npr("0%2d. %-2.2s  5�0%-8.8s5�0%-7.7s ",x,tg[x].type,tg[x].key,tg[x].sl);
        n++;
        if(n==3) { 
            nl(); 
            n=0; 
        }
    }
    nl();
    nl();
}

void menu(char fn[15])
{
    char ch,s[20],done=0;
    char names[100][20];
    int count, mi;

    if(!checkpw())
        return;

    count = menudb_list(names, 100);
    outchr(12);
    pl("5Menu Files Available to Edit");
    nl();
    for(mi=0; mi<count; mi++) npr("0%-20s",names[mi]);

    do {
        nl();
        nl();
        outstr(get_string2(1));
        ch=onek("QIMD\r");
        switch(ch) {
        case 13 : 
            count = menudb_list(names, 100);
            outchr(12);
            pl("5Menu Files Available to Edit");
            nl();
            for(mi=0; mi<count; mi++) npr("0%-20s",names[mi]);
            break;
        case 'Q': 
            done=1; 
            break;
        case 'M': 
            nl();
            inputdat("File to Edit",s,8,0);
            if(menudb_exists(s)) menued(s);
            else npr("\r\n4%s Not Found\r\n`P",s);
            break;
        case 'D': 
            nl();
            inputdat("File to Delete",s,8,0);
            if(!s[0]) break;
            if(menudb_delete(s) != 0)
                npr("\r\n4%s Not Found\r\n`P",s);
            break;
        case 'I': 
            nl();
            inputdat("File to Add",s,8,0);
            if(!s[0]) break;
            if(!menudb_exists(s)) addmenu(s);
            else npr("\r\n4%s All Ready Exists\r\n`P",s);
            break;
        }
    } 
    while(!done&&!hangup);
}

void addmenu(char fn[15])
{
    menu_data_t data;

    memset(&data, 0, sizeof(data));
    strcpy(data.header.prompt,"Select: ");
    strcpy(data.header.title1,"�Dominion Bulletin Board System");
    strcpy(data.header.title2,"�A New Menu");
    strcpy(data.header.altmenu,"");
    strcpy(data.header.pausefile,"");
    strcpy(data.header.slneed,"");
    strcpy(data.header.format,"");
    data.header.columns=4;
    data.header.battr=5;
    data.header.attr=0;
    data.header.attr |= menu_extprompt;
    data.header.attr |= menu_format;
    data.header.attr |= menu_pulldown;
    strcpy(data.commands[0].desc,"<NEW> Menu Command");
    strcpy(data.commands[0].type,"OL");
    strcpy(data.commands[0].line,"");
    data.commands[0].attr=0;
    strcpy(data.commands[0].ms,"");
    data.commands[1] = data.commands[0];
    data.count = 2;
    menudb_create(fn, &data);
    menued(fn);
}


void extractheader()
{
    FILE *f;
    char s[MAX_PATH_LEN],fn[MAX_PATH_LEN];

    inputdat("Filename to Write to",s,12,0);
    if(!s[0]) return;

    f=fopen(s,"wb");
    strcpy(fn,s);

    fputs(pp.prompt,f); 
    fputs("\n",f);
#ifdef PD
    fputs(pp.prompt2,f); 
    fputs("\n",f);
#endif
    fputs(pp.title1,f); 
    fputs("\n",f);
    fputs(pp.title2,f); 
    fputs("\n",f);
    fputs(pp.format,f); 
    fputs("\n",f);
    itoa(pp.columns,s,10);
    fputs(s,f); 
    fputs("\n",f);


    fclose(f);
    npr("Menu Header Extracted to %s\r\n",fn);
    pausescr();
}


void readheader()
{
    FILE *f;
    char s[161],fn[MAX_PATH_LEN];

    inputdat("Filename to Read",fn,12,0);
    if(!fn[0]) return;
    f=fopen(fn,"rt");

    fgets(s,161,f); 
    filter(s,'\n'); 
    strcpy(pp.prompt,s);
#ifdef PD
    fgets(s,161,f); 
    filter(s,'\n'); 
    strcpy(pp.prompt2,s);
#endif
    fgets(s,161,f); 
    filter(s,'\n'); 
    strcpy(pp.title1,s);
    fgets(s,161,f); 
    filter(s,'\n'); 
    strcpy(pp.title2,s);
    fgets(s,161,f); 
    filter(s,'\n'); 
    strcpy(pp.format,s);
    fgets(s,161,f); 
    filter(s,'\n'); 
    pp.columns=atoi(s);

    fclose(f);

    npr("Menu Header Read from %s\r\n",fn);
    pausescr();
}

void listmform(void)
{
    struct ffblk f;
    char s[MAX_PATH_LEN];

    sprintf(s,"%s*.fmt",syscfg.menudir);
    findfirst(s,&f,0);
    nl();
    pl("5Available Format Files");
    nl();
    do pl(f.ff_name); 
    while(findnext(&f)!=-1);
    nl();

}

void menued(char fn[15])
{
    int i,x=0,y,z,type=sys.status.net_version,d1=0;
    char ch,s[161],s1[161],done=0;
    menurec back;
    menu_data_t data;

    if(menudb_load(fn, &data) != 0) {
        npr("4Error loading menu %s\r\n", fn);
        return;
    }
    pp = data.header;
    memcpy(tg, data.commands, sizeof(menurec) * data.count);
    maxcmd = data.count;
    readmnufmt(pp);

    if(type) top(fn); 
    else menuinfoed(fn);

    do {
        outstr("5Menu Editor (?=Help):0 ");
        ch=onek("PQMDI?XV1234567890ERABCFGH\r");
        switch(ch) {
        case 'E': 
            extractheader(); 
            break;
        case 'R': 
            readheader(); 
            break;
        case '?': 
            printmenu(8); 
            break;
        case  13: 
            if(type) top(fn); 
            else menuinfoed(fn); 
            break;
        case '1': 
            pl("Prompt:");
#ifdef PD
            outstr(": "); 
            inli(s,pp.prompt,99,1);
#else
            outstr(": "); 
            inli(s,pp.prompt,192,1);
#endif
            if(s[0]) strcpy(pp.prompt,s);
            break;
#ifdef PD
        case '0': 
            pl("PullDown Prompt:");
            outstr(": "); 
            inli(s,pp.prompt2,99,1);
            if(s[0]) strcpy(pp.prompt2,s);
            break;
#endif
        case '2': 
            pl("Title 1:");
            outstr(": "); 
            inli(s,pp.title1,51,1);
            if(s[0]) strcpy(pp.title1,s);
            pl("Title 2:");
            outstr(": "); 
            inli(s,pp.title2,51,1);
            if(s[0]) strcpy(pp.title2,s);  
            break;
        case '3': 
            inputdat("External Menu",s,8,0);
            if(s[0]) strcpy(pp.altmenu,s);
            break;
        case '4': 
            inputdat("Security",pp.slneed,sizeof(pp.slneed),1);
            break;
        case '6': 
            nl();
            printmenu(25);
            inputdat("Forced Help Level for Menu, -1 Disables",s,2,0);
            if(s[0]) pp.helplevel=atoi(s)+1;
            break;
        case '7': 
            listmform(); 
            inputdat("Format File",pp.format,8,0); 
            break;
        case '8': 
            inputdat("Menu's MCI Name",pp.pausefile,9,1); 
            break;
        case '9': 
            inputdat("External Help File .HLP",pp.helpfile,8,0); 
            break;
        case 'A': 
            nl();
            npr("3Hotkeys: 0F5orced, 0O5ff, 0N5ormal: ");
            ch=onek("\rNOF");
            if(ch=='N'||ch=='\r') pp.boarder=0;
            else if(ch=='F') pp.boarder=1;
            else if(ch=='O') pp.boarder=2;
            break;
        case 'B': 
            togglebit((long *)&pp.attr,menu_extprompt); 
            break;
        case 'C': 
            togglebit((long *)&pp.attr,menu_pulldown); 
            break;
        case 'F': 
            togglebit((long *)&pp.attr,menu_promptappend); 
            break;
        case 'G': 
            togglebit((long *)&pp.attr,menu_popup); 
            break;
        case 'H': 
            togglebit((long *)&pp.attr,menu_noglobal); 
            break;
        case '5': 
            d1=0;
            do {
                outchr(12);
                npr("0Menu Style Editor\r\n"); 
                nl();
                npr("1. Brackets    : %d\r\n",pp.col[0]);
                npr("2. Commands    : %d\r\n",pp.col[1]);
                npr("3. Description : %d\r\n",pp.col[2]);
                npr("4. Columns     : %d\r\n",pp.columns);
                nl();
                outstr("Select: ");
                ch=onek("1234VQ\r");
                switch(ch) {
                case '4':  
                    inputdat("Enter Number of Columns (1,2,3,4,5)",s,2,0);
                    if(s[0]) pp.columns=atoi(s);
                    break;
                case '1':  
                    inputdat("Bracket Attr",s,2,0);
                    if(s[0]) pp.col[0]=atoi(s);
                    break;
                case '2':  
                    inputdat("Command Key Attr",s,2,0);
                    if(s[0]) pp.col[1]=atoi(s);
                    break;
                case '3':  
                    inputdat("Description Attr",s,2,0);
                    if(s[0]) pp.col[2]=atoi(s);
                    break;
                case 'V':  
                    showmenucol();
                    pausescr(); 
                    break;
                case '\r':
                case 'Q': 
                    d1=1; 
                    break;
                }
            } 
            while(!d1);
            break;
        case 'Q': 
            done=1; 
            break;
        case 'X': 
            type=opp(type);
            if(type) top(fn); 
            else menuinfoed(fn);
            break;
        case 'M': 
            nl();
            inputdat("Modify starting at which Command",s,2,0);
            x=atoi(s);
            edmenu(x); 
            break;
        case 'I': 
            if(maxcmd<50)
                nl();
            inputdat("Insert before which command",s,2,0);
            if(!s[0]) break;
            y=atoi(s);
            inputdat("Insert How Many? ",s,2,0);
            if(!s[0])
                i=1;
            else
                i=atoi(s);
            for(x=0;x<i;x++) {
                maxcmd++;
                for(z=maxcmd-1;z>=y; z--)
                    tg[z+1]=tg[z];
                strcpy(tg[y].desc,"<NEW> Command");
                strcpy(tg[y].key,"NEW");
                strcpy(tg[y].line,"");
                strcpy(tg[y].sl,"");
                strcpy(tg[y].type,"OL");
                strcpy(tg[y].ms,"");
                tg[y].attr=0;
            }
            break;
        case 'D': 
            inputdat("Delete which command",s,2,0);
            if(!s[0]) break;
            y=atoi(s);
            for(x=y;x<maxcmd;x++)
                tg[x]=tg[x+1];
            maxcmd--;
            break;
        case 'P': 
            inputdat("Posistion which command",s,2,0);
            if(!s[0]) break;
            y=atoi(s);
            back=tg[y];
            for(x=y;x<maxcmd;x++)
                tg[x]=tg[x+1];
            inputdat("Posistion where",s,2,0);
            if(!s[0]) break;
            y=atoi(s);
            for(z=maxcmd-1;z>=y; z--)
                tg[z+1]=tg[z];
            tg[y]=back;
            break;
        case 'V': 
            showmenucol();
            pausescr(); 
            break;
        }
    } 
    while(!done&&!hangup);
    data.header = pp;
    memcpy(data.commands, tg, sizeof(menurec) * maxcmd);
    data.count = maxcmd;
    menudb_save(fn, &data);

    if(!wfc)
        read_menu(menuat,0);

    sys.status.net_version=type;
    save_status();
}
