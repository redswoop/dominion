#include <stdio.h>
#include <string.h>

void strip_file(char *fn, FILE *out)
{
    FILE *f;
    char s[161],*ss;

    f=fopen(fn,"r");
    if (!f)
        return;

    fprintf(out,"\n\n/* File: %s */\n\n",fn);

    while (fgets(s,160,f)) {
        ss=strchr(s,'\n');
        if (ss)
            *ss=0;
        if ((s[0]) && (strchr("{}\t/# ",s[0])==NULL) && s[strlen(s)-1]==')' &&!strstr(s,"if")) {
            fprintf(out,"%s;\n",s);
        }
    }
    fclose(f);
}


/****************************************************************************/

void main(int argc, char *argv[])
{
    int i,i1;
    FILE *out, *tmpin;
    char *ss,s[161];

    if (argc!=3) {
        printf("Run the STRIP program only from the makefile.\n\n");
        exit(-1);
    }

    printf("Stripping %s",argv[2]);

    out=fopen(argv[1],"w");
    fprintf(out,"#ifndef _FCNS_H_\n#define _FCNS_H_\n\n");
    fprintf(out,"#include \"vardec.h\"\n\n");
    fprintf(out,"#include \"jammb.h\"\n\n");

    tmpin = fopen(argv[2],"r");
    do {
        i1=fscanf(tmpin,"%s",s);
        if (i1>0) {
            if ((ss=strstr(s,".obj"))!=NULL) {
                *ss=0;
                strcat(s,".c");
                ss=strrchr(s,'\\');
                if (!ss)
                    ss=s;
                else
                    ++ss;
            } 
            else ss=s;
            puts(ss);
            strip_file(ss,out);
        }
    } 
    while (i1>0);
    fclose(tmpin);

    fprintf(out,"\n#endif\n");
    fclose(out);

}
