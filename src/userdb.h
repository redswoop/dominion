/*
 * userdb.h — User database: load/save, index, lookup
 *
 * class UserDB is a singleton managing user records stored as
 * individual JSON files (data/users/NNNN.json) with an in-memory
 * name-to-number index for fast lookup.
 *
 * Layer 4: depends on user.h (User)
 */

#ifndef _USERDB_H_
#define _USERDB_H_

#include "user.h"
#include <string>
#include <unordered_map>
#include <optional>
#include <vector>
#include <memory>

class UserDB {
public:
    static UserDB& instance();

    /* Lifecycle */
    void init(const std::string& datadir, int maxusers);
    void rebuild_index();

    /* Read — loads from disk, returns shared_ptr (null if missing/deleted) */
    std::shared_ptr<const User> get(int id);

    /* Write — serializes to disk. Returns 0=ok, -1=fail. */
    int store(int id, const User& u);

    /* Allocation */
    int max_id();          /* highest user# on disk */
    int next_id();         /* max_id() + 1 */

    /* Index queries */
    std::optional<int> lookup(const std::string& name);   /* exact, case-insensitive */
    std::vector<std::pair<std::string,int>> get_matching(const std::string& substring);
    int user_count() const;

    /* Index mutation */
    void index_add(int id, const std::string& name);
    int  index_remove(const std::string& name);  /* 0=ok, -1=not found */

    /* String-to-id resolution: handles "NEW", numeric IDs, name lookup */
    std::optional<int> resolve(const std::string& input);

private:
    UserDB() = default;
    std::string datadir_;
    int maxusers_ = 0;

    /* Sorted name-to-id index (replaces smalrec[]) */
    struct IndexEntry {
        std::string name;
        int number;
    };
    std::vector<IndexEntry> index_;
};


/* User lookup — thin adapter: optional<int> → unsigned int (0=not found) */
unsigned int finduser(const char *s);

#endif /* _USERDB_H_ */
