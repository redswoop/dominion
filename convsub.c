#include "fido.h"
#include <fcntl.h>
#include <sys\stat.h>
#include "vardec.h"


#define anony_enable_anony 0x01
#define anony_enable_dear_abby 0x02
#define anony_force_anony 0x04
#define anony_real_name 0x08
#define anony_val_net 0x10
#define anony_ansi_only 0x20
#define anony_no_tag 0x40
#define anony_require_sv 0x80


// Subboardrec.anony
#define nanony_enable_anony 0x01
#define nanony_force_anony 0x04
#define nanony_real_name 0x08

// Subboardrec.mattr
#define mattr_netmail 0x0001
#define mattr_ansi_only 0x0002
#define mattr_autoscan 0x0004
#define mattr_fidonet 0x0008

// Message Base Record
typedef struct {
	char		name[41],		/* board name */
			filename[9],		/* board database filename */
                        conf;                   // Which Conference
        unsigned char   readacs[21],            // Acs required to read
                        postacs[21],            // Acs required to post
                        anony,                  // Anonymous Type
                        age;                    /* minimum age for sub */
        unsigned long   attr;
        unsigned short  maxmsgs,                /* max # of msgs */
                        ar,         /* AR for sub-board */
                        storage_type;           /* how messages are stored */
} noldsubboardrec;

oldsubboardrec ss[64];
subboardrec n[64];

void fix(int which)
{
    int i,i1;

    pr("3Converting 3%s - ",ss[which].name);

    strcpy(n[which].name,ss[which].name);
    strcpy(n[which].filename,ss[which].filename);
    strcpy(n[which].postacs,"S30");
    strcpy(n[which].readacs,"S30");
    n[which].conf=ss[which].key;
    n[which].ar=ss[which].ar;
    n[which].maxmsgs=ss[which].maxmsgs;
    n[which].storage_type=ss[which].storage_type;
    n[which].age=ss[which].age;
    n[which].anony=0;
    n[which].attr=0;

    if(ss[which].anony & anony_enable_anony) n[which].anony=anony_enable_anony;
    else if(ss[which].anony & anony_real_name) n[which].anony=anony_real_name;

    if(ss[which].anony & anony_require_sv) n[which].attr |= mattr_fidonet;
    if(ss[which].anony & anony_no_tag) n[which].attr |= mattr_autoscan;
    if(ss[which].anony & anony_ansi_only) n[which].attr |= mattr_ansi_only;

    npr("5Done. ");
}

void main(void)
{
    configrec syscfg;
    char s[81];
    int i,numsubs;

    i=open("config.dat",O_RDWR|O_BINARY);
    if(i<0) {
        npr("7Config.dat Not found!");
        exit(1);
    }
    read(i,&syscfg,sizeof(syscfg));
    close(i);
    sprintf(s,"%ssubs.dat",syscfg.datadir);
    i=open(s,O_RDWR|O_BINARY);

    numsubs=(read(i,ss, (64*sizeof(oldsubboardrec))))/
           sizeof(oldsubboardrec);;

    close(i);

    for(i=0;i<numsubs;i++)
        fix(i);

    i=open(s,O_BINARY|O_RDWR|O_TRUNC);
    write(i,&n[0],numsubs*sizeof(n[0]));
    close(i);
}
