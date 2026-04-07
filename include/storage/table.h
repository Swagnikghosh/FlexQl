#pragma once

#include <functional>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_set>
#include <vector>

#include "common/types.h"
#include "concurrency/lockfree_append.h"
#include "concurrency/lock_manager.h"
#include "index/btree_index.h"
#include "storage/mmap_reader.h"
#include "storage/row.h"
#include "storage/schema.h"
#include "utils/helpers.h"

namespace flexql {

class Table {
public:
    Table(const std::string &root, std::string database_name, std::string table_name, LockManager &lock_manager);
    ~Table();

    bool create(const std::vector<ColumnDef> &columns, std::string &error);
    bool drop(std::string &error);
    bool load(std::string &error);
    bool insert_row(const std::vector<std::string> &values, std::time_t expiration, std::string &error);
    bool insert_rows(
        const std::vector<std::vector<std::string>> &rows,
        std::time_t expiration,
        std::string &error);
    bool read_all(std::vector<Row> &rows, std::string &error);
    bool read_by_primary_key(const std::string &key, Row &row, std::string &error);
    bool scan_rows(const std::function<bool(const Row &)> &visitor, std::string &error);
    bool scan_rows_matching(const Condition &condition, const std::function<bool(const Row &)> &visitor, std::string &error);

    // Template overloads: the visitor type is known at each call site so the
    // compiler can inline the lambda body directly — no std::function heap
    // allocation or virtual dispatch per row.
    template <typename Visitor>
    bool scan_rows_t(Visitor &&visitor, std::string &error) {
        {
            std::shared_lock<std::shared_mutex> slock(mutex());
            if (loaded_ && !reader_is_stale()) {
                return for_each_row_view(reader_.view(), std::forward<Visitor>(visitor), error);
            }
        }
        {
            std::unique_lock<std::shared_mutex> ulock(mutex());
            if (!ensure_loaded_locked(error)) return false;
            if (reader_is_stale() && !refresh_reader_locked(error)) return false;
        }
        std::shared_lock<std::shared_mutex> slock(mutex());
        return for_each_row_view(reader_.view(), std::forward<Visitor>(visitor), error);
    }

    // scan_rows_matching_t: visitor receives a RowView — no allocation for
    // non-matching rows.
    template <typename Visitor>
    bool scan_rows_matching_t(const Condition &condition, Visitor &&visitor, std::string &error) {
        if (!condition.enabled) {
            return scan_rows_t(std::forward<Visitor>(visitor), error);
        }
        std::string col = trim(condition.column);
        const auto dot = col.find('.');
        if (dot != std::string::npos) col = col.substr(dot + 1);
        const int col_idx = schema_.column_index(col);
        if (col_idx < 0) { error = "unknown WHERE column"; return false; }
        std::string needle = trim(condition.value);
        if (needle.size() >= 2 && needle.front() == '\'' && needle.back() == '\'') {
            needle = needle.substr(1, needle.size() - 2);
        }
        const DataType type = schema_.columns()[static_cast<std::size_t>(col_idx)].type;
        return scan_rows_t(
            [&](const RowView &rv) {
                if (col_idx < static_cast<int>(rv.values.size()) &&
                    compare_values(std::string(rv.values[static_cast<std::size_t>(col_idx)]), needle, type, condition.op)) {
                    return visitor(rv);
                }
                return true;
            },
            error);
    }
    const Schema &schema() const;
    std::vector<Row> filter_rows(const Condition &condition, std::string &error);
    BTreeIndex &index();
    std::shared_mutex &mutex();
    std::uintmax_t data_size(std::string &error) const;

private:
    // Hot scan loop: parse each record as a zero-copy RowView (string_views into
    // the mmap).  Visitors receive a RowView; call row_from_view() only for rows
    // that actually need to be materialised (e.g. after passing a WHERE filter).
    template <typename Visitor>
    static bool for_each_row_view(std::string_view data, Visitor &&visitor, std::string &error) {
        std::size_t offset = 0;
        while (offset < data.size()) {
            RowView rv;
            std::size_t next = 0;
            if (!read_row_record_view(data, offset, rv, next, error)) return false;
            if (!visitor(rv)) break;
            offset = next;
        }
        return true;
    }

    bool validate_row(const std::vector<std::string> &values, std::string &error) const;
    bool ensure_loaded_locked(std::string &error);
    bool open_append_file_locked(std::string &error);
    bool refresh_reader_locked(std::string &error);
    bool reader_is_stale() const;
    std::string database_root() const;
    std::string table_path() const;

    std::string root_;
    std::string database_name_;
    std::string name_;
    Schema schema_;
    BTreeIndex index_;
    LockManager &lock_manager_;
    bool loaded_ = false;
    LockFreeAppendFile append_file_;
    MmapReader reader_;
    mutable std::mutex write_state_mutex_;
    // Serialises concurrent delta log appends that happen outside write_state_mutex_.
    std::mutex index_flush_mutex_;
    std::unordered_set<std::string> inflight_primary_keys_;
};

}  // namespace flexql
