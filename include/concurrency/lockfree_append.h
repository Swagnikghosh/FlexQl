#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>

namespace flexql {

class LockFreeAppendFile {
public:
    LockFreeAppendFile() = default;
    ~LockFreeAppendFile();

    LockFreeAppendFile(const LockFreeAppendFile &) = delete;
    LockFreeAppendFile &operator=(const LockFreeAppendFile &) = delete;

    bool open(const std::string &path, std::string &error);
    void close();
    bool is_open() const;

    std::uint64_t reserve(std::size_t bytes);
    bool write_at(std::uint64_t offset, std::string_view data, std::string &error);
    std::uint64_t size() const;

private:
    void maybe_extend(std::uint64_t required_end);

    int fd_ = -1;
    std::atomic<std::uint64_t> append_offset_{0};
    std::atomic<std::uint64_t> allocated_size_{0};
    std::mutex fallocate_mutex_;
};

}  // namespace flexql
