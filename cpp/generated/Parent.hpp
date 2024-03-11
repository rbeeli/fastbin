#pragma once

#include <cstddef>
#include <cassert>
#include <ostream>
#include <span>
#include <cstdint>
#include <string_view>
#include "ChildFixed.hpp"
#include "ChildVar.hpp"

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
struct Parent
{
    std::byte* buffer{nullptr};
    size_t buffer_size{0};
    bool owns_buffer{false};

    explicit Parent(std::byte* buffer, size_t binary_size, bool owns_buffer) noexcept
        : buffer(buffer), buffer_size(binary_size), owns_buffer(owns_buffer)
    {
    }

    ~Parent() noexcept
    {
        if (owns_buffer && buffer != nullptr)
        {
            delete[] buffer;
            buffer = nullptr;
        }
    }

    // disable copy
    Parent(const Parent&) = delete;
    Parent& operator=(const Parent&) = delete;

    // enable move
    Parent(Parent&& other) noexcept
        : buffer(other.buffer), buffer_size(other.buffer_size), owns_buffer(other.owns_buffer)
    {
        other.buffer = nullptr;
        other.buffer_size = 0;
    }
    Parent& operator=(Parent&& other) noexcept
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

    inline ChildFixed child1() const noexcept
    {
        auto ptr = buffer + fastbin_child1_offset();
        return ChildFixed(ptr, fastbin_child1_size(), false);
    }

    inline void child1(const ChildFixed& value) noexcept
    {
        assert(value.fastbin_binary_size() > 0 && "Cannot set struct value, struct ChildFixed not finalized, call fastbin_finalize() on struct after creation.");
        size_t offset = fastbin_child1_offset();
        size_t size = value.fastbin_binary_size();
        std::copy(value.buffer, value.buffer + size, buffer + offset);
    }

    constexpr inline size_t fastbin_child1_offset() const noexcept
    {
        return 16;
    }

    constexpr inline size_t fastbin_child1_size() const noexcept
    {
        return 16;
    }

    inline ChildVar child2() const noexcept
    {
        auto ptr = buffer + fastbin_child2_offset();
        return ChildVar(ptr, fastbin_child2_size(), false);
    }

    inline void child2(const ChildVar& value) noexcept
    {
        assert(value.fastbin_binary_size() > 0 && "Cannot set struct value, struct ChildVar not finalized, call fastbin_finalize() on struct after creation.");
        size_t offset = fastbin_child2_offset();
        size_t size = value.fastbin_binary_size();
        std::copy(value.buffer, value.buffer + size, buffer + offset);
    }

    constexpr inline size_t fastbin_child2_offset() const noexcept
    {
        return 32;
    }

    constexpr inline size_t fastbin_child2_size() const noexcept
    {
        return *reinterpret_cast<size_t*>(buffer + fastbin_child2_offset());
    }

    inline std::string_view str() const noexcept
    {
        size_t n_bytes = fastbin_str_size_unaligned() - 8;
        size_t count = n_bytes;
        auto ptr = reinterpret_cast<const char*>(buffer + fastbin_str_offset() + 8);
        return std::string_view(ptr, count);
    }

    inline void str(const std::string_view value) noexcept
    {
        size_t offset = fastbin_str_offset();
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

    constexpr inline size_t fastbin_str_offset() const noexcept
    {
        return fastbin_child2_offset() + fastbin_child2_size();
    }

    constexpr inline size_t fastbin_str_size() const noexcept
    {
        size_t stored_size = *reinterpret_cast<size_t*>(buffer + fastbin_str_offset());
        size_t aligned_size = stored_size & 0x00FFFFFFFFFFFFFF;
        return aligned_size;
    }

    constexpr inline size_t fastbin_str_size_unaligned() const noexcept
    {
        size_t stored_size = *reinterpret_cast<size_t*>(buffer + fastbin_str_offset());
        size_t aligned_diff = stored_size >> 56;
        size_t aligned_size = stored_size & 0x00FFFFFFFFFFFFFF;
        return aligned_size - aligned_diff;
    }

    constexpr inline size_t fastbin_compute_binary_size() const noexcept
    {
        return fastbin_str_offset() + fastbin_str_size();
    }

    inline void fastbin_finalize() const noexcept
    {
        *reinterpret_cast<size_t*>(buffer) = fastbin_compute_binary_size();
    }
};
}; // namespace my_models

inline std::ostream& operator<<(std::ostream& os, const my_models::Parent& obj)
{
    os << "[my_models::Parent size=" << obj.fastbin_binary_size() << " bytes]\n";
    os << "    field1: " << obj.field1() << "\n";
    os << "    child1: " << obj.child1() << "\n";
    os << "    child2: " << obj.child2() << "\n";
    os << "    str: " << std::string(obj.str()) << "\n";
    return os;
}
