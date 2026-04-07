#pragma once

#include <string>
#include <vector>

#include "common/types.h"

namespace flexql {

class Schema {
public:
    Schema() = default;
    Schema(std::string database_name, std::string table_name);

    const std::string &database_name() const;
    const std::string &table_name() const;
    const std::vector<ColumnDef> &columns() const;
    void set_columns(std::vector<ColumnDef> columns);
    int primary_key_index() const;
    std::string primary_key_name() const;

    int column_index(const std::string &name) const;
    bool save(const std::string &root, std::string &error) const;
    bool load(const std::string &root, const std::string &database_name, const std::string &table_name, std::string &error);

private:
    std::string database_name_;
    std::string table_name_;
    std::vector<ColumnDef> columns_;
};

}  // namespace flexql
