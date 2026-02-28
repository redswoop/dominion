/*
 * userdb.cpp — UserDB class implementation
 *
 * Manages user records stored as JSON files (data/users/NNNN.json)
 * with a sorted in-memory name index for fast lookup.
 */

#include "user/userdb.h"
#include "platform.h"
#include "json_io.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>


/*--- Singleton ---*/

UserDB& UserDB::instance()
{
    static UserDB db;
    return db;
}


/*--- Lifecycle ---*/

void UserDB::init(const std::string& datadir, int maxusers)
{
    assert(!datadir.empty());
    assert(maxusers > 0);

    datadir_ = datadir;
    maxusers_ = maxusers;
    index_.clear();

    /* Build index by scanning JSON files in data/users/ */
    std::string udir = datadir_ + "users";
    DIR *d = opendir(udir.c_str());
    if (!d) {
        rebuild_index();
        return;
    }

    struct dirent *de;
    while ((de = readdir(d)) != NULL) {
        if (de->d_name[0] == '.') continue;
        if (!strstr(de->d_name, ".json")) continue;
        int un = atoi(de->d_name);
        if (un <= 0) continue;

        char upath[MAX_PATH_LEN];
        sprintf(upath, "%susers/%s", datadir_.c_str(), de->d_name);
        cJSON *uroot = read_json_file(upath);
        if (!uroot) continue;

        cJSON *uname = cJSON_GetObjectItemCaseSensitive(uroot, "name");
        if (cJSON_IsString(uname) && uname->valuestring[0]) {
            cJSON *uinact = cJSON_GetObjectItemCaseSensitive(uroot, "inact");
            int inact_val = cJSON_IsNumber(uinact) ? uinact->valueint : 0;
            if (!(inact_val & inact_deleted)) {
                index_add(un, uname->valuestring);
            }
        }
        cJSON_Delete(uroot);
    }
    closedir(d);
}


void UserDB::rebuild_index()
{
    index_.clear();

    int hi = max_id();
    for (int i = 1; i <= hi; i++) {
        auto u = get(i);
        if (u && !u->is_deleted())
            index_add(i, u->name());
    }
}


/*--- Record I/O ---*/

std::shared_ptr<const User> UserDB::get(int id)
{
    assert(id > 0);

    if (id > max_id()) return nullptr;

    char path[MAX_PATH_LEN];
    sprintf(path, "%susers/%04d.json", datadir_.c_str(), id);

    cJSON *root = read_json_file(path);
    if (!root) return nullptr;

    auto u = std::make_shared<User>(User::from_json(root));
    cJSON_Delete(root);

    return u;
}


int UserDB::store(int id, const User& u)
{
    assert(id > 0);

    char path[MAX_PATH_LEN];
    sprintf(path, "%susers/%04d.json", datadir_.c_str(), id);

    cJSON *root = u.to_json();
    int rc = write_json_file(path, root);
    cJSON_Delete(root);
    return rc;
}


/*--- Allocation ---*/

int UserDB::max_id()
{
    std::string udir = datadir_ + "users";
    DIR *d = opendir(udir.c_str());
    if (!d) return 0;

    int highest = 0;
    struct dirent *de;
    while ((de = readdir(d)) != NULL) {
        if (de->d_name[0] == '.') continue;
        if (!strstr(de->d_name, ".json")) continue;
        int n = atoi(de->d_name);
        if (n > highest) highest = n;
    }
    closedir(d);
    return highest;
}

int UserDB::next_id()
{
    return max_id() + 1;
}


/*--- Index queries ---*/

std::optional<int> UserDB::lookup(const std::string& name)
{
    /* Binary search on sorted index (same as old bsearch on smalrec[]) */
    int lo = 0, hi = (int)index_.size() - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        int cmp = strcmp(name.c_str(), index_[mid].name.c_str());
        if (cmp == 0) return index_[mid].number;
        if (cmp < 0) hi = mid - 1;
        else lo = mid + 1;
    }
    return std::nullopt;
}


std::vector<std::pair<std::string,int>> UserDB::get_matching(const std::string& substring)
{
    std::vector<std::pair<std::string,int>> results;
    /* Upper-case the search string */
    std::string upper = substring;
    for (auto& c : upper) c = toupper(c);

    for (auto& e : index_) {
        if (e.name.find(upper) != std::string::npos)
            results.push_back({e.name, e.number});
    }
    return results;
}


int UserDB::user_count() const
{
    return (int)index_.size();
}


/*--- Index mutation ---*/

void UserDB::index_add(int id, const std::string& name)
{
    assert(id > 0);
    assert(!name.empty());

    /* Insert in sorted order (binary search for position) */
    int lo = 0, hi = (int)index_.size();
    while (lo < hi) {
        int mid = (lo + hi) / 2;
        if (strcmp(name.c_str(), index_[mid].name.c_str()) > 0)
            lo = mid + 1;
        else
            hi = mid;
    }
    index_.insert(index_.begin() + lo, {name, id});
}

int UserDB::index_remove(const std::string& name)
{
    assert(!name.empty());

    for (auto it = index_.begin(); it != index_.end(); ++it) {
        if (it->name == name) {
            index_.erase(it);
            return 0;
        }
    }
    return -1;
}


/*--- String-to-id resolution ---*/

std::optional<int> UserDB::resolve(const std::string& input)
{
    if (input == "NEW") return -1;

    /* Try numeric */
    int un = atoi(input.c_str());
    if (un > 0) {
        if (un > max_id()) return std::nullopt;
        auto u = get(un);
        if (!u || u->is_deleted()) return std::nullopt;
        return un;
    }

    /* Try name lookup */
    auto r = lookup(input);
    if (!r) return std::nullopt;

    auto u = get(*r);
    if (!u || u->is_deleted()) return std::nullopt;
    return *r;
}


unsigned int UserDB::finduser(const char *s)
{
    auto r = resolve(s);
    if (!r) return 0;
    if (*r == -1) return (unsigned int)-1;  /* "NEW" */
    return *r;
}


/* finduser1() moved to bbs_ui.cpp — UI belongs in the UI layer */
