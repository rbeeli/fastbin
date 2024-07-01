#pragma once

#include <cstddef>
#include <cassert>
#include <ostream>
#include <span>
#include <cstdint>

namespace my_models
{
/**
 * Binary serializable data container generated by `fastbin`.
 * 
 * This container has fixed size of 16 bytes.
 *
 * The `finalize()` method MUST be called after all setter methods have been called.
 * 
 * It is the responsibility of the caller to ensure that the buffer is
 * large enough to hold all data.
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

    // Field: field1 [std::int32_t]

    inline std::int32_t field1() const noexcept
    {
        return *reinterpret_cast<const std::int32_t*>(buffer + _field1_offset());
    }

    inline void field1(const std::int32_t value) noexcept
    {
        *reinterpret_cast<std::int32_t*>(buffer + _field1_offset()) = value;
    }

    constexpr inline size_t _field1_offset() const noexcept
    {
        return 0;
    }

    constexpr inline size_t _field1_size_aligned() const noexcept
    {
        return 8;
    }

    // Field: field2 [std::int32_t]

    inline std::int32_t field2() const noexcept
    {
        return *reinterpret_cast<const std::int32_t*>(buffer + _field2_offset());
    }

    inline void field2(const std::int32_t value) noexcept
    {
        *reinterpret_cast<std::int32_t*>(buffer + _field2_offset()) = value;
    }

    constexpr inline size_t _field2_offset() const noexcept
    {
        return 8;
    }

    constexpr inline size_t _field2_size_aligned() const noexcept
    {
        return 8;
    }

    // --------------------------------------------------------------------------------

    constexpr inline size_t fastbin_calc_binary_size() const noexcept
    {
        return 16;
    }

    /**
     * Returns the stored (aligned) binary size of the object.
     * This function should only be called after `fastbin_finalize()`.
     */
    constexpr inline size_t fastbin_binary_size() const noexcept
    {
        return 16;
    }

    /**
     * Finalizes the object by writing the binary size to the beginning of its buffer.
     * After calling this function, the underlying buffer can be used for serialization.
     * To get the actual buffer size, call `fastbin_binary_size()`.
     */
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
