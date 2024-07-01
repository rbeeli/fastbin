#pragma once

#include <cstddef>
#include <cassert>
#include <ostream>
#include <span>
#include <cstdint>
#include <string_view>

namespace my_models
{
/**
 * Binary serializable data container generated by `fastbin`.
 * 
 * This container has variable size.
 * All setter methods starting from the first variable-sized member and afterwards MUST be called in order.
 *
 * Fields in order
 * ===============
 * - `field1::std::int32_t`   (fixed)
 * - `field2::std::string_view` (variable)
 *
 * The `finalize()` method MUST be called after all setter methods have been called.
 * 
 * It is the responsibility of the caller to ensure that the buffer is
 * large enough to hold all data.
 */
struct ChildVar
{
    std::byte* buffer{nullptr};
    size_t buffer_size{0};
    bool owns_buffer{false};

    explicit ChildVar(std::byte* buffer, size_t binary_size, bool owns_buffer) noexcept
        : buffer(buffer), buffer_size(binary_size), owns_buffer(owns_buffer)
    {
    }

    explicit ChildVar(std::span<std::byte> buffer, bool owns_buffer) noexcept
        : ChildVar(buffer.data(), buffer.size(), owns_buffer)
    {
    }

    ~ChildVar() noexcept
    {
        if (owns_buffer && buffer != nullptr)
        {
            delete[] buffer;
            buffer = nullptr;
        }
    }

    // disable copy
    ChildVar(const ChildVar&) = delete;
    ChildVar& operator=(const ChildVar&) = delete;

    // enable move
    ChildVar(ChildVar&& other) noexcept
        : buffer(other.buffer), buffer_size(other.buffer_size), owns_buffer(other.owns_buffer)
    {
        other.buffer = nullptr;
        other.buffer_size = 0;
    }
    ChildVar& operator=(ChildVar&& other) noexcept
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
        return 8;
    }

    constexpr inline size_t _field1_size_aligned() const noexcept
    {
        return 8;
    }

    // Field: field2 [std::string_view]

    inline std::string_view field2() const noexcept
    {
        size_t n_bytes = _field2_size_unaligned() - 8;
        size_t count = n_bytes;
        auto ptr = reinterpret_cast<const char*>(buffer + _field2_offset() + 8);
        return std::string_view(ptr, count);
    }

    inline void field2(const std::string_view value) noexcept
    {
        size_t offset = _field2_offset();
        size_t contents_size = value.size() * 1;
        size_t unaligned_size = 8 + contents_size;
        size_t aligned_size = (unaligned_size + 7) & ~7;
        size_t aligned_diff = aligned_size - unaligned_size;
        size_t aligned_size_high = aligned_size | (aligned_diff << 56);
        *reinterpret_cast<size_t*>(buffer + offset) = aligned_size_high;
        auto dest_ptr = reinterpret_cast<std::byte*>(buffer + offset + 8);
        auto src_ptr = reinterpret_cast<const std::byte*>(value.data());
        std::copy(src_ptr, src_ptr + contents_size, dest_ptr);
    }

    constexpr inline size_t _field2_offset() const noexcept
    {
        return 16;
    }

    constexpr inline size_t _field2_size_aligned() const noexcept
    {
        size_t stored_size = *reinterpret_cast<size_t*>(buffer + _field2_offset());
        size_t aligned_size = stored_size & 0x00FFFFFFFFFFFFFF;
        return aligned_size;
    }

    constexpr inline size_t _field2_size_unaligned() const noexcept
    {
        size_t stored_size = *reinterpret_cast<size_t*>(buffer + _field2_offset());
        size_t aligned_diff = stored_size >> 56;
        size_t aligned_size = stored_size & 0x00FFFFFFFFFFFFFF;
        return aligned_size - aligned_diff;
    }

    // --------------------------------------------------------------------------------

    constexpr inline size_t fastbin_calc_binary_size() const noexcept
    {
        return _field2_offset() + _field2_size_aligned();
    }

    /**
     * Returns the stored (aligned) binary size of the object.
     * This function should only be called after `fastbin_finalize()`.
     */
    constexpr inline size_t fastbin_binary_size() const noexcept
    {
        return *reinterpret_cast<size_t*>(buffer);
    }

    /**
     * Finalizes the object by writing the binary size to the beginning of its buffer.
     * After calling this function, the underlying buffer can be used for serialization.
     * To get the actual buffer size, call `fastbin_binary_size()`.
     */
    inline void fastbin_finalize() const noexcept
    {
        *reinterpret_cast<size_t*>(buffer) = fastbin_calc_binary_size();
    }
};
}; // namespace my_models

inline std::ostream& operator<<(std::ostream& os, const my_models::ChildVar& obj)
{
    os << "[my_models::ChildVar size=" << obj.fastbin_binary_size() << " bytes]\n";
    os << "    field1: " << obj.field1() << "\n";
    os << "    field2: " << std::string(obj.field2()) << "\n";
    return os;
}
