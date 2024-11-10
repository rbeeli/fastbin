#pragma once

#include <cstdint>
#include <cstring>
#include <cassert>
#include <cstddef>
#include <vector>
#include <stdexcept>
#include <type_traits>
#include <concepts>
#include <ranges>

#include "_traits.hpp"
#include "_BufferStored.hpp"

namespace my_models
{
/**
 * A contiguous array of elements of type T.
 * 
 * Memory Layout:
 * [size_: size_t][count_: size_t][element1_size: size_t][element1: T][element2_size: size_t][element2: T]...
 * ^                              ^
 * buffer points here             bufferptr() points here
 * 
 * - size_ stores total size of the StructArray in bytes, which includes size_, count_ and all elements
 * - Elements are stored sequentially
 * - No separate allocations - everything in one contiguous block
 * - Total size = 2 * sizeof(size_t) + sum(element_size for each element)
 */
template <typename T>
class StructArray final : public BufferStored<StructArray<T>>
{
public:
    using BufferStored<StructArray<T>>::buffer;
    using BufferStored<StructArray<T>>::buffer_size;
    using BufferStored<StructArray<T>>::owns_buffer;

protected:
    friend class BufferStored<StructArray<T>>;

    // constructor forwards to base class
    StructArray(std::byte* buffer, size_t buffer_size, bool owns_buffer) noexcept
        : BufferStored<StructArray<T>>(buffer, buffer_size, owns_buffer)
    {
    }

    inline size_t& size_ref() const noexcept
    {
        return *reinterpret_cast<size_t*>(buffer);
    }

    inline size_t& count_ref() const noexcept
    {
        return *reinterpret_cast<size_t*>(buffer + sizeof(size_t));
    }

    inline std::byte* bufferptr() const noexcept
    {
        return buffer + 2 * sizeof(size_t);
    }

    // Fixed-size types - non-const version
    template <typename U = T>
    typename std::enable_if<!is_variable_size<U>::value, U>::type get_nth_element(size_t n)
    {
        if (n >= count_ref())
            throw std::out_of_range("Index out of bounds");

        std::byte* ptr = bufferptr() + n * U::fastbin_fixed_size();
        return U::open(ptr, U::fastbin_fixed_size(), false);
    }

    // Fixed-size types - const version
    template <typename U = T>
    typename std::enable_if<!is_variable_size<U>::value, U>::type get_nth_element(size_t n) const
    {
        if (n >= count_ref())
            throw std::out_of_range("Index out of bounds");

        const std::byte* ptr = bufferptr() + n * U::fastbin_fixed_size();
        return U::open(const_cast<std::byte*>(ptr), U::fastbin_fixed_size(), false);
    }

    // Variable-size types - non-const version
    template <typename U = T>
    typename std::enable_if<is_variable_size<U>::value, U>::type get_nth_element(size_t n)
    {
        if (n >= count_ref())
            throw std::out_of_range("Index out of bounds");

        std::byte* current = bufferptr();
        size_t size = *reinterpret_cast<const size_t*>(current);

        for (size_t i = 0; i < n; ++i)
        {
            size = *reinterpret_cast<const size_t*>(current);
            current += size;
        }

        return U::open(current, size, false);
    }

    // Variable-size types - const version
    template <typename U = T>
    typename std::enable_if<is_variable_size<U>::value, U>::type get_nth_element(size_t n) const
    {
        if (n >= count_ref())
            throw std::out_of_range("Index out of bounds");

        const std::byte* current = bufferptr();
        size_t size = *reinterpret_cast<const size_t*>(current);

        for (size_t i = 0; i < n; ++i)
        {
            size = *reinterpret_cast<const size_t*>(current);
            current += size;
        }

        return U::open(const_cast<std::byte*>(current), size, false);
    }

public:
    // copy
    StructArray(const StructArray&) = delete;
    StructArray& operator=(const StructArray&) = delete;

    // move
    StructArray(StructArray&&) noexcept = default;
    StructArray& operator=(StructArray&&) noexcept = default;

    static StructArray<T> create(std::byte* buffer, size_t buffer_size, bool owns_buffer) noexcept
    {
        // Zero-initialize the buffer
        std::memset(buffer, 0, buffer_size);

        // Create StructArray object in the buffer
        auto arr = StructArray<T>(buffer, buffer_size, owns_buffer);

        // Initial size is 2 * sizeof(size_t) for size_ and count_
        arr.size_ref() = 2 * sizeof(size_t);

        // Initial count is 0 already due to memset

        return arr;
    }

    /**
     * Returns the number of elements in the array.
     */
    inline size_t size() const noexcept
    {
        return count_ref();
    }

    void append(const T& element)
    {
        size_t el_size = element.fastbin_calc_binary_size();

        // Check if there is enough space in the buffer
        assert(buffer_size - size_ref() >= el_size && "Buffer overflow in StructArray append");

        // Write element to the end of the buffer
        std::byte* dest = bufferptr() + (size_ref() - 2 * sizeof(size_t)); // Adjust for header size
        std::memcpy(dest, element.buffer, el_size);

        // Increase total size
        size_ref() += el_size;

        // Increment count
        ++count_ref();
    }

    inline size_t fastbin_calc_binary_size() const noexcept
    {
        // Size of the StructArray is already stored in the buffer
        return size_ref();
    }

    /**
     * Calculates the binary size of the StructArray with the given elements.
     * The elements must be of the same type as the StructArray.
     * This function is used to pre-calculate the size before creating the StructArray.
     */
    template <std::ranges::range Container>
        requires std::same_as<std::ranges::range_value_t<Container>, T>
    static inline size_t fastbin_calc_binary_size(const Container& iterable) noexcept
    {
        size_t size = 2 * sizeof(size_t);
        for (const auto& item : iterable)
            size += item.fastbin_calc_binary_size();
        return size;
    }

    /**
     * Returns the stored binary size of the object (always aligned to 8 bytes).
     * This function should only be called after `fastbin_finalize()`.
     */
    inline size_t fastbin_binary_size() const noexcept
    {
        return size_ref();
    }

    /**
     * Finalizes the object by writing the binary size to the beginning of its buffer.
     * After calling this function, the underlying buffer can be used for serialization.
     * To get the actual buffer size, call `fastbin_binary_size()`.
     */
    inline void fastbin_finalize() noexcept
    {
        // no-op for StructArray - size is already stored when appending elements
    }

    /**
     * Copies the object to a new buffer.
     * The new buffer must be large enough to hold all data.
     */
    [[nodiscard]] StructArray<T> copy(
        std::byte* dest_buffer, size_t dest_buffer_size, bool owns_buffer
    ) const noexcept
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
    [[nodiscard]] StructArray<T> copy() const noexcept
    {
        size_t size = fastbin_binary_size();
        auto dest_buffer = new std::byte[size];
        std::memcpy(dest_buffer, buffer, size);
        return {dest_buffer, size, true};
    }

    inline T operator[](size_t index)
    {
        return get_nth_element(index);
    }

    inline const T operator[](size_t index) const
    {
        return get_nth_element(index);
    }

    // iterator
    class iterator
    {
    private:
        StructArray* arr_;
        size_t index_;

    public:
        iterator(StructArray* arr, size_t index) : arr_(arr), index_(index)
        {
        }

        iterator& operator++()
        {
            ++index_;
            return *this;
        }

        bool operator!=(const iterator& other) const
        {
            return index_ != other.index_;
        }

        T operator*()
        {
            return arr_->get_nth_element(index_);
        }
    };

    // Const iterator
    class const_iterator
    {
    private:
        const StructArray* arr_;
        size_t index_;

    public:
        const_iterator(const StructArray* arr, size_t index) : arr_(arr), index_(index)
        {
        }

        const_iterator& operator++()
        {
            ++index_;
            return *this;
        }

        bool operator!=(const const_iterator& other) const
        {
            return index_ != other.index_;
        }

        T operator*() const // Note: returns by value since T is a struct
        {
            return arr_->get_nth_element(index_);
        }
    };

    // Non-const begin/end
    iterator begin()
    {
        return iterator(this, 0);
    }

    iterator end()
    {
        return iterator(this, count_ref());
    }

    // Const begin/end
    const_iterator begin() const
    {
        return const_iterator(this, 0);
    }

    const_iterator end() const
    {
        return const_iterator(this, count_ref());
    }

    // make non-const begin()/end() use const versions when called on const objects
    const_iterator cbegin() const
    {
        return begin();
    }

    const_iterator cend() const
    {
        return end();
    }
};

// Type traits
template <typename T>
struct is_variable_size<StructArray<T>>
{
    static constexpr bool value = true;
};
}; // namespace my_models
