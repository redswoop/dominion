#include <stdio.h>
#include <fcntl.h>
#include "cindent.h"


int slevel[10],clevel,spflg[20][10],sind[20][10],siflev[10],sifflg[10],iflev;
int ifflg,level,ind[10] = {0,0,0,0,0,0,0,0,0,0},eflg,paren;
int pflg[10] = {0,0,0,0,0,0,0,0,0,0};
int aflg,ct,stabs[20][10],qflg,sflg,bflg,peak,tabs,chr,lastchar,j,loop;

char lchar,pchar;
char *wif[2],*welse[2], *wfor[2],*wds[3];
char string[200],cc,c;


FILE *infile,*outfile;

void main(int argc,char *argv[])
{
    int k;

    if(argc<2)
        return;

    if ((infile = fopen(argv[1],"r")) == 0){
        printf("File not found\n\n");
        return;
    }

    outfile=fopen("Cindtemp.!!!","w");

    clevel = iflev = level = eflg = paren = 0;
    aflg = qflg = j = bflg = tabs = 0;
    ifflg = peak = -1;
    sflg = 1;      
    wif[0] = "if";
    welse[0] = "else";
    wfor[0] = "for";
    wds[0] = "case";
    wds[1] = "default";
    wif[1] = welse[1] = wfor[1] = wds[2] = 0;

    /*  End of initialization.  */

    while((chr = getchr()) != EOF){
        c = (char) chr;
        switch(c){
        default:
            string[j++] = c;
            if(c != ',')lchar = c;
            break;
        case ' ':
            if(lookup(welse) == 1){
                gotelse();
                if(sflg == 0 || j > 0) string[j++] = c;
                putstr();
                sflg = 0;
                break;
            }
            if(sflg == 0 || j > 0)  string[j++] = c;
            break;

        case '\t':
            if(lookup(welse) == 1){
                gotelse();
                if(sflg == 0 || j > 0)
                    for (loop=0;loop <= 4;loop++) string[j++] = ' ';
                putstr();
                sflg = 0;
                break;
            }
            if(sflg == 0 || j > 0)
                for (loop=0;loop <= 4;loop++) string[j++] = ' ';
            break;
        case '\n':
            if(eflg = lookup(welse) == 1)
                gotelse();
            putstr();
            printit("\n",0);
            sflg = 1;
            if(eflg == 1){
                pflg[level]++;
                tabs++;
            }
            else
                if(pchar == lchar)
                    aflg = 1;
            break;
        case '{':
            if(lookup(welse) == 1)gotelse();
            siflev[clevel] = iflev;
            sifflg[clevel] = ifflg;
            iflev = ifflg = 0;
            clevel++;
            if(sflg == 1 && pflg[level] != 0){
                pflg[level]--;
                tabs--;
            }
            string[j++] = c;
            putstr();
            getnl();
            putstr();
            printit("\n",0);
            tabs++;
            sflg = 1;
            if(pflg[level] > 0){
                ind[level] = 1;
                level++;
                slevel[level] = clevel;
            }
            break;
        case '}':
            clevel--;
            if((iflev = siflev[clevel]-1) < 0)iflev = 0;
            ifflg = sifflg[clevel];
            putstr();
            tabs--;
            ptabs();
            if((peak = getchr()) == ';'){
                printit("%c;",c);
                peak = -1;
            }
            else printit("%c",c);
            getnl();
            putstr();
            printit("\n",0);
            sflg = 1;
            if(clevel < slevel[level])if(level > 0)level--;
            if(ind[level] != 0){
                tabs -= pflg[level];
                pflg[level] = 0;
                ind[level] = 0;
            }
            break;
        case '"':
        case '\'':
            string[j++] = c;
            while((cc = getchr()) != c){
                string[j++] = cc;
                if(cc == '\\'){
                    string[j++] = getchr();
                }
                if(cc == '\n'){
                    putstr();
                    sflg = 1;
                }
            }
            string[j++] = cc;
            if(getnl() == 1){
                lchar = cc;
                peak = '\n';
            }
            break;
        case ';':
            string[j++] = c;
            putstr();
            if(pflg[level] > 0 && ind[level] == 0){
                tabs -= pflg[level];
                pflg[level] = 0;
            }
            getnl();
            putstr();
            printit("\n",0);
            sflg = 1;
            if(iflev > 0)
                if(ifflg == 1){
                    iflev--;
                    ifflg = 0;
                }
                else iflev = 0;
            break;
        case '\\':
            string[j++] = c;
            string[j++] = getchr();
            break;
        case '?':
            qflg = 1;
            string[j++] = c;
            break;
        case ':':
            string[j++] = c;
            if(qflg == 1){
                qflg = 0;
                break;
            }
            if(lookup(wds) == 0){
                sflg = 0;
                putstr();
            }
            else{
                tabs--;
                putstr();
                tabs++;
            }

            peak = getchr();
            if(peak == ';') {
                printit(";",0);
                peak = -1;
            }

            getnl();
            putstr();
            printit("\n",0);
            sflg = 1;
            break;
        case '/':
            string[j++] = c;
            if((peak = getchr()) != '*')break;
            string[j++] = peak;
            peak = -1;
            comment();
            break;
        case ')':
            paren--;
            string[j++] = c;
            putstr();
            if(getnl() == 1){
                peak = '\n';
                if(paren != 0)aflg = 1;
                else if(tabs > 0){
                    pflg[level]++;
                    tabs++;
                    ind[level] = 0;
                }
            }
            break;
        case '#':
            string[j++] = c;
            while((cc = getchr()) != '\n'){
                if (cc != '\t') string[j++] = cc;
                else for (loop=0;
                loop <= 4;
                loop++) string[j++] = ' ';
            }
            string[j++] = cc;
            sflg = 0;
            putstr();
            sflg = 1;
            break;
        case '(':
            string[j++] = c;
            paren++;
            if(lookup(wfor) == 1){
                while((c = getstr()) != ';');
                ct=0;
cont:
                while((c = getstr()) != ')'){
                    if(c == '(') ct++;
                }
                if(ct != 0){
                    ct--;
                    goto cont;
                }
                paren--;
                putstr();
                if(getnl() == 1){
                    peak = '\n';
                    pflg[level]++;
                    tabs++;
                    ind[level] = 0;
                }
                break;
            }
            if(lookup(wif) == 1){
                putstr();
                stabs[clevel][iflev] = tabs;
                spflg[clevel][iflev] = pflg[level];
                sind[clevel][iflev] = ind[level];
                iflev++;
                ifflg = 1;
            }
        }
    }        
    fclose(infile);
    fclose(outfile);

    unlink(argv[1]);
    rename("CINDTEMP.!!!",argv[1]);

    return;
}

void ptabs(void)
{
    int i;
    for(i=0; i < tabs; i++)
        printit("    ",0);
}

char getchr(void)
{
    if(peak < 0 && lastchar != ' ' && lastchar != '\t')pchar = lastchar;
    if (peak < 0)
        lastchar = getc(infile);        
    else
        lastchar = peak;
    peak = -1;
    return(lastchar == '\r' ? getchr():lastchar);
}


void putstr(void)
{
    if(j > 0){
        if(sflg != 0){
            ptabs();
            sflg = 0;
            if(aflg == 1){
                aflg = 0;
                if(tabs > 0)
                    printit("    ",0);
            }
        }
        string[j] = '\0';
        fprintf(outfile,"%s",string);
        j = 0;
    }
    else{
        if(sflg != 0){
            sflg = 0;
            aflg = 0;
        }
    }
}

int lookup(char *tab[])
{
    char r;
    int l,kk,k,i;
    if(j < 1)return(0);
    kk=0;
    while(string[kk] == ' ')kk++;
    for(i=0; tab[i] != 0; i++){
        l=0;
        for(k=kk;(r = tab[i][l++]) == string[k] && r != '\0';k++);
        if(r == '\0' && (string[k] < 'a' || string[k] > 'z'))return(1);
    }
    return(0);
}

char getstr(void)
{
    char ch;
beg:
    if((ch = string[j++] = getchr()) == '\\'){
        string[j++] = getchr();
        goto beg;
    }
    if(ch == '\'' || ch == '"'){
        while((cc = string[j++] = getchr()) != ch)if(cc == '\\')string[j++] = getchr();
        goto beg;
    }
    if(ch == '\n'){
        putstr();
        aflg = 1;
        goto beg;
    }
    else return(ch);
}

void gotelse(void)
{
    tabs = stabs[clevel][iflev];
    pflg[level] = spflg[clevel][iflev];
    ind[level] = sind[clevel][iflev];
    ifflg = 1;
}

int getnl(void)
{
    while((peak = getchr()) == '\t' || peak == ' '){
        if ( peak == '\t')
            for (loop=0;loop <= 4;loop++) string[j++] = ' ';
        if (peak == ' ') string[j++] = peak;
        peak = -1;
    }
    if((peak = getchr()) == '/'){
        peak = -1;
        if((peak = getchr()) == '*'){
            string[j++] = '/';
            string[j++] = '*';
            peak = -1;
            comment();
        }
        else string[j++] = '/';
    }
    if((peak = getchr()) == '\n'){
        peak = -1;
        return(1);
    }
    return(0);
}

void comment(void)
{
rep:
    while((c = getchr()) != '*'){
        if (c != '\t') string[j++] = c;
        else  for (loop=0;loop <= 4;loop++) string[j++] = ' ';
        if(c == '\n'){
            putstr();
            sflg = 1;
        } 
    }
    string[j++] = c;
gotstar:
    if((c = string[j++] = getchr()) != '/'){
        if(c == '*')goto gotstar;
        goto rep;
    }
}


void printit(char *format,unsigned int arg)
{
    if (fprintf(outfile,format,arg) == -1){
        perror("Cindent");
        printf("Disk write error\n");
        exit();
    }
}
