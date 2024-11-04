#pragma once

#include <cstddef>
#include <cstring>
#include <cassert>
#include <ostream>
#include <span>
#include <string_view>
#include "_traits.hpp"

namespace my_models
{
/**
 * Binary serializable data container generated by `fastbin`.
 * 
 * This container has variable size.
 * All setter methods starting from the first variable-sized member and afterwards MUST be called in order.
 *
 * Members in order
 * ================
 * - `values` [`std::span<std::uint32_t>`] (variable)
 * - `str` [`std::string_view`] (variable)
 *
 * The `finalize()` method MUST be called after all setter methods have been called.
 * 
 * It is the responsibility of the caller to ensure that the buffer is
 * large enough to hold all data.
 */
class VectorOfUInt32 final
{
public:
    std::byte* buffer{nullptr};
    size_t buffer_size{0};
    bool owns_buffer{false};

private:
    VectorOfUInt32(
        std::byte* buffer, size_t buffer_size, bool owns_buffer
    ) noexcept
        : buffer(buffer), buffer_size(buffer_size), owns_buffer(owns_buffer)
    {
    }

public:
    static VectorOfUInt32 create(std::byte* buffer, size_t buffer_size, bool owns_buffer) noexcept
    {
        std::memset(buffer, 0, buffer_size);
        return {buffer, buffer_size, owns_buffer};
    }

    static VectorOfUInt32 create(std::span<std::byte> buffer, bool owns_buffer) noexcept
    {
        return create(buffer.data(), buffer.size(), owns_buffer);
    }

    static VectorOfUInt32 open(std::byte* buffer, size_t buffer_size, bool owns_buffer) noexcept
    {
        return {buffer, buffer_size, owns_buffer};
    }
    
    static VectorOfUInt32 open(std::span<std::byte> buffer, bool owns_buffer) noexcept
    {
        return VectorOfUInt32(buffer.data(), buffer.size(), owns_buffer);
    }
    
    // destructor
    ~VectorOfUInt32() noexcept
    {
        if (owns_buffer && buffer != nullptr)
        {
            delete[] buffer;
            buffer = nullptr;
        }
    }

    // disable copy
    VectorOfUInt32(const VectorOfUInt32&) = delete;
    VectorOfUInt32& operator=(const VectorOfUInt32&) = delete;

    // enable move
    VectorOfUInt32(VectorOfUInt32&& other) noexcept
        : buffer(other.buffer), buffer_size(other.buffer_size), owns_buffer(other.owns_buffer)
    {
        other.buffer = nullptr;
        other.buffer_size = 0;
    }
    VectorOfUInt32& operator=(VectorOfUInt32&& other) noexcept
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

    // Member: values [std::span<std::uint32_t>]

    inline std::span<std::uint32_t> values() const noexcept
    {
        size_t n_bytes = _values_size_unaligned() - 8;
        size_t count = n_bytes / 4;
        auto ptr = reinterpret_cast<std::uint32_t*>(buffer + _values_offset() + 8);
        return std::span<std::uint32_t>(ptr, count);
    }

    inline void values(const std::span<std::uint32_t> value) noexcept
    {
        size_t offset = _values_offset();
        size_t contents_size = value.size() * 4;
        size_t unaligned_size = 8 + contents_size;
        size_t aligned_size = (unaligned_size + 7) & ~7;
        size_t aligned_diff = aligned_size - unaligned_size;
        size_t aligned_size_high = aligned_size | (aligned_diff << 56);
        *reinterpret_cast<size_t*>(buffer + offset) = aligned_size_high;
        auto dest_ptr = reinterpret_cast<std::byte*>(buffer + offset + 8);
        auto src_ptr = reinterpret_cast<const std::byte*>(value.data());
        std::copy(src_ptr, src_ptr + contents_size, dest_ptr);
    }

    constexpr inline size_t _values_offset() const noexcept
    {
        return 8;
    }

    constexpr inline size_t _values_size_aligned() const noexcept
    {
        size_t stored_size = *reinterpret_cast<size_t*>(buffer + _values_offset());
        size_t aligned_size = stored_size & 0x00FFFFFFFFFFFFFF;
        return aligned_size;
    }

    static size_t _values_calc_size_aligned(const std::span<std::uint32_t>& value)
    {
        size_t contents_size = value.size() * 4;
        size_t unaligned_size = 8 + contents_size;
        return (unaligned_size + 7) & ~7;
    }

    constexpr inline size_t _values_size_unaligned() const noexcept
    {
        size_t stored_size = *reinterpret_cast<size_t*>(buffer + _values_offset());
        size_t aligned_diff = stored_size >> 56;
        size_t aligned_size = stored_size & 0x00FFFFFFFFFFFFFF;
        return aligned_size - aligned_diff;
    }

    // Member: str [std::string_view]

    inline std::string_view str() const noexcept
    {
        size_t n_bytes = _str_size_unaligned() - 8;
        auto ptr = reinterpret_cast<const char*>(buffer + _str_offset() + 8);
        return std::string_view(ptr, n_bytes);
    }

    inline void str(const std::string_view value) noexcept
    {
        size_t offset = _str_offset();
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

    constexpr inline size_t _str_offset() const noexcept
    {
        return _values_offset() + _values_size_aligned();
    }

    constexpr inline size_t _str_size_aligned() const noexcept
    {
        size_t stored_size = *reinterpret_cast<size_t*>(buffer + _str_offset());
        size_t aligned_size = stored_size & 0x00FFFFFFFFFFFFFF;
        return aligned_size;
    }

    static size_t _str_calc_size_aligned(const std::string_view& value)
    {
        size_t contents_size = value.size() * 1;
        size_t unaligned_size = 8 + contents_size;
        return (unaligned_size + 7) & ~7;
    }

    constexpr inline size_t _str_size_unaligned() const noexcept
    {
        size_t stored_size = *reinterpret_cast<size_t*>(buffer + _str_offset());
        size_t aligned_diff = stored_size >> 56;
        size_t aligned_size = stored_size & 0x00FFFFFFFFFFFFFF;
        return aligned_size - aligned_diff;
    }

    // --------------------------------------------------------------------------------

    constexpr inline size_t fastbin_calc_binary_size() const noexcept
    {
        return _str_offset() + _str_size_aligned();
    }

    static size_t fastbin_calc_binary_size(
        const std::span<std::uint32_t>& values,
        const std::string_view& str
    )
    {
        return 8 + _values_calc_size_aligned(values) +
            _str_calc_size_aligned(str);
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
    inline void fastbin_finalize() noexcept
    {
        *reinterpret_cast<size_t*>(buffer) = fastbin_calc_binary_size();
    }

    /**
     * Copies the object to a new buffer.
     * The new buffer must be large enough to hold all data.
     */
    [[nodiscard]] VectorOfUInt32 copy(std::byte* dest_buffer, size_t dest_buffer_size, bool owns_buffer) const noexcept
    {
        size_t size = fastbin_binary_size();
        assert(dest_buffer_size >= size && "New buffer size too small.");
        std::memcpy(dest_buffer, buffer, size);
        return {dest_buffer, dest_buffer_size, owns_buffer};
    }

    /**
     * Creates a copy of this object.
     * The returned copy is completely independent of the original object.
     */
    [[nodiscard]] VectorOfUInt32 copy() const noexcept
    {
        size_t size = fastbin_binary_size();
        auto dest_buffer = new std::byte[size];
        std::memcpy(dest_buffer, buffer, size);
        return {dest_buffer, size, true};
    }
};

// Type traits
template <>
struct is_variable_size<VectorOfUInt32>
{
    static constexpr bool value = true;
};
}; // namespace my_models

inline std::ostream& operator<<(std::ostream& os, const my_models::VectorOfUInt32& obj)
{
    os << "[my_models::VectorOfUInt32 size=" << obj.fastbin_binary_size() << " bytes]\n";
    os << "    values: " << "[vector<uint32> count=" << obj.values().size() << "]" << "\n";
    os << "    str: " << std::string(obj.str()) << "\n";
    return os;
}
