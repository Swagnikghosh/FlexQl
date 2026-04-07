#pragma once

#include <string_view>

namespace flexql {

bool vectorized_equals(std::string_view lhs, std::string_view rhs);

}  // namespace flexql
