#pragma hdrstop

#include "bp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


char registered_name[201];
unsigned long supplied_key;

void checkreg(void)
{
    FILE *fp;
    unsigned long correct_key;
    char registered=0;

    if((fp=fopen("dom.key","r"))!=NULL)
    {
        fgets(registered_name,200,fp);
        if(registered_name[strlen(registered_name)-1]=='\n')
            registered_name[strlen(registered_name)-1]='\0';

        fscanf(fp,"%lu",&supplied_key);

        fclose(fp);

        correct_key=bp(registered_name,69851);

        if(correct_key==supplied_key)
        {
            registered=1;
        }
    }

    if(registered==1)
    {
        printf("This program is registered to: %s\n",registered_name);
    }
    else if(registered==0)
    {
        printf("This program is UNREGISTERED!!!\n");
        exit(0);
    }
}
