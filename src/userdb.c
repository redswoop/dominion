#include "userdb.h"

#include <assert.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "json_io.h"


/* Internal state — replaces syscfg/status globals */
static char db_datadir[MAX_PATH_LEN];
static int  db_maxusers;
static int  db_user_count;

/* The in-memory name-to-number index, hidden from the rest of the BBS */
static smalrec *smallist;


static void fix_user_rec(userrec *u)
{
    assert(u != NULL);
    u->name[30]=0;
    u->realname[20]=0;
    u->callsign[6]=0;
    u->phone[12]=0;
    u->pw[19]=0;
    u->note[40]=0;
    u->macros[0][80]=0;
    u->macros[1][80]=0;
    u->macros[2][80]=0;
}


int userdb_max_num()
{
    /* Scan data/users/ for highest-numbered .json file */
    DIR *d;
    struct dirent *de;
    char path[MAX_PATH_LEN];
    int highest = 0;

    sprintf(path, "%susers", db_datadir);
    d = opendir(path);
    if (!d)
        return 0;
    while ((de = readdir(d)) != NULL) {
        int n;
        char *dot;
        if (de->d_name[0] == '.')
            continue;
        dot = strstr(de->d_name, ".json");
        if (!dot)
            continue;
        n = atoi(de->d_name);
        if (n > highest)
            highest = n;
    }
    closedir(d);
    return highest;
}

void userdb_load(unsigned int un, userrec *u)
{
    char path[MAX_PATH_LEN];
    cJSON *root;

    assert(u != NULL);
    assert(un > 0);

    if (un > (unsigned int)userdb_max_num()) {
        u->inact=inact_deleted;
        fix_user_rec(u);
        return;
    }

    sprintf(path, "%susers/%04d.json", db_datadir, un);
    root = read_json_file(path);
    if (!root) {
        memset(u, 0, sizeof(userrec));
        u->inact=inact_deleted;
        fix_user_rec(u);
        return;
    }
    json_to_userrec(root, u);
    cJSON_Delete(root);
    fix_user_rec(u);
}


int userdb_save(unsigned int un, userrec *u)
{
    char path[MAX_PATH_LEN];
    cJSON *root;
    int rc;

    assert(u != NULL);
    assert(un > 0);

    sprintf(path, "%susers/%04d.json", db_datadir, un);
    root = userrec_to_json(u);
    rc = write_json_file(path, root);
    cJSON_Delete(root);
    return rc;
}


/* Insert into sorted index without saving status (internal helper) */
static void isr_nosave(int un, char *name)
{
    int cp;
    smalrec sr;

    assert(smallist != NULL);
    assert(un > 0);
    assert(name != NULL && name[0] != '\0');
    assert(db_user_count < db_maxusers);

    cp=0;
    while ((cp<db_user_count) && (strcmp(name,(smallist[cp].name))>0))
        ++cp;
    memmove(&(smallist[cp+1]),&(smallist[cp]),sizeof(smalrec)*(db_user_count-cp));
    strcpy(sr.name,name);
    sr.number=un;
    smallist[cp]=sr;
    ++db_user_count;
}


/* --- Public index mutation API --- */

void userdb_index_add(int un, char *name)
{
    assert(un > 0);
    assert(name != NULL && name[0] != '\0');
    isr_nosave(un, name);
}

int userdb_index_remove(char *name)
{
    int cp;

    assert(smallist != NULL);
    assert(name != NULL && name[0] != '\0');

    cp=0;
    while ((cp<db_user_count) && (strcmp(name,(smallist[cp].name))!=0))
        ++cp;
    if (strcmp(name,(smallist[cp].name))) {
        return -1;
    }
    memmove(&(smallist[cp]),&(smallist[cp+1]),sizeof(smalrec)*(db_user_count-cp));
    --db_user_count;
    return 0;
}


/* --- Index lifecycle --- */

void userdb_init(const char *datadir, int maxusers)
{
    char s[MAX_PATH_LEN];
    DIR *ud;
    struct dirent *de;

    assert(datadir != NULL);
    assert(maxusers > 0);

    strncpy(db_datadir, datadir, MAX_PATH_LEN - 1);
    db_datadir[MAX_PATH_LEN - 1] = '\0';
    db_maxusers = maxusers;
    db_user_count = 0;

    smallist=(smalrec *) malloc((long)db_maxusers * (long)sizeof(smalrec));
    assert(smallist != NULL);

    /* Build smallist by scanning JSON files in data/users/ */
    sprintf(s, "%susers", db_datadir);
    ud = opendir(s);
    if (!ud) {
        userdb_rebuild_index();
    } else {
        while ((de = readdir(ud)) != NULL) {
            int un;
            char upath[MAX_PATH_LEN];
            cJSON *uroot, *uname;
            if (de->d_name[0] == '.') continue;
            if (!strstr(de->d_name, ".json")) continue;
            un = atoi(de->d_name);
            if (un <= 0) continue;
            sprintf(upath, "%susers/%s", db_datadir, de->d_name);
            uroot = read_json_file(upath);
            if (!uroot) continue;
            uname = cJSON_GetObjectItemCaseSensitive(uroot, "name");
            if (cJSON_IsString(uname) && uname->valuestring[0]) {
                cJSON *uinact = cJSON_GetObjectItemCaseSensitive(uroot, "inact");
                int inact_val = cJSON_IsNumber(uinact) ? uinact->valueint : 0;
                if (!(inact_val & inact_deleted)) {
                    isr_nosave(un, uname->valuestring);
                }
            }
            cJSON_Delete(uroot);
        }
        closedir(ud);
    }
}


void userdb_rebuild_index(void)
{
    int i, i1;
    userrec u;

    assert(smallist != NULL);

    db_user_count=0;

    i1=userdb_max_num();
    for (i=1; i<=i1; i++) {
        userdb_load(i,&u);
        if ((u.inact & inact_deleted)==0)
            isr_nosave(i,u.name);
    }
}


/* --- Index queries --- */

unsigned int userdb_find(char *name)
{
    smalrec *sr;

    assert(name != NULL);

    sr=(smalrec *) bsearch((void *)name,
    (void *)smallist,
    (size_t)db_user_count,
    (size_t)sizeof(smalrec),
    (int (*) (const void *, const void *))strcmp);
    if (sr==0L)
        return(0);
    else
        return(sr->number);
}


void userdb_get_entry(int index, smalrec *out)
{
    assert(smallist != NULL);
    assert(out != NULL);
    assert(index >= 0 && index < db_user_count);
    *out = smallist[index];
}


int userdb_user_count(void)
{
    return db_user_count;
}
