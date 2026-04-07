#pragma once

#include <ctime>
#include <string>
#include <vector>

#include "common/types.h"

namespace flexql {

std::string trim(const std::string &input);
std::string to_upper(std::string input);
std::string normalize_identifier(const std::string &input);
std::vector<std::string> split_csv(const std::string &input);
std::vector<std::string> split(const std::string &input, char delimiter);
std::string join(const std::vector<std::string> &items, const std::string &delimiter);
std::string normalize_sql(const std::string &sql);
bool validate_value_for_type(const std::string &value, DataType type);
std::string data_type_to_string(DataType type);
bool data_type_from_string(const std::string &value, DataType &type);
std::string escape_field(const std::string &value);
std::string unescape_field(const std::string &value);
bool parse_datetime_value(const std::string &value, std::time_t &timestamp);
bool compare_values(const std::string &lhs, const std::string &rhs, DataType type, ComparisonOp op);

}  // namespace flexql
