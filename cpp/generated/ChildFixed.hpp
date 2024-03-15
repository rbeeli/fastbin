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
 * This container has fixed size of 16 bytes.
 * 
 * The `finalize()` method MUST be called after all setter methods have been called.
 * 
 * It is the responsibility of the caller to ensure that the buffer is
 * large enough to hold the serialized data.
 */
struct ChildFixed
{
    std::byte* buffer{nullptr};
    size_t buffer_size{0};
    bool owns_buffer{false};

    explicit ChildFixed(std::byte* buffer, size_t binary_size, bool owns_buffer) noexcept
        : buffer(buffer), buffer_size(binary_size), owns_buffer(owns_buffer)
    {
    }

    explicit ChildFixed(std::span<std::byte> buffer, bool owns_buffer) noexcept
        : ChildFixed(buffer.data(), buffer.size(), owns_buffer)
    {
    }

    ~ChildFixed() noexcept
    {
        if (owns_buffer && buffer != nullptr)
        {
            delete[] buffer;
            buffer = nullptr;
        }
    }

    // disable copy
    ChildFixed(const ChildFixed&) = delete;
    ChildFixed& operator=(const ChildFixed&) = delete;

    // enable move
    ChildFixed(ChildFixed&& other) noexcept
        : buffer(other.buffer), buffer_size(other.buffer_size), owns_buffer(other.owns_buffer)
    {
        other.buffer = nullptr;
        other.buffer_size = 0;
    }
    ChildFixed& operator=(ChildFixed&& other) noexcept
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
        return 16;
    }

    inline std::int32_t field1() const noexcept
    {
        return *reinterpret_cast<const std::int32_t*>(buffer + fastbin_field1_offset());
    }

    inline void field1(const std::int32_t value) noexcept
    {
        *reinterpret_cast<std::int32_t*>(buffer + fastbin_field1_offset()) = value;
    }

    constexpr inline size_t fastbin_field1_offset() const noexcept
    {
        return 0;
    }

    constexpr inline size_t fastbin_field1_size() const noexcept
    {
        return 8;
    }

    inline std::int32_t field2() const noexcept
    {
        return *reinterpret_cast<const std::int32_t*>(buffer + fastbin_field2_offset());
    }

    inline void field2(const std::int32_t value) noexcept
    {
        *reinterpret_cast<std::int32_t*>(buffer + fastbin_field2_offset()) = value;
    }

    constexpr inline size_t fastbin_field2_offset() const noexcept
    {
        return 8;
    }

    constexpr inline size_t fastbin_field2_size() const noexcept
    {
        return 8;
    }

    constexpr inline size_t fastbin_compute_binary_size() const noexcept
    {
        return 16;
    }

    inline void fastbin_finalize() const noexcept
    {
    }
};
}; // namespace my_models

inline std::ostream& operator<<(std::ostream& os, const my_models::ChildFixed& obj)
{
    os << "[my_models::ChildFixed size=" << obj.fastbin_binary_size() << " bytes]\n";
    os << "    field1: " << obj.field1() << "\n";
    os << "    field2: " << obj.field2() << "\n";
    return os;
}
