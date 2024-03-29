#pragma once

#include <cstddef>
#include <cassert>
#include <ostream>
#include <span>
#include <cstdint>

namespace my_models
{
/**
 * Binary serializable data container.
 * 
 * This container has variable size.
 * 
 * Setter methods from the first variable-sized field onwards
 * MUST be called in order.
 * 
 * The `finalize()` method MUST be called after all setter methods have been called.
 * 
 * It is the responsibility of the caller to ensure that the buffer is
 * large enough to hold the serialized data.
 */
struct StructVector
{
    std::byte* buffer{nullptr};
    size_t buffer_size{0};
    bool owns_buffer{false};

    explicit StructVector(std::byte* buffer, size_t binary_size, bool owns_buffer) noexcept
        : buffer(buffer), buffer_size(binary_size), owns_buffer(owns_buffer)
    {
    }

    explicit StructVector(std::span<std::byte> buffer, bool owns_buffer) noexcept
        : StructVector(buffer.data(), buffer.size(), owns_buffer)
    {
    }

    ~StructVector() noexcept
    {
        if (owns_buffer && buffer != nullptr)
        {
            delete[] buffer;
            buffer = nullptr;
        }
    }

    // disable copy
    StructVector(const StructVector&) = delete;
    StructVector& operator=(const StructVector&) = delete;

    // enable move
    StructVector(StructVector&& other) noexcept
        : buffer(other.buffer), buffer_size(other.buffer_size), owns_buffer(other.owns_buffer)
    {
        other.buffer = nullptr;
        other.buffer_size = 0;
    }
    StructVector& operator=(StructVector&& other) noexcept
    {
        if (this != &other)
        {
            if (owns_buffer && buffer != nullptr)
               delete[] buffer;
            buffer = other.buffer;
            buffer_size = other.buffer_size;
            owns_buffer = other.owns_buffer;
            other.buffer = nullptr;
            other.buffer_size = 0;
            other.owns_buffer = false;
        }
        return *this;
    }

    constexpr inline size_t fastbin_binary_size() const noexcept
    {
        return *reinterpret_cast<size_t*>(buffer);
    }

    inline std::span<ChildFixed> values() const noexcept
    {
        size_t n_bytes = fastbin_values_size_unaligned() - 8;
        size_t count = n_bytes / 16;
        auto ptr = reinterpret_cast<ChildFixed*>(buffer + fastbin_values_offset() + 8);
        return std::span<ChildFixed>(ptr, count);
    }

    inline void values(const std::span<ChildFixed> value) noexcept
    {
        size_t offset = fastbin_values_offset();
        size_t elements_size = value.size() * 16;
        *reinterpret_cast<size_t*>(buffer + offset) = 8 + elements_size;
        auto dest_ptr = reinterpret_cast<std::byte*>(buffer + offset + 8);
        auto src_ptr = reinterpret_cast<const std::byte*>(value.data());
        std::copy(src_ptr, src_ptr + elements_size, dest_ptr);
    }

    constexpr inline size_t fastbin_values_offset() const noexcept
    {
        return 8;
    }

    constexpr inline size_t fastbin_values_size() const noexcept
    {
        size_t stored_size = *reinterpret_cast<size_t*>(buffer + fastbin_values_offset());
        return stored_size;
    }

    constexpr inline size_t fastbin_values_size_unaligned() const noexcept
    {
        size_t stored_size = *reinterpret_cast<size_t*>(buffer + fastbin_values_offset());
        return stored_size;
    }

    inline std::uint32_t count() const noexcept
    {
        return *reinterpret_cast<const std::uint32_t*>(buffer + fastbin_count_offset());
    }

    inline void count(const std::uint32_t value) noexcept
    {
        *reinterpret_cast<std::uint32_t*>(buffer + fastbin_count_offset()) = value;
    }

    constexpr inline size_t fastbin_count_offset() const noexcept
    {
        return fastbin_values_offset() + fastbin_values_size();
    }

    constexpr inline size_t fastbin_count_size() const noexcept
    {
        return 8;
    }

    constexpr inline size_t fastbin_compute_binary_size() const noexcept
    {
        return fastbin_count_offset() + fastbin_count_size();
    }

    inline void fastbin_finalize() const noexcept
    {
        *reinterpret_cast<size_t*>(buffer) = fastbin_compute_binary_size();
    }
};
}; // namespace my_models

inline std::ostream& operator<<(std::ostream& os, const my_models::StructVector& obj)
{
    os << "[my_models::StructVector size=" << obj.fastbin_binary_size() << " bytes]\n";
    os << "    values: " << "[vector<struct:ChildFixed> count=" << obj.values().size() << "]" << "\n";
    os << "    count: " << obj.count() << "\n";
    return os;
}
