#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace flexql {

class MmapReader {
public:
    MmapReader();
    ~MmapReader();

    bool open(const std::string &path, std::string &error);
    void close();

    const char *data() const;
    std::size_t size() const;
    bool empty() const;
    std::string_view view() const;

private:
    int fd_;
    char *data_;
    std::size_t size_;
};

}  // namespace flexql
