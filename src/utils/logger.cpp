#include "utils/logger.h"

#include <iostream>
#include <mutex>

namespace flexql {
namespace {
std::mutex g_log_guard;
}

void Logger::info(const std::string &text) {
    std::lock_guard<std::mutex> guard(g_log_guard);
    std::cerr << "[INFO] " << text << '\n';
}

void Logger::error(const std::string &text) {
    std::lock_guard<std::mutex> guard(g_log_guard);
    std::cerr << "[ERROR] " << text << '\n';
}

}  // namespace flexql
