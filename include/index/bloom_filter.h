#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace flexql {

class BloomFilter {
public:
    BloomFilter(std::size_t bit_count = 8192, std::size_t hash_count = 3);

    void clear();
    void reset(std::size_t bit_count, std::size_t hash_count);
    void add(const std::string &value);
    bool might_contain(const std::string &value) const;

    bool save(const std::string &path, std::string &error) const;
    bool load(const std::string &path, std::string &error);

private:
    std::uint64_t hash_value(const std::string &value, std::uint64_t seed) const;
    std::size_t bit_count_;
    std::size_t hash_count_;
    std::vector<std::uint8_t> bits_;
};

}  // namespace flexql
