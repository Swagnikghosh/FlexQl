#pragma once

#include <list>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <vector>

#include "common/types.h"

namespace flexql {

// Segmented LRU cache with two zones:
//   probation  — cold segment; new entries land here, evicted first
//   protected  — hot segment; entries promoted here on second access
//
// This models temporal locality: frequently reused query results survive
// longer than one-off results that happen to be recent.
class LruCache {
public:
    explicit LruCache(std::size_t capacity = 256);

    bool get(const std::string &key, QueryResult &value);
    void put(const std::string &key, const QueryResult &value, const std::vector<std::string> &dependencies = {});
    void clear();
    void invalidate_table(const std::string &table_key);

private:
    using Entry = std::pair<std::string, QueryResult>;

    // Tracks which list an entry lives in and its iterator within that list.
    struct Slot {
        std::list<Entry>::iterator it;
        bool in_protected = false;
    };

    std::size_t probation_capacity_;   // capacity / 3
    std::size_t protected_capacity_;   // capacity * 2 / 3

    std::list<Entry> probation_;   // cold — new entries, evicted first
    std::list<Entry> protected_;   // hot  — promoted on second access
    std::unordered_map<std::string, Slot> map_;
    std::unordered_map<std::string, std::unordered_set<std::string>> dependency_map_;
    mutable std::shared_mutex mutex_;
};

}  // namespace flexql
