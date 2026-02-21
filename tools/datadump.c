/*
 * datadump.c - Dump and validate Dominion BBS binary data files
 *
 * Auto-detects file type from filename, validates sizes and field values,
 * dumps key fields in human-readable format.
 *
 * Usage: ./datadump [--validate] [--type TYPE] <file>
 *        ./datadump config.dat
 *        ./datadump --validate data/status.dat
 *        ./datadump --type menu menus/main.mnu
 *
 * Build: cc -std=gnu89 -DPD -fsigned-char -Isrc -o datadump tools/datadump.c
 *
 * Exit codes:
 *   0 = all OK (or dump completed with no validation errors)
 *   1 = validation errors found
 *   2 = usage error or file not found
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#include "vardec.h"

static int errors = 0;
static int validate_only = 0;  /* --validate: errors only, no dump */

#define ERR(...) do { fprintf(stderr, "ERROR: "); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); errors++; } while(0)
#define INFO(...) do { if (!validate_only) printf(__VA_ARGS__); } while(0)

/* Old menu format sizes */
#define OLD_MMREC_SIZE   312
#define OLD_MENUREC_SIZE 189

/* warnings count separately — trailing bytes from DOS-era files aren't fatal */
static int warnings = 0;
#define WARN(...) do { fprintf(stderr, "WARNING: "); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); warnings++; } while(0)

/* ========================================================================
 * Utility helpers
 * ======================================================================== */

/*
 * safe_str - Return a printable version of a fixed-size char field.
 * Copies up to maxlen bytes, stopping at null, into a static buffer.
 */
static char *safe_str(const char *src, int maxlen)
{
    static char buf[4][512];
    static int idx = 0;
    char *out;
    int i;

    out = buf[idx & 3];
    idx++;
    for (i = 0; i < maxlen && i < 510 && src[i]; i++)
        out[i] = isprint((unsigned char)src[i]) ? src[i] : '?';
    out[i] = 0;
    return out;
}

/*
 * basename_lower - Extract lowercase basename from a path.
 */
static void basename_lower(const char *path, char *out, int outsz)
{
    const char *p;
    int i;

    p = strrchr(path, '/');
    if (p)
        p++;
    else
        p = path;

    for (i = 0; i < outsz - 1 && p[i]; i++)
        out[i] = tolower((unsigned char)p[i]);
    out[i] = 0;
}

/*
 * check_nonempty - Validate a string field is non-empty.
 */
static void check_nonempty(const char *field, const char *val, int maxlen)
{
    if (!val[0])
        ERR("%s is empty", field);
}

/*
 * check_array_size - Check file size for an array of fixed-size records.
 * Returns the number of complete records. Warns (not errors) about trailing bytes.
 */
static int check_array_size(long fsize, size_t recsz, const char *typename)
{
    int nrecs = fsize / recsz;
    long trailing = fsize % recsz;
    if (trailing)
        WARN("File size %ld has %ld trailing byte(s) after %d %s records "
             "(expected multiple of %zu)", fsize, trailing, nrecs, typename, recsz);
    return nrecs;
}

/*
 * check_ends_with_slash - Validate a path ends with '/'.
 */
static void check_path(const char *field, const char *val, int maxlen)
{
    int len;
    if (!val[0]) {
        ERR("%s is empty", field);
        return;
    }
    len = strlen(val);
    if (val[len - 1] != '/')
        ERR("%s does not end with '/': \"%s\"", field, safe_str(val, maxlen));
}

/* ========================================================================
 * Config.dat handler
 * ======================================================================== */

static void dump_config(const unsigned char *data, long fsize, const char *path)
{
    configrec *cfg;
    niftyrec *nif;
    long expected;

    expected = (long)(sizeof(configrec) + sizeof(niftyrec));

    INFO("=== %s: configrec (%zu) + niftyrec (%zu) = %zu bytes ===\n",
         path, sizeof(configrec), sizeof(niftyrec), sizeof(configrec) + sizeof(niftyrec));

    if (fsize != expected) {
        ERR("File size %ld != expected %ld", fsize, expected);
        return;
    }

    cfg = (configrec *)data;
    nif = (niftyrec *)(data + sizeof(configrec));

    INFO("  System name  : \"%s\"\n", safe_str(cfg->systemname, sizeof(cfg->systemname)));
    INFO("  Sysop        : \"%s\"\n", safe_str(cfg->sysopname, sizeof(cfg->sysopname)));
    INFO("  Phone        : \"%s\"\n", safe_str(cfg->systemphone, sizeof(cfg->systemphone)));
    INFO("  Data dir     : \"%s\"\n", safe_str(cfg->datadir, sizeof(cfg->datadir)));
    INFO("  Msgs dir     : \"%s\"\n", safe_str(cfg->msgsdir, sizeof(cfg->msgsdir)));
    INFO("  Menu dir     : \"%s\"\n", safe_str(cfg->menudir, sizeof(cfg->menudir)));
    INFO("  GFiles dir   : \"%s\"\n", safe_str(cfg->gfilesdir, sizeof(cfg->gfilesdir)));
    INFO("  DLoads dir   : \"%s\"\n", safe_str(cfg->dloadsdir, sizeof(cfg->dloadsdir)));
    INFO("  Temp dir     : \"%s\"\n", safe_str(cfg->tempdir, sizeof(cfg->tempdir)));
    INFO("  Batch dir    : \"%s\"\n", safe_str(cfg->batchdir, sizeof(cfg->batchdir)));
    INFO("  Max users    : %u\n", cfg->maxusers);
    INFO("  New user SL  : %u\n", (unsigned)cfg->newusersl);
    INFO("  New user DSL : %u\n", (unsigned)cfg->newuserdsl);
    INFO("  User rec len : %d (expected %zu) %s\n",
         cfg->userreclen, sizeof(userrec),
         cfg->userreclen == (int)sizeof(userrec) ? "OK" : "MISMATCH");
    INFO("  First menu   : \"%s\"\n", safe_str(nif->firstmenu, sizeof(nif->firstmenu)));
    INFO("  Matrix type  : %d\n", (int)nif->matrixtype);
    INFO("  System type  : %d\n", (int)nif->systemtype);

    /* Validations */
    check_nonempty("systemname", cfg->systemname, sizeof(cfg->systemname));
    check_nonempty("sysopname", cfg->sysopname, sizeof(cfg->sysopname));
    check_path("datadir", cfg->datadir, sizeof(cfg->datadir));
    check_path("msgsdir", cfg->msgsdir, sizeof(cfg->msgsdir));
    check_path("menudir", cfg->menudir, sizeof(cfg->menudir));
    check_path("gfilesdir", cfg->gfilesdir, sizeof(cfg->gfilesdir));
    check_nonempty("nifty.firstmenu", nif->firstmenu, sizeof(nif->firstmenu));

    if (cfg->userreclen != (int)sizeof(userrec))
        ERR("userreclen %d != sizeof(userrec) %zu", cfg->userreclen, sizeof(userrec));
}

/* ========================================================================
 * Status.dat handler
 * ======================================================================== */

static void dump_status(const unsigned char *data, long fsize, const char *path)
{
    statusrec *st;

    INFO("=== %s: statusrec (%zu bytes) ===\n", path, sizeof(statusrec));

    if (fsize != (long)sizeof(statusrec)) {
        ERR("File size %ld != expected %zu", fsize, sizeof(statusrec));
        return;
    }

    st = (statusrec *)data;

    INFO("  Date1       : \"%s\"\n", safe_str(st->date1, sizeof(st->date1)));
    INFO("  Date2       : \"%s\"\n", safe_str(st->date2, sizeof(st->date2)));
    INFO("  Date3       : \"%s\"\n", safe_str(st->date3, sizeof(st->date3)));
    INFO("  Users       : %u\n", st->users);
    INFO("  Caller num  : %u (long: %lu)\n", st->callernum, st->callernum1);
    INFO("  Calls today : %u\n", st->callstoday);
    INFO("  Posts today : %u\n", st->msgposttoday);
    INFO("  Email today : %u\n", st->emailtoday);
    INFO("  Last user   : \"%s\"\n", safe_str(st->lastuser, sizeof(st->lastuser)));

    if (st->users < 1)
        ERR("users = %u, expected >= 1 (sysop)", st->users);
    check_nonempty("date1", st->date1, sizeof(st->date1));
}

/* ========================================================================
 * User.lst handler
 * ======================================================================== */

static void dump_users(const unsigned char *data, long fsize, const char *path)
{
    int nrecs, i;
    userrec *u;

    INFO("=== %s: userrec[] (%zu bytes each) ===\n", path, sizeof(userrec));

    nrecs = check_array_size(fsize, sizeof(userrec), "userrec");
    INFO("  Records: %d\n", nrecs);

    if (nrecs < 2)
        ERR("Expected >= 2 records (sentinel + sysop), got %d", nrecs);

    for (i = 0; i < nrecs; i++) {
        u = (userrec *)(data + i * sizeof(userrec));

        if (i == 0) {
            /* Sentinel record */
            INFO("  [%d] SENTINEL inact=0x%02x\n", i, (unsigned)u->inact);
            if (!(u->inact & 0x01))
                ERR("Record 0 (sentinel) inact=0x%02x, expected inact_deleted (0x01) set",
                    (unsigned)u->inact);
            continue;
        }

        if (u->inact & 0x01) {
            /* Deleted user — skip silently */
            continue;
        }

        INFO("  [%d] \"%s\" SL=%u DSL=%u screen=%ux%u%s\n",
             i,
             safe_str(u->name, sizeof(u->name)),
             (unsigned)u->sl, (unsigned)u->dsl,
             (unsigned)u->screenchars, (unsigned)u->screenlines,
             (u->inact & 0x02) ? " INACTIVE" : "");

        /* Validate active users */
        check_nonempty("name", u->name, sizeof(u->name));

        if (u->screenchars != 40 && u->screenchars != 80 && u->screenchars != 132)
            ERR("User %d \"%s\": screenchars=%u (expected 40, 80, or 132)",
                i, safe_str(u->name, sizeof(u->name)), (unsigned)u->screenchars);

        if (u->screenlines != 24 && u->screenlines != 25 && u->screenlines != 50)
            ERR("User %d \"%s\": screenlines=%u (expected 24, 25, or 50)",
                i, safe_str(u->name, sizeof(u->name)), (unsigned)u->screenlines);

        if (i == 1 && u->sl != 255)
            ERR("User 1 (sysop) SL=%u, expected 255", (unsigned)u->sl);
    }
}

/* ========================================================================
 * User.idx handler
 * ======================================================================== */

static void dump_useridx(const unsigned char *data, long fsize, const char *path)
{
    int nrecs, i;
    smalrec *idx;

    INFO("=== %s: smalrec[] (%zu bytes each) ===\n", path, sizeof(smalrec));

    nrecs = check_array_size(fsize, sizeof(smalrec), "smalrec");
    INFO("  Records: %d\n", nrecs);

    if (nrecs < 1)
        ERR("Expected >= 1 index record (sysop), got %d", nrecs);

    for (i = 0; i < nrecs; i++) {
        idx = (smalrec *)(data + i * sizeof(smalrec));

        INFO("  [%d] \"%s\" -> user #%u\n",
             i, safe_str(idx->name, sizeof(idx->name)), idx->number);

        check_nonempty("name", idx->name, sizeof(idx->name));
        if (idx->number < 1)
            ERR("Index %d: number=%u, expected >= 1", i, idx->number);

        /* Check sort order */
        if (i > 0) {
            smalrec *prev = (smalrec *)(data + (i - 1) * sizeof(smalrec));
            if (strcmp(prev->name, idx->name) > 0)
                ERR("Index not sorted: [%d] \"%s\" > [%d] \"%s\"",
                    i - 1, safe_str(prev->name, sizeof(prev->name)),
                    i, safe_str(idx->name, sizeof(idx->name)));
        }
    }
}

/* ========================================================================
 * Subs.dat handler
 * ======================================================================== */

static void dump_subs(const unsigned char *data, long fsize, const char *path)
{
    int nrecs, i;
    subboardrec *sub;

    INFO("=== %s: subboardrec[] (%zu bytes each) ===\n", path, sizeof(subboardrec));

    nrecs = check_array_size(fsize, sizeof(subboardrec), "subboardrec");
    INFO("  Records: %d\n", nrecs);

    if (nrecs > 200)
        ERR("Record count %d exceeds max 200", nrecs);

    for (i = 0; i < nrecs; i++) {
        sub = (subboardrec *)(data + i * sizeof(subboardrec));

        INFO("  [%d] \"%s\" file=\"%s\" type=%u maxmsgs=%u conf='%c'\n",
             i,
             safe_str(sub->name, sizeof(sub->name)),
             safe_str(sub->filename, sizeof(sub->filename)),
             sub->storage_type,
             sub->maxmsgs,
             sub->conf ? sub->conf : '?');

        check_nonempty("name", sub->name, sizeof(sub->name));
        check_nonempty("filename", sub->filename, sizeof(sub->filename));

        if (sub->storage_type > 2)
            ERR("Sub %d \"%s\": storage_type=%u (expected 0, 1, or 2)",
                i, safe_str(sub->name, sizeof(sub->name)), sub->storage_type);
    }
}

/* ========================================================================
 * Dirs.dat handler
 * ======================================================================== */

static void dump_dirs(const unsigned char *data, long fsize, const char *path)
{
    int nrecs, i;
    directoryrec *dir;

    INFO("=== %s: directoryrec[] (%zu bytes each) ===\n", path, sizeof(directoryrec));

    nrecs = check_array_size(fsize, sizeof(directoryrec), "directoryrec");
    INFO("  Records: %d\n", nrecs);

    if (nrecs > 200)
        ERR("Record count %d exceeds max 200", nrecs);

    for (i = 0; i < nrecs; i++) {
        dir = (directoryrec *)(data + i * sizeof(directoryrec));

        INFO("  [%d] \"%s\" file=\"%s\" maxfiles=%u mask=0x%04x\n",
             i,
             safe_str(dir->name, sizeof(dir->name)),
             safe_str(dir->filename, sizeof(dir->filename)),
             dir->maxfiles,
             dir->mask);

        check_nonempty("name", dir->name, sizeof(dir->name));
        check_nonempty("filename", dir->filename, sizeof(dir->filename));

        if (dir->maxfiles == 0)
            ERR("Dir %d \"%s\": maxfiles=0", i, safe_str(dir->name, sizeof(dir->name)));
    }
}

/* ========================================================================
 * Protocol.dat handler
 * ======================================================================== */

static void dump_protocol(const unsigned char *data, long fsize, const char *path)
{
    int nrecs, i;
    protocolrec *p;

    INFO("=== %s: protocolrec[] (%zu bytes each) ===\n", path, sizeof(protocolrec));

    nrecs = check_array_size(fsize, sizeof(protocolrec), "protocolrec");
    INFO("  Records: %d\n", nrecs);

    for (i = 0; i < nrecs; i++) {
        p = (protocolrec *)(data + i * sizeof(protocolrec));

        INFO("  [%d] \"%s\" key='%c' single=%d\n",
             i,
             safe_str(p->description, sizeof(p->description)),
             p->key ? p->key : '?',
             p->singleok);

        check_nonempty("description", p->description, sizeof(p->description));
        if (!p->key)
            ERR("Protocol %d: key is null", i);
    }
}

/* ========================================================================
 * Conf.dat handler
 * ======================================================================== */

static void dump_conf(const unsigned char *data, long fsize, const char *path)
{
    int nrecs, i;
    confrec *c;

    INFO("=== %s: confrec[] (%zu bytes each) ===\n", path, sizeof(confrec));

    nrecs = check_array_size(fsize, sizeof(confrec), "confrec");
    INFO("  Records: %d\n", nrecs);

    for (i = 0; i < nrecs; i++) {
        c = (confrec *)(data + i * sizeof(confrec));

        INFO("  [%d] \"%s\" tag=\"%s\" type=%d\n",
             i,
             safe_str(c->name, sizeof(c->name)),
             safe_str(c->tag, sizeof(c->tag)),
             (int)c->type);

        check_nonempty("name", c->name, sizeof(c->name));
    }
}

/* ========================================================================
 * Archive.dat handler
 * ======================================================================== */

static void dump_archive(const unsigned char *data, long fsize, const char *path)
{
    int nrecs, i;
    xarcrec *x;

    INFO("=== %s: xarcrec[] (%zu bytes each) ===\n", path, sizeof(xarcrec));

    nrecs = check_array_size(fsize, sizeof(xarcrec), "xarcrec");
    INFO("  Records: %d\n", nrecs);

    if (nrecs != 8)
        ERR("Expected exactly 8 archive records, got %d", nrecs);

    for (i = 0; i < nrecs; i++) {
        x = (xarcrec *)(data + i * sizeof(xarcrec));

        if (x->extension[0]) {
            INFO("  [%d] ext=\"%s\" test=\"%s\" compress=\"%s\"\n",
                 i,
                 safe_str(x->extension, sizeof(x->extension)),
                 safe_str(x->arct, sizeof(x->arct)),
                 safe_str(x->arcc, sizeof(x->arcc)));
        } else {
            INFO("  [%d] (empty)\n", i);
        }
    }
}

/* ========================================================================
 * Results.dat handler
 * ======================================================================== */

static void dump_results(const unsigned char *data, long fsize, const char *path)
{
    int nrecs, i;
    resultrec *r;

    INFO("=== %s: resultrec[] (%zu bytes each) ===\n", path, sizeof(resultrec));

    nrecs = check_array_size(fsize, sizeof(resultrec), "resultrec");
    INFO("  Records: %d\n", nrecs);

    for (i = 0; i < nrecs; i++) {
        r = (resultrec *)(data + i * sizeof(resultrec));

        INFO("  [%d] speed=\"%s\" code=\"%s\" modem=%u com=%u\n",
             i,
             safe_str(r->curspeed, sizeof(r->curspeed)),
             safe_str(r->return_code, sizeof(r->return_code)),
             r->modem_speed, r->com_speed);
    }
}

/* ========================================================================
 * Acs.dat handler
 * ======================================================================== */

static void dump_acs(const unsigned char *data, long fsize, const char *path)
{
    acsrec *a;

    INFO("=== %s: acsrec (%zu bytes) ===\n", path, sizeof(acsrec));

    if (fsize != (long)sizeof(acsrec)) {
        ERR("File size %ld != expected %zu", fsize, sizeof(acsrec));
        return;
    }

    a = (acsrec *)data;

    INFO("  epcr      : \"%s\"\n", safe_str(a->epcr, sizeof(a->epcr)));
    INFO("  eratio    : \"%s\"\n", safe_str(a->eratio, sizeof(a->eratio)));
    INFO("  efpts     : \"%s\"\n", safe_str(a->efpts, sizeof(a->efpts)));
    INFO("  etc       : \"%s\"\n", safe_str(a->etc, sizeof(a->etc)));
    INFO("  syspw     : \"%s\"\n", safe_str(a->syspw, sizeof(a->syspw)));
    INFO("  showpw    : \"%s\"\n", safe_str(a->showpw, sizeof(a->showpw)));
    INFO("  cosysop   : \"%s\"\n", safe_str(a->cosysop, sizeof(a->cosysop)));
    INFO("  sysop     : \"%s\"\n", safe_str(a->sysop, sizeof(a->sysop)));
    INFO("  echat     : \"%s\"\n", safe_str(a->echat, sizeof(a->echat)));
    INFO("  readunval : \"%s\"\n", safe_str(a->readunval, sizeof(a->readunval)));
    INFO("  dlunval   : \"%s\"\n", safe_str(a->dlunval, sizeof(a->dlunval)));
    INFO("  anyul     : \"%s\"\n", safe_str(a->anyul, sizeof(a->anyul)));
    INFO("  readanon  : \"%s\"\n", safe_str(a->readanon, sizeof(a->readanon)));
    INFO("  delmsg    : \"%s\"\n", safe_str(a->delmsg, sizeof(a->delmsg)));
    INFO("  zapmail   : \"%s\"\n", safe_str(a->zapmail, sizeof(a->zapmail)));
}

/* ========================================================================
 * Menu handler (*.mnu — new v3.x format only for validation)
 * ======================================================================== */

static int is_valid_cmd_prefix(char c)
{
    switch (c) {
    case 'M': case 'F': case 'D': case 'S': case 'O':
    case 'I': case '=': case '?': case 'Q': case 'J':
    case 'W': case 0:
        return 1;
    }
    return 0;
}

static void dump_menu(const unsigned char *data, long fsize, const char *path)
{
    mmrec hdr;
    menurec cmd;
    int ncmds, i;
    long rem;

    INFO("=== %s: mmrec (%zu) + menurec[] (%zu each) ===\n",
         path, sizeof(mmrec), sizeof(menurec));

    /* Check new format first */
    rem = fsize - (long)sizeof(mmrec);
    if (rem < 0 || (rem % (long)sizeof(menurec)) != 0) {
        /* Try old format detection */
        rem = fsize - OLD_MMREC_SIZE;
        if (rem >= 0 && (rem % OLD_MENUREC_SIZE) == 0) {
            ERR("File is old v2.x format (%ld bytes, %ld cmds) — needs conversion with mnuconv",
                fsize, rem / OLD_MENUREC_SIZE);
        } else {
            ERR("File size %ld does not match menu format (hdr=%zu + N*%zu)",
                fsize, sizeof(mmrec), sizeof(menurec));
        }
        return;
    }

    ncmds = rem / (long)sizeof(menurec);
    if (ncmds > 64)
        ERR("Command count %d exceeds max 64", ncmds);

    memcpy(&hdr, data, sizeof(mmrec));

    INFO("  Prompt     : \"%s\"\n", safe_str(hdr.prompt, sizeof(hdr.prompt)));
    INFO("  Title 1    : \"%s\"\n", safe_str(hdr.title1, sizeof(hdr.title1)));
    if (hdr.title2[0])
        INFO("  Title 2    : \"%s\"\n", safe_str(hdr.title2, sizeof(hdr.title2)));
    if (hdr.altmenu[0])
        INFO("  ANSI Menu  : \"%s\"\n", safe_str(hdr.altmenu, sizeof(hdr.altmenu)));
    INFO("  Columns    : %d\n", (int)hdr.columns);
    INFO("  Hotkeys    : %d\n", (int)hdr.boarder);
    INFO("  Security   : \"%s\"\n", safe_str(hdr.slneed, sizeof(hdr.slneed)));
    INFO("  Commands   : %d\n", ncmds);

    if ((unsigned char)hdr.boarder > 2)
        ERR("boarder=%d, expected 0, 1, or 2", (int)(unsigned char)hdr.boarder);

    for (i = 0; i < ncmds && i < 64; i++) {
        int off = sizeof(mmrec) + i * sizeof(menurec);
        memcpy(&cmd, &data[off], sizeof(menurec));

        INFO("  [%2d] type=\"%s\" key=\"%s\" desc=\"%s\"\n",
             i,
             safe_str(cmd.type, sizeof(cmd.type)),
             safe_str(cmd.key, sizeof(cmd.key)),
             safe_str(cmd.desc, sizeof(cmd.desc)));

        if (cmd.type[0] && !is_valid_cmd_prefix(cmd.type[0]))
            ERR("Command %d: unknown type prefix '%c' (0x%02x)",
                i, cmd.type[0], (unsigned char)cmd.type[0]);
    }
}

/* ========================================================================
 * File type detection and dispatch
 * ======================================================================== */

enum filetype {
    FT_UNKNOWN = 0,
    FT_CONFIG,
    FT_STATUS,
    FT_USERS,
    FT_USERIDX,
    FT_SUBS,
    FT_DIRS,
    FT_PROTOCOL,
    FT_CONF,
    FT_ARCHIVE,
    FT_RESULTS,
    FT_ACS,
    FT_MENU
};

static const char *ft_names[] = {
    "unknown", "config", "status", "users", "useridx",
    "subs", "dirs", "protocol", "conf", "archive",
    "results", "acs", "menu"
};

static enum filetype detect_type(const char *path)
{
    char base[256];
    char *dot;

    basename_lower(path, base, sizeof(base));

    if (strcmp(base, "config.dat") == 0) return FT_CONFIG;
    if (strcmp(base, "status.dat") == 0) return FT_STATUS;
    if (strcmp(base, "user.lst") == 0) return FT_USERS;
    if (strcmp(base, "user.idx") == 0) return FT_USERIDX;
    if (strcmp(base, "subs.dat") == 0) return FT_SUBS;
    if (strcmp(base, "dirs.dat") == 0) return FT_DIRS;
    if (strcmp(base, "protocol.dat") == 0) return FT_PROTOCOL;
    if (strcmp(base, "conf.dat") == 0) return FT_CONF;
    if (strcmp(base, "archive.dat") == 0) return FT_ARCHIVE;
    if (strcmp(base, "results.dat") == 0) return FT_RESULTS;
    if (strcmp(base, "acs.dat") == 0) return FT_ACS;

    /* Check extension for .mnu */
    dot = strrchr(base, '.');
    if (dot && strcmp(dot, ".mnu") == 0) return FT_MENU;

    return FT_UNKNOWN;
}

static enum filetype parse_type_arg(const char *arg)
{
    if (strcmp(arg, "config") == 0) return FT_CONFIG;
    if (strcmp(arg, "status") == 0) return FT_STATUS;
    if (strcmp(arg, "users") == 0) return FT_USERS;
    if (strcmp(arg, "useridx") == 0) return FT_USERIDX;
    if (strcmp(arg, "subs") == 0) return FT_SUBS;
    if (strcmp(arg, "dirs") == 0) return FT_DIRS;
    if (strcmp(arg, "protocol") == 0) return FT_PROTOCOL;
    if (strcmp(arg, "conf") == 0) return FT_CONF;
    if (strcmp(arg, "archive") == 0) return FT_ARCHIVE;
    if (strcmp(arg, "results") == 0) return FT_RESULTS;
    if (strcmp(arg, "acs") == 0) return FT_ACS;
    if (strcmp(arg, "menu") == 0) return FT_MENU;
    return FT_UNKNOWN;
}

static void dispatch(enum filetype ft, const unsigned char *data, long fsize, const char *path)
{
    switch (ft) {
    case FT_CONFIG:   dump_config(data, fsize, path); break;
    case FT_STATUS:   dump_status(data, fsize, path); break;
    case FT_USERS:    dump_users(data, fsize, path); break;
    case FT_USERIDX:  dump_useridx(data, fsize, path); break;
    case FT_SUBS:     dump_subs(data, fsize, path); break;
    case FT_DIRS:     dump_dirs(data, fsize, path); break;
    case FT_PROTOCOL: dump_protocol(data, fsize, path); break;
    case FT_CONF:     dump_conf(data, fsize, path); break;
    case FT_ARCHIVE:  dump_archive(data, fsize, path); break;
    case FT_RESULTS:  dump_results(data, fsize, path); break;
    case FT_ACS:      dump_acs(data, fsize, path); break;
    case FT_MENU:     dump_menu(data, fsize, path); break;
    default:
        ERR("Unknown file type");
        break;
    }
}

/* ========================================================================
 * Main
 * ======================================================================== */

static void usage(void)
{
    fprintf(stderr,
        "Usage: datadump [--validate] [--type TYPE] <file>\n"
        "\n"
        "Dump and validate Dominion BBS binary data files.\n"
        "\n"
        "Options:\n"
        "  --validate   Suppress normal output, only report errors\n"
        "  --type TYPE  Override auto-detection (config, status, users,\n"
        "               useridx, subs, dirs, protocol, conf, archive,\n"
        "               results, acs, menu)\n"
        "  --sizes      Print struct sizes and exit\n"
        "\n"
        "Exit codes: 0=OK  1=validation errors  2=usage/file error\n");
}

int main(int argc, char **argv)
{
    const char *filepath;
    const char *type_arg;
    enum filetype ft;
    FILE *f;
    unsigned char *data;
    long fsize;
    int show_sizes;
    int i;

    filepath = NULL;
    type_arg = NULL;
    show_sizes = 0;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--validate") == 0) {
            validate_only = 1;
        } else if (strcmp(argv[i], "--type") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "datadump: --type requires an argument\n");
                return 2;
            }
            type_arg = argv[++i];
        } else if (strcmp(argv[i], "--sizes") == 0) {
            show_sizes = 1;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            usage();
            return 0;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "datadump: unknown option: %s\n", argv[i]);
            return 2;
        } else {
            filepath = argv[i];
        }
    }

    if (show_sizes) {
        printf("Struct sizes:\n");
        printf("  configrec    = %4zu\n", sizeof(configrec));
        printf("  niftyrec     = %4zu\n", sizeof(niftyrec));
        printf("  statusrec    = %4zu\n", sizeof(statusrec));
        printf("  userrec      = %4zu\n", sizeof(userrec));
        printf("  smalrec      = %4zu\n", sizeof(smalrec));
        printf("  subboardrec  = %4zu\n", sizeof(subboardrec));
        printf("  directoryrec = %4zu\n", sizeof(directoryrec));
        printf("  protocolrec  = %4zu\n", sizeof(protocolrec));
        printf("  confrec      = %4zu\n", sizeof(confrec));
        printf("  xarcrec      = %4zu\n", sizeof(xarcrec));
        printf("  resultrec    = %4zu\n", sizeof(resultrec));
        printf("  acsrec       = %4zu\n", sizeof(acsrec));
        printf("  mmrec        = %4zu\n", sizeof(mmrec));
        printf("  menurec      = %4zu\n", sizeof(menurec));
        printf("  config+nifty = %4zu\n", sizeof(configrec) + sizeof(niftyrec));
        return 0;
    }

    if (!filepath) {
        usage();
        return 2;
    }

    /* Determine file type */
    if (type_arg) {
        ft = parse_type_arg(type_arg);
        if (ft == FT_UNKNOWN) {
            fprintf(stderr, "datadump: unknown type: %s\n", type_arg);
            return 2;
        }
    } else {
        ft = detect_type(filepath);
        if (ft == FT_UNKNOWN) {
            fprintf(stderr, "datadump: cannot auto-detect type for: %s\n", filepath);
            fprintf(stderr, "Use --type to specify manually.\n");
            return 2;
        }
    }

    INFO("File type: %s\n", ft_names[ft]);

    /* Read file */
    f = fopen(filepath, "rb");
    if (!f) {
        fprintf(stderr, "datadump: cannot open %s: ", filepath);
        perror("");
        return 2;
    }

    fseek(f, 0, SEEK_END);
    fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fsize == 0) {
        ERR("File is empty");
        fclose(f);
        goto done;
    }

    data = (unsigned char *)malloc(fsize);
    if (!data) {
        fprintf(stderr, "datadump: out of memory (%ld bytes)\n", fsize);
        fclose(f);
        return 2;
    }

    if ((long)fread(data, 1, fsize, f) != fsize) {
        fprintf(stderr, "datadump: error reading %s\n", filepath);
        free(data);
        fclose(f);
        return 2;
    }
    fclose(f);

    /* Dispatch */
    dispatch(ft, data, fsize, filepath);
    free(data);

done:
    if (errors) {
        INFO("\nValidation: %d error(s)", errors);
        if (warnings) INFO(", %d warning(s)", warnings);
        INFO("\n");
        if (validate_only)
            fprintf(stderr, "Validation: %d error(s) in %s\n", errors, filepath);
        return 1;
    }
    if (warnings) {
        INFO("\nValidation: OK (%d warning(s))\n", warnings);
    } else {
        INFO("\nValidation: OK\n");
    }
    return 0;
}
