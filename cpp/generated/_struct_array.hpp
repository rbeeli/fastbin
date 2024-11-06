#pragma once

#include <cstdint>
#include <cstring>
#include <cassert>
#include <cstddef>
#include <vector>
#include <stdexcept>
#include <type_traits>

#include "_traits.hpp"

namespace my_models
{
/*
Memory Layout:
[size_: size_t][count_: size_t][element1_size: size_t][element1: T][element2_size: size_t][element2: T]...
^                              ^
buffer points here             bufferptr() points here

- size_ stores total size of the struct_array in bytes, which includes size_, count_ and all elements
- Elements are stored sequentially
- No separate allocations - everything in one contiguous block
- Total size = 2 * sizeof(size_t) + sum(element_size for each element)
*/
template <typename T>
class struct_array
{
public:
    std::byte* buffer{nullptr}; // Points to the start of the memory block
    size_t buffer_size{0};
    bool owns_buffer{false};

private:
    inline size_t& size_ref() const
    {
        return *reinterpret_cast<size_t*>(buffer);
    }

    inline size_t& count_ref() const
    {
        return *reinterpret_cast<size_t*>(buffer + sizeof(size_t));
    }

    inline std::byte* bufferptr() const
    {
        return buffer + 2 * sizeof(size_t);
    }

    // For fixed-size types
    template <typename U = T>
    typename std::enable_if<!is_variable_size<U>::value, U>::type get_nth_element(size_t n)
    {
        if (n >= count_ref())
            throw std::out_of_range("Index out of bounds");

        // For fixed-size types, we can calculate the offset directly
        std::byte* ptr = bufferptr() + n * U::fastbin_fixed_size();

        return U::open(ptr, U::fastbin_fixed_size(), false);
    }

    // For variable-size types
    template <typename U = T>
    typename std::enable_if<is_variable_size<U>::value, U>::type get_nth_element(size_t n)
    {
        if (n >= count_ref())
            throw std::out_of_range("Index out of bounds");

        std::byte* current = bufferptr();
        size_t size = 0;

        // Need to walk through the buffer for variable-sized elements
        for (size_t i = 0; i < n; ++i)
        {
            size = *reinterpret_cast<const size_t*>(current);
            current += size;
        }

        return U::open(current, size, false);
    }

    // private constructor
    struct_array(std::byte* buffer, size_t buffer_size, bool owns_buffer) noexcept
        : buffer(buffer), buffer_size(buffer_size), owns_buffer(owns_buffer)
    {
    }

public:
    static struct_array<T> create(std::byte* buffer, size_t buffer_size, bool owns_buffer) noexcept
    {
        // Zero-initialize the buffer
        std::memset(buffer, 0, buffer_size);

        // Create struct_array object
        auto arr = struct_array<T>(buffer, buffer_size, owns_buffer);

        // Initial size is 2 * sizeof(size_t) for size_ and count_
        arr.size_ref() = 2 * sizeof(size_t);

        // Initial count is 0 already due to memset

        return arr;
    }

    static struct_array<T> create(std::span<std::byte> buffer, bool owns_buffer) noexcept
    {
        return create(buffer.data(), buffer.size(), owns_buffer);
    }

    static struct_array<T> open(std::byte* buffer, size_t buffer_size, bool owns_buffer) noexcept
    {
        return {buffer, buffer_size, owns_buffer};
    }

    static struct_array<T> open(std::span<std::byte> buffer, bool owns_buffer) noexcept
    {
        return open(buffer.data(), buffer.size(), owns_buffer);
    }

    // destructor
    ~struct_array() noexcept
    {
        if (owns_buffer && buffer != nullptr)
        {
            delete[] buffer;
            buffer = nullptr;
        }
    }

    // disable copy
    struct_array(const struct_array&) = delete;
    struct_array& operator=(const struct_array&) = delete;

    // enable move
    struct_array(struct_array&& other) noexcept
        : buffer(other.buffer), buffer_size(other.buffer_size), owns_buffer(other.owns_buffer)
    {
        other.buffer = nullptr;
        other.buffer_size = 0;
    }
    struct_array& operator=(struct_array&& other) noexcept
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

    /**
     * Returns the number of elements in the array.
     */
    inline size_t size() const
    {
        return count_ref();
    }

    void append(const T& element)
    {
        size_t el_size = element.fastbin_calc_binary_size();

        // Check if there is enough space in the buffer
        // assert(buffer_size - size_ref() >= el_size && "Buffer overflow in struct_array append");

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
        // Size of the struct_array is already stored in the buffer
        return size_ref();
    }

    /**
     * Calculates the binary size of the struct_array with the given elements.
     * The elements must be of the same type as the struct_array.
     * This function is used to pre-calculate the size before creating the struct_array.
     */
    template <std::ranges::range Container>
        requires std::same_as<std::ranges::range_value_t<Container>, T>
    static size_t fastbin_calc_binary_size(const Container& iterable)
    {
        size_t base_size = 2 * sizeof(size_t);
        size_t total_size = base_size;
        for (const auto& item : iterable)
            total_size += item.fastbin_calc_binary_size();
        return total_size;
    }

    /**
     * Returns the stored (aligned) binary size of the object.
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
        // no-op for struct_array - size is already stored when appending elements
    }

    /**
     * Copies the object to a new buffer.
     * The new buffer must be large enough to hold all data.
     */
    [[nodiscard]] struct_array<T> copy(
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
    [[nodiscard]] struct_array<T> copy() const noexcept
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

    // Iterator support
    class iterator
    {
    private:
        struct_array* span_;
        size_t index_;

    public:
        iterator(struct_array* span, size_t index) //
            : span_(span), index_(index)
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

        T& operator*()
        {
            return get_nth_element(index_);
        }
    };

    iterator begin()
    {
        return iterator(this, 0);
    }

    iterator end()
    {
        return iterator(this, count_ref());
    }
};
}; // namespace my_models
