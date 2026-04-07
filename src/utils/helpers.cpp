#include "utils/helpers.h"

#include <algorithm>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace flexql {

std::string trim(const std::string &input) {
    std::size_t first_non_space = 0;
    while (first_non_space < input.size() &&
           std::isspace(static_cast<unsigned char>(input[first_non_space]))) {
        ++first_non_space;
    }

    std::size_t last_non_space = input.size();
    while (last_non_space > first_non_space &&
           std::isspace(static_cast<unsigned char>(input[last_non_space - 1]))) {
        --last_non_space;
    }

    return input.substr(first_non_space, last_non_space - first_non_space);
}

std::string to_upper(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });
    return text;
}

std::string normalize_identifier(const std::string &identifier) {
    std::string normalized_name = trim(identifier);
    std::transform(
        normalized_name.begin(),
        normalized_name.end(),
        normalized_name.begin(),
        [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
    return normalized_name;
}

std::vector<std::string> split_csv(const std::string &input) {
    std::vector<std::string> fragments;
    std::string current_fragment;
    bool inside_quotes = false;
    int parenthesis_depth = 0;

    for (char current_char : input) {
        if (current_char == '\'') {
            inside_quotes = !inside_quotes;
            current_fragment.push_back(current_char);
            continue;
        }
        if (!inside_quotes) {
            if (current_char == '(') {
                ++parenthesis_depth;
            } else if (current_char == ')' && parenthesis_depth > 0) {
                --parenthesis_depth;
            }
        }
        if (current_char == ',' && !inside_quotes && parenthesis_depth == 0) {
            fragments.push_back(trim(current_fragment));
            current_fragment.clear();
            continue;
        }
        current_fragment.push_back(current_char);
    }
    if (!current_fragment.empty()) {
        fragments.push_back(trim(current_fragment));
    }
    return fragments;
}

std::vector<std::string> split(const std::string &input, char delimiter) {
    std::vector<std::string> pieces;
    std::stringstream stream(input);
    std::string piece;
    while (std::getline(stream, piece, delimiter)) {
        pieces.push_back(piece);
    }
    return pieces;
}

std::string join(const std::vector<std::string> &items, const std::string &delimiter) {
    std::ostringstream builder;
    bool first_item = true;
    for (const auto &item : items) {
        if (!first_item) {
            builder << delimiter;
        }
        builder << item;
        first_item = false;
    }
    return builder.str();
}

std::string normalize_sql(const std::string &sql) {
    std::string cleaned_sql = trim(sql);
    while (!cleaned_sql.empty() && cleaned_sql.back() == ';') {
        cleaned_sql.pop_back();
    }
    return cleaned_sql;
}

bool validate_value_for_type(const std::string &value, DataType type) {
    std::string normalized_value = trim(value);
    if (normalized_value.size() >= 2 &&
        normalized_value.front() == '\'' &&
        normalized_value.back() == '\'') {
        normalized_value = normalized_value.substr(1, normalized_value.size() - 2);
    }

    try {
        switch (type) {
            case DataType::Int: {
                std::size_t parsed_length = 0;
                std::stoi(normalized_value, &parsed_length);
                return parsed_length == normalized_value.size();
            }
            case DataType::Decimal: {
                std::size_t parsed_length = 0;
                std::stod(normalized_value, &parsed_length);
                return parsed_length == normalized_value.size();
            }
            case DataType::Varchar:
                return true;
            case DataType::Datetime: {
                std::time_t parsed_timestamp = 0;
                return parse_datetime_value(normalized_value, parsed_timestamp);
            }
        }
    } catch (...) {
        return false;
    }
    return false;
}

std::string data_type_to_string(DataType type) {
    switch (type) {
        case DataType::Int:
            return "INT";
        case DataType::Decimal:
            return "DECIMAL";
        case DataType::Varchar:
            return "VARCHAR";
        case DataType::Datetime:
            return "DATETIME";
    }
    return "UNKNOWN";
}

bool data_type_from_string(const std::string &value, DataType &type) {
    const std::string upper_name = to_upper(trim(value));
    if (upper_name == "INT" || upper_name == "INTEGER") {
        type = DataType::Int;
        return true;
    }
    if (upper_name == "DECIMAL" || upper_name == "FLOAT" || upper_name == "DOUBLE") {
        type = DataType::Decimal;
        return true;
    }
    if (upper_name == "VARCHAR" || upper_name == "TEXT") {
        type = DataType::Varchar;
        return true;
    }
    if (upper_name == "DATETIME") {
        type = DataType::Datetime;
        return true;
    }
    return false;
}

std::string escape_field(const std::string &value) {
    std::string escaped_value;
    escaped_value.reserve(value.size());
    for (char current_char : value) {
        if (current_char == '\\' || current_char == '|') {
            escaped_value.push_back('\\');
        }
        escaped_value.push_back(current_char);
    }
    return escaped_value;
}

std::string unescape_field(const std::string &value) {
    std::string plain_value;
    plain_value.reserve(value.size());
    bool is_escaped = false;
    for (char current_char : value) {
        if (is_escaped) {
            plain_value.push_back(current_char);
            is_escaped = false;
        } else if (current_char == '\\') {
            is_escaped = true;
        } else {
            plain_value.push_back(current_char);
        }
    }
    return plain_value;
}

bool parse_datetime_value(const std::string &value, std::time_t &timestamp) {
    const std::string cleaned_value = trim(value);
    if (cleaned_value.empty()) {
        return false;
    }

    const auto try_parse = [&](const char *format_string) -> bool {
        std::tm parsed_tm {};
        std::istringstream input_stream(cleaned_value);
        input_stream >> std::get_time(&parsed_tm, format_string);
        if (input_stream.fail()) {
            return false;
        }

        const std::time_t parsed_timestamp = std::mktime(&parsed_tm);
        if (parsed_timestamp == static_cast<std::time_t>(-1)) {
            return false;
        }

        timestamp = parsed_timestamp;
        return true;
    };

    if (try_parse("%Y-%m-%d %H:%M:%S") ||
        try_parse("%Y-%m-%dT%H:%M:%S") ||
        try_parse("%Y-%m-%d")) {
        return true;
    }

    try {
        std::size_t parsed_length = 0;
        const long long raw_timestamp = std::stoll(cleaned_value, &parsed_length);
        if (parsed_length != cleaned_value.size()) {
            return false;
        }

        timestamp = static_cast<std::time_t>(raw_timestamp);
        return true;
    } catch (...) {
        return false;
    }
}

bool compare_values(
    const std::string &lhs,
    const std::string &rhs,
    DataType type,
    ComparisonOp op) {
    const auto compare_ordering = [&](auto left_value, auto right_value) {
        switch (op) {
            case ComparisonOp::Equal:
                return left_value == right_value;
            case ComparisonOp::NotEqual:
                return left_value != right_value;
            case ComparisonOp::Greater:
                return left_value > right_value;
            case ComparisonOp::GreaterEqual:
                return left_value >= right_value;
            case ComparisonOp::Less:
                return left_value < right_value;
            case ComparisonOp::LessEqual:
                return left_value <= right_value;
        }
        return false;
    };

    try {
        switch (type) {
            case DataType::Int:
                return compare_ordering(std::stoll(lhs), std::stoll(rhs));
            case DataType::Decimal:
                return compare_ordering(std::stod(lhs), std::stod(rhs));
            case DataType::Datetime: {
                std::time_t left_timestamp = 0;
                std::time_t right_timestamp = 0;
                if (!parse_datetime_value(lhs, left_timestamp) ||
                    !parse_datetime_value(rhs, right_timestamp)) {
                    return false;
                }
                return compare_ordering(left_timestamp, right_timestamp);
            }
            case DataType::Varchar:
                return compare_ordering(lhs, rhs);
        }
    } catch (...) {
        return false;
    }
    return false;
}

}  // namespace flexql
