#pragma once

#include <ctime>
#include <string>
#include <string_view>
#include <vector>

#include "common/types.h"

namespace flexql {

// Zero-copy view into mmap memory — values are string_views into the mapped
// region and are valid only while the MmapReader that owns the memory is open.
struct RowView {
    std::vector<std::string_view> values;
    std::time_t expiration = 0;
};

std::string serialize_row(const Row &row);
bool deserialize_row(std::string_view payload, Row &row, std::string &error);
bool deserialize_row(const std::string &line, Row &row, std::string &error);
bool read_row_record(std::string_view data, std::size_t offset, Row &row, std::size_t &next_offset, std::string &error);
bool read_row_record_view(std::string_view data, std::size_t offset, RowView &row, std::size_t &next_offset, std::string &error);

// Convert a RowView to an owning Row (copies strings out of the mmap).
inline Row row_from_view(const RowView &rv) {
    Row r;
    r.expiration = rv.expiration;
    r.values.reserve(rv.values.size());
    for (const auto &sv : rv.values) {
        r.values.emplace_back(sv);
    }
    return r;
}

}  // namespace flexql
