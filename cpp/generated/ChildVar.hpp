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
            delete[] buffer;
            buffer = other.buffer;
            buffer_size = other.buffer_size;
            owns_buffer = other.owns_buffer;
            other.buffer = nullptr;
            other.buffer_size = 0;
        }
        return *this;
    }

    constexpr inline size_t fastbin_binary_size() const noexcept
    {
        return *reinterpret_cast<size_t*>(buffer);
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
        return 8;
    }

    constexpr inline size_t fastbin_field1_size() const noexcept
    {
        return 8;
    }

    inline std::string_view field2() const noexcept
    {
        size_t n_bytes = fastbin_field2_size_unaligned() - 8;
        size_t count = n_bytes;
        auto ptr = reinterpret_cast<const char*>(buffer + fastbin_field2_offset() + 8);
        return std::string_view(ptr, count);
    }

    inline void field2(const std::string_view value) noexcept
    {
        size_t offset = fastbin_field2_offset();
        size_t elements_size = value.size() * 1;
        size_t unaligned_size = 8 + elements_size;
        size_t aligned_size = (unaligned_size + 7) & ~7;
        size_t aligned_diff = aligned_size - unaligned_size;
        size_t aligned_size_high = aligned_size | (aligned_diff << 56);
        *reinterpret_cast<size_t*>(buffer + offset) = aligned_size_high;
        auto dest_ptr = reinterpret_cast<std::byte*>(buffer + offset + 8);
        auto src_ptr = reinterpret_cast<const std::byte*>(value.data());
        std::copy(src_ptr, src_ptr + elements_size, dest_ptr);
    }

    constexpr inline size_t fastbin_field2_offset() const noexcept
    {
        return 16;
    }

    constexpr inline size_t fastbin_field2_size() const noexcept
    {
        size_t stored_size = *reinterpret_cast<size_t*>(buffer + fastbin_field2_offset());
        size_t aligned_size = stored_size & 0x00FFFFFFFFFFFFFF;
        return aligned_size;
    }

    constexpr inline size_t fastbin_field2_size_unaligned() const noexcept
    {
        size_t stored_size = *reinterpret_cast<size_t*>(buffer + fastbin_field2_offset());
        size_t aligned_diff = stored_size >> 56;
        size_t aligned_size = stored_size & 0x00FFFFFFFFFFFFFF;
        return aligned_size - aligned_diff;
    }

    constexpr inline size_t fastbin_compute_binary_size() const noexcept
    {
        return fastbin_field2_offset() + fastbin_field2_size();
    }

    inline void fastbin_finalize() const noexcept
    {
        *reinterpret_cast<size_t*>(buffer) = fastbin_compute_binary_size();
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
