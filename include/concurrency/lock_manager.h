#pragma once

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>

namespace flexql {

class LockManager {
public:
    std::shared_mutex &table_mutex(const std::string &table_name);

private:
    std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<std::shared_mutex>> table_locks_;
};

}  // namespace flexql
