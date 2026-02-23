/*
 * mkconfig.c - Generate config.json, status.json, user JSON files,
 * and all required binary data files for Dominion BBS.
 *
 * Usage: ./mkconfig [basedir]
 *   basedir defaults to current directory
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>

#include "vardec.h"
#include "json_io.h"

static void mkd(const char *path)
{
    struct stat st;
    if (stat(path, &st) != 0)
        mkdir(path, 0755);
}

int main(int argc, char **argv)
{
    configrec syscfg;
    niftyrec nifty;
    statusrec status;
    subboardrec sub;
    directoryrec dir;
    protocolrec proto;
    resultrec result;
    confrec conf;
    xarcrec xarc;
    userrec sysop_user;
    userrec empty_user;
    char base[256];
    char path[512];
    FILE *f;
    int i;
    cJSON *root;

    if (argc > 1)
        snprintf(base, sizeof(base), "%s/", argv[1]);
    else
        strcpy(base, "./");

    printf("Struct sizes:\n");
    printf("  configrec    = %4zu\n", sizeof(configrec));
    printf("  niftyrec     = %4zu\n", sizeof(niftyrec));
    printf("  statusrec    = %4zu\n", sizeof(statusrec));
    printf("  subboardrec  = %4zu\n", sizeof(subboardrec));
    printf("  directoryrec = %4zu\n", sizeof(directoryrec));
    printf("  protocolrec  = %4zu\n", sizeof(protocolrec));
    printf("  resultrec    = %4zu\n", sizeof(resultrec));
    printf("  confrec      = %4zu\n", sizeof(confrec));
    printf("  xarcrec      = %4zu\n", sizeof(xarcrec));
    printf("  userrec      = %4zu\n", sizeof(userrec));
    printf("\n");

    /* --- Create directories --- */
    {
        char *dirs[] = {"data", "data/users", "afiles", "temp", "msgs",
                        "batch", "menus", "dls", "scripts", "data/dir", NULL};
        for (i = 0; dirs[i]; i++) {
            snprintf(path, sizeof(path), "%s%s", base, dirs[i]);
            mkd(path);
            printf("  dir: %s\n", path);
        }
    }

    /* =================================================================
     * config.json  (configrec + niftyrec)
     * ================================================================= */
    memset(&syscfg, 0, sizeof(syscfg));

    strcpy(syscfg.systemname, "Dominion BBS");
    strcpy(syscfg.sysopname, "SysOp");
    strcpy(syscfg.systempw, "SYSOP");
    strcpy(syscfg.systemphone, "000-000-0000");

    snprintf(syscfg.datadir,    sizeof(syscfg.datadir),    "%sdata/", base);
    snprintf(syscfg.gfilesdir,  sizeof(syscfg.gfilesdir),  "%safiles/", base);
    snprintf(syscfg.tempdir,    sizeof(syscfg.tempdir),    "%stemp/", base);
    snprintf(syscfg.msgsdir,    sizeof(syscfg.msgsdir),    "%smsgs/", base);
    snprintf(syscfg.batchdir,   sizeof(syscfg.batchdir),   "%sbatch/", base);
    snprintf(syscfg.menudir,    sizeof(syscfg.menudir),    "%smenus/", base);
    snprintf(syscfg.dloadsdir,  sizeof(syscfg.dloadsdir),  "%sdls/", base);

    strcpy(syscfg.bbs_init_modem, "ATZ");
    strcpy(syscfg.answer, "ATA");
    strcpy(syscfg.no_carrier, "NO CARRIER");
    strcpy(syscfg.ring, "RING");
    strcpy(syscfg.hangupphone, "ATH0");
    strcpy(syscfg.pickupphone, "ATH1");
    strcpy(syscfg.modem_type, "NONE");

    syscfg.primaryport = 0;
    syscfg.maxusers = 500;
    syscfg.maxwaiting = 5;
    syscfg.newusersl = 100;
    syscfg.newuserdsl = 100;
    syscfg.newusergold = 100.0;

    /* autoval[0] is the new user validation level (nifty.nulevel=0).
       set_autoval() in newuser.c applies this to set the user's SL/DSL. */
    syscfg.autoval[0].sl = 100;
    syscfg.autoval[0].dsl = 100;
    syscfg.autoval[0].ar = 0;
    syscfg.autoval[0].dar = 0;
    syscfg.autoval[0].restrict = 0;
    syscfg.req_ratio = 0.0;
    syscfg.post_call_ratio = 0.0;
    syscfg.sysoplowtime = 0;
    syscfg.sysophightime = 0;
    syscfg.closedsystem = 0;
    syscfg.userreclen = sizeof(userrec);
    syscfg.sysconfig = 0;

    for (i = 0; i < 256; i++) {
        syscfg.sl[i].time_per_day = (i > 0) ? (i * 2) : 0;
        syscfg.sl[i].time_per_logon = (i > 0) ? (i * 2) : 0;
        syscfg.sl[i].emails = (i >= 10) ? 20 : 0;
        syscfg.sl[i].posts = (i >= 10) ? 20 : 0;
        syscfg.sl[i].ability = (i >= 200) ? 0xFFFFFFFFL : 0;
    }

    memset(&nifty, 0, sizeof(nifty));
    nifty.matrixtype = 0;
    nifty.systemtype = 0;
    strcpy(nifty.firstmenu, "main");
    /* defaultcol is a byte array of DOS color attributes, NOT a string */
    nifty.defaultcol[0]  = 7;   /* default text: white on black */
    nifty.defaultcol[1]  = 11;  /* highlight: bright cyan on black */
    nifty.defaultcol[2]  = 14;  /* input: yellow on black */
    nifty.defaultcol[3]  = 5;   /* YN prompt: magenta on black */
    nifty.defaultcol[4]  = 31;  /* prompt: bright white on blue */
    nifty.defaultcol[5]  = 2;   /* info: green on black */
    nifty.defaultcol[6]  = 12;  /* warning: bright red on black */
    nifty.defaultcol[7]  = 9;   /* title: bright blue on black */
    nifty.defaultcol[8]  = 6;   /* misc: brown on black */
    nifty.defaultcol[9]  = 3;   /* misc2: cyan on black */
    nifty.defaultcol[10] = 112; /* inverse: black on white */
    nifty.defaultcol[11] = 7;   /* extra1 */
    nifty.defaultcol[12] = 15;  /* extra2 */
    nifty.defaultcol[13] = 7;   /* extra3 */
    nifty.defaultcol[14] = 15;  /* extra4 */
    nifty.defaultcol[15] = 7;   /* extra5 */
    nifty.defaultcol[16] = (char)0x8F; /* extra6: blinking bright white */
    nifty.defaultcol[17] = 112; /* extra7: inverse */
    nifty.defaultcol[18] = (char)0x8F; /* extra8 */
    nifty.defaultcol[19] = 7;   /* extra9 */

    snprintf(path, sizeof(path), "%sconfig.json", base);
    root = configrec_to_json(&syscfg, &nifty);
    if (write_json_file(path, root) != 0) { perror(path); return 1; }
    cJSON_Delete(root);
    printf("Wrote %s\n", path);

    /* =================================================================
     * data/status.json  (statusrec)
     * ================================================================= */
    memset(&status, 0, sizeof(status));
    {
        time_t t = time(NULL);
        struct tm *tm = localtime(&t);
        snprintf(status.date1, sizeof(status.date1), "%02d/%02d/%02d",
                 tm->tm_mon + 1, tm->tm_mday, tm->tm_year % 100);
        strcpy(status.date2, status.date1);
        strcpy(status.date3, status.date1);
    }
    status.users = 1;
    status.callernum = 65535;
    status.callernum1 = 0;
    status.qscanptr = 1;
    strcpy(status.lastuser, "SysOp");

    snprintf(path, sizeof(path), "%sdata/status.json", base);
    root = statusrec_to_json(&status);
    if (write_json_file(path, root) != 0) { perror(path); return 1; }
    cJSON_Delete(root);
    printf("Wrote %s\n", path);

    /* =================================================================
     * mnudata.dat  (text file with ~N markers)
     * ================================================================= */
    snprintf(path, sizeof(path), "%safiles/mnudata.dat", base);
    f = fopen(path, "w");
    if (!f) { perror(path); return 1; }
    fprintf(f, "~0\n");
    fprintf(f, "No menu data loaded.\n");
    fclose(f);
    printf("Wrote %s\n", path);

    /* =================================================================
     * subs.dat  (subboardrec array -- need at least sub 0 = email)
     * ================================================================= */
    memset(&sub, 0, sizeof(sub));
    strcpy(sub.name, "Private Mail");
    strcpy(sub.filename, "email");
    sub.conf = '@';
    sub.maxmsgs = 50;
    sub.storage_type = 2;   /* JAM */
    sub.attr = 0x0020;      /* mattr_private */

    snprintf(path, sizeof(path), "%sdata/subs.dat", base);
    f = fopen(path, "wb");
    if (!f) { perror(path); return 1; }
    fwrite(&sub, sizeof(sub), 1, f);
    /* Add a General sub */
    memset(&sub, 0, sizeof(sub));
    strcpy(sub.name, "General Discussion");
    strcpy(sub.filename, "general");
    sub.conf = 'A';
    sub.maxmsgs = 100;
    sub.storage_type = 2;
    sub.attr = 0;
    fwrite(&sub, sizeof(sub), 1, f);
    fclose(f);
    printf("Wrote %s\n", path);

    /* =================================================================
     * dirs.dat  (directoryrec array -- at least one file directory)
     * ================================================================= */
    memset(&dir, 0, sizeof(dir));
    strcpy(dir.name, "General Files");
    strcpy(dir.filename, "general");
    snprintf(dir.dpath, sizeof(dir.dpath), "%sdls/", base);
    dir.maxfiles = 100;
    dir.mask = 0;
    dir.type = 0;

    snprintf(path, sizeof(path), "%sdata/dirs.dat", base);
    f = fopen(path, "wb");
    if (!f) { perror(path); return 1; }
    fwrite(&dir, sizeof(dir), 1, f);
    fclose(f);
    printf("Wrote %s\n", path);

    /* =================================================================
     * protocol.dat  (protocolrec array)
     * ================================================================= */
    memset(&proto, 0, sizeof(proto));
    strcpy(proto.description, "ASCII Transfer");
    proto.key = 'A';
    proto.singleok = 1;

    snprintf(path, sizeof(path), "%sdata/protocol.dat", base);
    f = fopen(path, "wb");
    if (!f) { perror(path); return 1; }
    fwrite(&proto, sizeof(proto), 1, f);
    fclose(f);
    printf("Wrote %s\n", path);

    /* =================================================================
     * results.dat  (resultrec array -- modem result codes)
     * ================================================================= */
    memset(&result, 0, sizeof(result));
    strcpy(result.curspeed, "TCP/IP");
    strcpy(result.return_code, "CONNECT");
    result.modem_speed = 38400;
    result.com_speed = 38400;
    result.mode = 0;
    result.attr = 0;

    snprintf(path, sizeof(path), "%sdata/results.dat", base);
    f = fopen(path, "wb");
    if (!f) { perror(path); return 1; }
    fwrite(&result, sizeof(result), 1, f);
    fclose(f);
    printf("Wrote %s\n", path);

    /* =================================================================
     * conf.dat  (confrec array -- at least one conference)
     * ================================================================= */
    memset(&conf, 0, sizeof(conf));
    strcpy(conf.name, "Main");
    strcpy(conf.tag, "MAIN");
    conf.type = 0;

    snprintf(path, sizeof(path), "%sdata/conf.dat", base);
    f = fopen(path, "wb");
    if (!f) { perror(path); return 1; }
    fwrite(&conf, sizeof(conf), 1, f);
    fclose(f);
    printf("Wrote %s\n", path);

    /* =================================================================
     * archive.dat  (xarcrec array -- archive definitions)
     * ================================================================= */
    snprintf(path, sizeof(path), "%sdata/archive.dat", base);
    f = fopen(path, "wb");
    if (!f) { perror(path); return 1; }
    for (i = 0; i < 8; i++) {
        memset(&xarc, 0, sizeof(xarc));
        if (i == 0) {
            strcpy(xarc.extension, "ZIP");
            strcpy(xarc.arct, "zip -j %s %s");
            strcpy(xarc.arcc, "zip -j %s %s");
            strcpy(xarc.arca, "zip -j %s %s");
            strcpy(xarc.arce, "unzip -o %s -d %s");
            strcpy(xarc.arcl, "unzip -l %s");
        }
        fwrite(&xarc, sizeof(xarc), 1, f);
    }
    fclose(f);
    printf("Wrote %s\n", path);

    /* =================================================================
     * modem.dat  (modem init strings -- just create empty)
     * ================================================================= */
    snprintf(path, sizeof(path), "%sdata/modem.dat", base);
    f = fopen(path, "wb");
    if (!f) { perror(path); return 1; }
    /* Write minimal modem data: "ATZ\r" as init string */
    fprintf(f, "ATZ\r\n");
    fclose(f);
    printf("Wrote %s\n", path);

    /* =================================================================
     * User #0 (sentinel) -- data/users/0000.json
     * ================================================================= */
    memset(&empty_user, 0, sizeof(empty_user));
    empty_user.inact = 0x01;  /* inact_deleted */

    snprintf(path, sizeof(path), "%sdata/users/0000.json", base);
    root = userrec_to_json(&empty_user);
    if (write_json_file(path, root) != 0) { perror(path); return 1; }
    cJSON_Delete(root);
    printf("Wrote %s (sentinel)\n", path);

    /* =================================================================
     * User #1 (SysOp) -- data/users/0001.json
     * ================================================================= */
    memset(&sysop_user, 0, sizeof(sysop_user));
    strcpy(sysop_user.name, "SYSOP");
    strcpy(sysop_user.realname, "System Operator");
    strcpy(sysop_user.pw, "SYSOP");
    sysop_user.sl = 255;
    sysop_user.dsl = 255;
    sysop_user.exempt = 0xFF;
    sysop_user.ar = 0xFFFF;
    sysop_user.dar = 0xFFFF;
    sysop_user.sysstatus = 0x0003; /* sysstatus_ansi | sysstatus_color */
    sysop_user.inact = 0;
    sysop_user.screenchars = 80;
    sysop_user.screenlines = 25;
    sysop_user.pos_account = 1000.0;
    sysop_user.colors[0] = 7;
    sysop_user.colors[1] = 11;
    sysop_user.colors[2] = 14;
    sysop_user.colors[3] = 5;
    sysop_user.colors[4] = 31;
    sysop_user.colors[5] = 2;
    sysop_user.colors[6] = 12;
    sysop_user.colors[7] = 9;
    sysop_user.colors[8] = 6;
    sysop_user.colors[9] = 3;
    sysop_user.colors[10] = 112;
    sysop_user.colors[11] = 7;
    sysop_user.colors[12] = 15;
    sysop_user.colors[13] = 7;
    sysop_user.colors[14] = 15;
    sysop_user.colors[15] = 7;
    sysop_user.colors[16] = (char)0x8F;
    sysop_user.colors[17] = 112;
    sysop_user.colors[18] = (char)0x8F;
    sysop_user.colors[19] = 7;
    sysop_user.month = 1;
    sysop_user.day = 1;
    sysop_user.year = 80;
    sysop_user.age = 46;
    sysop_user.flisttype = 1;
    sysop_user.mlisttype = 1;
    strcpy(sysop_user.street, "123 BBS Street");
    strcpy(sysop_user.city, "Anytown, USA");

    snprintf(path, sizeof(path), "%sdata/users/0001.json", base);
    root = userrec_to_json(&sysop_user);
    if (write_json_file(path, root) != 0) { perror(path); return 1; }
    cJSON_Delete(root);
    printf("Wrote %s (sysop)\n", path);

    /* =================================================================
     * names.lst -- username to number mapping (text file)
     * ================================================================= */
    snprintf(path, sizeof(path), "%sdata/names.lst", base);
    f = fopen(path, "w");
    if (!f) { perror(path); return 1; }
    fprintf(f, "SYSOP 1\n");
    fclose(f);
    printf("Wrote %s\n", path);

    /* =================================================================
     * acs.dat -- access control strings (all empty = no restrictions)
     * ================================================================= */
    {
        acsrec acs;
        memset(&acs, 0, sizeof(acs));
        strcpy(acs.syspw, "S999");
        snprintf(path, sizeof(path), "%sdata/acs.dat", base);
        f = fopen(path, "wb");
        if (!f) { perror(path); return 1; }
        fwrite(&acs, sizeof(acs), 1, f);
        fclose(f);
        printf("Wrote %s\n", path);
    }

    printf("\nAll data files generated. Run: ./dominion -M\n");
    return 0;
}
