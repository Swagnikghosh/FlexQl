#pragma once

#include <string>
#include <utility>
#include <string>
#include <vector>

#include "common/types.h"
#include "index/bloom_filter.h"

namespace flexql {

class BTreeIndex {
public:
    BTreeIndex(std::string database_name = "", std::string table_name = "");

    void set_location(const std::string &database_name, const std::string &table_name);
    bool load(const std::string &root, std::string &error);
    // Hot-path flush: appends only the pending delta log entries to disk (O(k)).
    // Never writes the full snapshot. Safe to call without any lock.
    bool save(const std::string &root, std::string &error);
    // Shutdown-only: writes full snapshot + rebuilds and saves bloom filter (O(N)).
    bool compact(const std::string &root, std::string &error);
    // Drains pending_delta_ into `out` and resets the flush counter.
    // Must be called under write_state_mutex_.
    void drain_delta(std::vector<std::pair<std::string, std::uint64_t>> &out);
    // Appends a pre-drained delta batch to the on-disk delta log.
    // Operates only on the caller-supplied vector — no shared mutable state.
    bool write_delta_entries(const std::string &root,
                             const std::vector<std::pair<std::string, std::uint64_t>> &entries,
                             std::string &error) const;
    void put(const std::string &key, std::uint64_t offset);
    bool get(const std::string &key, std::uint64_t &offset) const;
    const IndexMap &entries() const;
    bool needs_flush() const;
    void mark_flushed();

private:
    std::string index_dir(const std::string &root) const;
    std::string snapshot_path(const std::string &root) const;
    std::string delta_path(const std::string &root) const;
    std::string bloom_path(const std::string &root) const;
    bool load_snapshot(const std::string &path, std::string &error);
    bool load_delta(const std::string &path, std::string &error);
    bool load_legacy_index(const std::string &path, std::string &error);
    bool save_snapshot(const std::string &path, std::string &error) const;
    bool append_delta(const std::string &path, const std::vector<std::pair<std::string, std::uint64_t>> &updates, std::string &error) const;
    void rebuild_bloom_filter();

    std::string database_name_;
    std::string table_name_;
    IndexMap entries_;
    std::vector<std::pair<std::string, std::uint64_t>> pending_delta_;
    BloomFilter bloom_filter_;
    bool dirty_ = false;
    std::size_t pending_updates_ = 0;
};

}  // namespace flexql
