#include "expiration/ttl_manager.h"

#include <ctime>

namespace flexql {

TtlManager::TtlManager(std::time_t ttl_seconds)
    : default_ttl_seconds_(ttl_seconds) {}

std::time_t TtlManager::compute_expiration() const {
    const std::time_t current_time = std::time(nullptr);
    return current_time + default_ttl_seconds_;
}

bool TtlManager::is_expired(std::time_t expires_at) const {
    if (expires_at == 0) {
        return false;
    }

    const std::time_t current_time = std::time(nullptr);
    return current_time > expires_at;
}

}  // namespace flexql
