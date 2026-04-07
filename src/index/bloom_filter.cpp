#include "index/bloom_filter.h"

#include <fstream>

namespace flexql {

namespace {

constexpr std::uint64_t kFnvOffset = 1469598103934665603ULL;
constexpr std::uint64_t kFnvPrime = 1099511628211ULL;

}  // namespace

BloomFilter::BloomFilter(std::size_t initial_bit_count, std::size_t initial_hash_count) {
    reset(initial_bit_count, initial_hash_count);
}

void BloomFilter::clear() {
    std::fill(bits_.begin(), bits_.end(), 0);
}

void BloomFilter::reset(std::size_t requested_bits, std::size_t requested_hashes) {
    bit_count_ = requested_bits == 0 ? 8 : requested_bits;
    hash_count_ = requested_hashes == 0 ? 1 : requested_hashes;
    bits_.assign((bit_count_ + 7) / 8, 0);
}

void BloomFilter::add(const std::string &key) {
    for (std::size_t hash_index = 0; hash_index < hash_count_; ++hash_index) {
        const std::size_t bit_position = static_cast<std::size_t>(
            hash_value(key, static_cast<std::uint64_t>(hash_index + 1)) % bit_count_);
        bits_[bit_position / 8] |= static_cast<std::uint8_t>(1U << (bit_position % 8));
    }
}

bool BloomFilter::might_contain(const std::string &key) const {
    for (std::size_t hash_index = 0; hash_index < hash_count_; ++hash_index) {
        const std::size_t bit_position = static_cast<std::size_t>(
            hash_value(key, static_cast<std::uint64_t>(hash_index + 1)) % bit_count_);
        if ((bits_[bit_position / 8] &
             static_cast<std::uint8_t>(1U << (bit_position % 8))) == 0) {
            return false;
        }
    }
    return true;
}

bool BloomFilter::save(const std::string &file_path, std::string &failure_reason) const {
    std::ofstream output_stream(file_path, std::ios::binary | std::ios::trunc);
    if (!output_stream) {
        failure_reason = "failed to write bloom filter";
        return false;
    }

    const std::uint64_t stored_bit_count = static_cast<std::uint64_t>(bit_count_);
    const std::uint64_t stored_hash_count = static_cast<std::uint64_t>(hash_count_);
    const std::uint64_t stored_byte_count = static_cast<std::uint64_t>(bits_.size());
    output_stream.write(reinterpret_cast<const char *>(&stored_bit_count), sizeof(stored_bit_count));
    output_stream.write(reinterpret_cast<const char *>(&stored_hash_count), sizeof(stored_hash_count));
    output_stream.write(reinterpret_cast<const char *>(&stored_byte_count), sizeof(stored_byte_count));
    output_stream.write(
        reinterpret_cast<const char *>(bits_.data()),
        static_cast<std::streamsize>(bits_.size()));
    return static_cast<bool>(output_stream);
}

bool BloomFilter::load(const std::string &file_path, std::string &failure_reason) {
    std::ifstream input_stream(file_path, std::ios::binary);
    if (!input_stream) {
        failure_reason = "bloom filter not found";
        return false;
    }

    std::uint64_t stored_bit_count = 0;
    std::uint64_t stored_hash_count = 0;
    std::uint64_t stored_byte_count = 0;
    input_stream.read(reinterpret_cast<char *>(&stored_bit_count), sizeof(stored_bit_count));
    input_stream.read(reinterpret_cast<char *>(&stored_hash_count), sizeof(stored_hash_count));
    input_stream.read(reinterpret_cast<char *>(&stored_byte_count), sizeof(stored_byte_count));
    if (!input_stream) {
        failure_reason = "invalid bloom filter";
        return false;
    }

    bit_count_ = static_cast<std::size_t>(stored_bit_count);
    hash_count_ = static_cast<std::size_t>(stored_hash_count);
    bits_.assign(static_cast<std::size_t>(stored_byte_count), 0);
    input_stream.read(reinterpret_cast<char *>(bits_.data()), static_cast<std::streamsize>(bits_.size()));
    if (!input_stream) {
        failure_reason = "invalid bloom filter";
        return false;
    }
    return true;
}

std::uint64_t BloomFilter::hash_value(const std::string &key, std::uint64_t salt) const {
    std::uint64_t hash_state = kFnvOffset ^ (salt * 0x9E3779B185EBCA87ULL);
    for (const unsigned char byte : key) {
        hash_state ^= static_cast<std::uint64_t>(byte);
        hash_state *= kFnvPrime;
    }
    return hash_state;
}

}  // namespace flexql
